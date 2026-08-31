#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
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
    int kq = kqueue ();

    if (kq < 0)
        return kq;

    if ((flags & XPOLL_CLOEXEC) && fcntl (kq, F_SETFD, FD_CLOEXEC) < 0)
    {
        int err = errno;
        close (kq);
        errno = err;
        return -1;
    }

    return kq;
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
              xpoll_event_type_t events)
{
#if defined(FH_PLATFORM_BSDLIKE)
    (void) op_generic;

    struct kevent event = { 0 };
    int mask = (events & XPOLL_EDGE) == XPOLL_EDGE ? EV_CLEAR : 0;

    /* If registration fails, we silently ignore the already registered
       filters.  This is intentional, as if something goes wrong registering
       an event, something terribly went wrong and needs to be handled outside.
     */

    if ((events & XPOLL_READ) == XPOLL_READ)
    {
        EV_SET (&event, fd, EVFILT_READ, op_bsd | mask, 0, 0, NULL);

        if (kevent (xp, &event, 1, NULL, 0, NULL) < 0)
            return false;
    }

    if ((events & XPOLL_WRITE) == XPOLL_WRITE)
    {
        EV_SET (&event, fd, EVFILT_WRITE, op_bsd | mask, 0, 0, NULL);

        if (kevent (xp, &event, 1, NULL, 0, NULL) < 0)
            return false;
    }

    return true;
#elif defined(FH_PLATFORM_UNKNOWN)
    (void) op_bsd;

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
    return false;
#endif
}

bool
xpoll_add_fd (xpoll_t xp, fd_t fd, xpoll_event_type_t events)
{
#if defined(FH_PLATFORM_BSDLIKE)
    return xpoll_ctl_fd (xp, fd, EV_ADD, 0, events);
#elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, 0, XPOLL_CTL_ADD, events);
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
    xpoll_ctl_fd (xp, fd, EV_DELETE, 0, XPOLL_READ | XPOLL_WRITE);
    return xpoll_ctl_fd (xp, fd, EV_ADD, 0, events);
#elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, 0, XPOLL_CTL_MOD, events);
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
    return xpoll_ctl_fd (xp, fd, EV_DELETE, 0, XPOLL_READ | XPOLL_WRITE);
#elif defined(FH_PLATFORM_UNKNOWN)
    return xpoll_ctl_fd (xp, fd, 0, XPOLL_CTL_DEL, XPOLL_READ | XPOLL_WRITE);
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
    int ret = poll (xp->fds, xp->fd_count, timeout_ms);

    if (ret < 0)
        return ret;

    int count = 0;

    for (size_t i = 0; i < xp->fd_count && count < max_events; i++)
    {
        if (!xp->fds[i].revents)
            continue;

        events_out[count].data.fd = xp->fds[i].fd;
        events_out[count].events = xp->fds[i].revents;

        if (xp->fds[i].revents & POLLNVAL)
            events_out[count].events |= XPOLL_ERROR;

        count++;
    }

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
