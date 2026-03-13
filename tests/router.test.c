#undef NDEBUG

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server/router.h"

int
main (void)
{
    char cwd[PATH_MAX] = { 0 };

    if (!getcwd (cwd, sizeof cwd - 1))
    {
        perror ("getcwd");
        exit (EXIT_FAILURE);
    }

    struct fh_config *config = calloc (1, sizeof (*config));

    config->host_count = 1;
    config->worker_count = 8;
    config->hosts = calloc (1, sizeof (struct fh_config_host));
    config->hosts->docroot = strdup (cwd);
    config->hosts->hostnames = calloc (1, sizeof (char *));
    config->hosts->hostnames[0] = strdup ("localhost");
    config->hosts->hostname_ports = calloc (1, sizeof (uint16_t));
    config->hosts->hostname_ports[0] = 8080;
    config->hosts->hostname_count = 1;
    config->hosts->ports = calloc (4, sizeof (uint16_t));
    config->hosts->ports[0] = 8080;
    config->hosts->ports[1] = 4080;
    config->hosts->ports[2] = 4443;
    config->hosts->ports[3] = 8443;
    config->hosts->routes = calloc (4, sizeof (struct fh_config_route *));
    config->hosts->routes[0] = calloc (1, sizeof (struct fh_config_route));
    config->hosts->routes[0]->route = strdup (
        "/secret/.well-known/pki-validation/verification-284648262752263.txt");
    config->hosts->routes[0]->redirect_url = strdup ("https://www.google.com");
    config->hosts->routes[0]->redirect_status = 302;
    config->hosts->routes[0]->redirect_url_len
        = strlen (config->hosts->routes[0]->redirect_url);
    config->hosts->routes[1] = calloc (1, sizeof (struct fh_config_route));
    config->hosts->routes[1]->route
        = strdup ("/secret2/.well-known2/pki-validation/"
                  "verification-284648262752263.txt");
    config->hosts->routes[1]->redirect_url = strdup ("https://www.google.com");
    config->hosts->routes[1]->redirect_status = 302;
    config->hosts->routes[1]->redirect_url_len
        = strlen (config->hosts->routes[0]->redirect_url);
    config->hosts->routes[2] = calloc (1, sizeof (struct fh_config_route));
    config->hosts->routes[2]->route
        = strdup ("/secret2/.well-known2/pki-validation2/"
                  "verification2-284648262752263.txt");
    config->hosts->routes[2]->redirect_url = strdup ("https://www.google.com");
    config->hosts->routes[2]->redirect_status = 302;
    config->hosts->routes[2]->redirect_url_len
        = strlen (config->hosts->routes[0]->redirect_url);
    config->hosts->routes[3] = calloc (1, sizeof (struct fh_config_route));
    config->hosts->routes[3]->route
        = strdup ("/test_route/path");
    config->hosts->routes[3]->is_wildcard = true;
    config->hosts->routes[3]->redirect_url = strdup ("https://www.google.com");
    config->hosts->routes[3]->redirect_status = 302;
    config->hosts->routes[3]->redirect_url_len
        = strlen (config->hosts->routes[0]->redirect_url);
    config->hosts->route_count = 4;
    config->hosts->port_count = 4;

    struct fh_router *router = fh_router_create (config, &config->hosts[0]);

    assert (fh_router_set_route_config (router, config->hosts->routes[0]));
    assert (fh_router_set_route_config (router, config->hosts->routes[1]));
    assert (fh_router_set_route_config (router, config->hosts->routes[2]));
    assert (fh_router_set_route_config (router, config->hosts->routes[3]));

    assert (
        fh_router_get_route (router, config->hosts->routes[0]->route, 0)->config
        == config->hosts->routes[0]);
    assert (
        fh_router_get_route (router, config->hosts->routes[1]->route, 0)->config
        == config->hosts->routes[1]);
    assert (
        fh_router_get_route (router, config->hosts->routes[2]->route, 0)->config
        == config->hosts->routes[2]);
    assert (
        fh_router_get_route (router, config->hosts->routes[3]->route, 0)->config
        == config->hosts->routes[3]);
    assert (
        fh_router_get_route (router, "/test_route", 0)->config
        == router->root_route_config);
    assert (
        fh_router_get_route (router, "/test_route/path/subpath", 0)->config
        == config->hosts->routes[3]);

    fh_router_print_routes (router);

    fh_router_destroy (router);
    fh_config_free (config);

    return 0;
}
