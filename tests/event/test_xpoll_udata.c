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

#include "test-xpoll-common.h"

#define CONNS 3

struct conn
{
    fd_t fd;
    int id;
};

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    struct conn conns[CONNS];
    fd_t pairs[CONNS][2];

    for (int i = 0; i < CONNS; i++)
    {
        CHECK (xpoll_test_socketpair (pairs[i]));

        conns[i].fd = pairs[i][0];
        conns[i].id = 100 + i;

        CHECK (xpoll_add_fd (xp, pairs[i][0], &conns[i], XPOLL_READ));
    }

    /* Only two of the three are made ready, so that the reported udata
       has to match the fds that actually woke: a backend that hands back
       udata by position, or reuses one entry for every event, reports a
       pointer belonging to the idle fd here. */
    CHECK (xpoll_test_write_byte (pairs[0][1]));
    CHECK (xpoll_test_write_byte (pairs[2][1]));

    xpoll_event_t events[CONNS * 2];
    int ret = xpoll_test_wait (xp, events, CONNS * 2, 1000);
    CHECK_MSG (ret >= 2, "%d ready fds reported, expected 2", ret);

    int seen[CONNS] = { 0 };

    for (int i = 0; i < ret; i++)
    {
        int matched = -1;

        for (int j = 0; j < CONNS; j++)
            if (XPOLL_TEST_UDATA (events[i]) == &conns[j])
                matched = j;

        CHECK_MSG (matched >= 0,
                   "event %d carried udata %p, which was never "
                   "registered",
                   i, XPOLL_TEST_UDATA (events[i]));

        if (matched >= 0)
            seen[matched]++;
    }

    CHECK_MSG (seen[0] >= 1, "udata for ready fd %d was not reported",
               conns[0].fd);
    CHECK_MSG (seen[2] >= 1, "udata for ready fd %d was not reported",
               conns[2].fd);
    CHECK_MSG (seen[1] == 0,
               "udata for idle fd %d was reported %d time(s); an event was "
               "paired with the wrong registration",
               conns[1].fd, seen[1]);

    /* Changing an interest set replaces the udata too, so that a caller
       can re-point an fd at new state without unregistering it.  The
       replacement is a separate object, not another conns[] entry, so a
       backend that leaves the old pointer in place cannot pass by
       accident. */
    struct conn replacement = { pairs[0][0], 200 };

    CHECK (xpoll_modify_fd (xp, pairs[0][0], &replacement, XPOLL_READ));

    ret = xpoll_test_wait (xp, events, CONNS * 2, 1000);
    CHECK (ret >= 1);

    int stale = 0, replaced = 0;

    for (int i = 0; i < ret; i++)
    {
        stale += XPOLL_TEST_UDATA (events[i]) == &conns[0];
        replaced += XPOLL_TEST_UDATA (events[i]) == &replacement;
    }

    CHECK_MSG (stale == 0,
               "%d event(s) still carried the udata that xpoll_modify_fd() "
               "replaced",
               stale);
    CHECK_MSG (replaced >= 1,
               "the udata installed by xpoll_modify_fd() was never reported");

    /* NULL is a value, not "leave it alone": a backend that treats it as
       absent hands back the previous pointer here.  fd 1 is the only
       registration carrying NULL at this point, so an event with NULL
       udata can only be its own. */
    CHECK (xpoll_test_write_byte (pairs[1][1]));
    CHECK (xpoll_modify_fd (xp, pairs[1][0], NULL, XPOLL_READ));

    ret = xpoll_test_wait (xp, events, CONNS * 2, 1000);
    CHECK (ret >= 1);

    int cleared = 0;

    for (int i = 0; i < ret; i++)
    {
        CHECK_MSG (XPOLL_TEST_UDATA (events[i]) != &conns[1],
                   "udata cleared to NULL came back as the old pointer");

        cleared += XPOLL_TEST_UDATA (events[i]) == NULL;
    }

    CHECK_MSG (cleared >= 1,
               "fd %d was not reported after its udata was "
               "cleared to NULL",
               conns[1].fd);

    for (int i = 0; i < CONNS; i++)
    {
        CHECK (xpoll_remove_fd (xp, pairs[i][0]));
        xpoll_test_close_pair (pairs[i]);
    }

    xpoll_close (xp);

    return test_report ();
}
