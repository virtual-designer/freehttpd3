/* Registering an fd that is already registered must fail, and a single
   xpoll_remove_fd() must be enough to unregister it.

   These two go together.  A backend that lets the same fd in twice does
   not just disagree with epoll's EEXIST: it also makes removal a lie,
   because taking one registration away leaves the other one firing.  A
   caller that adds an fd, later removes it and then closes it would keep
   getting events for a descriptor number it no longer owns.

   Known gap: the generic poll() backend appends unconditionally and so
   fails this test today.  That is an accepted, tracked drawback rather
   than a stale assertion -- do not relax the checks below to make it
   pass; the fix belongs in XPOLL_CTL_ADD.

   kqueue is a different case and is exempted rather than tracked.  Its
   EV_ADD is idempotent: re-adding a filter updates the existing one in
   place, so there is no duplicate to reject and none to outlive the
   removal either.  Only the rejection is skipped there; the removal
   below still has to silence the fd, and that is checked everywhere. */

#include "test-xpoll-common.h"

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    fd_t pair[2];
    CHECK (xpoll_test_socketpair (pair));

    CHECK (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ));

#ifndef FH_PLATFORM_BSDLIKE
    CHECK_MSG (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ)
            == false,
        "adding an already-registered fd succeeded; use "
        "xpoll_modify_fd() to change an interest set");
#else
    CHECK_MSG (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ),
        "re-adding a registered fd failed; kqueue's EV_ADD updates an "
        "existing filter in place and does not report a conflict");
#endif

    /* One removal, for one registration. */
    CHECK (xpoll_remove_fd (xp, pair[0]));

    CHECK (xpoll_test_write_byte (pair[1]));

    xpoll_event_t events[16];
    int ret = xpoll_test_wait (xp, events, 16, 0);

    CHECK_MSG (ret == 0,
               "fd %d still reported after xpoll_remove_fd() (%d entries); "
               "a duplicate registration outlived the removal",
               pair[0], ret);

    /* Re-adding after a genuine removal is fine, and must be reported
       once more.  This also catches a backend that removed the entry but
       left stale bookkeeping behind. */
    CHECK (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ));

    ret = xpoll_test_wait (xp, events, 16, 1000);
    CHECK (ret >= 1);
    CHECK (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

#ifndef FH_PLATFORM_BSDLIKE
    /* kqueue reports one entry per filter and xpoll_wait() deliberately
       passes that through, so an entry count is only meaningful on the
       backends that coalesce.  Here the fd is registered for reads only
       and once, so exactly one entry is expected. */
    CHECK_MSG (xpoll_test_count_for (events, ret, pair[0]) == 1,
               "fd %d produced %d entries for a single read registration",
               pair[0], xpoll_test_count_for (events, ret, pair[0]));
#endif

    CHECK (xpoll_remove_fd (xp, pair[0]));

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    return test_report ();
}
