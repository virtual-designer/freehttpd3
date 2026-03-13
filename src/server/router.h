#ifndef FH_ROUTER_H
#define FH_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/config.h"

struct fh_route
{
    char *part;
    size_t len;
    struct fh_route **children;
    size_t child_count;
    const struct fh_config_route *config;
};

struct fh_router
{
    const struct fh_config *config;
    const struct fh_config_host *host_config;
    struct fh_config_route *root_route_config;
    struct fh_route *root_route;
};

struct fh_router *fh_router_create (const struct fh_config *config,
                                    const struct fh_config_host *host_config);
void fh_router_destroy (struct fh_router *router);
void fh_router_print_routes (const struct fh_router *router);
bool fh_router_set_route_config (struct fh_router *router,
                                 struct fh_config_route *config);
const struct fh_route *fh_router_get_route (struct fh_router *router,
                                            const char *uri, size_t uri_len);

#endif /* FH_ROUTER_H */
