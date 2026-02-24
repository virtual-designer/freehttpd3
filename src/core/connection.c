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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "server.h"
#include "mm/pool.h"

struct fh_conn *
fh_conn_create (const struct fh_server *server, fd_t client_fd,
				const struct sockaddr_storage *client_addr)
{
	struct fh_pool *pool
		= fh_pool_create (4096 + server->module_conn_ctx_total_size);

	if (!pool)
		return NULL;

	struct fh_conn *conn = fh_pool_alloc (
		pool, sizeof (*conn) + (client_addr ? sizeof (*conn->client_addr) : 0));

	if (!conn)
	{
		fh_pool_free (pool);
		return NULL;
	}

	conn->pool = pool;
	conn->sockfd = client_fd;
	conn->module_data
		= fh_pool_alloc (pool, server->module_conn_ctx_total_size);
	conn->module_data_size = server->module_conn_ctx_total_size;

	if (!conn->module_data)
	{
		fh_pool_free (pool);
		return NULL;
	}

	if (client_addr)
	{
		conn->client_addr = (struct sockaddr_storage *) (conn + 1);
		memcpy (conn->client_addr, client_addr,
				sizeof (struct sockaddr_storage));

		if (conn->client_addr->ss_family == AF_INET6)
			conn->port = ntohs (
				((struct sockaddr_in6 *) conn->client_addr)->sin6_port);
		else
			conn->port
				= ntohs (((struct sockaddr_in *) conn->client_addr)->sin_port);
	}
	else
	{
		conn->client_addr = NULL;
		conn->port = 0;
	}

	return conn;
}

void
fh_conn_destroy (struct fh_conn *conn)
{
	close (conn->sockfd);
	fh_pool_free (conn->pool);
}
