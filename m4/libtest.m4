AC_DEFUN([LIBTEST_INIT], [
    AC_REQUIRE_AUX_FILE([runtest])

    LIBTEST_DIR='$(top_srcdir)/libtest'
    LIBTEST_LA='$(top_builddir)/libtest/libtest.la'
    RUNTEST='$(top_srcdir)/build-aux/runtest'

    RUNTEST_FLAGS_DEFAULT=""
    test "$CI" = "true" && RUNTEST_FLAGS_DEFAULT="--log-dir logs"

    AC_ARG_VAR([RUNTEST_FLAGS], [Flags to be passed to build-aux/runtest])
    : ${RUNTEST_FLAGS:="$RUNTEST_FLAGS_DEFAULT"}

    AC_SUBST([LIBTEST_DIR])
    AC_SUBST([LIBTEST_LA])
    AC_SUBST([RUNTEST])
    AC_SUBST([RUNTEST_FLAGS])
    AC_SUBST([RUNTEST_EXEC], ['MAKEFLAGS="$(MAKEFLAGS)" $(RUNTEST) $(RUNTEST_FLAGS) '])
    AC_SUBST([RUNTEST_CLEANUP_EXEC], ['$(RUNTEST) $(RUNTEST_FLAGS) --clear-logs'])
])
