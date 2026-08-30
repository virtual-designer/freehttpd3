#ifndef FH_XPOLL_H
#define FH_XPOLL_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "utils/compat.h"
#include "utils/types.h"

#if defined(FH_PLATFORM_LINUX)
#    include <sys/epoll.h>

enum xpoll_event_type
{
    XPOLL_IN = EPOLLIN,
    XPOLL_OUT = EPOLLOUT,
    XPOLL_ERR = EPOLLERR,
    XPOLL_HUP = EPOLLHUP,
};
#elif defined(FH_PLATFORM_BSDLIKE)
#    include <sys/event.h>

enum xpoll_event_type
{
    XPOLL_IN = 0x10,
    XPOLL_OUT,
    XPOLL_ERR,
    XPOLL_HUP,
};
#else
#    include <poll.h>

enum xpoll_event_type
{
    XPOLL_IN = POLLIN,
    XPOLL_OUT = POLLOUT,
    XPOLL_ERR = POLLERR,
    XPOLL_HUP = POLLHUP,
};
#endif

#ifdef FH_PLATFORM_LINUX
typedef struct epoll_event xpoll_event_t;
#else
typedef struct xpoll_event
{
    uint32_t events;
    union
    {
        fd_t fd;
    } data;
} xpoll_event_t;
#endif

typedef fd_t xpoll_t;
typedef enum xpoll_event_type xpoll_event_type_t;

xpoll_t xpoll_create (void);
bool xpoll_add_fd (xpoll_t xp, fd_t fd, xpoll_event_type_t events);
bool xpoll_modify_fd (xpoll_t xp, fd_t fd, xpoll_event_type_t events);
bool xpoll_remove_fd (xpoll_t xp, fd_t fd);

#ifdef FH_PLATFORM_LINUX
#define xpoll_wait epoll_wait
#else
int xpoll_wait (xpoll_t xp, xpoll_event_t *events_out, int max_events, int timeout_ms);
#endif

#endif /* FH_XPOLL_H */
