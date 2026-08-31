/* Read readiness: an fd becomes ready when its peer writes, stays ready
   while the data is unread (all three backends are level-triggered by
   default), and goes quiet once drained. */

#include "test-xpoll-common.h"

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    fd_t pair[2];
    CHECK (xpoll_test_socketpair (pair));
    CHECK (xpoll_add_fd (xp, pair[0], XPOLL_READ));

    xpoll_event_t events[8];

    /* Nothing written yet. */
    CHECK (xpoll_test_wait (xp, events, 8, 0) == 0);

    CHECK (xpoll_test_write_byte (pair[1]));

    int ret = xpoll_test_wait (xp, events, 8, 1000);
    CHECK (ret >= 1);
    CHECK_MSG (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ,
               "read readiness not reported for fd %d (ret=%d)", pair[0], ret);

    /* Level-triggered: the same event is reported again until the data is
       actually read. */
    ret = xpoll_test_wait (xp, events, 8, 0);
    CHECK (ret >= 1);
    CHECK (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

    CHECK (xpoll_test_drain (pair[0]));
    CHECK (xpoll_test_wait (xp, events, 8, 0) == 0);

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    /* XPOLL_EDGE is a hint: epoll and kqueue honour it, the poll backend
       ignores it.  What every backend must agree on is that the first
       readiness after registration is still reported. */
    xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));
    CHECK (xpoll_test_socketpair (pair));
    CHECK (xpoll_add_fd (xp, pair[0], XPOLL_READ | XPOLL_EDGE));
    CHECK (xpoll_test_write_byte (pair[1]));

    ret = xpoll_test_wait (xp, events, 8, 1000);
    CHECK (ret >= 1);
    CHECK (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    return test_report ();
}
