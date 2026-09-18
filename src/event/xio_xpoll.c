/* XIO implementation, using the xpoll backend. */

#define FH_LOG_MODULE_NAME "xio:xpoll"

#define _GNU_SOURCE
#define _DARWIN_C_SOURCE

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "hash/int_htable.h"
#include "log/log.h"
#include "utils/compat.h"
#include "utils/utils.h"
#include "xio.h"
#include "xpoll.h"

#define XIO_XPOLL_MAX_EVENTS 2048
#define XIO_XPOLL_MAX_TASK_COUNT 2048
#define XIO_BUF_SIZE 4096
#define XIO_BUF_COUNT 2048

static_assert ((XIO_XPOLL_MAX_EVENTS & (XIO_XPOLL_MAX_EVENTS - 1)) == 0,
               "XIO_XPOLL_MAX_EVENTS must be a power of 2");
static_assert ((XIO_BUF_SIZE & (XIO_BUF_SIZE - 1)) == 0,
               "XIO_BUF_SIZE must be a power of 2");
static_assert ((XIO_BUF_COUNT & (XIO_BUF_COUNT - 1)) == 0,
               "XIO_BUF_COUNT must be a power of 2");
static_assert (XIO_BUF_COUNT <= UINT16_MAX,
               "XIO_BUF_COUNT must fit in uint16_t");
static_assert (XIO_BUF_COUNT * XIO_BUF_SIZE <= UINT32_MAX,
               "XIO_BUF_COUNT * XIO_BUF_SIZE must fit in uint32_t");
static_assert ((XIO_XPOLL_MAX_TASK_COUNT & (XIO_XPOLL_MAX_TASK_COUNT - 1)) == 0,
               "XIO_XPOLL_MAX_SQE_COUNT must be a power of 2");
static_assert (XIO_XPOLL_MAX_TASK_COUNT <= UINT16_MAX,
               "XIO_XPOLL_MAX_SQE_COUNT must fit in uint16_t");

struct xio_slice
{
    uint8_t *buf;
    uint32_t off;
    uint16_t bid;
    uint16_t refs;
};

struct xio_task
{
    uint16_t op;
    uint16_t flags;
    uint16_t idx;
    uint16_t bid;
    void *udata;

    union
    {
        struct
        {
            fd_t fd;
            void *buf;
            size_t size;
            int flags;
        } recv;
    } opdata;

    struct xio_task *next;
};

struct xio_fd_state
{
    fd_t fd;
    xpoll_event_type_t events;
    struct xio_task *task_in_head;
    struct xio_task *task_out_head;
    uint16_t idx;
};

struct fh_xio
{
    xpoll_t xp;
    /* (fd_t) => (struct fh_xio_fd_state *) */
    int_htable_t *fd_state_table;
    uint8_t *mem_base;
    xpoll_event_t events[XIO_XPOLL_MAX_EVENTS];
    struct xio_slice buf_list[XIO_BUF_COUNT];
    uint16_t free_buf_list[XIO_BUF_COUNT];
    struct xio_fd_state fd_state_list[XIO_XPOLL_MAX_TASK_COUNT];
    uint16_t free_fd_state_list[XIO_XPOLL_MAX_TASK_COUNT];
    struct xio_task task_list[XIO_XPOLL_MAX_TASK_COUNT];
    uint16_t free_task_list[XIO_XPOLL_MAX_TASK_COUNT];
    uint16_t free_buf_list_top;
    uint16_t free_task_list_top;
    uint16_t free_fd_state_list_top;
};

static inline struct xio_task *
fh_xio_acquire_task (struct fh_xio *xio)
{
    if (!xio->free_task_list_top)
    {
        errno = ENOMEM;
        return NULL;
    }

    struct xio_task *task
        = &xio->task_list[xio->free_task_list[--xio->free_task_list_top]];
    return task;
}

static inline void
fh_xio_release_task (struct fh_xio *xio, struct xio_task *task)
{
    assert (xio->free_task_list_top < XIO_XPOLL_MAX_TASK_COUNT);
    xio->free_task_list[xio->free_task_list_top++] = task->idx;
}

static inline struct xio_fd_state *
fh_xio_acquire_fd_state (struct fh_xio *xio)
{
    if (!xio->free_fd_state_list_top)
    {
        errno = ENOMEM;
        return NULL;
    }

    struct xio_fd_state *state
        = &xio->fd_state_list
               [xio->free_fd_state_list[--xio->free_fd_state_list_top]];
    return state;
}

static inline void
fh_xio_release_fd_state (struct fh_xio *xio, struct xio_fd_state *state)
{
    assert (xio->free_fd_state_list_top < XIO_XPOLL_MAX_TASK_COUNT);
    xio->free_fd_state_list[xio->free_fd_state_list_top++] = state->idx;
}

