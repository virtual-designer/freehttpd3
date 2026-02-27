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

#ifndef FH_HOOKS_H
#define FH_HOOKS_H

#include <stdbool.h>
#include <stdint.h>

#define FH_HOOK_CB(cb) ((fh_hook_cb_generic_t) cb)

struct fh_module;
struct fh_conn;

typedef bool (*fh_hook_cb_generic_t) (void *, ...);

typedef bool (*fh_hook_conn_probe_cb_t) (struct fh_module *module,
										 struct fh_conn *conn);
typedef bool (*fh_hook_conn_cleanup_cb_t) (struct fh_module *module,
										   struct fh_conn *conn);

struct fh_mod_hooks
{
	fh_hook_conn_probe_cb_t conn_probe_cb;
	fh_hook_conn_cleanup_cb_t conn_cleanup_cb;
};

enum fh_hook_type
{
	FH_HOOK_CONN_PROBE,
	FH_HOOK_CONN_CLEANUP,
	__FH_HOOK_TYPE_MAX
};

struct fh_hook_cb
{
	struct fh_hook_cb *next;
	fh_hook_cb_generic_t cb_ptr;
	uint32_t module_id;
};

struct fh_hook_list
{
	struct fh_hook_cb *heads[__FH_HOOK_TYPE_MAX];
	struct fh_hook_cb *tails[__FH_HOOK_TYPE_MAX];
};

struct fh_hook_list *fh_hook_list_create (void);
void fh_hook_list_free (struct fh_hook_list *list);
bool fh_hook_add (struct fh_hook_list *list, uint32_t module_id,
				  enum fh_hook_type type, fh_hook_cb_generic_t cb_ptr);

#endif /* FH_HOOKS_H */
