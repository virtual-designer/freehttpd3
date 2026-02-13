/*
 * This file is part of OSN freehttpd.
 *
 * Copyright (C) 2025-2026  OSN Developers.
 *
 * OSN freehttpd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OSN freehttpd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with OSN freehttpd.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FHTTPD_XPOLL_H
#define FHTTPD_XPOLL_H

#include "platform.h"

#if PLATFORM_LINUX
#	include <sys/epoll.h>
#elif PLATFORM_BSD
#	include <sys/event.h>
#	include <sys/time.h>
#endif /* PLATFORM_BSD */

enum xpoll_event_type
{
#if PLATFORM_LINUX
	XPOLL_IN = EPOLLIN,
	XPOLL_OUT = EPOLLOUT,
#elif PLATFORM_BSD
	XPOLL_IN = 1,
	XPOLL_OUT = 2,
#else /* not PLATFORM_BSD */
#	error "This platform is not supported"
#endif
};

enum xpoll_event_option
{
	XPOLL_ET = 1,
	XPOLL_ONESHOT,
};

typedef enum xpoll_event_type xevent_type_t;
typedef enum xpoll_event_option xevent_opt_t;

struct xpoll
{
	int fd;
};

union xpoll_event
{
#if PLATFORM_LINUX
	struct epoll_event linux_ev;
#elif PLATFORM_BSD
	struct kevent bsd_ev;
#else /* not PLATFORM_BSD */
#	error "This platform is not supported"
#endif
};

#if PLATFORM_LINUX
#	define XPOLL_EVENT_RAW(ev) ((ev)->linux_ev)
#	define XPOLL_EVENT_FD(ev) ((XPOLL_EVENT_RAW (ev)).data.fd)
#	define XPOLL_EVENT_KINDS(ev) ((XPOLL_EVENT_RAW (ev)).events)
#	define XPOLL_EVENT_IS_ERR(ev) (XPOLL_EVENT_KINDS(ev) & EPOLLERR)
#elif PLATFORM_BSD
#	define XPOLL_EVENT_RAW(ev) ((ev)->bsd_ev)
#	define XPOLL_EVENT_FD(ev) ((int) (XPOLL_EVENT_RAW (ev)).ident)
#	define XPOLL_EVENT_IS_ERR(ev) ((XPOLL_EVENT_RAW(ev)).flags & EV_ERROR)
#	if EVFILT_READ < 0
#		define XPOLL_EVENT_KINDS(ev) (-(XPOLL_EVENT_RAW (ev)).filter)
#	else /* not EVFILT_READ < 0 */
#		define XPOLL_EVENT_KINDS(ev) ((XPOLL_EVENT_RAW (ev)).filter)
#	endif /* EVFILT_READ < 0 */
#else	   /* not PLATFORM_BSD */
#	error "This platform is not supported"
#endif

typedef union xpoll_event xevent_t;

struct xpoll *xpoll_create (void);
void xpoll_destroy (struct xpoll *xp);
int xpoll_register_fd (struct xpoll *xp, int fd, xevent_type_t events,
					   xevent_opt_t opts);
int xpoll_unregister_fd (struct xpoll *xp, int fd, xevent_type_t events);
int xpoll_modify_registered_fd (struct xpoll *xp, int fd, xevent_type_t events,
								xevent_opt_t opts);
int xpoll_wait (struct xpoll *xp, xevent_t *events, int max_events,
				int timeout);

#endif /* FHTTPD_XPOLL_H */
