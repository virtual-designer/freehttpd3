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

#define FH_LOG_MODULE_NAME "server"

#include "platform.h"

#if PLATFORM_LINUX
#	define _GNU_SOURCE
#elif PLATFORM_BSD
#	define _DARWIN_C_SOURCE
#	define _BSD_SOURCE
#endif /* PLATFORM_LINUX */

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "compat.h"
#include "core/config.h"
#include "hash/itable.h"
#include "log/log.h"
#include "server.h"
#include "utils/utils.h"
#include "worker.h"

#define XPOLL_MAX_EVENTS 4096

static bool should_terminate = false;

struct fh_server *
fh_server_create (struct fh_config *config)
{
	struct fh_server *server = calloc (1, sizeof (*server));

	if (!server)
		return server;

	server->config = config;
	return server;
}

void
fh_server_destroy (struct fh_server *server)
{
	if (server->xp)
		xpoll_destroy (server->xp);

	for_each_itable_entry (server->sockfd_table, sockfd)
		close ((fd_t) sockfd->key);

	fh_pr_info ("Closed %" PRIu64 " sockets", server->sockfd_table->count);
	itable_destroy (server->sockfd_table);

	if (!server->is_worker)
	{
		for (size_t i = 0; i < server->worker_count; i++)
		{
			fh_pr_info ("Killing worker process: %zu [%d]", i,
						server->workers[i]);
			kill (server->workers[i], SIGTERM);
		}
	}

	free (server->workers);
	fh_config_free (server->config);
	free (server);
}

static fd_t
fh_server_create_socket (struct fh_server *server, int domain, uint16_t port)
{
#ifdef SOCK_NONBLOCK
	fd_t sockfd = socket (domain, SOCK_STREAM | SOCK_NONBLOCK, 0);

	if (sockfd < 0)
		return -1;
#else  /* not SOCK_NONBLOCK */
	fd_t sockfd = socket (domain, SOCK_STREAM, 0);

	if (sockfd < 0)
		return -1;

	if (!fd_set_nonblocking (sockfd))
		return -1;
#endif /* SOCK_NONBLOCK */

	if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 },
					sizeof (int))
		< 0)
		goto fh_server_create_socket_end;

#if PLATFORM_LINUX || PLATFORM_BSD
	if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEPORT, &(int) { 1 },
					sizeof (int))
		< 0)
		goto fh_server_create_socket_end;
#endif /* PLATFORM_LINUX */

	struct timeval timeout = {
		.tv_sec = 5,
		.tv_usec = 0,
	};

	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof (timeout))
		< 0)
		goto fh_server_create_socket_end;

	if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof (timeout))
		< 0)
		goto fh_server_create_socket_end;

	void *addr_ptr = NULL;
	socklen_t addr_len = 0;
	struct sockaddr_in addr4 = { 0 };
	struct sockaddr_in6 addr6 = { 0 };

	if (domain == AF_INET)
	{
		addr4.sin_family = domain;
		addr4.sin_port = htons (port);
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr_ptr = &addr4;
		addr_len = sizeof addr4;
	}
	else
	{
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons (port);
		inet_pton (AF_INET6, "::1", &addr6.sin6_addr);
		addr_ptr = &addr6;
		addr_len = sizeof addr6;
	}

	if (bind (sockfd, addr_ptr, addr_len) < 0)
		goto fh_server_create_socket_end;

	if (listen (sockfd, SOMAXCONN) < 0)
		goto fh_server_create_socket_end;

	return sockfd;

fh_server_create_socket_end:
	close (sockfd);
	return -1;
}

