#ifndef FHTTPD_CONF_H
#define FHTTPD_CONF_H

#include <stdint.h>
#include <stddef.h>

struct fh_config_vhost_id
{
    char *hostname;
    uint16_t port;
};

struct fh_config_vhost
{
    struct fh_config_vhost_id *id_list;
    size_t id_count;
    char *docroot;
};

struct fh_config
{
    struct fh_config_vhost *vhosts;
    size_t vhost_count;
};

#endif /* FHTTPD_CONF_H */
