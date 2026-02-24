#ifndef FH_CORE_MODULE_H
#define FH_CORE_MODULE_H

#include "core/server.h"
#include <stdbool.h>
#include <stdint.h>

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
	uint32_t conn_ctx_off;
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
void fh_module_handle_cleanup (struct fh_module_handle *handle);
const char *fh_module_handle_get_last_err (const struct fh_module_handle *handle);

bool fh_module_conn_ctx_alloc (struct fh_module *module, size_t size);
void *fh_module_get_conn_ctx (struct fh_module *module, struct fh_conn *conn);

#endif /* FH_CORE_MODULE_H */
