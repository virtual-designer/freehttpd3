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
#    define _GNU_SOURCE
#elif PLATFORM_BSD
#    define _DARWIN_C_SOURCE
#    define _BSD_SOURCE
#endif /* PLATFORM_LINUX */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "compat.h"
#include "connection.h"
#include "core/config.h"
#include "core/hooks.h"
#include "core/xpoll.h"
#include "hash/itable.h"
#include "log/log.h"
#include "mm/chain.h"
#include "mm/pool.h"
#include "module.h"
#include "server.h"
#include "types.h"
#include "utils/utils.h"
#include "worker.h"

#ifdef HAVE_CONFIG_H
#    include "../../config.h"
#endif /* HAVE_CONFIG_H */

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
    server->hook_list = fh_hook_list_create ();

    if (!server->hook_list)
    {
        free (server);
        return NULL;
    }

    return server;
}

void
fh_server_destroy (struct fh_server *server)
{
    if (server->xpoll)
        xpoll_destroy (server->xpoll);

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

    fh_hook_list_free (server->hook_list);

    for (size_t i = 0; i < server->module_count; i++)
        fh_module_handle_cleanup (server->modules[i], false);

    free (server->modules);

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

static bool
fh_server_register_module (struct fh_server *server,
                           struct fh_module_handle *handle)
{
    if (handle->public_module->id >= server->module_count)
    {
        struct fh_module_handle **modules
            = realloc (server->modules,
                       sizeof (*modules) * (handle->public_module->id + 1));

        if (!modules)
            return false;

        server->modules = modules;
        server->module_count = handle->public_module->id + 1;
    }

    server->modules[handle->public_module->id] = handle;
    return true;
}

__attribute__ ((unused)) static bool
fh_server_unregister_module (struct fh_server *server,
                             const struct fh_module_handle *handle)
{
    if (handle->public_module->id >= server->module_count)
        return false;

    server->modules[handle->public_module->id] = NULL;
    return true;
}

static bool
fh_server_load_modules (struct fh_server *server, char *module_dir)
{
    char *MODULE_DIR_PATH = module_dir ? module_dir : FH_MODULE_PATH;
    char *paths[] = { MODULE_DIR_PATH, NULL };

    FTS *fts = fts_open (paths, FTS_LOGICAL, NULL);

    if (!fts)
        return false;

    FTSENT *ent;

    while ((ent = fts_read (fts)) != NULL)
    {
        if (!(ent->fts_info & FTS_F))
            continue;

        const char *ext = get_file_ext (fts->fts_path);

        if (!ext
            || strncmp (ext, SHARED_LIBRARY_EXTENSION,
                        sizeof (SHARED_LIBRARY_EXTENSION)))
            continue;

        fh_pr_info ("Loading module: %s", ent->fts_path);

        struct fh_module_handle *handle
            = fh_module_handle_load (server, ent->fts_path);

        if (!handle || handle->err)
        {
            fh_pr_err ("Error loading module: %s: %s", ent->fts_path,
                       fh_module_handle_get_last_err (handle));

            if (handle)
                fh_module_handle_cleanup (handle, false);

            continue;
        }

        if (!fh_server_register_module (server, handle))
        {
            fh_pr_err ("Unable to register module: %s: %s", ent->fts_path,
                       strerror (errno));
            fh_module_handle_cleanup (handle, false);
        }
    }

    fts_close (fts);
    return true;
}

static fd_t
fh_server_create_socket (struct fh_server *server __attribute__ ((unused)),
                         int domain, uint16_t port)
{
#ifdef SOCK_NONBLOCK
    fd_t sockfd
        = socket (domain, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

    if (sockfd < 0)
        return -1;
#else  /* not SOCK_NONBLOCK */
    fd_t sockfd = socket (domain, SOCK_STREAM, 0);

    if (sockfd < 0)
        return -1;

    if (!fd_add_flags (sockfd, O_NONBLOCK | FD_CLOEXEC))
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

            if (!server->xpoll)
                server->xpoll = xpoll_create ();

            if (!server->xpoll)
                _exit (EXIT_FAILURE);

            for_each_itable_entry (server->sockfd_table, sockfd)
            {
                if (xpoll_register_fd (server->xpoll, (fd_t) sockfd->key,
                                       XPOLL_IN, 0)
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

int last_signum = -1;

static void
handle_exit_signal (int signum __attribute__ ((unused)))
{
    should_terminate = true;
    last_signum = signum;
}

bool
fh_server_start (struct fh_server *server)
{
    if (!fh_server_create_sockets (server))
        return false;

    if (!fh_server_load_modules (server, NULL))
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
        for (size_t i = 0; i < server->worker_count; i++)
        {
            int stat_loc;
            waitpid (server->workers[i], &stat_loc, 0);

            if (should_terminate)
            {
                fh_pr_warn ("Signal: %s", strsignal (last_signum));
                return true;
            }
        }
    }

    return true;
}

static bool
fh_server_add_conn (struct fh_server *server, fd_t client_fd,
                    const struct sockaddr_storage *client_addr)
{
    struct fh_conn *conn = fh_conn_create (server, client_fd, client_addr);

    if (!conn)
        return false;

    if (xpoll_register_fd (server->xpoll, client_fd, XPOLL_IN, XPOLL_ET) != 0)
    {
        fh_conn_destroy (conn);
        return false;
    }

    if (!itable_set (server->conn_table, (uint64_t) client_fd, conn))
    {
        xpoll_unregister_fd (server->xpoll, client_fd, XPOLL_IN);
        fh_conn_destroy (conn);
        return false;
    }

    for (struct fh_hook_cb *cb = server->hook_list->heads[FH_HOOK_CONN_PROBE];
         cb; cb = cb->next)
    {
        if (cb->module_id >= server->module_count)
            continue;

        fh_hook_conn_probe_cb_t final_cb = (fh_hook_conn_probe_cb_t) cb->cb_ptr;
        bool ret
            = final_cb (server->modules[cb->module_id]->public_module, conn);

        if (!ret)
            fh_pr_err ("Module '%s' CONN_PROBE hook failed",
                       server->modules[cb->module_id]->public_module->name);
    }

    return true;
}

static bool
fh_server_close_conn (struct fh_server *server, struct fh_conn *conn)
{
    xpoll_unregister_fd (server->xpoll, conn->sockfd, XPOLL_IN | XPOLL_OUT);
    itable_remove (server->conn_table, (uint64_t) conn->sockfd);

    for (struct fh_hook_cb *cb = server->hook_list->heads[FH_HOOK_CONN_CLEANUP];
         cb; cb = cb->next)
    {
        if (cb->module_id >= server->module_count)
            continue;

        fh_hook_conn_cleanup_cb_t final_cb
            = (fh_hook_conn_cleanup_cb_t) cb->cb_ptr;
        bool ret
            = final_cb (server->modules[cb->module_id]->public_module, conn);

        if (!ret)
            fh_pr_err ("Module '%s' CONN_CLEANUP hook failed",
                       server->modules[cb->module_id]->public_module->name);
    }

    fh_conn_destroy (conn);
    return true;
}

static bool
fh_server_close_fd (struct fh_server *server, fd_t fd)
{
    xpoll_unregister_fd (server->xpoll, fd, XPOLL_IN | XPOLL_OUT);
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
                             &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else  /* not HAVE_ACCEPT4 */
        client_fd = accept (server_fd, (struct sockaddr *) &client_addr,
                            &client_addr_len);
#endif /* HAVE_ACCEPT4 */

        if (client_fd < 0)
        {
            if (errno == EINTR)
                continue;

            if (ERR_WOULD_BLOCK)
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
        if (!fd_add_flags (client_fd, O_NONBLOCK | FD_CLOEXEC))
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
    if (!unlikely (server->hook_list->heads[FH_HOOK_STREAM_READ]))
    {
        fh_pr_err (
            "No STREAM_READ hook was registered. Please ensure all required "
            "protocol modules are correctly installed and loaded.");
        fh_server_close_conn (server, conn);
        return false;
    }

    if (!conn->chain_pool)
    {
        conn->chain_pool = fh_pool_create (4096);

        if (!conn->chain_pool)
        {
            fh_server_close_conn (server, conn);
            return false;
        }
    }

    if (!conn->chain_list)
    {
        conn->chain_list
            = fh_pool_zalloc (conn->pool, sizeof (*conn->chain_list)
                                              + sizeof (*conn->last_proc_cur));

        if (!conn->chain_list)
        {
            fh_server_close_conn (server, conn);
            return false;
        }

        conn->last_proc_cur = (struct fh_chain_cur *) (conn->chain_list + 1);
    }

    if (!fh_chain_read (conn->chain_pool, conn->sockfd, &conn->chain_list->head,
                        &conn->chain_list->tail))
    {
        fh_pr_err ("Connection %" PRIu64 ": Read error: %s", conn->id,
                   strerror (errno));
        fh_server_close_conn (server, conn);
        return false;
    }

    if (!conn->last_proc_cur->chain)
        conn->last_proc_cur->chain = conn->chain_list->head;

    for (struct fh_hook_cb *cb = server->hook_list->heads[FH_HOOK_STREAM_READ];
         cb; cb = cb->next)
    {
        if (cb->module_id >= server->module_count)
            continue;

        fh_hook_stream_read_cb_t final_cb
            = (fh_hook_stream_read_cb_t) cb->cb_ptr;
        bool ret = final_cb (server->modules[cb->module_id]->public_module,
                             conn, conn->last_proc_cur);

        if (!ret)
            fh_pr_err ("Module '%s' STREAM_READ hook failed",
                       server->modules[cb->module_id]->public_module->name);
    }

    conn->last_proc_cur->chain = conn->chain_list->tail;
    conn->last_proc_cur->off = conn->chain_list->tail->buf->mem_size;

    /* fh_server_close_conn (server, conn); */
    return true;
}

static bool
fh_server_on_write (struct fh_server *server, struct fh_conn *conn)
{
    if (!unlikely (server->hook_list->heads[FH_HOOK_STREAM_WRITE]))
    {
        fh_pr_err (
            "No STREAM_WRITE hook was registered. Please ensure all required "
            "protocol modules are correctly installed and loaded.");
        fh_server_close_conn (server, conn);
        return false;
    }

    fh_server_close_conn (server, conn);
    return true;
}

bool
fh_server_wait (struct fh_server *server, bool *should_terminate)
{
    xevent_t events[XPOLL_MAX_EVENTS];

    for (;;)
    {
        if (*should_terminate)
            return true;

        const int n_fds
            = xpoll_wait (server->xpoll, events, XPOLL_MAX_EVENTS, 5000);

        if (n_fds < 0)
        {
            if (ERR_WOULD_INTERRUPT)
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
                fh_pr_err ("xpoll error: client_fd=%d, errno=%d, msg=\"%s\"",
                           fd, errno, strerror (errno));
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
                }
            }
            else if (kind & XPOLL_OUT)
            {
                fh_pr_info ("xpoll write notification: client_fd=%d", fd);

                if (!fh_server_on_write (server, conn))
                {
                    fh_pr_err ("write event handler failed: %s",
                               strerror (errno));
                }
            }
        }
    }

    return true;
}
