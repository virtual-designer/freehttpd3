#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define FH_LOG_MODULE_NAME "main"

#include "core/server.h"
#include "log/log.h"

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif /* HAVE_CONFIG_H */

static struct option const long_options[] = {
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'V' },
};

static const char *short_options = "hV";
static const char *argv0 = NULL;

static void
usage (void)
{
    fprintf (stdout, "freehttpd is lightweight HTTP server daemon.\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "Usage:\n");
    fprintf (stdout, "  %s [option...]\n", argv0);
    fprintf (stdout, "\n");
    fprintf (stdout, "Options:\n");
    fprintf (stdout, "  -h, --help      Show this help and exit.\n");
    fprintf (stdout, "  -V, --version   Show version information.\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "Bug reports and general messages can be sent\n");
    fprintf (stdout, "to <rakinar2@osndevs.org> directly.\n");
}

static void
show_version (void)
{
    fprintf (stdout, PACKAGE_NAME " version " PACKAGE_VERSION ".\n");
    fprintf (stdout, "License GPLv3.0+: This is free software.\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "Written by Ar Rakin.\n");
}

int
main (int argc, char **argv)
{
    argv0 = argv[0];

    for (;;)
    {
        int longind = 0;
        int c = getopt_long (argc, argv, short_options, long_options, &longind);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                usage ();
                exit (EXIT_SUCCESS);

            case 'V':
                show_version ();
                exit (EXIT_SUCCESS);

            default:
                exit (EXIT_FAILURE);
        }
    }

    const struct fh_config config = {
        .vhost_count = 1,
        .vhosts = & (struct fh_config_vhost) {
            .id_count = 1,
            .id_list = & (struct fh_config_vhost_id) {
                .hostname = "localhost",
                .port = 8080,
            },
            .docroot = "/var/www/html",
        },
    };

    struct fh_server *server = fh_server_create (&config);

    if (!server)
    {
        perror ("Unable to create server");
        return EXIT_FAILURE;
    }

    fh_server_listen (server);
    fh_server_free (server);

    return 0;
}
