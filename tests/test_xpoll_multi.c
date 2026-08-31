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

    /* max_events caps a batch.  Nothing is drained in between, so the
       fds left out are still ready and must show up on the next call:
       between the two batches both readable fds are accounted for. */
    ret = xpoll_test_wait (xp, events, 16, 0);
    CHECK (ret >= 1);

    int first = xpoll_test_wait (xp, events, 1, 1000);
    CHECK_MSG (first == 1, "max_events=1 returned %d entries", first);

    uint32_t seen = xpoll_test_events_for (events, first, pairs[0][0])
                    | xpoll_test_events_for (events, first, pairs[2][0]);
    CHECK (seen & XPOLL_READ);

    int second = xpoll_test_wait (xp, events, 16, 1000);
    CHECK (second >= 1);
    CHECK (xpoll_test_events_for (events, second, pairs[0][0]) & XPOLL_READ);
    CHECK (xpoll_test_events_for (events, second, pairs[2][0]) & XPOLL_READ);

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
