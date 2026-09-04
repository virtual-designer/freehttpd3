/* Timeout contract, shared with poll(2)/epoll_wait(2): 0 means "return
   immediately", a positive value waits at most that many milliseconds,
   and a negative value blocks until something is ready. */

#include <time.h>

#include "libtest.h"
#include "test_xpoll_common.h"

static xpoll_t xp;
static fd_t pair[2];

static long
elapsed_ms_since (const struct timespec *start)
{
    struct timespec now;

    clock_gettime (CLOCK_MONOTONIC, &now);

    return (now.tv_sec - start->tv_sec) * 1000
           + (now.tv_nsec - start->tv_nsec) / 1000000;
}

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

/* A zero timeout polls and returns at once. */

static int
test_xpoll_zero_timeout_returns_at_once (void)
{
    xpoll_event_t events[8];
    struct timespec start;

    clock_gettime (CLOCK_MONOTONIC, &start);
    check_equal (xpoll_test_wait (xp, events, 8, 0), 0);

    long waited = elapsed_ms_since (&start);

    assert_true (waited < 100, "a zero timeout blocked for %ld ms", waited);

    return ASSERT_OK;
}

/* A positive timeout waits for roughly that long when nothing is ready.
   The bound is loose on purpose: this only has to catch a timeout that
   is ignored outright, not measure scheduler accuracy. */

static int
test_xpoll_positive_timeout_waits (void)
{
    xpoll_event_t events[8];
    struct timespec start;

    clock_gettime (CLOCK_MONOTONIC, &start);
    check_equal (xpoll_test_wait (xp, events, 8, 200), 0);

    long waited = elapsed_ms_since (&start);

    assert_true (waited >= 100, "a 200 ms timeout returned after %ld ms",
                 waited);

    /* And an upper bound, so that a backend which overshoots the timeout
       or blocks outright fails here rather than hanging until the test
       harness kills it.  The ceiling is deliberately far above 200 ms:
       this catches "waited forever", not scheduler jitter. */
    assert_true (waited < 2000, "a 200 ms timeout returned after %ld ms",
                 waited);

    return ASSERT_OK;
}

/* A negative timeout blocks until something is ready.  Data is made
   available first, so a correct implementation returns immediately and a
   broken one fails rather than hanging. */

static int
test_xpoll_infinite_timeout_returns_when_ready (void)
{
    xpoll_event_t events[8];
    struct timespec start;

    check_true (xpoll_test_write_byte (pair[1]));

    clock_gettime (CLOCK_MONOTONIC, &start);

    int ret = xpoll_test_wait (xp, events, 8, -1);

    assert_true (ret >= 1, "an infinite timeout returned %d with data pending",
                 ret);
    check_true (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

    long waited = elapsed_ms_since (&start);

    assert_true (waited < 2000,
                 "an infinite timeout took %ld ms to report a ready fd",
                 waited);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_zero_timeout_returns_at_once),
        define_test_case(test_xpoll_positive_timeout_waits),
        define_test_case(test_xpoll_infinite_timeout_returns_when_ready),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
