#ifndef FHTTPD_WORKER_H
#define FHTTPD_WORKER_H

#include "compat.h"
#include "server.h"

_noreturn void fh_worker_start (struct fh_server *server);

#endif /* FHTTPD_WORKER_H */