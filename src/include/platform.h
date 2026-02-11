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

#ifndef FHTTPD_PLATFORM_H
#define FHTTPD_PLATFORM_H

#define PLATFORM_LINUX 0
#define PLATFORM_BSD 0
#define PLATFORM_UNKNOWN 0

#if defined(__linux__)
#	undef PLATFORM_LINUX
#	define PLATFORM_LINUX 1
#	define PLATFORM "GNU/Linux"
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)      \
	|| defined(__DragonFly__) || defined(__APPLE__)
#	undef PLATFORM_BSD
#	define PLATFORM_BSD 1
#	define PLATFORM "BSD"
#else /* not defined(__linux__) */
#	undef PLATFORM_UNKNOWN
#	define PLATFORM_UNKNOWN 1
#	define PLATFORM "Unknown"
#endif /* defined(__linux__) */

#endif /* FHTTPD_PLATFORM_H */
