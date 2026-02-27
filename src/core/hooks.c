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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hooks.h"

struct fh_hook_list *
fh_hook_list_create (void)
{
	struct fh_hook_list *list = calloc (1, sizeof (*list));

	if (!list)
		return NULL;

	return list;
}

void
fh_hook_list_free (struct fh_hook_list *list)
{
	for (struct fh_hook_cb *current = list->heads[0]; current;)
	{
		struct fh_hook_cb *next = current->next;
		free (current);
		current = next;
	}

	free (list);
}

static inline bool
fh_hook_list_add_entry (struct fh_hook_cb **head, struct fh_hook_cb **tail, uint32_t module_id, fh_hook_cb_generic_t cb_ptr)
{
	struct fh_hook_cb *cb = malloc (sizeof (*cb));

	if (!cb)
		return false;

	cb->next = NULL;
	cb->module_id = module_id;
	cb->cb_ptr = cb_ptr;

	if (!*head)
	{
		*head = *tail = cb;
		return true;
	}

	(*tail)->next = cb;
	return true;
}

bool
fh_hook_add (struct fh_hook_list *list, uint32_t module_id, enum fh_hook_type type,
			 fh_hook_cb_generic_t cb_ptr)
{
	struct fh_hook_cb **head, **tail;

	if (type < 0 || type >= __FH_HOOK_TYPE_MAX)
		return false;

	head = &list->heads[type];
	tail = &list->tails[type];

	return fh_hook_list_add_entry (head, tail, module_id, cb_ptr);
}
