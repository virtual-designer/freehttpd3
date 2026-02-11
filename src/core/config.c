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
    }

    free (config->hosts);
    free (config);
}
