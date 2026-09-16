/* XIO implementation, using the xpoll backend. */

#define FH_LOG_MODULE_NAME "xio"

#define _GNU_SOURCE
#define _DARWIN_C_SOURCE

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "log/log.h"
#include "utils/compat.h"
#include "utils/utils.h"
#include "xio.h"
#include "xpoll.h"

#define XIO_XPOLL_MAX_EVENTS 2048
#define XIO_XPOLL_MAX_SQE_COUNT 2048
#define XIO_SLICE_BUF_SIZE 8192
#define XIO_SLICE_BUF_COUNT 1024

static_assert ((XIO_XPOLL_MAX_EVENTS & (XIO_XPOLL_MAX_EVENTS - 1)) == 0,
               "XIO_XPOLL_MAX_EVENTS must be a power of 2");
static_assert ((XIO_SLICE_BUF_SIZE & (XIO_SLICE_BUF_SIZE - 1)) == 0,
               "XIO_SLICE_BUF_SIZE must be a power of 2");
static_assert ((XIO_SLICE_BUF_COUNT & (XIO_SLICE_BUF_COUNT - 1)) == 0,
               "XIO_SLICE_BUF_COUNT must be a power of 2");
static_assert (XIO_SLICE_BUF_COUNT <= UINT16_MAX,
               "XIO_SLICE_BUF_COUNT must fit in uint16_t");
static_assert (XIO_SLICE_BUF_COUNT * XIO_SLICE_BUF_SIZE <= UINT32_MAX,
               "XIO_SLICE_BUF_COUNT * XIO_SLICE_BUF_SIZE must fit in uint32_t");
static_assert ((XIO_XPOLL_MAX_SQE_COUNT & (XIO_XPOLL_MAX_SQE_COUNT - 1)) == 0,
               "XIO_XPOLL_MAX_SQE_COUNT must be a power of 2");
static_assert (XIO_XPOLL_MAX_SQE_COUNT <= UINT16_MAX,
               "XIO_XPOLL_MAX_SQE_COUNT must fit in uint16_t");

struct fh_xio_slice
{
    uint32_t mem_off;
    uint16_t off;
    uint16_t flags;
    uint16_t refcount;
};

struct fh_xio_sqe
{
    enum fh_xio_op op;
    uint32_t flags;

    union
    {
        struct
        {
            fd_t fd;
            void *ptr;
            size_t size;
            int flags;
        } recv;
    } opdata;
};

struct fh_xio_xpoll_data
{
    struct fh_xio_sqe **sqe_list;
    size_t count;
};

struct fh_xio
{
    xpoll_t xp;
    uint8_t *mem_base;
    struct fh_xio_slice *slices;
    uint16_t slice_count, current_slice_index;
    uint16_t free_slice_stack_top, free_sqe_stack_top;
    uint16_t free_slice_stack[XIO_SLICE_BUF_COUNT];
    struct fh_xio_sqe sqe_list[XIO_XPOLL_MAX_SQE_COUNT];
    uint16_t free_sqe_stack[XIO_XPOLL_MAX_SQE_COUNT];
    xpoll_event_t events[XIO_XPOLL_MAX_EVENTS];
};

