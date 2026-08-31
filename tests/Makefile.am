AM_CFLAGS = -Wall -Wextra -pedantic -g

HASH_SOURCES = \
    $(top_srcdir)/src/hash/int_htable.c \
    $(top_srcdir)/src/hash/int_htable.h \
    $(top_srcdir)/src/hash/htable.h \
    $(top_srcdir)/src/hash/htable_decl_stub.h \
    $(top_srcdir)/src/utils/utils.c \
    $(top_srcdir)/src/utils/utils.h \
    $(top_srcdir)/src/utils/types.h

STR_HASH_SOURCES = \
    $(top_srcdir)/src/hash/str_htable.c \
    $(top_srcdir)/src/hash/str_htable.h \
    $(top_srcdir)/src/hash/rapidhash.h \
    $(top_srcdir)/src/hash/htable.h \
    $(top_srcdir)/src/hash/htable_decl_stub.h \
    $(top_srcdir)/src/utils/utils.c \
    $(top_srcdir)/src/utils/utils.h \
    $(top_srcdir)/src/utils/types.h

XPOLL_SOURCES = \
    $(top_srcdir)/src/event/xpoll.c \
    $(top_srcdir)/src/event/xpoll.h \
    $(top_srcdir)/src/utils/compat.h \
    $(top_srcdir)/src/utils/utils.h \
    $(top_srcdir)/src/utils/types.h

check_PROGRAMS = \
    test_xpoll_basic \
    test_xpoll_readiness \
    test_xpoll_modify \
    test_xpoll_timeout \
    test_xpoll_multi \
    test_xpoll_hangup \
    test_xpoll_cloexec \
    test_hash_int_basic \
    test_hash_int_delete \
    test_hash_int_grow \
    test_hash_int_free_cb \
    test_hash_int_churn \
    test_hash_int_min_capacity \
    test_hash_int_iterate \
    test_hash_int_iterate_delete \
    test_hash_str_basic \
    test_hash_str_delete \
    test_hash_str_grow \
    test_hash_str_free_cb \
    test_hash_str_churn \
    test_hash_str_ownership \
    test_hash_str_min_capacity \
    test_hash_str_iterate \
    test_hash_str_iterate_delete

TESTS = $(check_PROGRAMS)

XPOLL_TEST_COMMON = test-common.h test-xpoll-common.h

test_xpoll_basic_SOURCES = test_xpoll_basic.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_readiness_SOURCES = test_xpoll_readiness.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_modify_SOURCES = test_xpoll_modify.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_timeout_SOURCES = test_xpoll_timeout.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_multi_SOURCES = test_xpoll_multi.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_hangup_SOURCES = test_xpoll_hangup.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)
test_xpoll_cloexec_SOURCES = test_xpoll_cloexec.c $(XPOLL_TEST_COMMON) $(XPOLL_SOURCES)

test_hash_int_basic_SOURCES = test_hash_int_basic.c test-common.h $(HASH_SOURCES)
test_hash_int_delete_SOURCES = test_hash_int_delete.c test-common.h $(HASH_SOURCES)
test_hash_int_grow_SOURCES = test_hash_int_grow.c test-common.h $(HASH_SOURCES)
test_hash_int_free_cb_SOURCES = test_hash_int_free_cb.c test-common.h $(HASH_SOURCES)
test_hash_int_churn_SOURCES = test_hash_int_churn.c test-common.h $(HASH_SOURCES)
test_hash_int_min_capacity_SOURCES = test_hash_int_min_capacity.c test-common.h $(HASH_SOURCES)
test_hash_int_iterate_SOURCES = test_hash_int_iterate.c test-common.h $(HASH_SOURCES)
test_hash_int_iterate_delete_SOURCES = test_hash_int_iterate_delete.c test-common.h $(HASH_SOURCES)

test_hash_str_basic_SOURCES = test_hash_str_basic.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_delete_SOURCES = test_hash_str_delete.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_grow_SOURCES = test_hash_str_grow.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_free_cb_SOURCES = test_hash_str_free_cb.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_churn_SOURCES = test_hash_str_churn.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_ownership_SOURCES = test_hash_str_ownership.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_min_capacity_SOURCES = test_hash_str_min_capacity.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_iterate_SOURCES = test_hash_str_iterate.c test-common.h $(STR_HASH_SOURCES)
test_hash_str_iterate_delete_SOURCES = test_hash_str_iterate_delete.c test-common.h $(STR_HASH_SOURCES)

VALGRIND_WRAPPER = $(top_srcdir)/build-aux/valgrind

clean-local:
	$(RM) -f vgcore.*

check-valgrind:
	@if test -z "$(VALGRIND)"; then \
	    echo "valgrind not found; install it to use this target" >&2; \
	    exit 1; \
	fi
	VALGRIND="$(VALGRIND)" $(MAKE) $(AM_MAKEFLAGS) check LOG_COMPILER='$(VALGRIND_WRAPPER)'

.PHONY: check-valgrind
