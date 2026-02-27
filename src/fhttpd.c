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

#include <stdint.h>
#define FH_LOG_MODULE_NAME "main"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/config.h"
#include "core/server.h"
#include "log/log.h"

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
	config->worker_count = 4;
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
	config->hosts->port_count = 4;

	fh_pr_info ("Starting server");
	struct fh_server *server = fh_server_create (config);

	if (!server)
	{
		fh_pr_err ("fh_server_create: %s", strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (!fh_server_start (server))
	{
		fh_pr_err ("fh_server_start: %s", strerror (errno));
		exit (EXIT_FAILURE);
	}

	fh_pr_info ("Server terminated");
	fh_server_destroy (server);
	return EXIT_SUCCESS;
}