static bool
fh_server_create_sockets (struct fh_server *server)
{
	struct itable *table = itable_create (4096);

	if (!table)
		return false;

	bool data = true;
	server->sockfd_table = itable_create (4096);

	if (!server->sockfd_table)
	{
		itable_destroy (table);
		return false;
	}

	for (size_t i = 0; i < server->config->host_count; i++)
	{
		const struct fh_config_host *host = &server->config->hosts[i];

		for (size_t j = 0; j < host->port_count; j++)
		{
			uint16_t port = host->ports[j];

			if (itable_contains (table, (uint64_t) port))
				continue;

			if (!itable_set (table, (uint64_t) port, &data))
			{
				itable_destroy (table);
				return false;
			}

			fh_pr_info ("Creating socket for port: %i [IPv4]", port);
			fd_t sockfd = fh_server_create_socket (server, AF_INET, port);

			if (sockfd < 0)
			{
				int err = errno;
				itable_destroy (table);
				errno = err;
				return false;
			}

			itable_set (server->sockfd_table, (uint64_t) sockfd, (void *) 0x1);
		}
	}

	itable_destroy (table);
	return true;
}

static bool
fh_server_fork_workers (struct fh_server *server)
{
	fh_pr_info ("Spawning %zu workers", server->config->worker_count);
	server->workers = calloc (server->config->worker_count, sizeof (pid_t));

	if (!server->workers)
	{
		fh_pr_info ("Memory allocation error: %s", strerror (errno));
		return false;
	}

	for (size_t i = 0; i < server->config->worker_count; i++)
	{
		pid_t pid = fork ();

		if (pid < 0)
		{
			fh_pr_info ("Failed to spawn worker #%zu: %s", i, strerror (errno));
			return false;
		}

		if (pid == 0)
		{
			server->is_worker = true;
			server->current_worker_index = i;

			if (!server->xp)
				server->xp = xpoll_create ();

			if (!server->xp)
				_exit (EXIT_FAILURE);

			for_each_itable_entry (server->sockfd_table, sockfd)
			{
				if (xpoll_register_fd (server->xp, (fd_t) sockfd->key, XPOLL_IN,
									   0)
					< 0)
					_exit (EXIT_FAILURE);
			}

			fh_log_set_worker_pid (getpid ());
			fh_worker_start (server);
			_exit (EXIT_FAILURE);
		}

		server->workers[server->worker_count++] = pid;
		fh_pr_info ("Spawned worker #%zu [%d]", i, pid);
	}

	return true;
}

static void
handle_exit_signal (int signum __attribute__ ((unused)))
{
	should_terminate = true;
	fh_pr_warn ("Signal: %s", strsignal (signum));
}

bool
fh_server_start (struct fh_server *server)
{
	if (!fh_server_create_sockets (server))
		return false;

	struct sigaction act;

	sigemptyset (&act.sa_mask);
	act.sa_handler = &handle_exit_signal;
	act.sa_flags = SA_RESTART;

	if (sigaction (SIGINT, &act, NULL) < 0
		|| sigaction (SIGTERM, &act, NULL) < 0)
		return false;

	if (!fh_server_fork_workers (server))
		return false;

	while (true)
	{
		pause ();

		if (should_terminate)
			return true;
	}

	return true;
}

bool
fh_server_wait (struct fh_server *server, bool *should_terminate)
{
	xevent_t events[XPOLL_MAX_EVENTS];

	while (true)
	{
		if (*should_terminate)
			return true;

		int n_fds = xpoll_wait (server->xp, events, XPOLL_MAX_EVENTS, 5000);

		if (n_fds < 0)
		{
			if (would_interrupt ())
				continue;

			fh_pr_err ("xpoll_wait failed: %s", strerror (errno));
			continue;
		}

		for (int i = 0; i < n_fds; i++)
		{
			uint32_t kind = XPOLL_EVENT_KINDS (&events[i]);
			fd_t fd = XPOLL_EVENT_FD (&events[i]);

			if (itable_contains (server->sockfd_table, (uint64_t) fd)
				&& kind & XPOLL_IN)
			{
				fh_pr_debug (
					"New connections are available to accept [socket %i]", fd);

				/* TODO! */
				while (true)
				{
					errno = 0;
					fd_t client_fd = accept (fd, NULL, NULL);

					if (client_fd < 0 || errno != 0)
						break;

					fh_pr_debug ("Accepted [socket %i] [client %i]", fd,
								 client_fd);
					close (client_fd);
				}

				continue;
			}
		}
	}

	return true;
}
