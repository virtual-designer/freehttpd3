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

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/core -I$(top_srcdir)/src/utils -I$(top_srcdir)/src/include
AM_TESTS_ENVIRONMENT = \
	top_srcdir=$(top_srcdir) \
	top_builddir=$(top_builddir) \
	testdir=$(top_builddir)/tests

if PLAT_DARWIN
AM_TESTS_ENVIRONMENT += \
	VALGRIND=
AM_CFLAGS = -fsanitize=address -fsanitize=leak
AM_LDFLAGS = -fsanitize=address -fsanitize=leak
else
AM_TESTS_ENVIRONMENT += \
	VALGRIND=$(top_srcdir)/build-aux/valgrind
endif

check_PROGRAMS = itable.test.helper strtable.test.helper
TESTS = itable.test strtable.test

itable_test_helper_SOURCES = itable.test.c $(top_srcdir)/src/hash/itable.c $(top_srcdir)/src/hash/itable.h
strtable_test_helper_SOURCES = strtable.test.c $(top_srcdir)/src/hash/strtable.c $(top_srcdir)/src/hash/strtable.h

EXTRA_DIST = $(TESTS)

check-valgrind-benchmark:
	BINDIR=$(top_srcdir)/src $(SHELL) valgrind-benchmark.sh

check-benchmark:
	BINDIR=$(top_srcdir)/src $(SHELL) benchmark.sh

.PHONY: check-valgrind-benchmark check-benchmark

clean-local:
	rm -f vgcore.* *.log
	rm -rf .deps

