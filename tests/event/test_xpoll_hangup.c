/* A peer that closes its end must wake the poller.

   Which bit reports it is deliberately not asserted: epoll sets
   EPOLLIN|EPOLLHUP, kqueue sets EV_EOF (mapped to XPOLL_HANGUP), and
   poll() sets POLLIN|POLLHUP, so the portable contract is only that the
   fd is reported at all and that a read then sees EOF. */

#include "test-xpoll-common.h"

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    fd_t pair[2];
    CHECK (xpoll_test_socketpair (pair));
    CHECK (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ));

    xpoll_event_t events[8];
    CHECK (xpoll_test_wait (xp, events, 8, 0) == 0);

    close (pair[1]);

    int ret = xpoll_test_wait (xp, events, 8, 1000);
    CHECK_MSG (ret >= 1, "peer close did not wake the poller (ret=%d)", ret);

    uint32_t mask = xpoll_test_events_for (events, ret, pair[0]);
    CHECK_MSG (mask & (XPOLL_READ | XPOLL_HANGUP),
               "peer close reported as neither readable nor hangup "
               "(events=0x%x)",
               mask);

    /* The wake-up must be genuine: the fd is at EOF, not merely spinning. */
    char buf[16];
    CHECK (read (pair[0], buf, sizeof (buf)) == 0);

    close (pair[0]);
    xpoll_close (xp);

    return test_report ();
}
