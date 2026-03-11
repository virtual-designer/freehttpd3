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

#define FH_LOG_MODULE_NAME "mod_http1x"

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/connection.h"
#include "core/hooks.h"
#include "core/module.h"
#include "log/log.h"
#include "mm/chain.h"
#include "mm/pool.h"
#include "server/request.h"
#include "server/protocol.h"
#include "utils/strutils.h"

#define H1_METHOD_MAX_LEN 8
#define H1_URI_MAX_LEN 4096
#define H1_VERSION_MAX_LEN 8
#define H1_HEADER_NAME_MAX_LEN 256
#define H1_HEADER_VALUE_MAX_LEN 2048

#define H1_NEXT(state) (state & 0xFFFFU)
#define H1_ERR(code) ((1U << 16U) | (code))
#define H1_AGAIN (2U << 16U)
#define H1_DONE (3U << 16U)

enum mod_http1x_state
{
    H1_STATE_METHOD,
    H1_STATE_URI,
    H1_STATE_VERSION,
    H1_STATE_SPACE,
    H1_STATE_HEADER_NAME,
    H1_STATE_HEADER_VALUE,
    H1_STATE_BODY,
    H1_STATE_DONE
};

struct mod_http1x_ctx
{
    struct fh_pool *pool;
    bool pool_freeable;
    enum mod_http1x_state space_next_state;
    enum mod_http1x_state state;
    struct fh_chain_cur cur_processed, cur_seen;
    struct fh_request request;
    struct fh_header *current_header;
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
    ctx->request.headers = NULL;

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

static uint32_t
mod_http1x_parse_method (struct mod_http1x_ctx *ctx, struct fh_conn *conn)
{
    (void) conn;

    struct fh_chain_cur end_cur;
    int rc;

    fh_chain_print (ctx->cur_processed.chain);

    if ((rc = fh_chain_find_char (&end_cur, &ctx->cur_processed, ' ',
                                  H1_METHOD_MAX_LEN))
        != FIND_CHAR_OK)
        return rc == FIND_CHAR_NOT_FOUND ? H1_AGAIN
                                         : H1_ERR (FH_HTTP_BAD_REQUEST);

    struct fh_chain_cpbuf cpbuf;

    if (!fh_chain_copy_range (ctx->pool, &ctx->cur_processed, &end_cur,
                              H1_METHOD_MAX_LEN, &cpbuf))
        return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

    enum fh_method method;

    if (!strncmp ((const char *) cpbuf.raw_buf, "GET", cpbuf.len))
        method = HTTP_GET;
    else if (!strncmp ((const char *) cpbuf.raw_buf, "POST", cpbuf.len))
        method = HTTP_POST;
    else if (!strncmp ((const char *) cpbuf.raw_buf, "PUT", cpbuf.len))
        method = HTTP_PUT;
    else if (!strncmp ((const char *) cpbuf.raw_buf, "PATCH", cpbuf.len))
        method = HTTP_PATCH;
    else if (!strncmp ((const char *) cpbuf.raw_buf, "DELETE", cpbuf.len))
        method = HTTP_DELETE;
    else if (!strncmp ((const char *) cpbuf.raw_buf, "TRACE", cpbuf.len))
        method = HTTP_TRACE;
    else if (!strncmp ((const char *) cpbuf.raw_buf, "HEAD", cpbuf.len))
        method = HTTP_HEAD;
    else
        return H1_ERR (FH_HTTP_METHOD_NOT_ALLOWED);

    ctx->request.method = method;
    ctx->cur_processed.chain = end_cur.chain;
    ctx->cur_processed.off = end_cur.off;
    ctx->space_next_state = H1_STATE_URI;

    return H1_NEXT (H1_STATE_SPACE);
}

static uint32_t
mod_http1x_parse_space (struct mod_http1x_ctx *ctx, struct fh_conn *conn)
{
    (void) conn;

    struct fh_chain *chain = ctx->cur_processed.chain;
    size_t start_off = ctx->cur_processed.off;

    while (chain)
    {
        size_t off = ctx->cur_processed.chain == chain ? start_off : 0;
        size_t len = 0;
        const uint8_t *base = chain->buf->mem_ptr + off;

        while (len + off < chain->buf->mem_size && base[len] == ' ')
            len++;

        if (!chain->next
            || (len + off < chain->buf->mem_size && base[len] != ' '))
        {
            ctx->cur_processed.off += len;
            ctx->cur_processed.chain = chain;

            if (!(len + off < chain->buf->mem_size && base[len] != ' '))
                return H1_AGAIN;

            break;
        }

        chain = chain->next;
        ctx->cur_processed.off = 0;
    }

    return H1_NEXT (ctx->space_next_state);
}

static uint32_t
mod_http1x_parse_uri (struct mod_http1x_ctx *ctx, struct fh_conn *conn)
{
    (void) conn;

    struct fh_chain_cur end_cur;
    int rc;

    /* TODO: Optimization idea: Save another cursor, that represents data
       "seen".  It may be passed to fh_chain_find_char () to skip previously
       checked data.  Even if the character is not found, update the cursor and
       return.  Lastly, when the character is found, go ahead and use another
       separate cursor that represents "data processed" and copy the data
       appropriately. */

    if ((rc = fh_chain_find_char (&end_cur, &ctx->cur_processed, ' ',
                                  H1_URI_MAX_LEN))
        != FIND_CHAR_OK)
        return rc == FIND_CHAR_NOT_FOUND
                   ? H1_AGAIN
                   : H1_ERR (FH_HTTP_REQUEST_URI_TOO_LONG);

    struct fh_chain_cpbuf cpbuf;

    if (!fh_chain_copy_range (ctx->pool, &ctx->cur_processed, &end_cur,
                              H1_URI_MAX_LEN, &cpbuf))
        return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

    ctx->request.uri = (char *) cpbuf.raw_buf;
    ctx->request.uri_len = cpbuf.len;
    ctx->cur_processed.chain = end_cur.chain;
    ctx->cur_processed.off = end_cur.off;
    ctx->space_next_state = H1_STATE_VERSION;

    return H1_NEXT (H1_STATE_SPACE);
}

static uint32_t
mod_http1x_parse_version (struct mod_http1x_ctx *ctx, struct fh_conn *conn)
{
    (void) conn;

    struct fh_chain_cur end_cur;
    int rc;

    if ((rc = fh_chain_find_char (&end_cur, &ctx->cur_processed, '\n',
                                  H1_VERSION_MAX_LEN + 2))
        != FIND_CHAR_OK)
        return rc == FIND_CHAR_NOT_FOUND ? H1_AGAIN
                                         : H1_ERR (FH_HTTP_BAD_REQUEST);

    struct fh_chain_cpbuf cpbuf;

    if (!fh_chain_copy_range (ctx->pool, &ctx->cur_processed, &end_cur,
                              H1_VERSION_MAX_LEN + 2, &cpbuf))
        return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

    if (cpbuf.len < 2 || cpbuf.raw_buf[cpbuf.len - 1] != '\r')
        return H1_ERR (FH_HTTP_BAD_REQUEST);

    enum fh_http_version version;

    if (!strncmp ((const char *) cpbuf.raw_buf, "HTTP/1.1", cpbuf.len - 1))
        version = HTTP_VERSION_1_1;
    else if (!strncmp ((const char *) cpbuf.raw_buf, "HTTP/1.0", cpbuf.len - 1))
        version = HTTP_VERSION_1_0;
    else
        return H1_ERR (FH_HTTP_HTTP_VERSION_NOT_SUPPORTED);

    ctx->request.version = version;
    ctx->cur_processed.chain = end_cur.chain;
    ctx->cur_processed.off = end_cur.off + 1;

    return H1_NEXT (H1_STATE_HEADER_NAME);
}

static uint32_t
mod_http1x_parse_header_name (struct mod_http1x_ctx *ctx, struct fh_conn *conn)
{
    (void) conn;

    struct fh_chain_cur end_cur;
    int rc = fh_chain_find_char (&end_cur, &ctx->cur_processed, ':',
                                 H1_HEADER_NAME_MAX_LEN + 1);
    int rc2;

    switch (rc)
    {
        case FIND_CHAR_NOT_FOUND:
            rc2 = fh_chain_find_char (&end_cur, &ctx->cur_processed, '\n', 2);

            switch (rc2)
            {
                case FIND_CHAR_LIMIT:
                    return H1_ERR (FH_HTTP_BAD_REQUEST);

                case FIND_CHAR_NOT_FOUND:
                    return H1_AGAIN;
            }

            struct fh_chain_cpbuf cpbuf;

            if (!fh_chain_copy_range (ctx->pool, &ctx->cur_processed, &end_cur,
                                      2, &cpbuf))
                return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

            if (cpbuf.len == 1 && cpbuf.raw_buf[0] == '\r')
            {
                ctx->cur_processed.chain = end_cur.chain;
                ctx->cur_processed.off = end_cur.off + 1;
                return H1_DONE;
            }

            return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

        case FIND_CHAR_LIMIT:
            return H1_ERR (FH_HTTP_BAD_REQUEST);
    }

    struct fh_chain_cpbuf cpbuf;

    if (!fh_chain_copy_range (ctx->pool, &ctx->cur_processed, &end_cur,
                              H1_HEADER_NAME_MAX_LEN + 1, &cpbuf))
        return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

    if (cpbuf.len < 2)
        return H1_ERR (FH_HTTP_BAD_REQUEST);

    struct fh_header *header = fh_pool_alloc (ctx->pool, sizeof (*header));

    if (!header)
        return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

    header->name = (const char *) cpbuf.raw_buf;
    header->name_len = cpbuf.len;

    ctx->current_header = header;
    ctx->cur_processed.chain = end_cur.chain;
    ctx->cur_processed.off = end_cur.off + 1;

    fh_log_debug ("Header: (%zu) |%.*s|", cpbuf.len, (int) cpbuf.len,
                  header->name);

    return H1_NEXT (H1_STATE_HEADER_VALUE);
}

static uint32_t
mod_http1x_parse_header_value (struct mod_http1x_ctx *ctx, struct fh_conn *conn)
{
    (void) conn;

    struct fh_chain_cur end_cur;
    int rc;

    if ((rc = fh_chain_find_char (&end_cur, &ctx->cur_processed, '\n',
                                  H1_HEADER_VALUE_MAX_LEN + 2))
        != FIND_CHAR_OK)
        return rc == FIND_CHAR_NOT_FOUND ? H1_AGAIN
                                         : H1_ERR (FH_HTTP_BAD_REQUEST);

    struct fh_chain_cpbuf cpbuf;

    if (!fh_chain_copy_range (ctx->pool, &ctx->cur_processed, &end_cur,
                              H1_HEADER_VALUE_MAX_LEN + 2, &cpbuf))
        return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

    if (cpbuf.len < 2 || cpbuf.raw_buf[cpbuf.len - 1] != '\r')
        return H1_ERR (FH_HTTP_BAD_REQUEST);

    struct fh_header *header = ctx->current_header;

    header->value = str_trim_whitespace ((const char *) cpbuf.raw_buf,
                                         cpbuf.len - 1, &header->value_len);
    header->next = NULL;

    if (!ctx->request.headers)
    {
        ctx->request.headers
            = fh_pool_alloc (ctx->pool, sizeof (*ctx->request.headers));

        if (!ctx->request.headers)
            return H1_ERR (FH_HTTP_INTERNAL_SERVER_ERROR);

        ctx->request.headers->head = ctx->request.headers->tail = header;
    }
    else
    {
        ctx->request.headers->tail->next = header;
        ctx->request.headers->tail = header;
    }

    ctx->cur_processed.chain = end_cur.chain;
    ctx->cur_processed.off = end_cur.off + 1;

    fh_log_debug ("Header value: (%zu) |%.*s|", header->value_len,
                  (int) header->value_len, header->value);
    return H1_NEXT (H1_STATE_HEADER_NAME);
}

static bool
mod_http1x_parse (struct mod_http1x_ctx *ctx, struct fh_conn *conn)
{
    for (;;)
    {
        uint32_t ret;

        switch (ctx->state)
        {
            case H1_STATE_METHOD:
                if (!ctx->cur_processed.chain)
                {
                    ctx->cur_processed.chain = conn->chain_list->head;
                    ctx->cur_processed.off = 0;
                }

                ret = mod_http1x_parse_method (ctx, conn);
                break;

            case H1_STATE_SPACE:
                ret = mod_http1x_parse_space (ctx, conn);
                break;

            case H1_STATE_URI:
                ret = mod_http1x_parse_uri (ctx, conn);
                break;

            case H1_STATE_VERSION:
                ret = mod_http1x_parse_version (ctx, conn);
                break;

            case H1_STATE_HEADER_NAME:
                ret = mod_http1x_parse_header_name (ctx, conn);
                break;

            case H1_STATE_HEADER_VALUE:
                ret = mod_http1x_parse_header_value (ctx, conn);
                break;

            case H1_STATE_DONE:
                fh_log_debug ("Already done parsing");
                return true;

            default:
                fh_log_debug ("Invalid state: %i", ctx->state);
                return false;
        }

        switch (ret >> 16U)
        {
            case 0:
                ctx->state = ret & 0xFF;
                continue;

            case 1:
                fh_log_debug ("Parse error: %i", ret & 0xFFFF);
                return false;

            case 2:
                fh_log_debug ("Need more data");
                return true;

            case 3:
                ctx->state = H1_STATE_DONE;
                return true;

            default:
                fh_log_debug ("Invalid return code: %i", ret >> 16U);
                return false;
        }
    }
}

static bool
mod_http1x_conn_probe (struct fh_module *module, struct fh_conn *conn)
{
    struct mod_http1x_ctx *ctx = mod_http1x_ctx_create (
        conn->chain_list ? conn->chain_list->head : NULL, 0);

    if (!ctx)
        return false;

    fh_conn_set_module_data (module, conn, ctx,
                             (void (*) (void *)) &mod_http1x_ctx_free);

    fh_log_info ("New connection: %" PRIu64, conn->id);
    return true;
}

static bool
mod_http1x_conn_cleanup (struct fh_module *module, struct fh_conn *conn)
{
    (void) module;
    fh_log_info ("Closing connection: %" PRIu64, conn->id);
    return true;
}

static bool
mod_http1x_stream_read (struct fh_module *module, struct fh_conn *conn,
                        struct fh_chain_cur *last_cur)
{
    (void) last_cur;

