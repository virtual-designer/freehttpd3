/* Several fds registered at once: only the ready ones are reported, and
   a small max_events truncates the batch without losing the remainder,
   which the next call still reports. */

#include "libtest.h"
#include "test_xpoll_common.h"

#define PAIRS 4

static xpoll_t xp;
static fd_t pairs[PAIRS][2];

static int
before_all (void)
{
    xp = xpoll_create (XPOLL_CLOEXEC);
    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");

    for (int i = 0; i < PAIRS; i++)
    {
        assert_true (xpoll_test_socketpair (pairs[i]),
                     "failed to create socket pair %d", i);
        assert_true (xpoll_add_fd (xp, pairs[i][0],
                                   xpoll_test_fd_udata (pairs[i][0]),
                                   XPOLL_READ),
                     "registering fd %d failed", pairs[i][0]);
    }

    return ASSERT_OK;
}

static int
after_all (void)
{
    for (int i = 0; i < PAIRS; i++)
        xpoll_test_close_pair (pairs[i]);

    xpoll_close (xp);

    return ASSERT_OK;
}

/* Make every other pair readable. */

static int
test_xpoll_only_ready_fds_reported (void)
{
    xpoll_event_t events[16];

    check_true (xpoll_test_write_byte (pairs[0][1]));
    check_true (xpoll_test_write_byte (pairs[2][1]));

    int ret = xpoll_test_wait (xp, events, 16, 1000);
    check_true (ret >= 2);

    check_true (xpoll_test_events_for (events, ret, pairs[0][0]) & XPOLL_READ);
    check_true (xpoll_test_events_for (events, ret, pairs[2][0]) & XPOLL_READ);
    assert_equal (xpoll_test_events_for (events, ret, pairs[1][0]), 0,
                  "idle fd %d reported as ready", pairs[1][0]);
    assert_equal (xpoll_test_events_for (events, ret, pairs[3][0]), 0,
                  "idle fd %d reported as ready", pairs[3][0]);

    return ASSERT_OK;
}

/* max_events caps a batch.  Nothing is drained in between, so both fds
   stay ready throughout; what matters is that capping the batch does not
   pin the poller to one fd forever.  Asking for one entry at a time must
   therefore cover both readable fds within a few calls rather than
   returning the same one every time.

   Re-waiting with a large max_events would NOT test this: every backend
   is level-triggered here, so both fds come back regardless of how the
   capped calls behaved. */

static int
test_xpoll_capped_batches_rotate (void)
{
    xpoll_event_t events[16];
    bool seen_first = false, seen_third = false;

    for (int i = 0; i < 8 && !(seen_first && seen_third); i++)
    {
        int capped = xpoll_test_wait (xp, events, 1, 1000);

        assert_equal (capped, 1, "max_events=1 returned %d entries", capped);

        if (capped < 1)
            break;

        assert_true (xpoll_test_udata_fd (&events[0]) == pairs[0][0]
                         || xpoll_test_udata_fd (&events[0]) == pairs[2][0],
                     "capped batch reported unready fd %d",
                     xpoll_test_udata_fd (&events[0]));

        seen_first |= xpoll_test_udata_fd (&events[0]) == pairs[0][0];
        seen_third |= xpoll_test_udata_fd (&events[0]) == pairs[2][0];
    }

    assert_true (seen_first && seen_third,
                 "max_events=1 never rotated: fd %d seen=%d, fd %d seen=%d "
                 "(a fd that stays ready is starved by capped batches)",
                 pairs[0][0], seen_first, pairs[2][0], seen_third);

    return ASSERT_OK;
}

/* Draining every ready fd quiets the whole set. */

static int
test_xpoll_draining_quiets_the_set (void)
{
    xpoll_event_t events[16];

    check_true (xpoll_test_drain (pairs[0][0]));
    check_true (xpoll_test_drain (pairs[2][0]));
    check_equal (xpoll_test_wait (xp, events, 16, 0), 0);

    return ASSERT_OK;
}

/* Removing fds one at a time leaves the rest registered. */

static int
test_xpoll_remove_every_fd (void)
{
    xpoll_event_t events[16];

    for (int i = 0; i < PAIRS; i++)
        check_true (xpoll_remove_fd (xp, pairs[i][0]));

    check_true (xpoll_test_write_byte (pairs[0][1]));
    check_equal (xpoll_test_wait (xp, events, 16, 0), 0);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_only_ready_fds_reported),
        define_test_case(test_xpoll_capped_batches_rotate),
        define_test_case(test_xpoll_draining_quiets_the_set),
        define_test_case(test_xpoll_remove_every_fd),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
