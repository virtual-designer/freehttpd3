#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/compat.h"
#include "utils/utils.h"
#include "xpoll.h"

#ifdef xpoll_wait
    #undef xpoll_wait
    #undef xpoll_add_fd
    #undef xpoll_modify_fd
    #undef xpoll_remove_fd
#endif /* xpoll_wait */

#if defined(FH_PLATFORM_BSDLIKE)
struct xpoll
{
    fd_t kq;
    struct kevent event_list[XPOLL_MAX_EVENTS];
};
#elif defined(FH_PLATFORM_UNKNOWN)
struct xpoll_fd_info
{
    void *udata;
};

struct xpoll
{
    uint32_t pfd_next_idx;
    uint32_t pfd_count;
    uint32_t pfd_cap;
    uint32_t fd_table_cap;
    uint32_t pfd_last_wait_index;
    uint32_t *fd_table;
    struct pollfd *pfd_list;
    struct xpoll_fd_info *fd_info_list;
};

    #define XPOLL_CTL_ADD 0x1
    #define XPOLL_CTL_MOD 0x2
    #define XPOLL_CTL_DEL 0x3
#endif

xpoll_t
xpoll_create (enum xpoll_create_flag flags)
{
#if defined(FH_PLATFORM_LINUX)
    return epoll_create1 (flags);
#elif defined(FH_PLATFORM_BSDLIKE)
    int err;
    struct xpoll *xp = calloc (1, sizeof (*xp));

    if (!xp)
        return NULL;

    int kq = kqueue ();

    if (kq < 0)
    {
        free (xp);
        return NULL;
    }

    int fd_flags = fcntl (kq, F_GETFD);

    if (fd_flags < 0)
        goto xpoll_create_err;

    if (fcntl (kq, F_SETFD,
               flags & XPOLL_CLOEXEC ? (fd_flags | FD_CLOEXEC)
                                     : (fd_flags & ~FD_CLOEXEC))
        < 0)
        goto xpoll_create_err;

    xp->kq = kq;
    return xp;

xpoll_create_err:
    err = errno;
    free (xp);
    close (kq);
    errno = err;
    return NULL;
#else
    (void) flags;

    struct xpoll *xp = calloc (1, sizeof (*xp));

    if (!xp)
        return NULL;

    return xp;
#endif
}