    struct mod_http1x_ctx *ctx = fh_conn_get_module_data (module, conn);

    if (!ctx)
        return false;

    fh_log_debug ("State: %d", ctx->state);

    if (!mod_http1x_parse (ctx, conn))
    {
        fh_log_err ("Parser error, closing connection");
        fh_conn_close_from_module (conn);
        return false;
    }

    if (ctx->state == H1_STATE_DONE)
    {
        fh_request_print (&ctx->request);
        char text[] = "HTTP/1.1 200 OK\r\nServer: freehttpd\r\nContent-Type: "
                      "text/plain\r\n\r\nHello world!\n";
        send (conn->sockfd, text, sizeof text - 1, 0);
        fh_conn_close_from_module (conn);
        return true;
    }

    return true;
}

static bool
mod_http1x_init (struct fh_module *module)
{
    fh_module_register_hook (module, FH_HOOK_CONN_PROBE,
                             FH_HOOK_CB (&mod_http1x_conn_probe));
    fh_module_register_hook (module, FH_HOOK_CONN_CLEANUP,
                             FH_HOOK_CB (&mod_http1x_conn_cleanup));
    fh_module_register_hook (module, FH_HOOK_STREAM_READ,
                             FH_HOOK_CB (&mod_http1x_stream_read));

    fh_log_info ("Initialized");
    return true;
}

static void
mod_http1x_exit (struct fh_module *module)
{
    (void) module;
    fh_log_info ("De-initialized");
}

const fh_modinfo_t fh_modinfo = {
    .abi_version = FH_MODULE_ABI_VERSION,
    .init_cb = &mod_http1x_init,
    .exit_cb = &mod_http1x_exit,
};
