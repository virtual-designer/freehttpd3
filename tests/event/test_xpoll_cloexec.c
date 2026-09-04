/* XPOLL_CLOEXEC must actually set FD_CLOEXEC on the backing descriptor,
   and must not set it when the flag is absent.

   Only checkable where xpoll_t is itself a descriptor, which is now Linux
   alone.  The poll backend keeps its state in heap memory and documents
   the flag as a no-op there, and the kqueue backend has a real descriptor
   to check but hides it inside an opaque struct xpoll, so the flag it
   sets in xpoll_create() cannot be observed from out here.  Both fall
   back to asserting that creation itself works. */

#include <fcntl.h>

#include "libtest.h"
#include "test_xpoll_common.h"

static int
test_xpoll_cloexec_flag_is_set (void)
{
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);

    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");

#ifdef FH_PLATFORM_LINUX
    int flags = fcntl (xp, F_GETFD);

    check_true (flags >= 0);
    assert_true (flags & FD_CLOEXEC,
                 "XPOLL_CLOEXEC did not set FD_CLOEXEC (F_GETFD=0x%x)", flags);
#endif

    xpoll_close (xp);

    return ASSERT_OK;
}

/* No descriptor is reachable from here on the other backends, so only
   creation is checkable there. */

static int
test_xpoll_cloexec_flag_is_absent (void)
{
    xpoll_t xp = xpoll_create ((enum xpoll_create_flag) 0);

    assert_false (XPOLL_XP_ERR (xp), "failed to create xpoll");

#ifdef FH_PLATFORM_LINUX
    int flags = fcntl (xp, F_GETFD);

    check_true (flags >= 0);
    assert_equal (flags & FD_CLOEXEC, 0,
                  "FD_CLOEXEC set without XPOLL_CLOEXEC (F_GETFD=0x%x)",
                  flags);
#endif

    xpoll_close (xp);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_xpoll_cloexec_flag_is_set),
        define_test_case(test_xpoll_cloexec_flag_is_absent),
        NULL,
    },
};
