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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#include "types.h"

struct fh_server;
struct fh_module;

struct fh_conn_module_data
{
	void *ptr;
	void (*cleanup_cb) (void *);
	struct fh_conn_module_data *prev;
};

struct fh_conn
{
	uint64_t id;
	struct sockaddr_storage *client_addr;
	fd_t sockfd;
	uint16_t port;
	struct fh_pool *pool;
	struct fh_conn_module_data *module_data;
	bool *module_data_initialized;
	struct fh_conn_module_data *module_data_tail;
	size_t module_data_count;
};

struct fh_conn *fh_conn_create (const struct fh_server *server, fd_t client_fd,
								const struct sockaddr_storage *client_addr);
void fh_conn_destroy (struct fh_conn *conn);
void *fh_conn_get_module_data (struct fh_module *module, struct fh_conn *conn);
bool fh_conn_set_module_data (struct fh_module *module, struct fh_conn *conn,
							  void *data,
							  void (*cleanup_cb) (void *));

#endif /* FHTTPD_CONNECTION_H */
