/* xpoll_modify_fd() must replace the interest set, not add to it, and
   xpoll_remove_fd() must silence an fd whatever it was registered for.

   Note for the kqueue backend: modifying to a write-only interest set and
   then removing the fd exercises xpoll_ctl_fd()'s early return, which
   gives up on the write filter as soon as the read filter's EV_DELETE
   fails with ENOENT. */

#include "test-xpoll-common.h"

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    fd_t pair[2];
    CHECK (xpoll_test_socketpair (pair));
    CHECK (xpoll_add_fd (xp, pair[0], XPOLL_READ));

    /* Make the fd both readable and writable, so that a stale read
       registration is visible in the results. */
    CHECK (xpoll_test_write_byte (pair[1]));

    xpoll_event_t events[8];
    int ret = xpoll_test_wait (xp, events, 8, 1000);
    CHECK (ret >= 1);

    uint32_t mask = xpoll_test_events_for (events, ret, pair[0]);
    CHECK (mask & XPOLL_READ);
    CHECK_MSG ((mask & XPOLL_WRITE) == 0,
               "write readiness reported for a read-only registration");

    /* Switch the interest set over to writes only. */
    CHECK (xpoll_modify_fd (xp, pair[0], XPOLL_WRITE));

    ret = xpoll_test_wait (xp, events, 8, 1000);
    CHECK (ret >= 1);

    mask = xpoll_test_events_for (events, ret, pair[0]);
    CHECK (mask & XPOLL_WRITE);
    CHECK_MSG ((mask & XPOLL_READ) == 0,
               "read readiness still reported after modifying to write-only "
               "(stale registration)");

    /* After removal the fd must go quiet even though it is still readable
       and writable. */
    CHECK (xpoll_remove_fd (xp, pair[0]));
    CHECK_MSG (xpoll_test_wait (xp, events, 8, 0) == 0,
               "events still reported after xpoll_remove_fd()");

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    return test_report ();
}
