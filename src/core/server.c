#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define FH_LOG_MODULE_NAME "server"

#include "hash/int_htable.h"
#include "hash/str_htable.h"
#include "log/log.h"
#include "server.h"
#include "utils/types.h"
#include "utils/utils.h"

#define FH_MAX_CONN SOMAXCONN

struct fh_port_table_entry
{
    /* (const char *host) -> (const struct fh_config_vhost *vhost) */
    str_htable_t *host_config_table;
    fd_t sockfd;
};

struct fh_server
{
    const struct fh_config *config;
    /* (uint64_t port) -> (struct port_table_entry *) */
    int_htable_t *port_table;
};

static fd_t
fh_server_create_socket (uint16_t port)
{
    fd_t sockfd = socket (AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        return -1;

    int val = 1;

    if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val) < 0)
        goto create_socket_err;

    if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof val) < 0)
        goto create_socket_err;

    struct timeval tv = {
        .tv_sec = 10,
    };

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0)
        goto create_socket_err;

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv) < 0)
        goto create_socket_err;

    if (!util_fd_set_nonblocking (sockfd))
        goto create_socket_err;

    struct sockaddr_in srv_addr = { 0 };

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons (port);
    srv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind (sockfd, (struct sockaddr *) &srv_addr, sizeof srv_addr) < 0)
        goto create_socket_err;

    return sockfd;

create_socket_err:
    close (sockfd);
    return -1;
}

static bool
fh_server_process_config (struct fh_server *server)
{
    for (size_t i = 0; i < server->config->vhost_count; i++)
    {
        struct fh_config_vhost *vhost = &server->config->vhosts[i];

        for (size_t j = 0; j < vhost->id_count; j++)
        {
            const struct fh_config_vhost_id *id = &vhost->id_list[j];
            struct fh_port_table_entry *port_entry = int_htable_get (
                server->port_table, (uint64_t) id->port);

            if (!port_entry)
            {
                port_entry = malloc (sizeof (*port_entry));

                if (!port_entry)
                    return false;

                port_entry->host_config_table = str_htable_create (16);
                port_entry->sockfd = 0;

                if (!port_entry->host_config_table)
                {
                    free (port_entry);
                    return false;
                }

                int_htable_set (server->port_table,
                                (uint64_t) id->port, port_entry);
            }

            str_htable_set (port_entry->host_config_table, id->hostname, vhost);
        }
    }

    int_htable_foreach (server->port_table, port_it)
    {
        struct fh_port_table_entry *port_entry = port_it.entry->data;
        const uint16_t port = (uint16_t) port_it.entry->key;
        const fd_t sockfd = fh_server_create_socket (port);

        if (sockfd < 0)
            return false;

        port_entry->sockfd = sockfd;
        fh_pr_debug ("Created socket=%d, port=%u", sockfd, port);
    }

    return true;
}

static void
fh_server_port_table_entry_cleanup (void *ptr)
{
    struct fh_port_table_entry *port_entry = ptr;

    /* TODO: Free the config! */    
    str_htable_free (port_entry->host_config_table);
    free (port_entry);
}

void
fh_server_free (struct fh_server *server)
{
    int_htable_free_with_cleanup (server->port_table, &fh_server_port_table_entry_cleanup);
    free (server);
}

struct fh_server *
fh_server_create (const struct fh_config *config)
{
    struct fh_server *server = calloc (1, sizeof (*server));

    if (!server)
        return NULL;

    server->config = config;
    server->port_table = int_htable_create (16);

    if (!server->port_table)
    {
        free (server);
        return NULL;
    }

    if (!fh_server_process_config (server))
    {
        int_htable_free (server->port_table);
        free (server);
        return NULL;
    }

    return server;
}

bool
fh_server_listen (struct fh_server *server)
{
    int_htable_foreach (server->port_table, port_it)
    {
        struct fh_port_table_entry *port_entry = port_it.entry->data;
        const fd_t sockfd = port_entry->sockfd;

        if (listen (sockfd, FH_MAX_CONN) < 0)
            return false;

        fh_pr_debug ("Listening: port=%u", (uint16_t) port_it.entry->key);
    }

    return true;
}