static bool
fh_xio_slices_alloc (struct fh_xio *xio)
{
    const size_t size = XIO_SLICE_BUF_SIZE * XIO_SLICE_BUF_COUNT;
    uint8_t *mem = MAP_FAILED;

#ifdef FH_PLATFORM_LINUX
    mem = mmap (NULL, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
#else
    if (mem == MAP_FAILED)
    {
        mem = mmap (NULL, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (mem == MAP_FAILED)
            return false;

    #ifdef MADV_HUGEPAGE
        madvise (mem, size, MADV_HUGEPAGE);
    #endif /* MADV_HUGEPAGE */
    }
#endif     /* FH_PLATFORM_LINUX */

    xio->mem_base = mem;
    xio->slices = NULL;

    if (posix_memalign ((void **) &xio->slices, sizeof (size_t),
                        sizeof (struct fh_xio_slice) * XIO_SLICE_BUF_COUNT))
    {
        int err = errno;
        munmap (mem, size);
        errno = err;
        return false;
    }

    for (size_t i = 0; i < XIO_SLICE_BUF_COUNT; i++)
    {
        struct fh_xio_slice *slice = xio->slices + i;

        slice->mem_off = i * XIO_SLICE_BUF_SIZE;
        slice->flags = 0;
        slice->refcount = 0;
        slice->off = 0;

        xio->free_slice_stack[XIO_SLICE_BUF_COUNT - i - 1] = i;
    }

    xio->free_slice_stack_top = XIO_SLICE_BUF_COUNT - 1;
    return true;
}

struct fh_xio *
fh_xio_create (void)
{
    struct fh_xio *xio = calloc (1, sizeof (*xio));

    if (!xio)
        return NULL;

    xio->xp = xpoll_create (XPOLL_CLOEXEC);

    if (XPOLL_XP_ERR (xio->xp))
    {
        free (xio);
        return NULL;
    }

    if (!fh_xio_slices_alloc (xio))
    {
        xpoll_close (xio->xp);
        free (xio);
        return NULL;
    }

    for (uint16_t i = 0; i < XIO_XPOLL_MAX_SQE_COUNT; i++)
        xio->free_sqe_stack[XIO_XPOLL_MAX_SQE_COUNT - i - 1] = i;

    xio->free_sqe_stack_top = XIO_XPOLL_MAX_SQE_COUNT - 1;
    return xio;
}

void
fh_xio_free (struct fh_xio *xio)
{
    munmap (xio->mem_base, XIO_SLICE_BUF_COUNT * XIO_SLICE_BUF_SIZE);
    free (xio->slices);
    xpoll_close (xio->xp);
    free (xio);
}

bool
fh_xio_submit_requests (struct fh_xio *xio)
{
    (void) xio;
    return true;
}

static uint8_t *
fh_xio_get_request_buf (struct fh_xio *xio, size_t size)
{
    size = MIN_VALUE (size, XIO_SLICE_BUF_SIZE);

    const bool current_buffer_full
        = xio->slices[xio->current_slice_index].off + size
          >= XIO_SLICE_BUF_SIZE;
    const bool no_free_buffers = !xio->free_slice_stack_top;

    if (current_buffer_full && no_free_buffers)
    {
        errno = ENOBUFS;
        return NULL;
    }

    if (current_buffer_full)
    {
        xio->current_slice_index
            = xio->free_slice_stack[--xio->free_slice_stack_top];
        assert (xio->slices[xio->current_slice_index].off == 0);
    }

    struct fh_xio_slice *slice = &xio->slices[xio->current_slice_index];
    uint8_t *usable_mem = xio->mem_base + slice->mem_off + slice->off;
    slice->off += size;
    return usable_mem;
}

static inline struct fh_xio_sqe *
fh_xio_get_sqe (struct fh_xio *xio)
{
    if (!xio->free_sqe_stack_top)
    {
        errno = ENOBUFS;
        return NULL;
    }

    return &xio->sqe_list[--xio->free_sqe_stack_top];
}

static inline bool
fh_xio_get_sqe_with_buf (struct fh_xio *xio, size_t size,
                         struct fh_xio_sqe **sqe_out, uint8_t **buf_out)
{
    struct fh_xio_sqe *sqe = fh_xio_get_sqe (xio);

    if (!sqe)
        return false;

    uint8_t *buf = fh_xio_get_request_buf (xio, size);

    if (!buf)
    {
        xio->free_sqe_stack_top++;
        return false;
    }

    *sqe_out = sqe;
    *buf_out = buf;

    return true;
}

int
fh_xio_request_recv (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                     size_t size, int flags)
{
    /* TODO */
    return -ENOTSUP;
}

ssize_t
fh_xio_wait (struct fh_xio *xio, struct fh_xio_result *results,
             size_t max_results, uint64_t timeout_ms)
{
    max_results = MIN_VALUE (max_results, XIO_XPOLL_MAX_EVENTS);
    int count = xpoll_wait (xio->xp, xio->events, max_results, timeout_ms);

    if (count < 0)
        return count;

    for (int i = 0; i < count; i++)
    {
    }

    return (ssize_t) count;
}
