#define FH_LOG_MODULE_NAME "mod_http1x"

#include "core/module.h"
#include "core/connection.h"
#include "log/log.h"

static bool
mod_http1x_init (struct fh_module *module)
{
	if (!fh_module_conn_ctx_alloc (module, 16))
	{
		fh_pr_err ("Allocation failed");
		return false;
	}

	fh_pr_info ("Initialized");
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
