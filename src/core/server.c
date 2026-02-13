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
#include <fcntl.h>
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
#include "connection.h"
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
	server->conn_table = itable_create (4096);
	return server;
}

void
fh_server_destroy (struct fh_server *server)
{
	if (server->xp)
		xpoll_destroy (server->xp);

	for_each_itable_entry (server->sockfd_table, sockfd)
	{
		close ((fd_t) sockfd->key);
		free (sockfd->data);
	}

	for_each_itable_entry (server->conn_table, conn)
	{
		fh_conn_destroy (conn->data);
	}

	fh_pr_info ("Closed %" PRIu64 " sockets and %" PRIu64 " connections",
				server->sockfd_table->count, server->conn_table->count);
	itable_destroy (server->conn_table);
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
fh_server_create_socket (struct fh_server *server __attribute__((unused)), int domain, uint16_t port)
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

			struct sockfd_info *info = calloc (1, sizeof (*info));

			if (!info)
			{
				int err = errno;
				close (sockfd);
				itable_destroy (table);
				errno = err;
				return false;
			}

			info->family = AF_INET;
			itable_set (server->sockfd_table, (uint64_t) sockfd, info);
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

static bool
fh_server_add_conn (struct fh_server *server, fd_t client_fd,
					const struct sockaddr_storage *client_addr)
{
	struct fh_conn *conn = fh_conn_create (client_fd, client_addr);

	if (!conn)
		return false;

	if (xpoll_register_fd (server->xp, client_fd, XPOLL_IN, XPOLL_ET) != 0)
	{
		fh_conn_destroy (conn);
		return false;
	}

	if (!itable_set (server->conn_table, (uint64_t) client_fd, conn))
	{
		xpoll_unregister_fd (server->xp, client_fd, XPOLL_IN);
		fh_conn_destroy (conn);
		return false;
	}

	return true;
}

static bool
fh_server_close_conn (struct fh_server *server, struct fh_conn *conn)
{
	xpoll_unregister_fd (server->xp, conn->sockfd, XPOLL_IN | XPOLL_OUT);
	itable_remove (server->conn_table, (uint64_t) conn->sockfd);
	fh_conn_destroy (conn);
	return true;
}

static bool
fh_server_close_fd (struct fh_server *server, fd_t fd)
{
	xpoll_unregister_fd (server->xp, fd, XPOLL_IN | XPOLL_OUT);
	itable_remove (server->conn_table, (uint64_t) fd);
	close (fd);
	return true;
}

static bool
fh_server_accept (struct fh_server *server, fd_t server_fd,
				  const struct sockfd_info *info)
{
	size_t err_count = 0;

	while (true)
	{
		struct sockaddr_storage client_addr;
		socklen_t client_addr_len = sizeof (client_addr);
		bool is_ip6 = info->family == AF_INET6;
		fd_t client_fd;

#ifdef HAVE_ACCEPT4
		client_fd = accept4 (server_fd, (struct sockaddr *) &client_addr,
							 &client_addr_len, O_NONBLOCK);
#else  /* not HAVE_ACCEPT4 */
		client_fd = accept (server_fd, (struct sockaddr *) &client_addr,
							&client_addr_len);
#endif /* HAVE_ACCEPT4 */

		if (client_fd < 0)
		{
			if (errno == EINTR)
				continue;

			if (would_block ())
				break;

			fh_pr_err ("Unable to accept connection via socket %d [IPv%d]: %s",
					   server_fd, info->family == AF_INET6 ? 6 : 4,
					   strerror (errno));

			if (err_count >= 5)
				break;

			err_count++;
			continue;
		}

		err_count = 0;

		if (info->family != client_addr.ss_family)
		{
			close (client_fd);
			continue;
		}

#ifndef HAVE_ACCEPT4
		if (!fd_set_nonblocking (client_fd))
		{
			close (client_fd);
			continue;
		}
#endif /* HAVE_ACCEPT4 */

		char ip[INET6_ADDRSTRLEN] = { 0 };
		inet_ntop (
			info->family,
			is_ip6 ? (void *) &((struct sockaddr_in6 *) &client_addr)->sin6_addr
				   : (void *) &((struct sockaddr_in *) &client_addr)->sin_addr,
			ip, is_ip6 ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN);
		uint16_t port
			= ntohs (is_ip6 ? ((struct sockaddr_in6 *) &client_addr)->sin6_port
							: ((struct sockaddr_in *) &client_addr)->sin_port);

		fh_pr_info ("Accepted connection: server_fd=%d, client_fd=%d, "
					"client_addr=%s:%d",
					server_fd, client_fd, ip, port);

		if (!fh_server_add_conn (server, client_fd, &client_addr))
		{
			fh_pr_err ("Failed to add connection: %s", strerror (errno));
			close (client_fd);
			continue;
		}
	}

	return true;
}

static bool
fh_server_on_read (struct fh_server *server, struct fh_conn *conn)
{
	fh_server_close_conn (server, conn);
	return true;
}

static bool
fh_server_on_write (struct fh_server *server, struct fh_conn *conn)
{
	fh_server_close_conn (server, conn);
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
			const struct sockfd_info *info
				= itable_get (server->sockfd_table, (uint64_t) fd);

			if (info && kind & XPOLL_IN)
			{
				if (!fh_server_accept (server, fd, info))
					fh_pr_err ("accept failed: %s", strerror (errno));

				continue;
			}

			struct fh_conn *conn
				= itable_get (server->conn_table, (uint64_t) fd);

			if (!conn)
			{
				fh_server_close_fd (server, fd);
				continue;
			}

			if (XPOLL_EVENT_IS_ERR (&events[i]))
			{
				fh_pr_err ("xpoll error: client_fd=%d, errno=%d, msg=\"%s\"", fd,
						   errno, strerror (errno));
				fh_server_close_conn (server, conn);
				continue;
			}

			if (kind & XPOLL_IN)
			{
				fh_pr_info ("xpoll read notification: client_fd=%d", fd);

				if (!fh_server_on_read (server, conn))
				{
					fh_pr_err ("read event handler failed: %s",
							   strerror (errno));
					fh_server_close_conn (server, conn);
				}
			}
			else if (kind & XPOLL_OUT)
			{
				fh_pr_info ("xpoll write notification: client_fd=%d", fd);

				if (!fh_server_on_write (server, conn))
				{
					fh_pr_err ("write event handler failed: %s",
							   strerror (errno));
					fh_server_close_conn (server, conn);
				}
			}
		}
	}

	return true;
}
