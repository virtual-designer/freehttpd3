#define FH_LOG_MODULE_NAME "http1x"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "http1x.h"
#include "log/log.h"
#include "mm/chain.h"
#include "mm/pool.h"

bool
fh_http1x_ctx_init (struct fh_http1x_ctx *ctx, struct fh_chain *start_chain,
					size_t start_off, struct fh_pool *pool)
{
	if (!pool)
	{
		ctx->pool_freeable = true;
		pool = fh_pool_create (4096);
	}
	else
	{
		ctx->pool_freeable = false;
	}

	if (!pool)
		return false;

	ctx->pool = pool;
	ctx->cur_seen.chain = ctx->cur_processed.chain = start_chain;
	ctx->cur_seen.off = ctx->cur_processed.off = start_off;
	ctx->state = H1_STATE_METHOD;
	ctx->error = H1_ERR_NONE;

	return true;
}

struct fh_http1x_ctx *
fh_http1x_ctx_create (struct fh_chain *start_chain, size_t start_off)
{
	struct fh_pool *pool = fh_pool_create (4096);

	if (!pool)
		return NULL;

	struct fh_http1x_ctx *ctx = fh_pool_alloc (pool, sizeof (*ctx));

	if (!ctx || !fh_http1x_ctx_init (ctx, start_chain, start_off, pool))
	{
		fh_pool_free (pool);
		return NULL;
	}

	return ctx;
}

void
fh_http1x_ctx_cleanup (const struct fh_http1x_ctx *ctx)
{
	if (ctx->pool_freeable)
		fh_pool_free (ctx->pool);
}

void
fh_http1x_ctx_free (struct fh_http1x_ctx *ctx)
{
	fh_http1x_ctx_cleanup (ctx);
	fh_pool_free (ctx->pool);
}

bool
fh_http1x_parse (struct fh_http1x_ctx *ctx)
{
	for (;;)
	{
		switch (ctx->state)
		{
			case H1_STATE_METHOD:
				ctx->state = H1_STATE_SPACE;
				break;

			default:
				fh_pr_debug ("Invalid state: %i", ctx->state);
				return false;
		}
	}
}
