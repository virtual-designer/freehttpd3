#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
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

AC_PREREQ([2.61])

define([FHTTPD_VERSION], m4_esyscmd([build-aux/git-version-gen -g -d .github/version.json .version]))

AC_INIT([freehttpd],
  FHTTPD_VERSION,
  [bug-freehttpd@lists.onesoftnet.eu.org],
  [freehttpd],
  [https://github.com/onesoft-sudo/freehttpd])

AC_CONFIG_SRCDIR([src/fhttpd.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_REQUIRE_AUX_FILE([git-version-gen])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([-Wall gnu 1.11 info-in-builddir subdir-objects dist-xz dist-bzip2])
AM_SILENT_RULES([yes])

AC_SUBST([VERSION], FHTTPD_VERSION)

top_builddir=`pwd`
abs_srcdir="$(dirname "$0")/src"
abs_srcdir="$(readlink -f "$abs_srcdir")"

CPPFLAGS="-I$abs_srcdir/include -I$abs_srcdir -I$top_builddir/res -DHAVE_RESOURCES -DHAVE_CONFPATHS_H $CPPFLAGS"

case "$target" in
    *"darwin"*)
		darwin=1
		;;

	*)
		darwin=0
		;;
esac

AM_CONDITIONAL([PLAT_DARWIN], [test "$darwin" = "1"])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
AC_PROG_RANLIB
CHECK_PROG_GPG

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_FUNCS([accept4])

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 res/Makefile 
                 src/Makefile
                 src/include/Makefile
                 src/core/Makefile
                 src/hash/Makefile
                 src/log/Makefile
                 src/utils/Makefile
                 tests/Makefile])
AC_OUTPUT
PRINT_SUMMARY
