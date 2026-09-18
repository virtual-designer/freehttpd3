#ifndef FH_XPOLL_H
#define FH_XPOLL_H

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils/compat.h"
#include "utils/types.h"

#define XPOLL_MAX_EVENTS 1024

#if defined(FH_PLATFORM_LINUX)
    #include <sys/epoll.h>

enum xpoll_event_type
{
    XPOLL_READ = EPOLLIN,
    XPOLL_WRITE = EPOLLOUT,
    XPOLL_ERROR = EPOLLERR,
    XPOLL_READ_HANGUP = EPOLLRDHUP,
    XPOLL_HANGUP = EPOLLHUP,
};

enum xpoll_create_flag
{
    XPOLL_CLOEXEC = EPOLL_CLOEXEC
};

    #define XPOLL_EDGE EPOLLET
    #define XPOLL_EXCLUSIVE EPOLLEXCLUSIVE
#elif defined(FH_PLATFORM_BSDLIKE)
    #include <sys/event.h>

enum xpoll_event_type
{
    XPOLL_READ = EVFILT_READ,
    XPOLL_WRITE = EVFILT_WRITE,
    XPOLL_ERROR = 1U << 2,
    XPOLL_HANGUP = 1U << 3,
    XPOLL_READ_HANGUP = 1U << 4,
    XPOLL_EDGE = 1U << 5,
    XPOLL_EXCLUSIVE = 1U << 6
};

enum xpoll_create_flag
{
    XPOLL_CLOEXEC = 0x1
};
#else
    #include <poll.h>

enum xpoll_event_type
{
    XPOLL_READ = POLLIN,
    XPOLL_WRITE = POLLOUT,
    XPOLL_ERROR = POLLERR,
    XPOLL_HANGUP = POLLHUP,
    XPOLL_READ_HANGUP = POLLHUP,
    XPOLL_EDGE = 1 << 30,
    XPOLL_EXCLUSIVE = 1 << 31,
};

/* These have no effect if poll is used. */

enum xpoll_create_flag
{
    XPOLL_CLOEXEC = 0x1
};
#endif

#if defined(FH_PLATFORM_LINUX)
typedef fd_t xpoll_t;
    #define XPOLL_XP_ERR(xp) ((xp) < 0)
#elif defined(FH_PLATFORM_BSDLIKE)
struct xpoll;
typedef struct xpoll *xpoll_t;

    #define XPOLL_XP_ERR(xp) (!(xp))
#else
struct xpoll;
typedef struct xpoll *xpoll_t;
    #define XPOLL_XP_ERR(xp) (!(xp))
#endif

typedef enum xpoll_event_type xpoll_event_type_t;

#if defined(FH_PLATFORM_LINUX)
/* On Linux it is expected that you pass fd as udata whenever you
   want to access it that way, it is not automatically done for you. */

typedef struct epoll_event xpoll_event_t;

static FH_INLINE bool
xpoll_ctl_fd_ (xpoll_t xp, int op, fd_t fd, void *udata,
               xpoll_event_type_t events)
{
    return epoll_ctl (
               xp, op, fd,
               &(struct epoll_event) { .data.ptr = udata, .events = events })
           == 0;
}

    #define xpoll_wait epoll_wait
    #define xpoll_add_fd(xp_, fd_, udata, events_)                             \
        xpoll_ctl_fd_ (xp_, EPOLL_CTL_ADD, fd_, (void *) (udata), events_)
    #define xpoll_modify_fd(xp_, fd_, udata, events_)                          \
        xpoll_ctl_fd_ (xp_, EPOLL_CTL_MOD, fd_, (void *) (udata), events_)
    #define xpoll_remove_fd(xp_, fd_)                                          \
        (epoll_ctl (xp_, EPOLL_CTL_DEL, fd_, NULL) == 0)

    #define XPOLL_EVENT_FD(event) ((event)->data.fd)
    #define XPOLL_EVENT_UDATA(event) ((event)->data.ptr)
    #define XPOLL_EVENT_MASK(event) ((event)->events)
#else
typedef struct xpoll_event
{
    uint32_t events;
    fd_t fd;
    void *udata;
} xpoll_event_t;

    #define XPOLL_EVENT_FD(event) ((event)->fd)
    #define XPOLL_EVENT_UDATA(event) ((event)->udata)
    #define XPOLL_EVENT_MASK(event) ((event)->events)

int xpoll_wait (xpoll_t xp, xpoll_event_t *events_out, int max_events,
                int timeout_ms);
bool xpoll_add_fd (xpoll_t xp, fd_t fd, void *udata, xpoll_event_type_t events);
bool xpoll_modify_fd (xpoll_t xp, fd_t fd, void *udata,
                      xpoll_event_type_t events);
bool xpoll_remove_fd (xpoll_t xp, fd_t fd);
#endif

xpoll_t xpoll_create (enum xpoll_create_flag flags);
void xpoll_close (xpoll_t xp);
bool xpoll_add_or_modify_fd (xpoll_t xp, fd_t fd, void *udata,
                             xpoll_event_type_t events);

#endif /* FH_XPOLL_H */
