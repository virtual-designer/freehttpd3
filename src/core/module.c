#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"
#include "connection.h"

#define FH_MODULE_INFO_SYMBOL "fh_modinfo"

static uint32_t module_id = 1;

static const struct fh_modinfo *
fh_module_handle_get_info (struct fh_module_handle *handle)
{
	const struct fh_modinfo *mod_info = dlsym (handle->dl_handle, FH_MODULE_INFO_SYMBOL);

	if (!mod_info)
	{
		handle->err = "No symbol named '" FH_MODULE_INFO_SYMBOL "' could be located";
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
	handle->public_module->id = module_id;
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
fh_module_handle_cleanup (struct fh_module_handle *handle)
{
	const struct fh_modinfo *mod_info = handle->mod_info;

	if (mod_info && mod_info->init_cb)
		mod_info->exit_cb (handle->public_module);

	dlclose (handle->dl_handle);
	free (handle->path);
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
fh_module_conn_ctx_alloc (struct fh_module *module, size_t size)
{
	struct fh_module_handle *handle = fh_module_get_handle (module);
	handle->conn_ctx_off = module->server->module_conn_ctx_total_size;
	module->server->module_conn_ctx_total_size += size;
	return true;
}

void *
fh_module_get_conn_ctx (struct fh_module *module, struct fh_conn *conn)
{
	struct fh_module_handle *handle = fh_module_get_handle (module);

	if (handle->conn_ctx_off >= conn->module_data_size)
		return NULL;

	return (void *) (((uint8_t *) conn->module_data) + handle->conn_ctx_off);
}
