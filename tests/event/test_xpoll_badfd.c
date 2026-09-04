/* Argument handling that every backend agrees on, plus the descriptor
   validation that only some of them perform.

   Descriptor validity is deliberately NOT a portable guarantee here.
   The generic poll() backend treats "fd is a live descriptor" as a
   caller precondition: a negative fd trips an assert() rather than
   returning an error, and a closed one is stored as given, because
   poll(2) reports it as POLLNVAL rather than failing the registration.
   epoll validates eagerly instead and fails the call with EBADF.  Those
   checks are therefore scoped to the backends that make the promise, so
   that this test pins down real behaviour rather than an aspiration. */

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
test_xpoll_reject_bad_fd (void)
{
#ifdef FH_PLATFORM_LINUX
    assert_false (xpoll_add_fd (xp, -1, NULL, XPOLL_READ),
                  "a negative fd was accepted for registration");

    /* A descriptor that was valid but is closed by the time it is
       registered. */
    fd_t stale[2];
    check_true (xpoll_test_socketpair (stale));
    xpoll_test_close_pair (stale);

    assert_false (xpoll_add_fd (xp, stale[0], NULL, XPOLL_READ),
                  "a closed fd was accepted for registration");
#endif

    return ASSERT_OK;
}

/* Changing an interest set that was never established is an error, not a
   silent no-op and not an implicit add.

   kqueue cannot be made to agree and is not asked to.  EV_ADD is the
   only way to set a filter's parameters there, so a modify of an fd that
   was never registered installs it instead of failing; there is no
   "modify only" operation to fail with.  The check is therefore scoped
   to the backends that can promise it, and the registration kqueue
   leaves behind is undone here so that everything after this point
   starts from the same state on every backend. */

static int
test_xpoll_modify_unregistered (void)
{
#ifndef FH_PLATFORM_BSDLIKE
    assert_false (xpoll_modify_fd (xp, pair[0], NULL, XPOLL_READ),
                  "modifying an unregistered fd succeeded");
#else
    (void) xpoll_modify_fd (xp, pair[0], NULL, XPOLL_READ);
    check_true (xpoll_remove_fd (xp, pair[0]));
#endif

    return ASSERT_OK;
}

static int
test_xpoll_rejected_registration_is_quiet (void)
{
    xpoll_event_t events[16];

    check_true (xpoll_test_write_byte (pair[1]));
    assert_equal (xpoll_test_wait (xp, events, 16, 0), 0,
                  "a rejected registration still produced events");

    return ASSERT_OK;
}

/* max_events must be positive: epoll_wait(2) rejects zero with EINVAL and
   the other backends follow it explicitly, so that a caller passing an
   empty output buffer finds out instead of being told "nothing is ready"
   forever.

   xpoll_wait() is called directly here rather than through the harness
   helper, because the helper retries on EINTR and this call is expected
   to fail. */

static int
test_xpoll_zero_max_events (void)
{
    xpoll_event_t events[16];

    check_true (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ));

    errno = 0;

    int ret = xpoll_wait (xp, events, 0, 0);

    assert_true (ret < 0, "max_events=0 returned %d instead of failing", ret);
    assert_true (ret >= 0 || errno == EINVAL,
                 "max_events=0 failed with errno %d, expected EINVAL", errno);

    return ASSERT_OK;
}

/* The registration itself is still intact after the rejected call. */

static int
test_xpoll_registration_survives_rejection (void)
{
    xpoll_event_t events[16];
    int ret = xpoll_test_wait (xp, events, 16, 1000);

    check_true (ret >= 1);
    check_true (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);
    check_true (xpoll_remove_fd (xp, pair[0]));

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_reject_bad_fd),
        define_test_case(test_xpoll_modify_unregistered),
        define_test_case(test_xpoll_rejected_registration_is_quiet),
        define_test_case(test_xpoll_zero_max_events),
        define_test_case(test_xpoll_registration_survives_rejection),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
