/* XPOLL_CLOEXEC must actually set FD_CLOEXEC on the backing descriptor,
   and must not set it when the flag is absent.

   Only meaningful where xpoll_t is a descriptor; the poll backend keeps
   its state in heap memory and documents the flag as a no-op there. */

#include <fcntl.h>

#include "test-xpoll-common.h"

int
main (void)
{
#ifndef FH_PLATFORM_UNKNOWN
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));

    int flags = fcntl (xp, F_GETFD);
    CHECK (flags >= 0);
    CHECK_MSG (flags & FD_CLOEXEC,
               "XPOLL_CLOEXEC did not set FD_CLOEXEC (F_GETFD=0x%x)", flags);

    xpoll_close (xp);

    xp = xpoll_create ((enum xpoll_create_flag) 0);
    CHECK (!XPOLL_XP_ERR (xp));

    flags = fcntl (xp, F_GETFD);
    CHECK (flags >= 0);
    CHECK_MSG ((flags & FD_CLOEXEC) == 0,
               "FD_CLOEXEC set without XPOLL_CLOEXEC (F_GETFD=0x%x)", flags);

    xpoll_close (xp);
#else
    /* Nothing to assert; the instance is not a descriptor. */
    xpoll_t xp = xpoll_create (XPOLL_CLOEXEC);
    CHECK (!XPOLL_XP_ERR (xp));
    xpoll_close (xp);
#endif

    return test_report ();
}
