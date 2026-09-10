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
AC_CANONICAL_BUILD

AC_DEFINE_UNQUOTED([FH_BUILD_SYSTEM_TYPE], ["$build"], [Define the build system type])
AC_DEFINE_UNQUOTED([FH_TARGET_SYSTEM_TYPE], ["$target"], [Define the target system type])

AM_INIT_AUTOMAKE([-Wall gnu 1.11 info-in-builddir subdir-objects dist-xz dist-bzip2 parallel-tests])
AM_SILENT_RULES([yes])

AC_SUBST([VERSION], FHTTPD_VERSION)

AC_ARG_ENABLE([maintainer-mode], 
              [AS_HELP_STRING([--enable-maintainer-mode        Enable maintainer mode.])], 
              [enable_maintainer_mode=yes], [enable_maintainer_mode=no])
AC_ARG_ENABLE([linux-uring],
              [AS_HELP_STRING([--disable-linux-uring           Disable IO uring support on Linux. Default: no])],
              [], [enable_linux_uring=yes])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_GREP
AM_PROG_AR
AC_PROG_RANLIB
AC_PATH_PROG([VALGRIND], [valgrind])
AC_PATH_PROG([GPG], [gpg])
AC_SUBST([VALGRIND_WRAPPER], ['$''('abs_top_srcdir')'/build-aux/valgrind])

dnl Checks for libraries.
LT_INIT
LIBTEST_INIT

AS_IF([test "$enable_linux_uring" = yes], [
    AS_CASE(["$target"], 
            [*-linux-*|*-linux|linux-*], [check_uring=1],
            [check_uring=0])

    AS_IF([test $check_uring = 1], [
        PKG_CHECK_MODULES([LIBURING], [liburing >= 2.0], [
            AC_DEFINE([HAVE_LIBURING], [1], [Define to 1 if liburing is available])
            build_uring=1
        ], [
            AC_MSG_NOTICE([liburing is not available. failing back to using epoll() by default.])
        ])
    ])
])

AS_CASE(["$target"], 
        [*-linux-*|*-linux|linux-*], [target_linux=1],
        [*-darwin-*|*-darwin|darwin-*], [target_darwin=1],
        [*bsd*], [target_bsd=1],
        [])

AM_CONDITIONAL([FH_BUILD_LINUX_LIBURING], [test "$build_uring" = 1])
AM_CONDITIONAL([FH_BUILD_LINUX], [test "$target_linux" = 1])
AM_CONDITIONAL([FH_BUILD_DARWIN], [test "$target_darwin" = 1])
AM_CONDITIONAL([FH_BUILD_BSD], [test "$target_bsd" = 1])
AM_CONDITIONAL([FH_BUILD_BSDLIKE], [test "$target_darwin" = 1 || test "$target_bsd" = 1])

PKG_CHECK_MODULES([VALGRIND], [valgrind >= 3.0], [
    AC_DEFINE([HAVE_VALGRIND], [1], [Define to 1 if valgrind libraries and headers are available])
], [:])

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.
AC_MSG_CHECKING([whether __VA_OPT__ is supported])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#define TEST_MACRO(format, ...) fn (format "\n" __VA_OPT__(,) __VA_ARGS__)

static void 
fn (const char *format, ...) 
{ 
    (void) format; 
}

int 
main (void)
{
    TEST_MACRO("A");
    TEST_MACRO("A", 1);
    TEST_MACRO("A", 1, 2);
    return 0;
}
]])], [
    AC_DEFINE([STDC_HAVE_VA_OPT], [1], [__VA_OPT__ is safely usable])
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])
])

dnl Checks for library functions.

dnl Configure compiler flags
CONCAT_SPACED_STRING([CPPFLAGS], [-I\$(top_srcdir)/src])

AS_IF([test "$enable_maintainer_mode" = "yes"], [
	AC_MSG_NOTICE([enabling maintainer mode])
	CONCAT_SPACED_STRING([CFLAGS], [-Wall -Wextra -pedantic -std=gnu99])
], [])

dnl Support for one-off dynamic silent rules.
AC_SUBST([silent_rule_], [@])
AC_SUBST([silent_rule_0], [@])
AC_SUBST([silent_rule_1], [])
AC_SUBST([SILENT_RULE], ['$(silent_rule_$(V))'])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/core/Makefile
                 src/hash/Makefile
                 src/event/Makefile
                 src/log/Makefile
                 src/utils/Makefile
                 src/progs/Makefile
                 libtest/Makefile
                 tests/Makefile
                 tests/event/Makefile
                 tests/hash/Makefile])
AC_OUTPUT
PRINT_SUMMARY
