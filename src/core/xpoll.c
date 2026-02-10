#include "xpoll.h"
#include "utils/platform.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#if PLATFORM_LINUX
#   include <sys/epoll.h>
#elif PLATFORM_DARWIN
#   include <sys/time.h>
#   include <sys/event.h>
#endif /* PLATFORM_LINUX */

xpoll_t
xpoll_create (void)
{
#if PLATFORM_LINUX
    return epoll_create1 (0);
#elif PLATFORM_DARWIN
    return kqueue ();
#endif /* PLATFORM_LINUX */
}

void
xpoll_destroy (xpoll_t xpoll_fd)
{
    close (xpoll_fd);
}
