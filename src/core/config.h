#ifndef FHTTPD_CONFIG_H
#define FHTTPD_CONFIG_H

#include <stddef.h>
#include <stdint.h>

struct fh_config_host
{
    uint16_t *ports;
    size_t port_count;
    char **hostnames;
    uint16_t *hostname_ports;
    size_t hostname_count;
    char *docroot;
};

struct fh_config
{
    struct fh_config_host *hosts;
    size_t host_count;
    size_t worker_count;
};

void fh_config_free (struct fh_config *config);

#endif /* FHTTPD_CONFIG_H */