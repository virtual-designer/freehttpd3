/* Many fds at once: the registry has to grow and shrink correctly, and
   a capped batch must not starve the fds it left out.

   PAIRS is deliberately well past the poll backend's initial capacity of
   16, so that both its growth realloc and the shrink that follows mass
   removal are exercised; the smaller suites never reach either.

   Starvation is the reason for the fairness check below.  With more
   ready fds than max_events, a backend that always reports the batch
   from the start of its registry hands back the same few descriptors on
   every call, and the connections behind the rest are never served.  It
   cannot be caught by a single wait, only by capping repeatedly without
   draining in between. */

#include "libtest.h"
#include "test_xpoll_common.h"

#define PAIRS 64
#define BATCH 8

/* Enough calls to cover every ready fd twice over, so that this measures
   rotation rather than the exact order a backend happens to use. */

#define ROUNDS ((PAIRS / BATCH) * 2)

static xpoll_t xp;
static fd_t pairs[PAIRS][2];
static xpoll_event_t events[PAIRS * 2];

static int
before_all (void)
{
    xp = xpoll_create (XPOLL_CLOEXEC);
    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");

    for (int i = 0; i < PAIRS; i++)
    {
        check_true (xpoll_test_socketpair (pairs[i]));
        assert_true (xpoll_add_fd (xp, pairs[i][0],
                                   xpoll_test_fd_udata (pairs[i][0]),
                                   XPOLL_READ),
                     "registering fd %d (entry %d of %d) failed", pairs[i][0],
                     i, PAIRS);
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

/* Every registered fd is ready, and a batch large enough for all of them
   must report all of them. */

static int
test_xpoll_every_ready_fd_reported (void)
{
    for (int i = 0; i < PAIRS; i++)
        check_true (xpoll_test_write_byte (pairs[i][1]));

    int ret = xpoll_test_wait (xp, events, PAIRS * 2, 1000);

    assert_true (ret >= PAIRS, "%d of %d ready fds reported", ret, PAIRS);

    int missing = 0;

    for (int i = 0; i < PAIRS; i++)
        if (!(xpoll_test_events_for (events, ret, pairs[i][0]) & XPOLL_READ))
            missing++;

    assert_equal (missing, 0, "%d ready fds were not reported at all",
                  missing);

    return ASSERT_OK;
}

/* Nothing is drained, so all PAIRS fds stay ready for the whole loop.
   Over ROUNDS capped calls every one of them must come back at least
   once. */

static int
test_xpoll_capped_batches_do_not_starve (void)
{
    int seen[PAIRS] = { 0 };

    for (int round = 0; round < ROUNDS; round++)
    {
        int n = xpoll_test_wait (xp, events, BATCH, 1000);

        assert_true (n >= 1, "capped wait %d returned %d", round, n);

        for (int j = 0; j < n; j++)
            for (int i = 0; i < PAIRS; i++)
                if (xpoll_test_udata_fd (&events[j]) == pairs[i][0])
                    seen[i]++;
    }

    int starved = 0;

    for (int i = 0; i < PAIRS; i++)
        if (!seen[i])
            starved++;

    assert_equal (starved, 0,
                  "%d of %d ready fds were never reported across %d calls "
                  "with max_events=%d (capped batches do not rotate)",
                  starved, PAIRS, ROUNDS, BATCH);

    return ASSERT_OK;
}

/* A capped batch leaves the rotation cursor part-way through the
   registry.  If the entries from there to the end then go quiet while
   earlier ones are still ready, the next wait has to wrap round to find
   them -- a rotation that only advances while it is reporting events
   walks off the end and reports nothing, so the caller sees a spurious
   "nothing is ready" even though poll(2) said otherwise. */

static int
test_xpoll_rotation_wraps_around (void)
{
    for (int i = 0; i < PAIRS; i++)
        check_true (xpoll_test_write_byte (pairs[i][1]));

    check_equal (xpoll_test_wait (xp, events, BATCH, 1000), BATCH);

    /* Everything from the cursor onwards goes quiet; only the fds behind
       it stay ready. */
    for (int i = BATCH; i < PAIRS; i++)
        check_true (xpoll_test_drain (pairs[i][0]));

    int ret = xpoll_test_wait (xp, events, PAIRS * 2, 0);

    assert_true (ret >= 1,
                 "wait reported nothing while %d fds behind the rotation "
                 "cursor were still ready",
                 BATCH);

    for (int i = 0; i < BATCH; i++)
        assert_true (xpoll_test_events_for (events, ret, pairs[i][0])
                         & XPOLL_READ,
                     "ready fd %d behind the rotation cursor was skipped",
                     pairs[i][0]);

    return ASSERT_OK;
}

/* Same hazard from the other direction: removals shrink the registry
   below a cursor parked past the new end. */

static int
test_xpoll_registry_shrinks_below_cursor (void)
{
    for (int i = 0; i < PAIRS; i++)
        check_true (xpoll_test_write_byte (pairs[i][1]));

    check_equal (xpoll_test_wait (xp, events, BATCH, 1000), BATCH);

    for (int i = PAIRS - 1; i >= BATCH; i--)
        check_true (xpoll_remove_fd (xp, pairs[i][0]));

    int ret = xpoll_test_wait (xp, events, PAIRS * 2, 0);

    assert_true (ret >= 1,
                 "wait reported nothing after the registry shrank below the "
                 "rotation cursor");

    for (int i = BATCH; i < PAIRS; i++)
        check_true (xpoll_add_fd (xp, pairs[i][0],
                                  xpoll_test_fd_udata (pairs[i][0]),
                                  XPOLL_READ));

    return ASSERT_OK;
}

/* Draining every fd quiets the whole set. */

static int
test_xpoll_draining_quiets_the_set (void)
{
    for (int i = 0; i < PAIRS; i++)
        check_true (xpoll_test_drain (pairs[i][0]));

    check_equal (xpoll_test_wait (xp, events, PAIRS * 2, 0), 0);

    return ASSERT_OK;
}

/* Mass removal, which drives the poll backend's registry back down.
   Removing from the front each time also walks every entry past the
   hole, so a bad shift shows up as a later removal failing. */

static int
test_xpoll_mass_removal (void)
{
    for (int i = 0; i < PAIRS; i++)
        assert_true (xpoll_remove_fd (xp, pairs[i][0]),
                     "removing fd %d (entry %d of %d) failed", pairs[i][0], i,
                     PAIRS);

    for (int i = 0; i < PAIRS; i++)
        check_true (xpoll_test_write_byte (pairs[i][1]));

    assert_equal (xpoll_test_wait (xp, events, PAIRS * 2, 0), 0,
                  "events reported after every fd was removed");

    return ASSERT_OK;
}

/* The instance is still usable once it has shrunk. */

static int
test_xpoll_usable_after_shrink (void)
{
    check_true (xpoll_add_fd (xp, pairs[0][0],
                              xpoll_test_fd_udata (pairs[0][0]), XPOLL_READ));

    int ret = xpoll_test_wait (xp, events, PAIRS * 2, 1000);

    check_true (ret >= 1);
    check_true (xpoll_test_events_for (events, ret, pairs[0][0]) & XPOLL_READ);
    check_true (xpoll_remove_fd (xp, pairs[0][0]));

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_every_ready_fd_reported),
        define_test_case(test_xpoll_capped_batches_do_not_starve),
        define_test_case(test_xpoll_rotation_wraps_around),
        define_test_case(test_xpoll_registry_shrinks_below_cursor),
        define_test_case(test_xpoll_draining_quiets_the_set),
        define_test_case(test_xpoll_mass_removal),
        define_test_case(test_xpoll_usable_after_shrink),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
