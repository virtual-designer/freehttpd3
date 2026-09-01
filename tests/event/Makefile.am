AM_CFLAGS = -Wall -Wextra -pedantic -g
AM_CPPFLAGS = -I$(top_srcdir)/tests

XPOLL_SOURCES = \
    $(top_srcdir)/src/event/xpoll.c \
    $(top_srcdir)/src/event/xpoll.h \
    $(top_srcdir)/src/utils/compat.h \
    $(top_srcdir)/src/utils/utils.h \
    $(top_srcdir)/src/utils/types.h

XPOLL_TEST_COMMON = test-xpoll-common.h

XPOLL_TESTS = \
    test_xpoll_basic \
    test_xpoll_readiness \
    test_xpoll_modify \
    test_xpoll_timeout \
    test_xpoll_multi \
    test_xpoll_hangup \
    test_xpoll_cloexec \
    test_xpoll_dup_add \
    test_xpoll_badfd \
    test_xpoll_scale

check_PROGRAMS = $(XPOLL_TESTS)

TESTS = $(check_PROGRAMS)

test_xpoll_basic_SOURCES = test_xpoll_basic.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_readiness_SOURCES = test_xpoll_readiness.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_modify_SOURCES = test_xpoll_modify.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_timeout_SOURCES = test_xpoll_timeout.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_multi_SOURCES = test_xpoll_multi.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_hangup_SOURCES = test_xpoll_hangup.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_cloexec_SOURCES = test_xpoll_cloexec.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_dup_add_SOURCES = test_xpoll_dup_add.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_badfd_SOURCES = test_xpoll_badfd.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_scale_SOURCES = test_xpoll_scale.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)

# Only one xpoll backend is compiled per host, so on Linux the portable
# poll() implementation in xpoll.c is never built at all and its bugs
# cannot show up in "make check".  This target builds the same test
# sources against it: defining compat.h's include guard on the command
# line neutralises the header, letting the platform macro be supplied
# here instead of by the host detection.
#
# It is intentionally not part of TESTS: it only makes sense where the
# host backend is something else, and it is meant to be run explicitly.

XPOLL_GENERIC_BACKEND_CPPFLAGS = \
    -DFHTTPD_COMPAT_H=1 \
    -DFH_PLATFORM_UNKNOWN=1 \
    '-Dlikely(x)=(x)' \
    '-Dunlikely(x)=(x)'

# Guards against a hang rather than a failure: the poll() backend scans
# its own registry, so a mistake there can spin instead of returning.

XPOLL_GENERIC_BACKEND_TIMEOUT = 3

check-xpoll-generic-backend:
	@status=0; \
	for t in $(XPOLL_TESTS); do \
	    $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_CPPFLAGS) $(CPPFLAGS) \
	        $(XPOLL_GENERIC_BACKEND_CPPFLAGS) -I$(srcdir) \
	        -o generic-$$t $(srcdir)/$$t.c \
	        $(top_srcdir)/src/event/xpoll.c $(top_srcdir)/src/utils/utils.c \
	        || { echo "FAIL: $$t (generic backend, build)"; status=1; continue; }; \
	    if timeout $(XPOLL_GENERIC_BACKEND_TIMEOUT) ./generic-$$t >/dev/null 2>&1; \
	    then echo "PASS: $$t (generic backend)"; \
	    else rc=$$?; status=1; \
	        if test $$rc -eq 124; then \
	            echo "HANG: $$t (generic backend, killed after $(XPOLL_GENERIC_BACKEND_TIMEOUT)s)"; \
	        else echo "FAIL: $$t (generic backend, exit $$rc)"; fi; \
	    fi; \
	done; \
	exit $$status

clean-local:
	$(RM) -f vgcore.*
	$(RM) -f generic-test_xpoll_*

.PHONY: check-xpoll-generic-backend
