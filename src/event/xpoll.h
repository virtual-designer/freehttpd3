#ifndef FH_XPOLL_H
#define FH_XPOLL_H

#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "utils/compat.h"
#include "utils/types.h"

#if defined(FH_PLATFORM_LINUX)
#    include <sys/epoll.h>

enum xpoll_event_type
{
    XPOLL_READ = EPOLLIN,
    XPOLL_WRITE = EPOLLOUT,
    XPOLL_ERROR = EPOLLERR,
    XPOLL_HANGUP = EPOLLHUP,
};

enum xpoll_create_flag
{
    XPOLL_CLOEXEC = EPOLL_CLOEXEC
};

#    define XPOLL_EDGE EPOLLET
#elif defined(FH_PLATFORM_BSDLIKE)
#    include <sys/event.h>

enum xpoll_event_type
{
    XPOLL_READ = 1U << 4,
    XPOLL_WRITE = 1U << 5,
    XPOLL_ERROR = 1U << 6,
    XPOLL_HANGUP = 1U << 7,
    XPOLL_EDGE = 1U << 8
};

enum xpoll_create_flag
{
    XPOLL_CLOEXEC = 0x1
};
#else
#    include <poll.h>

enum xpoll_event_type
{
    XPOLL_READ = POLLIN,
    XPOLL_WRITE = POLLOUT,
    XPOLL_ERROR = POLLERR,
    XPOLL_HANGUP = POLLHUP,
    XPOLL_EDGE = 1 << 30
};

/* These have no effect if poll is used. */

enum xpoll_create_flag
{
    XPOLL_CLOEXEC = 0x1
};
#endif

#ifndef FH_PLATFORM_UNKNOWN
typedef fd_t xpoll_t;
#    define XPOLL_XP_ERR(xp) ((xp) < 0)
#else
struct xpoll;
typedef struct xpoll *xpoll_t;
#    define XPOLL_XP_ERR(xp) (!(xp))
#endif

typedef enum xpoll_event_type xpoll_event_type_t;

#if defined(FH_PLATFORM_LINUX)
typedef struct epoll_event xpoll_event_t;

/* The parameter names must not collide with the struct members they
   initialise: the preprocessor rewrites every occurrence of a parameter
   in the replacement list, designators included, so a parameter named
   "fd" would turn ".data.fd" into ".data.<argument>". */

#    define xpoll_wait epoll_wait
#    define xpoll_ctl_fd_(xp_, op_, fd_, events_)                              \
        (epoll_ctl (xp_, op_, fd_,                                             \
                    &(struct epoll_event) { .data.fd = (fd_),                  \
                                            .events = (events_) })             \
         == 0)
#    define xpoll_add_fd(xp_, fd_, events_)                                    \
        xpoll_ctl_fd_ (xp_, EPOLL_CTL_ADD, fd_, events_)
#    define xpoll_modify_fd(xp_, fd_, events_)                                 \
        xpoll_ctl_fd_ (xp_, EPOLL_CTL_MOD, fd_, events_)
#    define xpoll_remove_fd(xp_, fd_)                                          \
        (epoll_ctl (xp_, EPOLL_CTL_DEL, fd_, NULL) == 0)

#else
typedef struct xpoll_event
{
    uint32_t events;
    union
    {
        fd_t fd;
    } data;
} xpoll_event_t;

int xpoll_wait (xpoll_t xp, xpoll_event_t *events_out, int max_events,
                int timeout_ms);
bool xpoll_add_fd (xpoll_t xp, fd_t fd, xpoll_event_type_t events);
bool xpoll_modify_fd (xpoll_t xp, fd_t fd, xpoll_event_type_t events);
bool xpoll_remove_fd (xpoll_t xp, fd_t fd);
#endif

xpoll_t xpoll_create (enum xpoll_create_flag flags);
void xpoll_close (xpoll_t xp);

#endif /* FH_XPOLL_H */
