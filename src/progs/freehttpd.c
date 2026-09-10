#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FH_LOG_MODULE_NAME "main"

#include "core/server.h"
#include "log/log.h"
#include "utils/utils.h"
#include "event/xio.h"

#ifdef HAVE_CONFIG_H
    #include "config.h"
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
    fprintf (stdout, PACKAGE_NAME " version " PACKAGE_VERSION
                                  " (" FH_TARGET_SYSTEM_TYPE ")\n");
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

    fh_log_init ();

        struct fh_xio *xio = fh_xio_create ();

    if (!xio)
    {
        fprintf (stderr, "%s: failed to initialize XIO: %s\n", argv0,
                 strerror (errno));
        return EXIT_FAILURE;
    }
    int cnt = 0;

    int fds[5];

lbl:
    for (int i = 1; i <= 5; i++)
    {
        char path[512];
        snprintf (path, sizeof path,
                  "/home/rakinar2/Projects/freehttpd3/tmp/%d.txt", i);

        fd_t fd = open (path, O_RDONLY);

        if (fd < 0)
        {
            perror ("open");
            fh_xio_free (xio);
            return EXIT_FAILURE;
        }

        fds[i - 1] = fd;

        int rc = fh_xio_request_read (xio, (void *) i, fd, NULL, 128, 0);

        if (rc != 0)
        {
            perror ("fh_xio_request_read");
            fh_xio_free (xio);
            return EXIT_FAILURE;
        }
    }

    struct fh_xio_result results[64];
    ssize_t count = fh_xio_wait (xio, results, 64, 5000);

    if (count < 0)
    {
        perror ("fh_xio_wait");
        fh_xio_free (xio);
        return EXIT_FAILURE;
    }

    printf ("Completions: %zi\n", count);

    for (ssize_t i = 0; i < count; i++)
    {
        signed int status = fh_xio_result_status (&results[i]);

        if (status < 0)
        {
            printf ("Failed %zi: %s\n", i, strerror (errno));
            continue;
        }

        void *data = fh_xio_result_get_udata (&results[i]);
        
        printf ("[%zi] data: %p\n", i, data);
        printf ("[%zi] size: %i\n", i, status);
        void *buf = fh_xio_result_get_buf (&results[i]);
        printf ("[%zi] buf: %p\n", i, buf);

        printf ("-------:RAW DATA:---------\n");
        fwrite (buf, 1, (size_t) status, stdout);
        printf ("-------:RAW DATA END:---------\n");

        fh_xio_result_free (xio, &results[i]);
    }

    cnt++;

    if (cnt < 3)
    {
        for (int i = 0; i < 5; i++)
            close (fds[i]);
            
        puts ("+=======================AGAIN===================+");
        goto lbl;
    }

    fh_xio_free (xio);
    return 0;

    const struct fh_config config = {
        .vhost_count = 2,
        .vhosts = (struct fh_config_vhost []) {
            {
                .id_count = 1,
                .id_list = (struct fh_config_vhost_id []) {
                    { 
                        .hostname = "localhost",
                        .port = 8080,
                    },
                },
                .docroot = "/var/www/html",
            },
            {
                .id_count = 1,
                .id_list = (struct fh_config_vhost_id[]) {
                    { 
                        .hostname = "localhost",
                        .port = 8443,
                    },
                },
                .docroot = "/var/www/html",
            },
            {
                .id_count = 1,
                .id_list = (struct fh_config_vhost_id[]) {
                    { 
                        .hostname = "example.com",
                        .port = 8443,
                    },
                },
                .docroot = "/var/www/html",
            }
        },
    };

    struct fh_server *server = fh_server_create (&config);

    if (!server)
    {
        fh_pr_err ("Unable to create server: %s", strerror (errno));
        return EXIT_FAILURE;
    }

    fh_server_listen (server);
    fh_server_free (server);

    return 0;
}
