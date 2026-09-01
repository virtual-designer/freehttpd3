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

#include "test-xpoll-common.h"

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    fd_t pair[2];
    CHECK (xpoll_test_socketpair (pair));

#ifdef FH_PLATFORM_LINUX
    CHECK_MSG (xpoll_add_fd (xp, -1, XPOLL_READ) == false,
               "a negative fd was accepted for registration");

    /* A descriptor that was valid but is closed by the time it is
       registered. */
    fd_t stale[2];
    CHECK (xpoll_test_socketpair (stale));
    xpoll_test_close_pair (stale);

    CHECK_MSG (xpoll_add_fd (xp, stale[0], XPOLL_READ) == false,
               "a closed fd was accepted for registration");
#endif

    /* Changing an interest set that was never established is an error,
       not a silent no-op and not an implicit add.  Every backend does
       promise this one. */
    CHECK_MSG (xpoll_modify_fd (xp, pair[0], XPOLL_READ) == false,
               "modifying an unregistered fd succeeded");

    CHECK (xpoll_test_write_byte (pair[1]));

    xpoll_event_t events[16];

    CHECK_MSG (xpoll_test_wait (xp, events, 16, 0) == 0,
               "a rejected registration still produced events");

    /* max_events must be positive: epoll_wait(2) rejects zero with
       EINVAL and the other backends follow it explicitly, so that a
       caller passing an empty output buffer finds out instead of being
       told "nothing is ready" forever.

       xpoll_wait() is called directly here rather than through the
       harness helper, because the helper retries on EINTR and this call
       is expected to fail. */
    CHECK (xpoll_add_fd (xp, pair[0], XPOLL_READ));

    errno = 0;

    int ret = xpoll_wait (xp, events, 0, 0);
    CHECK_MSG (ret < 0, "max_events=0 returned %d instead of failing", ret);
    CHECK_MSG (ret >= 0 || errno == EINVAL,
               "max_events=0 failed with errno %d, expected EINVAL", errno);

    /* The registration itself is still intact after the rejected call. */
    ret = xpoll_test_wait (xp, events, 16, 1000);
    CHECK (ret >= 1);
    CHECK (xpoll_test_events_for (events, ret, pair[0]) & XPOLL_READ);

    CHECK (xpoll_remove_fd (xp, pair[0]));

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    return test_report ();
}
