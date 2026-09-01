#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/utils.h"
#include "xpoll.h"

#ifdef xpoll_wait
#    undef xpoll_wait
#    undef xpoll_add_fd
#    undef xpoll_modify_fd
#    undef xpoll_remove_fd
#endif /* xpoll_wait */

#ifdef FH_PLATFORM_UNKNOWN
struct xpoll
{
    struct pollfd *fds;
    size_t fd_count;
    size_t fd_cap;
    size_t fd_last_wait_index;
};

#    define XPOLL_CTL_ADD 0x1
#    define XPOLL_CTL_MOD 0x2
#    define XPOLL_CTL_DEL 0x3
#endif

xpoll_t
xpoll_create (enum xpoll_create_flag flags)
{
#if defined(FH_PLATFORM_LINUX)
    return epoll_create1 (flags);
#elif defined(FH_PLATFORM_BSDLIKE)
    int err;
    int kq = kqueue ();

    if (kq < 0)
        return kq;

    int fd_flags = fcntl (kq, F_GETFD);

    if (fd_flags < 0)
        goto xpoll_create_err;

    if (fcntl (kq, F_SETFD,
               flags & XPOLL_CLOEXEC ? (fd_flags | FD_CLOEXEC)
                                     : (fd_flags & ~FD_CLOEXEC))
        < 0)
        goto xpoll_create_err;

    return kq;

xpoll_create_err:
    err = errno;
    close (kq);
    errno = err;
    return -1;
#else
    (void) flags;

    struct xpoll *xp = calloc (1, sizeof (*xp));

    if (!xp)
        return NULL;

    return xp;
#endif
}

static bool __attribute__ ((unused))
xpoll_ctl_fd (xpoll_t xp, fd_t fd, int op_bsd, int op_generic,
              xpoll_event_type_t events, bool ret_on_failure)
{
#if defined(FH_PLATFORM_BSDLIKE)
    (void) op_generic;

    struct kevent event_read = { 0 };
    struct kevent event_write = { 0 };
    int mask = (events & XPOLL_EDGE) == XPOLL_EDGE ? EV_CLEAR : 0;
    bool ret = false;

    if ((events & XPOLL_READ) == XPOLL_READ)
    {
        EV_SET (&event_read, fd, EVFILT_READ, op_bsd | mask, 0, 0, NULL);

        if (kevent (xp, &event_read, 1, NULL, 0, NULL) < 0)
        {
            if (ret_on_failure)
                return false;
        }
        else
        {
            ret = true;
        }
    }

    if ((events & XPOLL_WRITE) == XPOLL_WRITE)
    {
        EV_SET (&event_write, fd, EVFILT_WRITE, op_bsd | mask, 0, 0, NULL);

        if (kevent (xp, &event_write, 1, NULL, 0, NULL) < 0)
        {
            if (ret_on_failure)
            {
                if (op_bsd == EV_ADD && (events & XPOLL_READ))
                {
                    event_read.flags = EV_DELETE;
                    kevent (xp, &event_read, 1, NULL, 0, NULL);
                }

                return false;
            }
        }
        else
        {
            ret = true;
        }
    }

    return ret;
#elif defined(FH_PLATFORM_UNKNOWN)
    (void) op_bsd;
    (void) ret_on_failure;

    switch (op_generic)
    {
        case XPOLL_CTL_ADD:
            if (xp->fd_count >= xp->fd_cap)
            {
                const size_t new_cap = xp->fd_cap < 16 ? 16 : (xp->fd_cap << 1);
                struct pollfd *new_fds
                    = realloc (xp->fds, sizeof (*xp->fds) * new_cap);

                if (!new_fds)
                    return false;

                xp->fds = new_fds;
                xp->fd_cap = new_cap;
            }

            xp->fds[xp->fd_count].fd = fd;
            xp->fds[xp->fd_count].events = events & ~XPOLL_EDGE;
            xp->fds[xp->fd_count].revents = 0;
            xp->fd_count++;

            return true;

        case XPOLL_CTL_MOD:
            for (size_t i = 0; i < xp->fd_count; i++)
            {
                if (xp->fds[i].fd == fd)
                {
                    xp->fds[i].events = events & ~XPOLL_EDGE;
                    return true;
                }
            }

            return false;

        case XPOLL_CTL_DEL:
            {
                bool found = false;
                size_t fd_index = 0;

                for (size_t i = 0; i < xp->fd_count; i++)
                {
                    if (xp->fds[i].fd == fd)
                    {
                        found = true;
                        fd_index = i;
                        break;
                    }
                }

                if (!found)
                    return false;

                memmove (xp->fds + fd_index, xp->fds + fd_index + 1,
                         sizeof (*xp->fds) * (xp->fd_count - fd_index - 1));
                xp->fd_count--;

                if (xp->fd_cap > 16 && xp->fd_count < (xp->fd_cap >> 2))
                {
                    const size_t new_cap = xp->fd_cap >> 2;
                    struct pollfd *new_fds
                        = realloc (xp->fds, sizeof (*xp->fds) * new_cap);

                    if (new_fds)
                    {
                        xp->fds = new_fds;
                        xp->fd_cap = new_cap;
                    }
                }

                return true;
            }

        default:
            return false;
    }

    return false;
#else
    (void) xp;
    (void) fd;
    (void) op_bsd;
    (void) op_generic;
    (void) events;
    (void) ret_on_failure;
    return false;
#endif
}

