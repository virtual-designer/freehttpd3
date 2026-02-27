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

#ifndef FHTTPD_POOL_H
#define FHTTPD_POOL_H

#include <stddef.h>

struct fh_pool;
typedef struct fh_pool pool_t;

#define fh_pool_calloc(pool, n, size) fh_pool_zalloc (pool, n *size)

struct fh_pool *fh_pool_create (size_t initial_capacity);
struct fh_pool *fh_pool_create_child (struct fh_pool *parent,
									  size_t initial_capacity);
void fh_pool_free (struct fh_pool *pool);
void *fh_pool_alloc (struct fh_pool *pool, size_t size);
void *fh_pool_zalloc (struct fh_pool *pool, size_t size);

#endif /* FHTTPD_POOL_H */
