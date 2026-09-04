/* Read readiness: an fd becomes ready when its peer writes, stays ready
   while the data is unread (all three backends are level-triggered by
   default), and goes quiet once drained. */

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

/* Nothing written yet. */

static int
test_xpoll_idle_fd_is_quiet (void)
{
    xpoll_event_t events[8];

    return check_equal (xpoll_test_wait (xp, events, 8, 0), 0);
}

static int
test_xpoll_read_readiness_reported (void)
{
    xpoll_event_t events[8];

    check_true (xpoll_test_write_byte (pair[1]));

    int ret = xpoll_test_wait (xp, events, 8, 1000);

    check_true (ret >= 1);
    assert_true (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ,
                 "read readiness not reported for fd %d (ret=%d)", pair[0],
                 ret);

    return ASSERT_OK;
}

/* Level-triggered: the same event is reported again until the data is
   actually read. */

static int
test_xpoll_readiness_is_level_triggered (void)
{
    xpoll_event_t events[8];
    int ret = xpoll_test_wait (xp, events, 8, 0);

    check_true (ret >= 1);
    check_true (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

    return ASSERT_OK;
}

static int
test_xpoll_drained_fd_goes_quiet (void)
{
    xpoll_event_t events[8];

    check_true (xpoll_test_drain (pair[0]));
    check_equal (xpoll_test_wait (xp, events, 8, 0), 0);

    return ASSERT_OK;
}

/* XPOLL_EDGE is a hint: epoll and kqueue honour it, the poll backend
   ignores it.  What every backend must agree on is that the first
   readiness after registration is still reported. */

static int
test_xpoll_edge_flag_reports_first_readiness (void)
{
    xpoll_event_t events[8];

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    xp = xpoll_create (XPOLL_CLOEXEC);
    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");
    check_true (xpoll_test_socketpair (pair));
    check_true (xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]),
                              XPOLL_READ | XPOLL_EDGE));
    check_true (xpoll_test_write_byte (pair[1]));

    int ret = xpoll_test_wait (xp, events, 8, 1000);

    check_true (ret >= 1);
    check_true (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_idle_fd_is_quiet),
        define_test_case(test_xpoll_read_readiness_reported),
        define_test_case(test_xpoll_readiness_is_level_triggered),
        define_test_case(test_xpoll_drained_fd_goes_quiet),
        define_test_case(test_xpoll_edge_flag_reports_first_readiness),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
