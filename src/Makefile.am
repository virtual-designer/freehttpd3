#           -*- mode: makefile-automake; -*-
# Process this file with automake to produce Makefile.in.
#
# This file is part of OSN freehttpd.
#
# Copyright (C) 2025  OSN Developers.
#
# OSN freehttpd is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OSN freehttpd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with OSN freehttpd.  If not, see <https://www.gnu.org/licenses/>.

SUBDIRS = include core hash mm log server utils modules

bin_PROGRAMS = freehttpd
freehttpd_SOURCES = freehttpd.c
freehttpd_LDADD = \
	core/libcore.a \
	hash/libhash.a \
	log/liblog.a \
	mm/libmm.a \
	server/libserver.a \
	utils/libutils.a \
	-ldl

freehttpd_LDFLAGS = -rdynamic

if PLAT_DARWIN
freehttpd_LDFLAGS += -all_load
else
freehttpd_LDFLAGS += -Wl,-ustr_trim_whitespace -Wl,-ufh_request_print
endif

distclean-local:
	rm -rf .deps
