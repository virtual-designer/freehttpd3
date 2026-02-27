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

#include "compat.h"
#include "core/server.h"
#define FH_LOG_MODULE_NAME "worker"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log/log.h"
#include "utils/utils.h"
#include "worker.h"

static bool should_terminate = false;

static void
handle_exit_signal (int signum __attribute__ ((unused)))
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

	if (sigaction (SIGINT, &act, NULL) < 0
		|| sigaction (SIGTERM, &act, NULL) < 0)
		_exit (EXIT_FAILURE);

	fh_pr_info ("Ready for connections");

	fh_server_wait (server, &should_terminate);
	fh_worker_terminate (server);
	exit (should_terminate ? EXIT_SUCCESS : EXIT_FAILURE);
}
