AC_DEFUN([LIBTEST_INIT], [
    AC_REQUIRE_AUX_FILE([runtest])

    LIBTEST_DIR='$(top_srcdir)/libtest'
    LIBTEST_LA='$(top_builddir)/libtest/libtest.la'
    RUNTESTS='$(top_srcdir)/build-aux/runtest'

    RUNTESTS_FLAGS_DEFAULT=""
    test "$CI" = "true" && RUNTESTS_FLAGS_DEFAULT="--log-dir logs"

    AC_ARG_VAR([RUNTESTS_FLAGS], [Flags to be passed to build-aux/runtest])
    : ${RUNTESTS_FLAGS:="$RUNTESTS_FLAGS_DEFAULT"}

    AC_SUBST([LIBTEST_DIR])
    AC_SUBST([LIBTEST_LA])
    AC_SUBST([RUNTESTS])
    AC_SUBST([RUNTESTS_FLAGS])
    AC_SUBST([RUNTESTS_EXEC], ['MAKEFLAGS="$(MAKEFLAGS)" $(RUNTESTS) $(RUNTESTS_FLAGS) '])
    AC_SUBST([RUNTESTS_CLEANUP_EXEC], ['$(RUNTESTS) $(RUNTESTS_FLAGS) --clear-logs'])
])
