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

#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "utils.h"

_noreturn void
freeze (void)
{
	while (true)
		sleep (1000);
}

bool
fd_set_nonblocking (fd_t fd)
{
	int flags = fcntl (fd, F_GETFL, 0);

	if (flags < 0)
		return false;

	return fcntl (fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool
fd_add_flags (fd_t fd, int new_flags)
{
	int flags = fcntl (fd, F_GETFL, 0);

	if (flags < 0)
		return false;

	return fcntl (fd, F_SETFL, flags | new_flags) == 0;
}

const char *
get_file_ext (const char *filename)
{
	size_t len = strlen (filename);

	for (size_t i = 1; i <= len; i++)
	{
		if (filename[len - i] == '/')
			break;

		if (filename[len - i] == '.')
			return i == 1 ? NULL : (filename + len - i + 1);
	}

	return NULL;
}

uint64_t
powull (uint64_t base, uint64_t exp)
{
    uint64_t result = 1;

    while (exp)
    {
        if (exp & 1)
            result *= base;

        exp >>= 1;
        base *= base;
    }

    return result;
}
