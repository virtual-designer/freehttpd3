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

#ifndef FHTTPD_CONNECTION_H
#define FHTTPD_CONNECTION_H

#include "types.h"
#include <stdint.h>
#include <sys/socket.h>

struct fh_server;

struct fh_conn
{
	struct sockaddr_storage *client_addr;
	fd_t sockfd;
	uint16_t port;
	struct fh_pool *pool;
	void *module_data;
	size_t module_data_size;
};

struct fh_conn *fh_conn_create (const struct fh_server *server, fd_t client_fd,
								const struct sockaddr_storage *client_addr);
void fh_conn_destroy (struct fh_conn *conn);

#endif /* FHTTPD_CONNECTION_H */
