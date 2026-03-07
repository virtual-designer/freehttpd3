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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection.h"
#include "mm/pool.h"
#include "module.h"
#include "server.h"
#include "types.h"

static uint64_t conn_id = 0;

struct fh_conn *
fh_conn_create (const struct fh_server *server, fd_t client_fd,
                const struct sockaddr_storage *client_addr)
{
    const size_t client_addr_size = (client_addr ? sizeof (*client_addr) : 0);
    const size_t pool_prealloc_size
        = 4096 + sizeof (struct fh_conn) + client_addr_size
          + (sizeof (struct fh_conn_module_data) * server->module_count)
          + (sizeof (bool) * server->module_count);

    struct fh_pool *pool = fh_pool_create (pool_prealloc_size);

    if (!pool)
        return NULL;

    struct fh_conn *conn = fh_pool_alloc (
        pool, sizeof (*conn) + client_addr_size
                  + (sizeof (struct fh_conn_module_data) * server->module_count)
                  + (sizeof (bool) * server->module_count));

    if (!conn)
    {
        fh_pool_free (pool);
        return NULL;
    }

    conn->id = conn_id++;
    conn->pool = pool;
    conn->chain_pool = NULL;
    conn->chain_list = NULL;
    conn->last_proc_cur = NULL;
	conn->to_be_closed = false;
    conn->sockfd = client_fd;
    conn->module_data = (struct fh_conn_module_data *) (((uint8_t *) (conn + 1))
                                                        + client_addr_size);
    conn->module_data_initialized
        = (bool *) (conn->module_data + server->module_count);
    conn->module_data_count = server->module_count;
    conn->module_data_tail = NULL;

    memset (conn->module_data_initialized, false,
            sizeof (bool) * server->module_count);

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
    struct fh_conn_module_data *module_data = conn->module_data_tail;

    while (module_data)
    {
        if (module_data->cleanup_cb)
            module_data->cleanup_cb (module_data->ptr);

        module_data = module_data->prev;
    }

    close (conn->sockfd);

    if (conn->chain_pool)
        fh_pool_free (conn->chain_pool);

    fh_pool_free (conn->pool);
}

void *
fh_conn_get_module_data (struct fh_module *module, struct fh_conn *conn)
{
    if (module->id >= conn->module_data_count)
        return NULL;

    return conn->module_data[module->id].ptr;
}

bool
fh_conn_set_module_data (struct fh_module *module, struct fh_conn *conn,
                         void *data, void (*cleanup_cb) (void *))
{
    if (module->id >= conn->module_data_count)
        return false;

    struct fh_conn_module_data *module_data = &conn->module_data[module->id];

    module_data->ptr = data;
    module_data->cleanup_cb = cleanup_cb;

    if (module_data->cleanup_cb && !conn->module_data_initialized[module->id])
    {
        if (conn->module_data_tail)
            module_data->prev = conn->module_data_tail;
        else
            module_data->prev = NULL;

        conn->module_data_tail = module_data;
        conn->module_data_initialized[module->id] = true;
    }

    return true;
}

void
fh_conn_close_from_module (struct fh_conn *conn)
{
	conn->to_be_closed = true;
}