#ifndef FH_PLATFORM_LINUX
static bool
xpoll_ctl_fd (xpoll_t xp, fd_t fd, void *udata, int op_bsd, int op_generic,
              xpoll_event_type_t events, bool ret_on_partial_failure)
{
    #if defined(FH_PLATFORM_BSDLIKE)
    (void) op_generic;

    bool ret = false;
    struct kevent event_list[2] = { 0 };
    int count = 0;
    int mask = (events & XPOLL_EDGE) == XPOLL_EDGE ? EV_CLEAR : 0;

    if (events & XPOLL_READ)
    {
        EV_SET (&event_list[0], fd, EVFILT_READ, op_bsd | mask, 0, 0, udata);

        if (kevent (xp->kq, &event_list[0], 1, NULL, 0, NULL) < 0)
        {
            if (ret_on_partial_failure)
                return false;
        }
        else
        {
            ret = true;
        }

        count++;
    }

    if (events & XPOLL_WRITE)
    {
        EV_SET (&event_list[1], fd, EVFILT_WRITE, op_bsd | mask, 0, 0, udata);

        if (kevent (xp->kq, &event_list[1], 1, NULL, 0, NULL) < 0)
        {
            if (ret_on_partial_failure)
            {
                if (op_bsd == EV_ADD && (events & XPOLL_READ))
                {
                    event_list[0].flags = EV_DELETE;
                    int err = errno;
                    kevent (xp->kq, &event_list[0], 1, NULL, 0, NULL);
                    errno = err;
                }

                return false;
            }
        }
        else
        {
            ret = true;
        }

        count++;
    }

    if (!count)
    {
        errno = EINVAL;
        return false;
    }

    return ret;

    #elif defined(FH_PLATFORM_UNKNOWN)

    (void) op_bsd;
    (void) ret_on_partial_failure;

    switch (op_generic)
    {
        case XPOLL_CTL_ADD:
            {
                if (fd >= xp->fd_table_cap)
                {
                    const uint32_t new_cap = xp->fd_table_cap < 16 ? 16
                                             : xp->fd_table_cap < (1U << 16U)
                                                 ? (xp->fd_table_cap << 1)
                                                 : (fd + 1);

                    uint32_t *new_fds = realloc (
                        xp->fd_table, sizeof (*xp->fd_table) * new_cap);

                    if (!new_fds)
                        return false;

                    memset (new_fds + xp->fd_table_cap, UINT32_MAX,
                            (new_cap - xp->fd_table_cap) * sizeof (*xp->fd_table));

                    xp->fd_table = new_fds;
                    xp->fd_table_cap = new_cap;
                }
                else if (xp->fd_table[fd] != UINT32_MAX)
                {
                    errno = EEXIST;
                    return false;
                }

                if (xp->pfd_count >= xp->pfd_cap)
                {
                    const uint32_t new_cap
                        = xp->pfd_cap < 16 ? 16 : (xp->pfd_cap << 1);
                    struct pollfd *new_pfd_list = realloc (
                        xp->pfd_list, sizeof (*xp->pfd_list) * new_cap);

                    if (!new_pfd_list)
                        return false;

                    xp->pfd_list = new_pfd_list;

                    struct xpoll_fd_info *new_fd_info_list = realloc (
                        xp->fd_info_list, sizeof (*xp->fd_info_list) * new_cap);

                    if (!new_fd_info_list)
                        return false;

                    xp->fd_info_list = new_fd_info_list;
                    xp->pfd_cap = new_cap;
                }

                const uint32_t idx = xp->pfd_count++;

                xp->pfd_list[idx].fd = fd;
                xp->pfd_list[idx].events = events & ~XPOLL_EDGE;
                xp->pfd_list[idx].revents = 0;
                xp->fd_info_list[idx].udata = udata;
                xp->fd_table[fd] = idx;

                return true;
            }

        case XPOLL_CTL_MOD:
            if (!xp->pfd_count || fd >= xp->fd_table_cap
                || xp->fd_table[fd] == UINT32_MAX)
            {
                errno = ENOENT;
                return false;
            }

            const uint32_t idx = xp->fd_table[fd];
            xp->pfd_list[idx].events = events & ~XPOLL_EDGE;
            xp->fd_info_list[idx].udata = udata;
            return true;

        case XPOLL_CTL_DEL:
            {
                if (!xp->pfd_count || fd >= xp->fd_table_cap
                    || xp->fd_table[fd] == UINT32_MAX)
                {
                    errno = ENOENT;
                    return false;
                }

                const uint32_t idx = xp->fd_table[fd];
                xp->fd_table[fd] = -1;

                if (xp->pfd_count > 1)
                {
                    xp->pfd_list[idx] = xp->pfd_list[xp->pfd_count - 1];
                    xp->fd_info_list[idx] = xp->fd_info_list[xp->pfd_count - 1];
                }

                xp->pfd_count--;

                if (xp->pfd_cap > 16 && xp->pfd_count < (xp->pfd_cap >> 2))
                {
                    const size_t new_cap = xp->pfd_cap >> 2;

                    struct pollfd *new_pfds = realloc (
                        xp->pfd_list, sizeof (*xp->pfd_list) * new_cap);

                    struct xpoll_fd_info *fd_info_list = realloc (
                        xp->fd_info_list, sizeof (*xp->fd_info_list) * new_cap);

                    if (new_pfds)
                        xp->pfd_list = new_pfds;

                    if (fd_info_list)
                        xp->fd_info_list = fd_info_list;

                    if (new_pfds || fd_info_list)
                        xp->pfd_cap = new_cap;
                }

                /* Do not attempt to shrink xp->fd_table as we don't know what
                   the largest fd stored is. */

                return true;
            }

        default:
            return false;
    }

    return false;
    #else
        #error "Unsupported platform"
    #endif
}

bool
xpoll_add_fd (xpoll_t xp, fd_t fd, void *udata, xpoll_event_type_t events)
{
    assert (fd >= 0);

    #if defined(FH_PLATFORM_BSDLIKE)
    return xpoll_ctl_fd (xp, fd, udata, EV_ADD, 0, events, true);
    #elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, udata, 0, XPOLL_CTL_ADD, events, true);
    #else
        #error "Unsupported platform"
    #endif
}

