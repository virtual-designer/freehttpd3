/* Lifecycle of an xpoll instance and the return-value contract shared by
   all three backends: add/modify/remove report success as a boolean, and
   removing an fd that was never registered fails. */

#include "libtest.h"
#include "test_xpoll_common.h"

static xpoll_t xp;
static fd_t pair[2];

static int
test_xpoll_create (void)
{
    xp = xpoll_create (XPOLL_CLOEXEC);
    return assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");
}

static int
test_xpoll_empty_zero_timeout (void)
{
    xpoll_event_t events[8];
    return assert_true (xpoll_test_wait (xp, events, 8, 0) == 0,
                        "xpoll_test_wait() did not report 0");
}

static int
test_xpoll_add_fd_remove (void)
{
    assert_true (xpoll_test_socketpair (pair),
                 "xpoll_test_socketpair() failed");
    assert_true (
        xpoll_add_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]), XPOLL_READ),
        "fd add failed");
    assert_true (xpoll_modify_fd (xp, pair[0], xpoll_test_fd_udata (pair[0]),
                                  XPOLL_READ | XPOLL_WRITE),
                 "fd modification failed");
    assert_true (xpoll_remove_fd (xp, pair[0]), "fd removal failed");
    assert_false (xpoll_remove_fd (xp, pair[0]),
                  "double removal of fd succeeded");
    assert_false (xpoll_remove_fd (xp, pair[1]),
                  "removal of non-existing fd succeeded");
    return ASSERT_OK;
}

static int
test_xpoll_zero_flag_creation (void)
{
    xpoll_test_close_pair (pair);
    xpoll_close (xp);

    xp = xpoll_create (0);
    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");
    xpoll_close (xp);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_create),
        define_test_case(test_xpoll_empty_zero_timeout),
        define_test_case(test_xpoll_add_fd_remove),
        define_test_case(test_xpoll_zero_flag_creation),
        NULL,
    },
};
