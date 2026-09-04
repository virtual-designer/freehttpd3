/* The pointer registered with an fd must come back on that fd's events,
   and on no other fd's.

   This is the one part of the interface with no fallback.  xpoll_wait()
   reports udata and nothing else portable -- epoll stores it in a union
   with the fd, so the descriptor is gone once a pointer is registered --
   which means a backend that drops it, truncates it or pairs it with the
   wrong entry leaves the caller unable to tell which connection woke up.

   The other suites register each fd as its own udata, so they would all
   still pass on a backend that quietly substituted the descriptor for
   whatever it was given.  The pointers here are addresses of an array
   that has nothing to do with any fd number, so only genuine round-trip
   storage satisfies them. */

#include "libtest.h"
#include "test_xpoll_common.h"

#define CONNS 3

struct conn
{
    fd_t fd;
    int id;
};

static xpoll_t xp;
static struct conn conns[CONNS];
static fd_t pairs[CONNS][2];
static xpoll_event_t events[CONNS * 2];

/* The replacement udata installed by the modify below outlives the test
   case that registers it, so it cannot be a local. */
static struct conn replacement;

static int
before_all (void)
{
    xp = xpoll_create (XPOLL_CLOEXEC);
    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");

    for (int i = 0; i < CONNS; i++)
    {
        check_true (xpoll_test_socketpair (pairs[i]));

        conns[i].fd = pairs[i][0];
        conns[i].id = 100 + i;

        check_true (xpoll_add_fd (xp, pairs[i][0], &conns[i], XPOLL_READ));
    }

    return ASSERT_OK;
}

static int
after_all (void)
{
    for (int i = 0; i < CONNS; i++)
    {
        check_true (xpoll_remove_fd (xp, pairs[i][0]));
        xpoll_test_close_pair (pairs[i]);
    }

    xpoll_close (xp);

    return ASSERT_OK;
}

/* Only two of the three are made ready, so that the reported udata has
   to match the fds that actually woke: a backend that hands back udata
   by position, or reuses one entry for every event, reports a pointer
   belonging to the idle fd here. */

static int
test_xpoll_udata_round_trip (void)
{
    check_true (xpoll_test_write_byte (pairs[0][1]));
    check_true (xpoll_test_write_byte (pairs[2][1]));

    int ret = xpoll_test_wait (xp, events, CONNS * 2, 1000);

    assert_true (ret >= 2, "%d ready fds reported, expected 2", ret);

    int seen[CONNS] = { 0 };

    for (int i = 0; i < ret; i++)
    {
        int matched = -1;

        for (int j = 0; j < CONNS; j++)
            if (XPOLL_TEST_UDATA (events[i]) == &conns[j])
                matched = j;

        assert_true (matched >= 0,
                     "event %d carried udata %p, which was never registered",
                     i, XPOLL_TEST_UDATA (events[i]));

        if (matched >= 0)
            seen[matched]++;
    }

    assert_true (seen[0] >= 1, "udata for ready fd %d was not reported",
                 conns[0].fd);
    assert_true (seen[2] >= 1, "udata for ready fd %d was not reported",
                 conns[2].fd);
    assert_equal (seen[1], 0,
                  "udata for idle fd %d was reported %d time(s); an event was "
                  "paired with the wrong registration",
                  conns[1].fd, seen[1]);

    return ASSERT_OK;
}

/* Changing an interest set replaces the udata too, so that a caller can
   re-point an fd at new state without unregistering it.  The replacement
   is a separate object, not another conns[] entry, so a backend that
   leaves the old pointer in place cannot pass by accident. */

static int
test_xpoll_modify_replaces_udata (void)
{
    replacement = (struct conn){ pairs[0][0], 200 };

    check_true (xpoll_modify_fd (xp, pairs[0][0], &replacement, XPOLL_READ));

    int ret = xpoll_test_wait (xp, events, CONNS * 2, 1000);
    check_true (ret >= 1);

    int stale = 0, replaced = 0;

    for (int i = 0; i < ret; i++)
    {
        stale += XPOLL_TEST_UDATA (events[i]) == &conns[0];
        replaced += XPOLL_TEST_UDATA (events[i]) == &replacement;
    }

    assert_equal (stale, 0,
                  "%d event(s) still carried the udata that xpoll_modify_fd() "
                  "replaced",
                  stale);
    assert_true (replaced >= 1,
                 "the udata installed by xpoll_modify_fd() was never "
                 "reported");

    return ASSERT_OK;
}

/* NULL is a value, not "leave it alone": a backend that treats it as
   absent hands back the previous pointer here.  fd 1 is the only
   registration carrying NULL at this point, so an event with NULL udata
   can only be its own. */

static int
test_xpoll_udata_cleared_to_null (void)
{
    check_true (xpoll_test_write_byte (pairs[1][1]));
    check_true (xpoll_modify_fd (xp, pairs[1][0], NULL, XPOLL_READ));

    int ret = xpoll_test_wait (xp, events, CONNS * 2, 1000);
    check_true (ret >= 1);

    int cleared = 0;

    for (int i = 0; i < ret; i++)
    {
        assert_true (XPOLL_TEST_UDATA (events[i]) != &conns[1],
                     "udata cleared to NULL came back as the old pointer");

        cleared += XPOLL_TEST_UDATA (events[i]) == NULL;
    }

    assert_true (cleared >= 1,
                 "fd %d was not reported after its udata was cleared to NULL",
                 conns[1].fd);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_udata_round_trip),
        define_test_case(test_xpoll_modify_replaces_udata),
        define_test_case(test_xpoll_udata_cleared_to_null),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
