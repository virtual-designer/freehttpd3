#include "xpoll.h"
#include "utils/platform.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#if PLATFORM_LINUX
	#include <sys/epoll.h>
#elif PLATFORM_BSD
	#include <sys/event.h>
	#include <sys/time.h>
#endif /* PLATFORM_LINUX */

struct xpoll *
xpoll_create (void)
{
	struct xpoll *xp = calloc (1, sizeof (*xp));

	if (!xp)
		return NULL;

#if PLATFORM_LINUX
	xp->fd = epoll_create1 (0);
#elif PLATFORM_BSD
	xp->fd = kqueue ();
#else /* not PLATFORM_BSD */
	#error "This platform is not supported"
#endif /* PLATFORM_LINUX */

	if (xp->fd < 0)
	{
		free (xp);
		return NULL;
	}

	return xp;
}

void
xpoll_destroy (struct xpoll *xp)
{
	close (xp->fd);
}

int
xpoll_register_fd (struct xpoll *xp, int fd, xevent_type_t events,
				   xevent_opt_t opts)
{
#if PLATFORM_LINUX
	struct epoll_event ev;

	ev.events = events | (opts & XPOLL_ET ? EPOLLET : 0)
				| (opts & XPOLL_ONESHOT ? EPOLLONESHOT : 0);
	ev.data.fd = fd;

	return epoll_ctl (xp->fd, EPOLL_CTL_ADD, fd, &ev);
#elif PLATFORM_BSD
	struct kevent ev[2];
	size_t count = 0;

	if (events & XPOLL_IN)
	{
		EV_SET (&ev[count++], fd, EVFILT_READ,
				EV_ADD | (opts & XPOLL_ET ? EV_CLEAR : 0)
					| (opts & XPOLL_ONESHOT ? EV_ONESHOT : 0),
				0, 0, NULL);
	}

	if (events & XPOLL_OUT)
	{
		EV_SET (&ev[count++], fd, EVFILT_WRITE,
				EV_ADD | (opts & XPOLL_ET ? EV_CLEAR : 0)
					| (opts & XPOLL_ONESHOT ? EV_ONESHOT : 0),
				0, 0, NULL);
	}

	return kevent (xp->fd, ev, count, NULL, 0, NULL);
#else /* not PLATFORM_BSD */
	#error "This platform is not supported"
#endif /* PLATFORM_LINUX */
}

int
xpoll_modify_registered_fd (struct xpoll *xp, int fd, xevent_type_t events,
							xevent_opt_t opts)
{
#if PLATFORM_LINUX
	struct epoll_event ev;

	ev.events = events | (opts & XPOLL_ET ? EPOLLET : 0)
				| (opts & XPOLL_ONESHOT ? EPOLLONESHOT : 0);
	ev.data.fd = fd;

	return epoll_ctl (xp->fd, EPOLL_CTL_MOD, fd, &ev);
#elif PLATFORM_BSD
	struct kevent ev;
	int rerr = 0, werr = 0;
	int rerrno = 0, werrno = 0;

	EV_SET (&ev, fd, EVFILT_READ,
			(events & XPOLL_IN ? EV_ADD : EV_DELETE)
				| (opts & XPOLL_ET ? EV_CLEAR : 0)
				| (opts & XPOLL_ONESHOT ? EV_ONESHOT : 0),
			0, 0, NULL);

	rerr = kevent (xp->fd, &ev, 1, NULL, 0, NULL);
	rerrno = errno;

	EV_SET (&ev, fd, EVFILT_WRITE,
			(events & XPOLL_OUT ? EV_ADD : EV_DELETE)
				| (opts & XPOLL_ET ? EV_CLEAR : 0)
				| (opts & XPOLL_ONESHOT ? EV_ONESHOT : 0),
			0, 0, NULL);

	werr = kevent (xp->fd, &ev, 1, NULL, 0, NULL);
	werrno = errno;

	if (rerr < 0 && werr < 0)
	{
		errno = rerrno != 0 ? rerrno : werrno;
		return -1;
	}

	return 0;
#else /* not PLATFORM_BSD */
	#error "This platform is not supported"
#endif /* PLATFORM_LINUX */
}

int
xpoll_unregister_fd (struct xpoll *xp, int fd, xevent_type_t events,
					 xevent_opt_t opts)
{
#if PLATFORM_LINUX
	return epoll_ctl (xp->fd, EPOLL_CTL_DEL, fd, NULL);
#elif PLATFORM_BSD
	struct kevent ev[2];
	size_t count = 0;

	if (events & XPOLL_IN)
	{
		EV_SET (&ev[count++], fd, EVFILT_READ,
				EV_DELETE | (opts & XPOLL_ET ? EV_CLEAR : 0)
					| (opts & XPOLL_ONESHOT ? EV_ONESHOT : 0),
				0, 0, NULL);
	}

	if (events & XPOLL_OUT)
	{
		EV_SET (&ev[count++], fd, EVFILT_WRITE,
				EV_DELETE | (opts & XPOLL_ET ? EV_CLEAR : 0)
					| (opts & XPOLL_ONESHOT ? EV_ONESHOT : 0),
				0, 0, NULL);
	}

	return kevent (xp->fd, ev, count, NULL, 0, NULL);
#else /* not PLATFORM_BSD */
	#error "This platform is not supported"
#endif /* PLATFORM_LINUX */
}

int
xpoll_wait (struct xpoll *xp, xevent_t *events, int max_events, int timeout)
{
#if PLATFORM_LINUX
	return epoll_wait (xp->fd, (struct epoll_event *) events, max_events,
					   timeout);
#elif PLATFORM_BSD
	struct timespec tspec = { 0 };

	if (timeout > 0)
	{
		tspec.tv_sec = ((long) timeout) / 1000L;
		tspec.tv_nsec = (((long) timeout) - (1000L * tspec.tv_sec)) * 1000000L;
	}

	return kevent (xp->fd, NULL, 0, (struct kevent *) events, max_events,
				   timeout >= 0 ? &tspec : NULL);
#else /* not PLATFORM_BSD */
	#error "This platform is not supported"
#endif
}
