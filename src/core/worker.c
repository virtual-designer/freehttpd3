#define FH_LOG_MODULE_NAME "worker"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

#include "worker.h"
#include "log/log.h"
#include "utils/utils.h"

static bool should_terminate = false;

static void
handle_exit_signal (int signum __attribute__((unused)))
{
	should_terminate = true;
}

static void
fh_worker_terminate (struct fh_server *server)
{
    fh_server_destroy (server);
}

_noreturn void
fh_worker_start (struct fh_server *server)
{
	struct sigaction act;

	sigemptyset (&act.sa_mask);
	act.sa_handler = &handle_exit_signal;
	act.sa_flags = SA_RESTART;

	if (sigaction (SIGINT, &act, NULL) < 0 || sigaction (SIGTERM, &act, NULL) < 0)
        _exit (EXIT_FAILURE);

    fh_pr_info ("Ready for connections");

    while (true)
    {
        pause ();

		if (should_terminate)
		{
            fh_worker_terminate (server);
            exit (EXIT_SUCCESS);
        }
    }

    _exit (EXIT_FAILURE);
}
