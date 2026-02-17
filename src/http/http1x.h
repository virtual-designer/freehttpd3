#ifndef FHTTPD_HTTP1X_H
#define FHTTPD_HTTP1X_H

#include <stdint.h>
#include "mm/pool.h"

struct fh_http1x_ctx
{
	struct fh_pool *pool;
	uint8_t *buf;
	size_t capacity, size;
};

#endif /* FHTTPD_HTTP1X_H */
