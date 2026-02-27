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

#define FH_LOG_MODULE_NAME "module"

#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"
#include "core/hooks.h"
#include "log/log.h"
#include "module.h"

#define FH_MODULE_INFO_SYMBOL "fh_modinfo"

struct fh_avail_module_id
{
	uint32_t id;
	struct fh_avail_module_id *next;
	struct fh_avail_module_id *prev;
};

static uint32_t module_id = 0;
static struct fh_avail_module_id *module_id_head = NULL;
static struct fh_avail_module_id *module_id_tail = NULL;

static void
fh_module_id_cleanup (void)
{
	while (module_id_head)
	{
		struct fh_avail_module_id *next = module_id_head->next;
		free (module_id_head);
		module_id_head = next;
	}
}

static void __attribute__ ((constructor))
fh_module_id_setup (void)
{
	atexit (&fh_module_id_cleanup);
}

static inline uint32_t
fh_module_new_id (void)
{
	if (module_id_head)
	{
		struct fh_avail_module_id *node = module_id_head;
		uint32_t id = node->id;

		if (node->prev)
			node->prev->next = node->next;

		if (node->next)
			node->next->prev = node->prev;

		if (node == module_id_head)
			module_id_head = NULL;

		if (node == module_id_tail)
			module_id_tail = NULL;

		free (node);
		return id;
	}

	return module_id++;
}

static inline void
fh_module_free_id (uint32_t id)
{
	struct fh_avail_module_id *node = calloc (1, sizeof (*node));

	if (!node)
		return;

	node->id = id;
	node->next = NULL;
	node->prev = module_id_tail;

	if (module_id_tail)
	{
		module_id_tail->next = node;
		module_id_tail = node;
	}
	else
	{
		module_id_tail = module_id_head = node;
	}
}

static const struct fh_modinfo *
fh_module_handle_get_info (struct fh_module_handle *handle)
{
	const struct fh_modinfo *mod_info
		= dlsym (handle->dl_handle, FH_MODULE_INFO_SYMBOL);

	if (!mod_info)
	{
		handle->err
			= "No symbol named '" FH_MODULE_INFO_SYMBOL "' could be located";
		return NULL;
	}

	if (mod_info->abi_version != FH_MODULE_ABI_VERSION)
	{
		handle->err = "ABI version mismatch";
		return NULL;
	}

	return mod_info;
}

struct fh_module_handle *
fh_module_handle_load (struct fh_server *server, const char *path)
{
	struct fh_module_handle *handle
		= calloc (1, sizeof (*handle) + sizeof (*handle->public_module));

	if (!handle)
		return NULL;

	handle->path = strdup (path);

	if (!handle->path)
	{
		free (handle);
		return NULL;
	}

	const char *name = basename (handle->path);

	if (!name)
	{
		free (handle->path);
		free (handle);
		return NULL;
	}

	void *dl_handle = dlopen (path, RTLD_NOW);

	if (!dl_handle)
	{
		free (handle->path);
		free (handle);
		return NULL;
	}

	handle->dl_handle = dl_handle;
	handle->public_module = (struct fh_module *) (handle + 1);
	handle->public_module->id = fh_module_new_id ();
	handle->public_module->name = name;
	handle->public_module->server = server;

	const struct fh_modinfo *mod_info = fh_module_handle_get_info (handle);

	if (!mod_info)
		return handle;

	handle->mod_info = mod_info;

	if (mod_info->init_cb && !mod_info->init_cb (handle->public_module))
	{
		handle->err = "Module initialization failed";
		return handle;
	}

	return handle;
}

const char *
fh_module_handle_get_last_err (const struct fh_module_handle *handle)
{
	const char *err;

	if (!handle || !handle->err)
		err = errno == 0 ? dlerror () : strerror (errno);
	else
		err = handle->err;

	return err && *err ? err : NULL;
}

void
fh_module_handle_cleanup (struct fh_module_handle *handle, bool discard_id)
{
	const struct fh_modinfo *mod_info = handle->mod_info;

	if (mod_info && mod_info->init_cb)
		mod_info->exit_cb (handle->public_module);

	dlclose (handle->dl_handle);
	free (handle->path);

	if (!discard_id)
		fh_module_free_id (handle->public_module->id);

	free (handle);
}

static struct fh_module_handle *
fh_module_get_handle (struct fh_module *module)
{
	uint8_t *ptr = (uint8_t *) module;
	ptr -= sizeof (struct fh_module_handle);
	return (struct fh_module_handle *) ptr;
}

bool
fh_module_register_hooks (struct fh_module *module,
						  const struct fh_mod_hooks *hooks)
{
	bool ret = true;

	if (hooks->conn_probe_cb)
		ret &= fh_hook_add (module->server->hook_list, module->id,
							FH_HOOK_CONN_PROBE,
							(fh_hook_cb_generic_t) hooks->conn_probe_cb);

	if (hooks->conn_cleanup_cb)
		ret &= fh_hook_add (module->server->hook_list, module->id,
							FH_HOOK_CONN_CLEANUP,
							(fh_hook_cb_generic_t) hooks->conn_cleanup_cb);

	return ret;
}

bool
fh_module_register_hook (struct fh_module *module, enum fh_hook_type type,
						 fh_hook_cb_generic_t cb)
{
	return fh_hook_add (module->server->hook_list, module->id, type, cb);
}