static inline struct xio_slice *
fh_xio_acquire_slice (struct fh_xio *xio, size_t size)
{
    assert (size <= XIO_BUF_SIZE);

    if (!xio->free_buf_list_top)
    {
        errno = ENOBUFS;
        return NULL;
    }

    struct xio_slice *slice = NULL;

    for (uint16_t off = 0; off < xio->free_buf_list_top && !slice; off++)
    {
        const uint16_t idx
            = xio->free_buf_list[xio->free_buf_list_top - off - 1];

        if (xio->buf_list[idx].off + size > XIO_BUF_SIZE)
            continue;

        slice = &xio->buf_list[idx];
        slice->off += size;
        slice->refs++;

        if (slice->off == XIO_BUF_SIZE)
        {
            xio->free_buf_list[xio->free_buf_list_top - off - 1]
                = xio->free_buf_list[xio->free_buf_list_top - 1];
            xio->free_buf_list_top--;
        }

        break;
    }

    if (!slice)
        errno = ENOBUFS;

    return slice;
}

static inline void
fh_xio_release_slice (struct fh_xio *xio, struct xio_slice *slice)
{
    assert (xio->free_buf_list_top < XIO_BUF_COUNT);
    assert (slice->refs > 0);

    if (--slice->refs)
        return;

    slice->off = 0;
    xio->free_buf_list[xio->free_buf_list_top++] = slice->bid;
}

