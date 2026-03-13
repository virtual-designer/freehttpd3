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

#include <stdlib.h>

#include "core/config.h"

void
fh_config_free (struct fh_config *config)
{
    for (size_t i = 0; i < config->host_count; i++)
    {
        free (config->hosts[i].docroot);
        free (config->hosts[i].ports);

        for (size_t j = 0; j < config->hosts[i].hostname_count; j++)
            free (config->hosts[i].hostnames[j]);

        free (config->hosts[i].hostnames);
        free (config->hosts[i].hostname_ports);

        for (size_t j = 0; j < config->hosts[i].route_count; j++)
        {
            free (config->hosts[i].routes[j]->docroot);
            free (config->hosts[i].routes[j]->route);
            free (config->hosts[i].routes[j]->redirect_url);
            free (config->hosts[i].routes[j]);
        }

        free (config->hosts[i].routes);
    }

    free (config->hosts);
    free (config);
}