bool
xpoll_add_fd (xpoll_t xp, fd_t fd, xpoll_event_type_t events)
{
#if defined(FH_PLATFORM_BSDLIKE)
    return xpoll_ctl_fd (xp, fd, EV_ADD, 0, events, true);
#elif defined(FH_PLATFORM_UNKNOWN)
    assert (fd >= 0);
    return xpoll_ctl_fd (xp, fd, 0, XPOLL_CTL_ADD, events, true);
#else
    (void) xp;
    (void) fd;
    (void) events;
    return false;
#endif
}

bool
xpoll_modify_fd (xpoll_t xp, fd_t fd, xpoll_event_type_t events)
{
#if defined(FH_PLATFORM_BSDLIKE)
    xpoll_ctl_fd (xp, fd, EV_DELETE, 0, XPOLL_READ | XPOLL_WRITE, false);
    return xpoll_ctl_fd (xp, fd, EV_ADD, 0, events, true);
#elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, 0, XPOLL_CTL_MOD, events, true);
#else
    (void) xp;
    (void) fd;
    (void) events;
    return false;
#endif
}

bool
xpoll_remove_fd (xpoll_t xp, fd_t fd)
{
#if defined(FH_PLATFORM_BSDLIKE)
    return xpoll_ctl_fd (xp, fd, EV_DELETE, 0, XPOLL_READ | XPOLL_WRITE, false);
#elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, 0, XPOLL_CTL_DEL, XPOLL_READ | XPOLL_WRITE,
                         false);
#else
    (void) xp;
    (void) fd;
    return false;
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

    struct kevent events[max_events + 1];
    struct timespec ts = { .tv_sec = timeout_ms / 1000,
                           .tv_nsec = (timeout_ms % 1000) * 1000000 };

    struct timespec *ts_ptr = timeout_ms >= 0 ? &ts : NULL;
    int ret = kevent (xp, NULL, 0, events, max_events, ts_ptr);

    if (ret < 0)
        return ret;

    assert (max_events >= ret);

    for (int i = 0; i < ret; i++)
    {
        events_out[i].data.fd = events[i].ident;
        events_out[i].events = events[i].filter == EVFILT_READ    ? XPOLL_READ
                               : events[i].filter == EVFILT_WRITE ? XPOLL_WRITE
                                                                  : 0;

        if (events[i].flags & EV_ERROR)
            events_out[i].events |= XPOLL_ERROR;

        if (events[i].flags & EV_EOF)
            events_out[i].events |= XPOLL_HANGUP;
    }

    return ret;
#elif defined(FH_PLATFORM_UNKNOWN)
    if (!max_events)
    {
        errno = EINVAL;
        return -1;
    }

    int ret = poll (xp->fds, xp->fd_count, timeout_ms);

    if (ret < 0)
        return ret;

    if (!xp->fd_count)
        return 0;

    int count = 0;
    size_t limit = xp->fd_count;
    bool rotated = false;
    size_t i;
    size_t begin = xp->fd_last_wait_index % xp->fd_count;

    for (i = begin; i < limit && count < max_events;)
    {
        if (xp->fds[i].revents)
        {
            events_out[count].data.fd = xp->fds[i].fd;
            events_out[count].events = xp->fds[i].revents;

            if (xp->fds[i].revents & POLLNVAL)
            {
                events_out[count].events |= XPOLL_ERROR;
                events_out[count].events &= ~POLLNVAL;
            }

            count++;
        }

        if (i + 1 >= xp->fd_count && !rotated && begin != 0)
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

    xp->fd_last_wait_index = i >= xp->fd_count ? 0 : i;
    return count;
#else
    (void) xp;
    (void) events_out;
    (void) max_events;
    (void) timeout_ms;
    return -1;
#endif
}

void
xpoll_close (xpoll_t xp)
{
#if defined(FH_PLATFORM_LINUX) || defined(FH_PLATFORM_BSDLIKE)
    close (xp);
#else
    free (xp->fds);
    free (xp);
#endif
}
