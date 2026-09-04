AM_CFLAGS = -std=gnu23 -Wall -Wextra -pedantic -g
AM_CPPFLAGS = -I$(LIBTEST_DIR) -I$(top_srcdir)/src
AM_LDFLAGS = $(LIBTEST_LA) \
    $(top_builddir)/src/event/libevent.a \
    $(top_builddir)/src/hash/libhash.a \
    $(top_builddir)/src/utils/libutils.a

EXTRA_DIST = test_xpoll_common.h

check_PROGRAMS = \
    test_xpoll_basic \
    test_xpoll_readiness \
    test_xpoll_modify \
    test_xpoll_timeout \
    test_xpoll_multi \
    test_xpoll_hangup \
    test_xpoll_cloexec \
    test_xpoll_dup_add \
    test_xpoll_badfd \
    test_xpoll_scale \
    test_xpoll_udata

test_xpoll_basic_SOURCES = test_xpoll_basic.c
test_xpoll_readiness_SOURCES = test_xpoll_readiness.c
test_xpoll_modify_SOURCES = test_xpoll_modify.c
test_xpoll_timeout_SOURCES = test_xpoll_timeout.c
test_xpoll_multi_SOURCES = test_xpoll_multi.c
test_xpoll_hangup_SOURCES = test_xpoll_hangup.c
test_xpoll_cloexec_SOURCES = test_xpoll_cloexec.c
test_xpoll_dup_add_SOURCES = test_xpoll_dup_add.c
test_xpoll_badfd_SOURCES = test_xpoll_badfd.c
test_xpoll_scale_SOURCES = test_xpoll_scale.c
test_xpoll_udata_SOURCES = test_xpoll_udata.c

$(check_PROGRAMS): $(LIBTEST_LA)
check-local: $(check_PROGRAMS)
	@$(RUNTESTS_EXEC) $(check_PROGRAMS)

clean-local:
	$(RM) -f vgcore.*
	$(RM) -f generic-test_xpoll_* generic-xpoll.o

XPOLL_GENERIC_BACKEND_CPPFLAGS = \
    -DFHTTPD_COMPAT_H=1 \
    -DFH_PLATFORM_UNKNOWN=1 \
    '-Dlikely(x)=(x)' \
    '-Dunlikely(x)=(x)'

XPOLL_GENERIC_BACKEND_TIMEOUT = 30

libtool_silent_ = echo "  CCLD     $$t"; $(LIBTOOL) --silent
libtool_silent_0 = $(libtool_silent_)
libtool_silent_1 = $(LIBTOOL)
libtool_invoke = $(libtool_silent_$(V))

generic-xpoll.o: $(top_srcdir)/src/event/xpoll.c
	$(AM_V_CC) $(CC) -I$(top_srcdir)/src $(CPPFLAGS) $(XPOLL_GENERIC_BACKEND_CPPFLAGS) \
		-c $< -o $@

check-xpoll-generic-backend: $(LIBTEST_LA) generic-xpoll.o
	@progs=''; \
	for t in $(check_PROGRAMS); do \
	    $(libtool_invoke) --tag=CC --mode=link $(CC) \
	        $(AM_CFLAGS) $(CFLAGS) $(AM_CPPFLAGS) $(CPPFLAGS) \
	        $(XPOLL_GENERIC_BACKEND_CPPFLAGS) -I$(srcdir) \
	        -o generic-$$t $(srcdir)/$$t.c generic-xpoll.o $(AM_LDFLAGS) \
	        || exit 1; \
	    progs="$$progs generic-$$t"; \
	done; \
	flags=""; \
	log_opt_found=0; \
	for arg in $(RUNTESTS_FLAGS); do \
		test $log_opt_found -eq 1 && flags="$flags generic-$arg" && log_opt_found=0 && continue; \
		case "$$arg" in \
			-L|--log-dir) log_opt_found=1; ;; \
			*) flags="$flags $arg"; ;; \
		esac; \
	done; \
	TEST_INTERPRETER="timeout $(XPOLL_GENERIC_BACKEND_TIMEOUT) $$TEST_INTERPRETER" \
	    $(RUNTESTS_EXEC) $$flags $$progs

.PHONY: check-xpoll-generic-backend
