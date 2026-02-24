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

COMMON_MODULE_CFLAGS = -module -avoid-version -shared

moduledir = $(FHTTPD_MODULE_PATH)
module_LTLIBRARIES =

if ENABLE_MOD_HTTP1X
module_LTLIBRARIES += mod_http1x.la
mod_http1x_la_SOURCES = mod_http1x.c
mod_http1x_la_LDFLAGS = $(COMMON_MODULE_CFLAGS)
endif

distclean-local:
	rm -rf .deps
