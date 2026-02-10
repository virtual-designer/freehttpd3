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

#include "utils/platform.h"

#ifdef PLATFORM_LINUX
#	define _GNU_SOURCE
#endif /* PLATFORM_LINUX */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "core/config.h"
#include "hash/itable.h"
#include "server.h"

struct fh_server *
fh_server_create (struct fh_config *config)
{
	struct fh_server *server = calloc (1, sizeof (*server));

	if (!server)
		return server;

	server->xp = xpoll_create ();

	if (!server->xp)
	{
		free (server);
		return NULL;
	}

	server->config = config;
	return server;
}

void
fh_server_destroy (struct fh_server *server)
{
	xpoll_destroy (server->xp);

	for (size_t i = 0; i < server->srv_socket_count; i++)
		close (server->srv_sockets[i]);

	free (server->srv_sockets);
	fh_config_free (server->config);
	free (server);
}

static fd_t
fh_server_create_socket (struct fh_server *server, int domain, uint16_t port)
{
	fd_t sockfd = socket (domain, SOCK_STREAM, 0);

	if (sockfd < 0)
		return -1;

	if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 },
					sizeof (int))
		< 0)
		goto fh_server_create_socket_end;

#if PLATFORM_LINUX
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

bool
fh_server_create_sockets (struct fh_server *server)
{
	struct itable *table = itable_create (4096);

	if (!table)
		return false;

	bool data = true;
	server->srv_sockets = NULL;
	server->srv_socket_count = 0;

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

			server->srv_sockets
				= realloc (server->srv_sockets, sizeof (fd_t) * (server->srv_socket_count + 1));

			if (!server->srv_sockets)
			{
				itable_destroy (table);
				return false;
			}

			fd_t sockfd = fh_server_create_socket (server, AF_INET, port);

			if (sockfd < 0)
			{
				int err = errno;
				itable_destroy (table);
				errno = err;
				return false;
			}

			server->srv_sockets[server->srv_socket_count++] = sockfd;
		}
	}

	itable_destroy (table);
	return true;
}
