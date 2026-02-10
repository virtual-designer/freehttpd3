#ifndef FHTTPD_XPOLL_H
#define FHTTPD_XPOLL_H

#include "utils/platform.h"

#if PLATFORM_LINUX
#   include <sys/epoll.h>
#elif PLATFORM_BSD
#   include <sys/time.h>
#   include <sys/event.h>
#endif /* PLATFORM_BSD */

enum xpoll_event_type
{
#if PLATFORM_LINUX
    XPOLL_IN = EPOLLIN,
    XPOLL_OUT = EPOLLOUT,
#elif PLATFORM_BSD
    XPOLL_IN = EVFILT_READ,
    XPOLL_OUT = EVFILT_WRITE,
#else /* not PLATFORM_BSD */
#   error "This platform is not supported"
#endif
};

enum xpoll_event_option
{
    XPOLL_ET,
    XPOLL_ONESHOT,
};

typedef enum xpoll_event_type xevent_type_t;
typedef enum xpoll_event_option xevent_opt_t;

struct xpoll
{
    int fd;
};

union xpoll_event
{
#if PLATFORM_LINUX
    struct epoll_event linux_ev;
#elif PLATFORM_BSD
    struct kevent bsd_ev;
#else /* not PLATFORM_BSD */
#   error "This platform is not supported"
#endif
};

#if PLATFORM_LINUX
#   define XPOLL_EVENT_RAW(ev) ((ev)->linux_ev)
#elif PLATFORM_BSD
#   define XPOLL_EVENT_RAW(ev) ((ev)->bsd_ev)
#else /* not PLATFORM_BSD */
#   error "This platform is not supported"
#endif

typedef union xpoll_event xevent_t;

struct xpoll *xpoll_create (void);
void xpoll_destroy (struct xpoll *xp);
int xpoll_register_fd (struct xpoll *xp, int fd, xevent_type_t events, xevent_opt_t opts);
int xpoll_unregister_fd (struct xpoll *xp, int fd, xevent_type_t events, xevent_opt_t opts);
int xpoll_modify_registered_fd (struct xpoll *xp, int fd, xevent_type_t events, xevent_opt_t opts);

#endif /* FHTTPD_XPOLL_H */