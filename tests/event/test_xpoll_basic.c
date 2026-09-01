/* Lifecycle of an xpoll instance and the return-value contract shared by
   all three backends: add/modify/remove report success as a boolean, and
   removing an fd that was never registered fails. */

#include "test-xpoll-common.h"

int
main (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    /* No fd registered: a zero timeout must report nothing, not block. */
    xpoll_event_t events[8];
    CHECK (xpoll_test_wait (xp, events, 8, 0) == 0);

    fd_t pair[2];
    CHECK (xpoll_test_socketpair (pair));

    CHECK (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ));
    CHECK (xpoll_modify_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]),
                            XPOLL_READ | XPOLL_WRITE));
    CHECK (xpoll_remove_fd (xp, pair[0]));

    /* Removing it a second time must fail: epoll reports ENOENT, kqueue
       reports ENOENT, and the poll backend finds no matching entry. */
    CHECK (xpoll_remove_fd (xp, pair[0]) == false);

    /* An fd that was never added must fail the same way. */
    CHECK (xpoll_remove_fd (xp, pair[1]) == false);

    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    /* Creating without any flag is valid too. */
    xp = xpoll_create ((enum xpoll_create_flag) 0);
    CHECK (!XPOLL_XP_ERR (xp));
    xpoll_close (xp);

    return test_report ();
}
