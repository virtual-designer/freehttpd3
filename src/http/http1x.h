#ifndef FHTTPD_HTTP1X_H
#define FHTTPD_HTTP1X_H

#include "mm/chain.h"
#include "mm/pool.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FH_HTTP1X_METHOD_MAX_LEN 8

enum fh_http1x_state
{
	H1_STATE_METHOD,
	H1_STATE_URI,
	H1_STATE_VERSION,
	H1_STATE_SPACE,
	H1_STATE_HEADER_NAME,
	H1_STATE_HEADER_VALUE,
	H1_STATE_BODY,
};

enum fh_http1x_err
{
	H1_ERR_NONE,
	H1_ERR_MEMORY,
	H1_ERR_PROTOCOL,
	H1_ERR_MALFORMED,
};

enum fh_http1x_method
{
	HTTP_GET,
	HTTP_POST,
};

struct fh_http1x_result
{
	enum fh_http1x_method method;
	char *uri;
	size_t uri_len;
};

struct fh_http1x_ctx
{
	struct fh_pool *pool;
	bool pool_freeable;
	enum fh_http1x_state state;
	enum fh_http1x_err error;
	struct fh_chain_cur cur_processed, cur_seen;
	struct fh_http1x_result result;
};

bool fh_http1x_ctx_init (struct fh_http1x_ctx *ctx,
						 struct fh_chain *start_chain, size_t start_off,
						 struct fh_pool *pool);
struct fh_http1x_ctx *fh_http1x_ctx_create (struct fh_chain *start_chain,
											size_t start_off);
void fh_http1x_ctx_cleanup (const struct fh_http1x_ctx *ctx);
void fh_http1x_ctx_free (struct fh_http1x_ctx *ctx);
bool fh_http1x_parse (struct fh_http1x_ctx *ctx);

#endif /* FHTTPD_HTTP1X_H */
