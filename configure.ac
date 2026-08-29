dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

m4_define([FHTTPD_VERSION], m4_esyscmd([build-aux/git-version-gen -g -d .github/version.json .version]))

AC_PREREQ([2.61])
AC_INIT([freehttpd],
  FHTTPD_VERSION,
  [bug-freehttpd@lists.onesoftnet.eu.org],
  [freehttpd],
  [https://github.com/onesoft-sudo/freehttpd])
AC_CONFIG_SRCDIR([src/progs/freehttpd.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([-Wall gnu 1.11 info-in-builddir subdir-objects dist-xz dist-bzip2 parallel-tests])
AM_SILENT_RULES([yes])

AC_SUBST([VERSION], FHTTPD_VERSION)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_GREP
AM_PROG_AR
AC_PROG_RANLIB
AC_PATH_PROG([VALGRIND], [valgrind])
AC_PATH_PROG([GPG], [gpg])

dnl Checks for libraries.

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

CONCAT_SPACED_STRING([CPPFLAGS], [-I\$(top_srcdir)/src])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/core/Makefile
                 src/hash/Makefile
                 src/log/Makefile
                 src/utils/Makefile
                 src/progs/Makefile
                 tests/Makefile])
AC_OUTPUT
PRINT_SUMMARY