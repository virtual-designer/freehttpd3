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

#include "core/hooks.h"
#define FH_LOG_MODULE_NAME "mod_http1x"

#include <stddef.h>

#include "core/connection.h"
#include "core/module.h"
#include "log/log.h"
#include "mm/chain.h"
#include "mm/pool.h"

#define mod_http1x_METHOD_MAX_LEN 8

enum mod_http1x_state
{
	H1_STATE_METHOD,
	H1_STATE_URI,
	H1_STATE_VERSION,
	H1_STATE_SPACE,
	H1_STATE_HEADER_NAME,
	H1_STATE_HEADER_VALUE,
	H1_STATE_BODY,
};

enum mod_http1x_err
{
	H1_ERR_NONE,
	H1_ERR_MEMORY,
	H1_ERR_PROTOCOL,
	H1_ERR_MALFORMED,
};

enum mod_http1x_method
{
	HTTP_GET,
	HTTP_POST,
};

struct mod_http1x_result
{
	enum mod_http1x_method method;
	char *uri;
	size_t uri_len;
};

struct mod_http1x_ctx
{
	struct fh_pool *pool;
	bool pool_freeable;
	enum mod_http1x_state state;
	enum mod_http1x_err error;
	struct fh_chain_cur cur_processed, cur_seen;
	struct mod_http1x_result result;
};

static bool
mod_http1x_ctx_init (struct mod_http1x_ctx *ctx, struct fh_chain *start_chain,
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

static struct mod_http1x_ctx *
mod_http1x_ctx_create (struct fh_chain *start_chain, size_t start_off)
{
	struct fh_pool *pool = fh_pool_create (4096);

	if (!pool)
		return NULL;

	struct mod_http1x_ctx *ctx = fh_pool_alloc (pool, sizeof (*ctx));

	if (!ctx || !mod_http1x_ctx_init (ctx, start_chain, start_off, pool))
	{
		fh_pool_free (pool);
		return NULL;
	}

	return ctx;
}

static void
mod_http1x_ctx_cleanup (const struct mod_http1x_ctx *ctx)
{
	if (ctx->pool_freeable)
		fh_pool_free (ctx->pool);
}

static void
mod_http1x_ctx_free (struct mod_http1x_ctx *ctx)
{
	mod_http1x_ctx_cleanup (ctx);
	fh_pool_free (ctx->pool);
}

static bool
mod_http1x_parse (struct mod_http1x_ctx *ctx)
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

static bool
fh_conn_probe (struct fh_module *module, struct fh_conn *conn)
{
	fh_pr_debug ("New connection");
	return true;
}

static bool
mod_http1x_init (struct fh_module *module)
{
	fh_pr_info ("Initialized");
	fh_module_register_hook (module, FH_HOOK_CONN_PROBE,
							 FH_HOOK_CB (&fh_conn_probe));
	return true;
}

static void
mod_http1x_exit (struct fh_module *module)
{
	(void) module;
	fh_pr_info ("De-initialized");
}

const fh_modinfo_t fh_modinfo = {
	.abi_version = FH_MODULE_ABI_VERSION,
	.init_cb = &mod_http1x_init,
	.exit_cb = &mod_http1x_exit,
};
