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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "core/config.h"
#include "core/server.h"

int
main (void)
{
	char cwd[PATH_MAX] = {0};
	
	if (!getcwd (cwd, sizeof cwd - 1))
	{
		perror ("getcwd");
		exit (EXIT_FAILURE);
	}

	struct fh_config *config = calloc (1, sizeof (*config));

	config->host_count = 1;
	config->hosts = calloc (1, sizeof (struct fh_config_host));
	config->hosts->docroot = strdup (cwd);
	config->hosts->hostnames = calloc (1, sizeof (char *));
	config->hosts->hostnames[0] = strdup ("localhost");
	config->hosts->hostname_count = 1;
	config->hosts->ports = calloc (1, sizeof (uint16_t));
	config->hosts->ports[0] = 8080;
	config->hosts->port_count = 1;

	struct fh_server *server = fh_server_create (config);

	if (!server)
	{
		perror ("fh_server_create");
		exit (EXIT_FAILURE);
	}

	if (!fh_server_create_sockets (server))
	{
		perror ("fh_server_create_sockets");
		exit (EXIT_FAILURE);
	}

	puts ("Server started");
	fh_server_destroy (server);
	return EXIT_SUCCESS;
}
