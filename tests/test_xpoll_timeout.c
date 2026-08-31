/* Timeout contract, shared with poll(2)/epoll_wait(2): 0 means "return
   immediately", a positive value waits at most that many milliseconds,
   and a negative value blocks until something is ready. */

#include <time.h>

#include "test-xpoll-common.h"

static long
elapsed_ms_since (const struct timespec *start)
{
    struct timespec now;

    clock_gettime (CLOCK_MONOTONIC, &now);

    return (now.tv_sec - start->tv_sec) * 1000
           + (now.tv_nsec - start->tv_nsec) / 1000000;
}

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    fd_t pair[2];
    CHECK (xpoll_test_socketpair (pair));
    CHECK (xpoll_add_fd (xp, pair[0], XPOLL_READ));

    xpoll_event_t events[8];
    struct timespec start;

    /* A zero timeout polls and returns at once. */
    clock_gettime (CLOCK_MONOTONIC, &start);
    CHECK (xpoll_test_wait (xp, events, 8, 0) == 0);
    CHECK_MSG (elapsed_ms_since (&start) < 100,
               "a zero timeout blocked for %ld ms", elapsed_ms_since (&start));

    /* A positive timeout waits for roughly that long when nothing is
       ready.  The bound is loose on purpose: this only has to catch a
       timeout that is ignored outright, not measure scheduler accuracy. */
    clock_gettime (CLOCK_MONOTONIC, &start);
    CHECK (xpoll_test_wait (xp, events, 8, 200) == 0);

    long waited = elapsed_ms_since (&start);
    CHECK_MSG (waited >= 100, "a 200 ms timeout returned after %ld ms", waited);

    /* A negative timeout blocks until something is ready.  Data is made
       available first, so a correct implementation returns immediately
       and a broken one fails rather than hanging. */
    CHECK (xpoll_test_write_byte (pair[1]));

    int ret = xpoll_test_wait (xp, events, 8, -1);
    CHECK_MSG (ret >= 1, "an infinite timeout returned %d with data pending",
               ret);
    CHECK (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    return test_report ();
}
