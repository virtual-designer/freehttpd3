#ifndef FHTTPD_SERVER_H
#define FHTTPD_SERVER_H

#include <stdbool.h>
#include "conf.h"

struct fh_server;

struct fh_server *fh_server_create (const struct fh_config *config);
bool fh_server_listen (struct fh_server *server);
void fh_server_free (struct fh_server *server);

#endif /* FHTTPD_SERVER_H */
