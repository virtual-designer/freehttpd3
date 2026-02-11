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

#ifndef FHTTPD_SERVER_H
#define FHTTPD_SERVER_H

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "core/config.h"
#include "hash/itable.h"
#include "types.h"
#include "xpoll.h"

struct fh_server
{
	struct xpoll *xp;
	struct fh_config *config;
	/* (fd_t) => (bool *) */
	struct itable *sockfd_table;
	pid_t *workers;
	size_t worker_count;
	size_t current_worker_index;
	bool is_worker;
};

struct fh_server *fh_server_create (struct fh_config *config);
void fh_server_destroy (struct fh_server *server);
bool fh_server_start (struct fh_server *server);
bool fh_server_wait (struct fh_server *server, bool *should_terminate);

#endif /* FHTTPD_SERVER_H */
