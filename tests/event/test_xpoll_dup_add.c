/* Registering an fd that is already registered must fail, and a single
   xpoll_remove_fd() must be enough to unregister it.

   These two go together.  A backend that lets the same fd in twice does
   not just disagree with epoll's EEXIST: it also makes removal a lie,
   because taking one registration away leaves the other one firing.  A
   caller that adds an fd, later removes it and then closes it would keep
   getting events for a descriptor number it no longer owns.

   The generic poll() backend used to append unconditionally and fail
   here; XPOLL_CTL_ADD now rejects a descriptor it already holds, so it
   passes under check-xpoll-generic-backend along with the others.  Do
   not relax the checks below: they are what keeps that fixed.

   kqueue is a different case and is exempted rather than tracked.  Its
   EV_ADD is idempotent: re-adding a filter updates the existing one in
   place, so there is no duplicate to reject and none to outlive the
   removal either.  Only the rejection is skipped there; the removal
   below still has to silence the fd, and that is checked everywhere. */

#include "libtest.h"
#include "test_xpoll_common.h"

static xpoll_t xp;
static fd_t pair[2];

static int
before_all (void)
{
    xp = xpoll_create (XPOLL_CLOEXEC);
    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");
    assert_true (xpoll_test_socketpair (pair), "failed to create socket pair");
    assert_true (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ),
        "fd add failed");

    return ASSERT_OK;
}

static int
after_all (void)
{
    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    return ASSERT_OK;
}

static int
test_xpoll_duplicate_add (void)
{
#ifndef FH_PLATFORM_BSDLIKE
    assert_false (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ),
        "adding an already-registered fd succeeded; use xpoll_modify_fd() "
        "to change an interest set");
#else
    assert_true (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ),
        "re-adding a registered fd failed; kqueue's EV_ADD updates an "
        "existing filter in place and does not report a conflict");
#endif

    return ASSERT_OK;
}

/* One removal, for one registration. */

static int
test_xpoll_single_removal_is_enough (void)
{
    xpoll_event_t events[16];

    check_true (xpoll_remove_fd (xp, pair[0]));
    check_true (xpoll_test_write_byte (pair[1]));

    int ret = xpoll_test_wait (xp, events, 16, 0);

    assert_equal (ret, 0,
                  "fd %d still reported after xpoll_remove_fd() (%d entries); "
                  "a duplicate registration outlived the removal",
                  pair[0], ret);

    return ASSERT_OK;
}

/* Re-adding after a genuine removal is fine, and must be reported once
   more.  This also catches a backend that removed the entry but left
   stale bookkeeping behind. */

static int
test_xpoll_readd_after_removal (void)
{
    xpoll_event_t events[16];

    check_true (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ));

    int ret = xpoll_test_wait (xp, events, 16, 1000);

    check_true (ret >= 1);
    check_true (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

#ifndef FH_PLATFORM_BSDLIKE
    /* kqueue reports one entry per filter and xpoll_wait() deliberately
       passes that through, so an entry count is only meaningful on the
       backends that coalesce.  Here the fd is registered for reads only
       and once, so exactly one entry is expected. */
    assert_equal (xpoll_test_count_for (events, ret, pair[0]), 1,
                  "fd %d produced %d entries for a single read registration",
                  pair[0], xpoll_test_count_for (events, ret, pair[0]));
#endif

    check_true (xpoll_remove_fd (xp, pair[0]));

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_duplicate_add),
        define_test_case(test_xpoll_single_removal_is_enough),
        define_test_case(test_xpoll_readd_after_removal),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
