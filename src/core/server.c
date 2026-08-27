#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "utils/types.h"
#include "utils/utils.h"

#define FH_MAX_CONN SOMAXCONN

struct fh_server
{
    fd_t *sockfd_list;
    size_t sockfd_count;
    const struct fh_config *config;
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

    struct sockaddr_in srv_addr = {0};

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
    uint16_t *port_list = NULL;
    size_t port_count = 0;

    for (size_t i = 0; i < server->config->vhost_count; i++)
    {
        const struct fh_config_vhost *vhost = &server->config->vhosts[i];

        for (size_t j = 0; j < vhost->id_count; j++)
        {
            const struct fh_config_vhost_id *id = &vhost->id_list[j];

            for (size_t k = 0; k < port_count; k++)
            {
                if (port_list[k] == id->port)
                    goto skip_id;
            }

            uint16_t *port_list_new
                = realloc (port_list, sizeof (uint16_t) * (port_count + 1));

            if (!port_list_new)
            {
                free (port_list);
                return false;
            }

            port_list = port_list_new;
            port_list[port_count++] = id->port;

        skip_id:
        }
    }

    server->sockfd_list = calloc (sizeof (fd_t), port_count);

    if (!server->sockfd_list)
        goto process_config_err;

    for (size_t i = 0; i < port_count; i++)
    {
        uint64_t port = port_list[i];
        fd_t sockfd = fh_server_create_socket (port);

        if (sockfd < 0)
            goto process_config_err;
        
        server->sockfd_list[server->sockfd_count++] = sockfd;
        printf("Created socket for port: %u\n", port);
    }

    free (port_list);
    return true;

process_config_err:
    free (port_list);
    return false;
}

void
fh_server_free (struct fh_server *server) 
{
    free (server->sockfd_list);
    free (server);
}

struct fh_server *
fh_server_create (const struct fh_config *config)
{
    struct fh_server *server = calloc (1, sizeof (*server));

    if (!server)
        return NULL;

    server->config = config;

    if (!fh_server_process_config (server))
    {
        free (server);
        return NULL;
    }

    return server;
}

bool 
fh_server_listen (struct fh_server *server)
{
    for (size_t i = 0; i < server->sockfd_count; i++)
    {
        fd_t sockfd = server->sockfd_list[i];

        if (listen (sockfd, FH_MAX_CONN) < 0)
            return false;
    }

    return true;
}