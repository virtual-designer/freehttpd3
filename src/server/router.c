#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "router.h"
#include "utils/strutils.h"

struct fh_router *
fh_router_create (const struct fh_config *config,
                  const struct fh_config_host *host_config)
{
    struct fh_router *router
        = calloc (1, sizeof (*router) + sizeof (*router->root_route_config));

    if (!router)
        return NULL;

    router->config = config;
    router->host_config = host_config;

    router->root_route = calloc (1, sizeof (*router->root_route));

    if (!router->root_route)
    {
        free (router);
        return NULL;
    }

    router->root_route_config = (struct fh_config_route *) (router + 1);
    router->root_route_config->route = strdup ("/");
    router->root_route_config->docroot = strdup (host_config->docroot);
    router->root_route_config->is_wildcard = true;

    router->root_route->part = strdup ("/");
    router->root_route->len = 1;
    router->root_route->config = router->root_route_config;

    return router;
}

static void
fh_route_destroy (struct fh_route *route)
{
    for (size_t i = 0; i < route->child_count; i++)
        fh_route_destroy (route->children[i]);

    free (route->children);
    free (route->part);
    free (route);
}

void
fh_router_destroy (struct fh_router *router)
{
    fh_route_destroy (router->root_route);
    free (router->root_route_config->route);
    free (router->root_route_config->docroot);
    free (router);
}

const struct fh_route *
fh_router_get_route (struct fh_router *router, const char *uri, size_t uri_len)
{
    uri_len = uri_len == 0 ? strlen (uri) : uri_len;

    struct fh_route *last_wildcard = router->root_route;
    struct fh_route *last = router->root_route;

    if (uri_len < 1)
        return last_wildcard;

    uri++;
    uri_len--;

    while (uri_len)
    {
        bool found = false;

        for (size_t i = 0; i < last->child_count; i++)
        {
            struct fh_route *route = last->children[i];

            if (route->len > uri_len || memcmp (route->part, uri, route->len)
                || (uri[route->len] != '/' && uri[route->len] != 0))
                continue;

            last = route;

            if (route->config && route->config->is_wildcard)
                last_wildcard = route;

            found = true;
            uri += route->len;
            uri_len -= route->len;

            if (*uri == '/')
            {
                uri++;
                uri_len--;
            }

            break;
        }

        if (!found)
            break;
    }

    return uri_len ? last_wildcard : last;
}

bool
fh_router_set_route_config (struct fh_router *router,
                            struct fh_config_route *config)
{
    struct fh_route *last_parent = router->root_route;
    const char *uri = config->route;
    size_t uri_len = strlen (uri);

    if (!*uri || uri[0] != '/')
        return false;

    uri++;
    uri_len--;
    bool found = true;

    while (uri_len && found)
    {
        found = false;

        for (size_t i = 0; i < last_parent->child_count; i++)
        {
            struct fh_route *route = last_parent->children[i];

            if (route->len > uri_len || memcmp (route->part, uri, route->len)
                || (uri[route->len] != '/' && uri[route->len] != 0))
                continue;

            last_parent = route;
            found = true;
            uri += route->len;
            uri_len -= route->len;

            if (*uri == '/')
            {
                uri++;
                uri_len--;
            }

            break;
        }
    }

    if (!uri_len)
    {
        last_parent->config = config;
        return true;
    }

    struct str_split_result *result = str_split (uri, "/");

    if (!result)
        return false;

    for (size_t i = 0; i < result->count; i++)
    {
        if (!result->strings[i] || !result->strings[i][0])
        {
            free (result->strings[i]);
            continue;
        }

        struct fh_route **children = realloc (
            last_parent->children,
            sizeof (struct fh_route *) * (last_parent->child_count + 1));

        if (!children)
            goto alloc_fail;

        last_parent->children = children;
        last_parent->child_count++;

        children[last_parent->child_count - 1]
            = calloc (1, sizeof (struct fh_route));

        if (!children[last_parent->child_count - 1])
            goto alloc_fail;

        struct fh_route *route = children[last_parent->child_count - 1];
        const bool is_last = i == result->count - 1;

        route->part = result->strings[i];
        route->len = strlen (result->strings[i]);
        route->config = is_last ? config : last_parent->config;

        last_parent = route;
    }

    result->count = 0;
    str_split_free (result);
    return true;

alloc_fail:
    result->count = 0;
    str_split_free (result);
    return false;
}

static inline void
fh_router_print_route (const struct fh_route *route, int indent)
{
    printf ("%*s%c %s\n", indent * 2, "", route->child_count ? '+' : '-',
            route->part);

    for (size_t i = 0; i < route->child_count; i++)
        fh_router_print_route (route->children[i], indent + 1);
}

void
fh_router_print_routes (const struct fh_router *router)
{
    fh_router_print_route (router->root_route, 0);
}
