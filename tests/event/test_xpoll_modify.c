/* xpoll_modify_fd() must replace the interest set, not add to it, and
   xpoll_remove_fd() must silence an fd whatever it was registered for.

   Note for the kqueue backend: after the modify below the fd carries only
   a write filter, so the removal's EV_DELETE for the read filter fails
   with ENOENT.  xpoll_remove_fd() passes ret_on_failure=false precisely
   so that this does not abandon the write filter, and the final
   assertion here is what pins that down: were the read filter's failure
   to stop the removal early, the fd would stay registered for writes and
   keep being reported. */

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

/* Make the fd both readable and writable, so that a stale read
   registration is visible in the results. */

static int
test_xpoll_read_only_registration (void)
{
    xpoll_event_t events[8];

    check_true (xpoll_test_write_byte (pair[1]));

    int ret = xpoll_test_wait (xp, events, 8, 1000);
    check_true (ret >= 1);

    uint32_t mask = xpoll_test_events_for (events, ret, pair[0]);

    check_true (mask & XPOLL_READ);
    assert_equal (mask & XPOLL_WRITE, 0,
                  "write readiness reported for a read-only registration");

    return ASSERT_OK;
}

/* Switch the interest set over to writes only. */

static int
test_xpoll_modify_replaces_interest_set (void)
{
    xpoll_event_t events[8];

    check_true (xpoll_modify_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]),
                                 XPOLL_WRITE));

    int ret = xpoll_test_wait (xp, events, 8, 1000);
    check_true (ret >= 1);

    uint32_t mask = xpoll_test_events_for (events, ret, pair[0]);

    check_true (mask & XPOLL_WRITE);
    assert_equal (mask & XPOLL_READ, 0,
                  "read readiness still reported after modifying to "
                  "write-only (stale registration)");

    return ASSERT_OK;
}

/* After removal the fd must go quiet even though it is still readable
   and writable. */

static int
test_xpoll_remove_silences_fd (void)
{
    xpoll_event_t events[8];

    check_true (xpoll_remove_fd (xp, pair[0]));
    assert_equal (xpoll_test_wait (xp, events, 8, 0), 0,
                  "events still reported after xpoll_remove_fd()");

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_read_only_registration),
        define_test_case(test_xpoll_modify_replaces_interest_set),
        define_test_case(test_xpoll_remove_silences_fd),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
