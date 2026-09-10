/* For now only work on Linux (io_uring), then move to xpoll support.
   In addition, we expect that users have the bleeding edge kernel
   when using the io_uring backend, preferably 7.0.0+.  Otherwise
   we can fall back to epoll(2) via xpoll. */

#define _DEFAULT_SOURCE
#define FH_LOG_MODULE_NAME "xio"

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include <liburing.h>
#include <sys/mman.h>

#ifdef HAVE_VALGRIND
    #include <valgrind/memcheck.h>
#endif /* HAVE_VALGRIND */

#include "log/log.h"
#include "utils/compat.h"
#include "utils/utils.h"
#include "xio.h"

#define XIO_RING_ENTRIES 512
#define XIO_RING_HUGE_SIZE (2 * 1024 * 1024)

#define XIO_RING_BUF_COUNT 1024
#define XIO_RING_BUF_SIZE 4096
#define XIO_RING_BUF_BGID_DEFAULT 1
#define XIO_RING_BUF_FLAG_DONE (1U << 31)

static_assert (XIO_RING_BUF_COUNT <= UINT16_MAX,
               "XIO_RING_BUF_COUNT must fit in a uint16_t");
static_assert ((XIO_RING_BUF_COUNT & (XIO_RING_BUF_COUNT - 1)) == 0,
               "XIO_RING_BUF_COUNT must be a power of 2");
static_assert ((XIO_RING_BUF_SIZE & (XIO_RING_BUF_SIZE - 1)) == 0,
               "XIO_RING_BUF_SIZE must be a power of 2");

#define XIO_MAX_DATA_FREELIST_COUNT 2048

enum fh_xio_op
{
    XIO_OP_READ = 1,
};

struct fh_xio_data
{
    struct fh_xio_data *next;
    struct fh_xio_data *prev;

    void *udata;
    enum fh_xio_op op;

    union
    {
        struct
        {
            fd_t fd;
            void *buf;
            size_t size;
        } read;
    } opdata;
};

struct fh_xio
{
    struct io_uring ring;
    struct io_uring_buf_ring *buf_ring;
    void *ring_kmem;
    size_t ring_kmem_size;
    void *buf;
    unsigned int buf_mask;
    size_t *buf_offsets;
    uint32_t *buf_refcount;
    struct fh_xio_data *data_head;
    struct fh_xio_data *data_tail;
    size_t data_count;
};

