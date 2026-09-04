/* A peer that closes its end must wake the poller.

   Which bit reports it is deliberately not asserted: epoll sets
   EPOLLIN|EPOLLHUP, kqueue sets EV_EOF (mapped to XPOLL_HANGUP), and
   poll() sets POLLIN|POLLHUP, so the portable contract is only that the
   fd is reported at all and that a read then sees EOF. */

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
    /* pair[1] is closed by the test cases themselves; closing it again
       here would report a spurious failure. */
    close (pair[0]);
    xpoll_close (xp);

    return ASSERT_OK;
}

static int
test_xpoll_live_peer_is_quiet (void)
{
    xpoll_event_t events[8];

    return check_equal (xpoll_test_wait (xp, events, 8, 0), 0);
}

static int
test_xpoll_peer_close_wakes_poller (void)
{
    xpoll_event_t events[8];

    close (pair[1]);

    int ret = xpoll_test_wait (xp, events, 8, 1000);

    assert_true (ret >= 1, "peer close did not wake the poller (ret=%d)", ret);

    uint32_t mask = xpoll_test_events_for (events, ret, pair[0]);

    assert_true (mask & (XPOLL_READ | XPOLL_HANGUP),
                 "peer close reported as neither readable nor hangup "
                 "(events=0x%x)",
                 mask);

    return ASSERT_OK;
}

/* The wake-up must be genuine: the fd is at EOF, not merely spinning. */

static int
test_xpoll_hangup_reaches_eof (void)
{
    char buf[16];

    return check_equal (read (pair[0], buf, sizeof (buf)), 0);
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_live_peer_is_quiet),
        define_test_case(test_xpoll_peer_close_wakes_poller),
        define_test_case(test_xpoll_hangup_reaches_eof),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
