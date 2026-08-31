#ifndef FHTTPD_TEST_XPOLL_COMMON_H
#define FHTTPD_TEST_XPOLL_COMMON_H

#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

#include "event/xpoll.h"
#include "test-common.h"

/* The backends disagree on how a single fd is reported: epoll coalesces
   read and write readiness into one entry, while kqueue returns one entry
   per filter.  Tests must therefore aggregate results by fd rather than
   index into the array and assume a layout. */

static inline uint32_t
xpoll_test_events_for (const xpoll_event_t *events, int n, fd_t fd)
{
    uint32_t mask = 0;

    for (int i = 0; i < n; i++)
        if (events[i].data.fd == fd)
            mask |= events[i].events;

    return mask;
}

/* xpoll_wait() maps onto syscalls that may be interrupted by a signal;
   a test harness should not fail because of that. */

static inline int
xpoll_test_wait (xpoll_t xp, xpoll_event_t *events, int max_events,
                 int timeout_ms)
{
    int ret;

    do
        ret = xpoll_wait (xp, events, max_events, timeout_ms);
    while (ret < 0 && errno == EINTR);

    return ret;
}

static inline bool
xpoll_test_socketpair (fd_t out[2])
{
    return socketpair (AF_UNIX, SOCK_STREAM, 0, out) == 0;
}

static inline bool
xpoll_test_write_byte (fd_t fd)
{
    return write (fd, "x", 1) == 1;
}

static inline bool
xpoll_test_drain (fd_t fd)
{
    char buf[64];

    return read (fd, buf, sizeof (buf)) > 0;
}

static inline void
xpoll_test_close_pair (fd_t pair[2])
{
    close (pair[0]);
    close (pair[1]);
}

#endif /* FHTTPD_TEST_XPOLL_COMMON_H */