static bool
fh_xio_slices_alloc (struct fh_xio *xio)
{
    const size_t size = XIO_BUF_SIZE * XIO_BUF_COUNT;
    uint8_t *mem = MAP_FAILED;

#ifdef FH_PLATFORM_LINUX
    mem = mmap (NULL, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
#endif /* FH_PLATFORM_LINUX */

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

    xio->mem_base = mem;

    for (uint16_t i = 0; i < XIO_BUF_COUNT; i++)
    {
        xio->buf_list[i].buf = mem + (i * XIO_BUF_SIZE);
        xio->buf_list[i].bid = i;
        xio->buf_list[i].off = 0;
        xio->buf_list[i].refs = 0;
        xio->free_buf_list[i] = i;
    }

    for (uint16_t i = 0; i < XIO_XPOLL_MAX_TASK_COUNT; i++)
    {
        xio->task_list[i].flags = 0;
        xio->task_list[i].idx = i;
        xio->fd_state_list[i].idx = i;
        xio->free_task_list[i] = i;
        xio->free_fd_state_list[i] = i;
    }

    xio->free_buf_list_top = XIO_BUF_COUNT;
    xio->free_task_list_top = XIO_XPOLL_MAX_TASK_COUNT;
    xio->free_fd_state_list_top = XIO_XPOLL_MAX_TASK_COUNT;

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

    xio->fd_state_table = int_htable_create (16);

    if (!xio->fd_state_table)
    {
        xpoll_close (xio->xp);
        free (xio);
        return NULL;
    }

    if (!fh_xio_slices_alloc (xio))
    {
        int_htable_free (xio->fd_state_table);
        xpoll_close (xio->xp);
        free (xio);
        return NULL;
    }

    return xio;
}

void
fh_xio_free (struct fh_xio *xio)
{
    int_htable_free (xio->fd_state_table);
    munmap (xio->mem_base, XIO_BUF_COUNT * XIO_BUF_SIZE);
    xpoll_close (xio->xp);
    free (xio);
}

bool
fh_xio_submit_requests (struct fh_xio *xio)
{
    (void) xio;
    return true;
}

struct xio_request_prep
{
    struct xio_task *task;
    struct xio_slice *slice;
};

static inline uint8_t *
fh_xio_request_prep_slice_get_buffer (struct xio_slice *slice,
                                      size_t alloc_size)
{
    assert (alloc_size <= UINT16_MAX);
    assert (slice->off >= (uint32_t) alloc_size);
    return slice->buf + slice->off - (uint16_t) alloc_size;
}

enum xio_request_prep_type
{
    XIO_PREP_IN = 1,
    XIO_PREP_OUT,
};

static inline struct xio_request_prep
fh_xio_request_prep_with_size (struct fh_xio *xio, enum fh_xio_op op,
                               enum xio_request_prep_type prep_type, fd_t fd,
                               void *udata, uint8_t *buf, size_t size)
{
    assert (buf || size <= XIO_BUF_SIZE);
    struct xio_task *task = fh_xio_acquire_task (xio);

    if (!task)
        return (struct xio_request_prep){ NULL, NULL };

    struct xio_slice *slice = NULL;

    if (buf)
    {
        task->flags = 0;
    }
    else
    {
        slice = fh_xio_acquire_slice (xio, size);

        if (!slice)
        {
            fh_xio_release_task (xio, task);
            return (struct xio_request_prep){ NULL, NULL };
        }

        task->flags = XIO_TASK_F_BUFFER;
        task->bid = slice->bid;
    }

    task->op = op;
    task->udata = udata;

    xpoll_event_type_t events = 0;

    switch (prep_type)
    {
        case XIO_PREP_IN:
            events = XPOLL_READ;
            break;

        case XIO_PREP_OUT:
            events = XPOLL_WRITE;
            break;

        default:
            assert (false);
    }

    struct xio_fd_state *state
        = int_htable_get (xio->fd_state_table, (uint64_t) fd);
    bool created = false;

    if (!state)
    {
        state = fh_xio_acquire_fd_state (xio);

        if (!state)
        {
            fh_xio_release_task (xio, task);

            if (slice)
                fh_xio_release_slice (xio, slice);

            return (struct xio_request_prep){ NULL, NULL };
        }

        state->events = XPOLL_EDGE | XPOLL_READ_HANGUP | XPOLL_HANGUP;
        state->fd = fd;
        state->task_in_head = state->task_out_head = NULL;
        created = true;
        assert (int_htable_set (xio->fd_state_table, (uint64_t) fd, state));
    }

    if ((state->events & events) != events)
    {
        int rc = 0;

        if (created)
            rc = xpoll_add_fd (xio->xp, fd, state, state->events | events);
        else
            rc = xpoll_modify_fd (xio->xp, fd, state, state->events | events);

        if (rc)
        {
            if (created)
            {
                int_htable_delete (xio->fd_state_table, (uint64_t) fd);
                fh_xio_release_fd_state (xio, state);
            }

            fh_xio_release_task (xio, task);

            if (slice)
                fh_xio_release_slice (xio, slice);

            return (struct xio_request_prep){ NULL, NULL };
        }

        state->events |= events;
    }

    switch (prep_type)
    {
        case XIO_PREP_IN:
            task->next = state->task_in_head;
            state->task_in_head = task;
            break;

        case XIO_PREP_OUT:
            task->next = state->task_out_head;
            state->task_out_head = task;
            break;

        default:
            assert (false);
    }

    return (struct xio_request_prep){ .slice = slice, .task = task };
}

static inline int
fh_xio_errno (void)
{
    return errno ? errno : ENOTRECOVERABLE;
}

int
fh_xio_request_recv (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                     size_t size, int flags)
{
    struct xio_request_prep prep = fh_xio_request_prep_with_size (
        xio, XIO_OP_RECV, XIO_PREP_IN, fd, udata, buf, size);

    if (!prep.task)
        return -fh_xio_errno ();

    prep.task->opdata.recv.fd = fd;
    prep.task->opdata.recv.size = size;
    prep.task->opdata.recv.flags = flags;
    prep.task->opdata.recv.buf
        = buf ? buf : fh_xio_request_prep_slice_get_buffer (prep.slice, size);

    return 0;
}

static inline bool
fh_xio_add_result (struct fh_xio_result *results,
                   const struct fh_xio_result *result, size_t *index,
                   size_t max_results)
{
    if (*index >= max_results)
        return false;

    results[(*index)++] = *result;
    return true;
}

ssize_t
fh_xio_wait (struct fh_xio *xio, struct fh_xio_result *results,
             size_t max_results, uint64_t timeout_ms)
{
    max_results = MIN_VALUE (max_results, XIO_XPOLL_MAX_EVENTS);
    int count = xpoll_wait (xio->xp, xio->events, max_results, timeout_ms);

    if (count < 0)
        return count;

    size_t result_index = 0;

    for (int i = 0; i < count; i++)
    {
        const xpoll_event_t *event = &xio->events[i];
        const uint32_t mask = XPOLL_EVENT_MASK (event);
        struct xio_fd_state *state = XPOLL_EVENT_UDATA (event);

        if (mask & XPOLL_ERROR)
        {
            while (state->task_in_head)
            {
                struct xio_task *task = state->task_in_head;
                struct fh_xio_result result = {
                    .buf = NULL,
                    .flags = XIO_TASK_F_INTERNAL_ERROR,
                    .res = -1,
                    .udata = state->task_in_head->udata,
                    .op = state->task_in_head->op,
                };

                if (!fh_xio_add_result (results, &result, &result_index,
                                        max_results))
                    goto loop_end;

                state->task_in_head = state->task_in_head->next;
                fh_xio_release_task (xio, task);
            }

            while (state->task_out_head)
            {
                struct xio_task *task = state->task_out_head;
                struct fh_xio_result result = {
                    .buf = NULL,
                    .flags = XIO_TASK_F_INTERNAL_ERROR,
                    .res = -1,
                    .udata = state->task_out_head->udata,
                    .op = state->task_out_head->op,
                };

                if (!fh_xio_add_result (results, &result, &result_index,
                                        max_results))
                    goto loop_end;

                state->task_out_head = state->task_out_head->next;
                fh_xio_release_task (xio, task);
            }
        }

        // TODO
    }
loop_end:

    return (ssize_t) result_index;
}
