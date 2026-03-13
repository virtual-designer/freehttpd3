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

#ifndef FHTTPD_CONFIG_H
#define FHTTPD_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct fh_config_host
{
    uint16_t *ports;
    size_t port_count;
    char **hostnames;
    uint16_t *hostname_ports;
    size_t hostname_count;
    char *docroot;
    struct fh_config_route **routes;
    size_t route_count;
};

struct fh_config_route
{
	char *route;
	char *docroot;
	char *redirect_url;
	size_t redirect_url_len;
	uint16_t redirect_status;
	bool is_wildcard : 1;
};

struct fh_config
{
    struct fh_config_host *hosts;
    size_t host_count;
    size_t worker_count;
};

void fh_config_free (struct fh_config *config);

#endif /* FHTTPD_CONFIG_H */
