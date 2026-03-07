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

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#include "core/config.h"
#include "core/hooks.h"
#include "hash/itable.h"
#include "types.h"
#include "xpoll.h"

struct sockfd_info
{
    int family;
};

struct fh_module_handle;

struct fh_server
{
    struct xpoll *xpoll;
    struct fh_config *config;
    /* (fd_t) => (struct sockfd_info *) */
    struct itable *sockfd_table;
    /* (fd_t) => (struct fh_conn *) */
    struct itable *conn_table;
    struct fh_module_handle **modules;
    size_t module_count;
    struct fh_hook_list *hook_list;
    pid_t *workers;
    size_t worker_count;
    size_t current_worker_index;
    bool is_worker;
};

struct fh_server *fh_server_create (struct fh_config *config);
void fh_server_destroy (struct fh_server *server);
bool fh_server_start (struct fh_server *server);
bool fh_server_wait (struct fh_server *server, bool *should_terminate);
bool fh_server_close_conn (struct fh_server *server, struct fh_conn *conn);

#endif /* FHTTPD_SERVER_H */
