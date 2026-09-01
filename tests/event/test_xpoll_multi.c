/* Several fds registered at once: only the ready ones are reported, and
   a small max_events truncates the batch without losing the remainder,
   which the next call still reports. */

#include "test-xpoll-common.h"

#define PAIRS 4

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    fd_t pairs[PAIRS][2];

    for (int i = 0; i < PAIRS; i++)
    {
        CHECK (xpoll_test_socketpair (pairs[i]));
        CHECK (xpoll_add_fd (xp, pairs[i][0], XPOLL_READ));
    }

    /* Make every other pair readable. */
    CHECK (xpoll_test_write_byte (pairs[0][1]));
    CHECK (xpoll_test_write_byte (pairs[2][1]));

    xpoll_event_t events[16];
    int ret = xpoll_test_wait (xp, events, 16, 1000);
    CHECK (ret >= 2);

    CHECK (xpoll_test_events_for (events, ret, pairs[0][0]) & XPOLL_READ);
    CHECK (xpoll_test_events_for (events, ret, pairs[2][0]) & XPOLL_READ);
    CHECK_MSG (xpoll_test_events_for (events, ret, pairs[1][0]) == 0,
               "idle fd %d reported as ready", pairs[1][0]);
    CHECK_MSG (xpoll_test_events_for (events, ret, pairs[3][0]) == 0,
               "idle fd %d reported as ready", pairs[3][0]);

    /* max_events caps a batch.  Nothing is drained in between, so both
       fds stay ready throughout; what matters is that capping the batch
       does not pin the poller to one fd forever.  Asking for one entry
       at a time must therefore cover both readable fds within a few
       calls rather than returning the same one every time.

       Re-waiting with a large max_events would NOT test this: every
       backend is level-triggered here, so both fds come back regardless
       of how the capped calls behaved. */

    bool seen_first = false, seen_third = false;

    for (int i = 0; i < 8 && !(seen_first && seen_third); i++)
    {
        int capped = xpoll_test_wait (xp, events, 1, 1000);

        CHECK_MSG (capped == 1, "max_events=1 returned %d entries", capped);

        if (capped < 1)
            break;

        CHECK_MSG (events[0].data.fd == pairs[0][0]
                       || events[0].data.fd == pairs[2][0],
                   "capped batch reported unready fd %d", events[0].data.fd);

        seen_first |= events[0].data.fd == pairs[0][0];
        seen_third |= events[0].data.fd == pairs[2][0];
    }

    CHECK_MSG (seen_first && seen_third,
               "max_events=1 never rotated: fd %d seen=%d, fd %d seen=%d "
               "(a fd that stays ready is starved by capped batches)",
               pairs[0][0], seen_first, pairs[2][0], seen_third);

    /* Draining every ready fd quiets the whole set. */
    CHECK (xpoll_test_drain (pairs[0][0]));
    CHECK (xpoll_test_drain (pairs[2][0]));
    CHECK (xpoll_test_wait (xp, events, 16, 0) == 0);

    /* Removing fds one at a time leaves the rest registered. */
    for (int i = 0; i < PAIRS; i++)
        CHECK (xpoll_remove_fd (xp, pairs[i][0]));

    CHECK (xpoll_test_write_byte (pairs[0][1]));
    CHECK (xpoll_test_wait (xp, events, 16, 0) == 0);

    for (int i = 0; i < PAIRS; i++)
        xpoll_test_close_pair (pairs[i]);

    xpoll_close (xp);

    return test_report ();
}