static void *
fh_xio_ring_alloc_mem (size_t requested_size, size_t *aligned_size)
{
    requested_size = (requested_size + XIO_RING_HUGE_SIZE - 1)
                     & ~((size_t) XIO_RING_HUGE_SIZE - 1);
    void *ptr = mmap (NULL, requested_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

    if (ptr == MAP_FAILED)
    {
        ptr = mmap (NULL, requested_size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (ptr == MAP_FAILED)
            return NULL;

        madvise (ptr, requested_size, MADV_HUGEPAGE);
    }

    *aligned_size = requested_size;
    return ptr;
}

/* On Linux (io_uring), this function initializes the ring with
   single issuer constraints, that is, it must be strictly owned
   by one process/thread at a time. If a new child is forked or a
   new thread is spawned while an xio instance is active in both,
   this causes undefined behavior.
   XIO does not support multithreading as of now, though it may
   use worker threads internally for asynchronous I/O. */

struct fh_xio *
fh_xio_create (void)
{
    struct fh_xio *xio = calloc (
        1, sizeof (*xio) + sizeof (*xio->buf_offsets) * XIO_RING_BUF_COUNT
               + sizeof (*xio->buf_refcount) * XIO_RING_BUF_COUNT);

    if (!xio)
        return NULL;

    xio->buf_offsets = (size_t *) (xio + 1);
    xio->buf_refcount = (uint32_t *) (xio->buf_offsets + XIO_RING_BUF_COUNT);

    struct io_uring_params params = { 0 };

    params.cq_entries = XIO_RING_ENTRIES * 4;
    params.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN
                   | IORING_SETUP_CQSIZE | IORING_SETUP_NO_SQARRAY
                   | IORING_SETUP_REGISTERED_FD_ONLY | IORING_SETUP_NO_MMAP;

    int rc = 0;

    const ssize_t need_size
        = io_uring_memory_size_params (XIO_RING_ENTRIES, &params);
    bool init_mem_succeeded = false;

    if (need_size > 0)
    {
        size_t aligned_size = 0;
        void *ptr = fh_xio_ring_alloc_mem ((size_t) need_size, &aligned_size);

        if (ptr
            && io_uring_queue_init_mem (XIO_RING_ENTRIES, &xio->ring, &params,
                                        ptr, aligned_size)
                   >= 0)
        {
            xio->ring_kmem = ptr;
            xio->ring_kmem_size = aligned_size;
            init_mem_succeeded = true;
        }
        else if (ptr)
            munmap (ptr, aligned_size);
    }

    if (!init_mem_succeeded)
    {
        params.flags
            &= ~(IORING_SETUP_REGISTERED_FD_ONLY | IORING_SETUP_NO_MMAP);
        rc = io_uring_queue_init_params (XIO_RING_ENTRIES, &xio->ring, &params);

        if (rc)
        {
            free (xio);
            errno = -rc;
            return NULL;
        }

        io_uring_register_ring_fd (&xio->ring);
    }

    xio->buf_mask = io_uring_buf_ring_mask (XIO_RING_BUF_COUNT);

    rc = 0;
    xio->buf_ring = io_uring_setup_buf_ring (&xio->ring, XIO_RING_BUF_COUNT,
                                             XIO_RING_BUF_BGID_DEFAULT,
                                             IOU_PBUF_RING_INC, &rc);

    if (!xio->buf_ring)
    {
        fh_xio_free (xio);
        errno = -rc;
        return NULL;
    }

    rc = posix_memalign (&xio->buf, XIO_RING_BUF_SIZE,
                         (size_t) XIO_RING_BUF_SIZE * XIO_RING_BUF_COUNT);

    if (rc)
    {
        xio->buf = NULL;
        fh_xio_free (xio);
        errno = rc;
        return NULL;
    }

    for (unsigned short int i = 0; i < XIO_RING_BUF_COUNT; i++)
        io_uring_buf_ring_add (xio->buf_ring,
                               ((uint8_t *) xio->buf) + (i * XIO_RING_BUF_SIZE),
                               XIO_RING_BUF_SIZE, i, xio->buf_mask, i);

    io_uring_buf_ring_advance (xio->buf_ring, XIO_RING_BUF_COUNT);
    return xio;
}

static inline struct io_uring_sqe *
fh_xio_ring_get_sqe (struct fh_xio *xio)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe (&xio->ring);

    if (!sqe)
    {
        io_uring_submit (&xio->ring);
        sqe = io_uring_get_sqe (&xio->ring);
    }

    return sqe;
}

static bool
fh_xio_drain (struct fh_xio *xio)
{
    struct io_uring_sqe *sqe = fh_xio_ring_get_sqe (xio);

    if (!sqe)
        return false;

    io_uring_prep_cancel (sqe, NULL, IORING_ASYNC_CANCEL_ANY);
    io_uring_sqe_set_data (sqe, NULL);

    if (!(sqe = fh_xio_ring_get_sqe (xio)))
        return false;

    io_uring_prep_nop (sqe);
    io_uring_sqe_set_data (sqe, xio);
    sqe->flags |= IOSQE_IO_DRAIN;

    struct __kernel_timespec ts = {
        .tv_sec = 1,
    };

    for (bool done = false; !done;)
    {
        struct io_uring_cqe *cqe_head = NULL;
        struct io_uring_cqe *cqe;
        unsigned int head, count = 0;

        const int rc = io_uring_submit_and_wait_timeout (&xio->ring, &cqe_head,
                                                         1, &ts, NULL);

        if (rc < 0)
        {
            if (rc == -EINTR)
                continue;

            fh_pr_debug ("drain failed (%d): forced to leak provided buffers\n",
                         rc);
            return false;
        }

        io_uring_for_each_cqe (&xio->ring, head, cqe)
        {
            struct fh_xio_data *data = io_uring_cqe_get_data (cqe);
            count++;

            if ((void *) data == (void *) xio)
            {
                done = true;
                break;
            }
            else if (!data || (cqe->flags & IORING_CQE_F_MORE))
                continue;

            free (data);
        }

        io_uring_cq_advance (&xio->ring, count);
    }

    return true;
}

/* For this function, xio is expected to be never be NULL, just like
   every other *_free() function in freehttpd's codebase. */

void
fh_xio_free (struct fh_xio *xio)
{
    fh_xio_drain (xio);

    if (xio->buf_ring)
        io_uring_free_buf_ring (&xio->ring, xio->buf_ring, XIO_RING_BUF_COUNT,
                                XIO_RING_BUF_BGID_DEFAULT);

    io_uring_queue_exit (&xio->ring);

    if (xio->buf)
        free (xio->buf);

    if (xio->ring_kmem)
        munmap (xio->ring_kmem, xio->ring_kmem_size);

    while (xio->data_head)
    {
        struct fh_xio_data *data = xio->data_head;
        xio->data_head = data->next;
        free (data);
    }

    free (xio);
}

static struct fh_xio_data *
fh_xio_data_alloc (void)
{
    struct fh_xio_data *data = malloc (sizeof (*data));

    if (!data)
        return NULL;

    data->next = data->prev = NULL;
    return data;
}

static void
fh_xio_data_disown (struct fh_xio *xio, struct fh_xio_data *data)
{
    if (xio->data_count >= XIO_MAX_DATA_FREELIST_COUNT)
    {
        free (data);
        return;
    }

    data->next = NULL;
    data->prev = xio->data_tail;

    if (xio->data_tail)
        xio->data_tail->next = data;
    else
        xio->data_head = data;

    xio->data_tail = data;
    xio->data_count++;
}

static struct fh_xio_data *
fh_xio_data_get (struct fh_xio *xio)
{
    if (!xio->data_count)
        return fh_xio_data_alloc ();

    struct fh_xio_data *data = xio->data_tail;
    xio->data_tail = data->prev;

    if (!xio->data_tail)
        xio->data_head = NULL;

    xio->data_count--;
    data->next = data->prev = NULL;
    return data;
}

static inline void
fh_xio_data_populate (const struct fh_xio_data *data,
                      const struct io_uring_cqe *cqe, size_t *request_size,
                      bool *size_skipped, void **buf_ptr)
{
    (void) cqe;
    size_t size;

    switch (data->op)
    {
        case XIO_OP_READ:
            size = data->opdata.read.size;

            if (data->opdata.read.buf)
                *buf_ptr = data->opdata.read.buf;

            break;

        default:
            assert (false && "Unknown operation");
            *size_skipped = true;
            return;
    }

    *request_size = size ? size : XIO_RING_BUF_SIZE;
}

int
fh_xio_request_read (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                     size_t size, off_t offset)
{
    struct fh_xio_data *data = fh_xio_data_get (xio);

    if (!data)
        return -ENOMEM;

    struct io_uring_sqe *sqe = fh_xio_ring_get_sqe (xio);

    if (!sqe)
    {
        fh_xio_data_disown (xio, data);
        return -EAGAIN;
    }

    data->op = XIO_OP_READ;
    data->udata = udata;
    data->opdata.read.fd = fd;
    data->opdata.read.size = size;
    data->opdata.read.buf = buf;

    io_uring_prep_read (sqe, fd, buf, size, (unsigned long long) offset);
    io_uring_sqe_set_data (sqe, data);

    if (!buf)
    {
        sqe->flags |= IOSQE_BUFFER_SELECT;
        sqe->buf_group = XIO_RING_BUF_BGID_DEFAULT;
    }

    return 0;
}

bool
fh_xio_submit_requests (struct fh_xio *xio)
{
    const int rc = io_uring_submit (&xio->ring);
    return rc >= 0;
}

static inline void *
fh_xio_ring_get_buffer (const struct fh_xio *xio, uint16_t bid)
{
    return ((char *) (xio->buf)) + (XIO_RING_BUF_SIZE * bid);
}

static inline void *
fh_xio_ring_get_buffer_off (const struct fh_xio *xio, uint16_t bid)
{
    return ((char *) fh_xio_ring_get_buffer (xio, bid)) + xio->buf_offsets[bid];
}

ssize_t
fh_xio_wait (struct fh_xio *xio, struct fh_xio_result *results,
             size_t max_results, uint64_t timeout_ms)
{
    ssize_t count = 0;
    struct io_uring_cqe *cqe_head = NULL, *cqe;
    struct __kernel_timespec ts = {
        .tv_sec = timeout_ms / 1000,
        .tv_nsec = (timeout_ms % 1000) * 1000000,
    };

    int rc = io_uring_wait_cqes_min_timeout (
        &xio->ring, &cqe_head, (unsigned int) max_results, &ts, 1, NULL);

    if (rc < 0 && rc != -ETIME && !io_uring_cq_ready (&xio->ring))
    {
        errno = -rc;
        return -1;
    }

    unsigned int i = 0;

    io_uring_for_each_cqe (&xio->ring, i, cqe)
    {
        if ((size_t) count >= max_results)
            break;

        const int32_t bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
        const int32_t f_buffer = cqe->flags & IORING_CQE_F_BUFFER;
        struct fh_xio_data *data = (struct fh_xio_data *) cqe->user_data;

        results[count].flags = cqe->flags;
        results[count].res = cqe->res;
        results[count].udata = data->udata;

        results[count].buf
            = (f_buffer) ? fh_xio_ring_get_buffer_off (xio, bid) : NULL;

        size_t size;
        bool size_skipped = false;
        fh_xio_data_populate (data, cqe, &size, &size_skipped,
                              &results[count].buf);

        if (size_skipped)
            size = cqe->res < 0 ? XIO_RING_BUF_SIZE - xio->buf_offsets[bid]
                                : (size_t) cqe->res;

#if !defined(NDEBUG) && defined(HAVE_VALGRIND)
        /* Silence valgrind false positives: io_uring completions
           write into results[count].buf via the kernel outside of a
           traced syscall, so memcheck can't see the data as
           initialized on its own. Explicitly mark the cqe->res
           bytes actually written as defined.
         */

        if (cqe->res > 0)
            VALGRIND_MAKE_MEM_DEFINED (results[count].buf, (size_t) cqe->res);
#endif /* !defined(NDEBUG) && defined(HAVE_VALGRIND) */

        if (f_buffer)
        {
            xio->buf_refcount[bid]++;

            if (!(cqe->flags & IORING_CQE_F_BUF_MORE))
                xio->buf_refcount[bid] |= XIO_RING_BUF_FLAG_DONE;

            if (xio->buf_offsets[bid] + size >= XIO_RING_BUF_SIZE)
                xio->buf_offsets[bid] = XIO_RING_BUF_SIZE;
            else
                xio->buf_offsets[bid] += size;

            assert ((cqe->flags & IORING_CQE_F_BUF_MORE)
                    || xio->buf_offsets[bid] == XIO_RING_BUF_SIZE);
        }

        fh_xio_data_disown (xio, data);
        count++;
    }

    io_uring_cq_advance (&xio->ring, count);
    return count;
}

static bool
fh_xio_result_free_internal (struct fh_xio *xio,
                             const struct fh_xio_result *result,
                             size_t ring_buf_offset)
{
    const int32_t bid = result->flags >> IORING_CQE_BUFFER_SHIFT;
    const int32_t f_buffer = result->flags & IORING_CQE_F_BUFFER;

    if (!f_buffer)
        return false;

    assert ((xio->buf_refcount[bid] & ~XIO_RING_BUF_FLAG_DONE) > 0);

    if (--xio->buf_refcount[bid] != XIO_RING_BUF_FLAG_DONE)
        return false;

    fh_pr_debug ("Restore: BID %i\n", bid);

    xio->buf_offsets[bid] = 0;
    xio->buf_refcount[bid] = 0;

    io_uring_buf_ring_add (xio->buf_ring, fh_xio_ring_get_buffer (xio, bid),
                           XIO_RING_BUF_SIZE, bid, xio->buf_mask,
                           ring_buf_offset);
    return true;
}

void
fh_xio_result_free (struct fh_xio *xio, const struct fh_xio_result *result)
{
    if (fh_xio_result_free_internal (xio, result, 0))
        io_uring_buf_ring_advance (xio->buf_ring, 1);
}

void
fh_xio_results_free (struct fh_xio *xio, struct fh_xio_result *results,
                     size_t count)
{
    size_t advance_count = 0;

    for (size_t i = 0; i < count; i++)
    {
        if (fh_xio_result_free_internal (xio, &results[i], advance_count))
            advance_count++;
    }

    io_uring_buf_ring_advance (xio->buf_ring, advance_count);
}