bool
xpoll_modify_fd (xpoll_t xp, fd_t fd, void *udata, xpoll_event_type_t events)
{
    #if defined(FH_PLATFORM_BSDLIKE)
    const int mask = (events & XPOLL_READ ? 0 : XPOLL_READ)
                     | (events & XPOLL_WRITE ? 0 : XPOLL_WRITE);

    if (mask)
        xpoll_ctl_fd (xp, fd, NULL, EV_DELETE, 0, mask, false);

    return xpoll_ctl_fd (xp, fd, udata, EV_ADD, 0, events, true);
    #elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, udata, 0, XPOLL_CTL_MOD, events, true);
    #else
        #error "Unsupported platform"
    #endif
}

bool
xpoll_remove_fd (xpoll_t xp, fd_t fd)
{
    #if defined(FH_PLATFORM_BSDLIKE)
    return xpoll_ctl_fd (xp, fd, NULL, EV_DELETE, 0, XPOLL_READ | XPOLL_WRITE,
                         false);
    #elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, NULL, 0, XPOLL_CTL_DEL,
                         XPOLL_READ | XPOLL_WRITE, false);
    #else
        #error "Unsupported platform"
    #endif
}

/* On Linux, xpoll_wait() maps to epoll_wait() directly, so event entries
   are not duplicated, yet on BSD-like systems, they may be duplicated since
   kevent treats the same fd with different event filters as different entry.
   This is intentional, and the caller must expect duplicated entries while
   processing events. */

int
xpoll_wait (xpoll_t xp, xpoll_event_t *events_out, int max_events,
            int timeout_ms)
{
    assert (max_events >= 0);

    #if defined(FH_PLATFORM_BSDLIKE)
    if (!max_events)
    {
        errno = EINVAL;
        return -1;
    }

    if (max_events > XPOLL_MAX_EVENTS)
        max_events = XPOLL_MAX_EVENTS;

    struct timespec ts = { .tv_sec = timeout_ms / 1000,
                           .tv_nsec = (timeout_ms % 1000) * 1000000 };

    struct timespec *ts_ptr = timeout_ms >= 0 ? &ts : NULL;
    int ret = kevent (xp->kq, NULL, 0, xp->event_list, max_events, ts_ptr);

    if (ret < 0)
        return ret;

    assert (max_events >= ret);

    for (int i = 0; i < ret; i++)
    {
        events_out[i].udata = xp->event_list[i].udata;
        events_out[i].events
            = xp->event_list[i].filter == EVFILT_READ    ? XPOLL_READ
              : xp->event_list[i].filter == EVFILT_WRITE ? XPOLL_WRITE
                                                         : 0;

        if (xp->event_list[i].flags & EV_ERROR)
            events_out[i].events |= XPOLL_ERROR;

        if (xp->event_list[i].flags & EV_EOF)
            events_out[i].events |= XPOLL_HANGUP;
    }

    return ret;
    #elif defined(FH_PLATFORM_UNKNOWN)
    if (unlikely (!max_events))
    {
        errno = EINVAL;
        return -1;
    }

    int ret = poll (xp->pfd_list, xp->pfd_count, timeout_ms);

    if (ret < 0)
        return ret;

    if (!xp->pfd_count)
        return 0;

    int count = 0;
    size_t limit = xp->pfd_count;
    bool rotated = false;
    size_t i;
    size_t begin = xp->pfd_last_wait_index % xp->pfd_count;

    for (i = begin; i < limit && count < max_events;)
    {
        if (xp->pfd_list[i].revents)
        {
            events_out[count].udata = xp->fd_info_list[i].udata;
            events_out[count].fd = xp->pfd_list[i].fd;
            events_out[count].events = xp->pfd_list[i].revents;

            if (xp->pfd_list[i].revents & POLLNVAL)
            {
                events_out[count].events |= XPOLL_ERROR;
                events_out[count].events &= ~POLLNVAL;
            }

            count++;
        }

        if (i + 1 >= xp->pfd_count && !rotated && begin != 0)
        {
            rotated = true;
            limit = begin;
            i = 0;
        }
        else
        {
            i++;
        }
    }

    xp->pfd_last_wait_index = i >= xp->pfd_count ? 0 : i;
    return count;
    #else
        #error "Unsupported platform"
    #endif
}
#endif

void
xpoll_close (xpoll_t xp)
{
#if defined(FH_PLATFORM_LINUX)
    close (xp);
#elif defined(FH_PLATFORM_BSDLIKE)
    close (xp->kq);
    free (xp);
#else
    free (xp->fd_table);
    free (xp->pfd_list);
    free (xp->fd_info_list);
    free (xp);
#endif
}
