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

#ifndef FH_CORE_MODULE_H
#define FH_CORE_MODULE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/hooks.h"
#include "core/server.h"

#define FH_MODULE_ABI_VERSION 0x0001

struct fh_modinfo;
struct fh_conn;

struct fh_module
{
	uint32_t id;
	const char *name;
	struct fh_server *server;
};

struct fh_module_handle
{
	struct fh_module *public_module;
	char *path;
	void *dl_handle;
	const struct fh_modinfo *mod_info;
	const char *err;
};

struct fh_modinfo
{
	uint32_t abi_version;
	bool (*init_cb) (struct fh_module *module);
	void (*exit_cb) (struct fh_module *module);
};

typedef struct fh_modinfo fh_modinfo_t;

struct fh_module_handle *fh_module_handle_load (struct fh_server *server,
												const char *path);
void fh_module_handle_cleanup (struct fh_module_handle *handle, bool discard_id);
const char *
fh_module_handle_get_last_err (const struct fh_module_handle *handle);
bool fh_module_register_hooks (struct fh_module *module,
							   const struct fh_mod_hooks *hooks);
bool fh_module_register_hook (struct fh_module *module, enum fh_hook_type type,
							  fh_hook_cb_generic_t cb);

#endif /* FH_CORE_MODULE_H */
