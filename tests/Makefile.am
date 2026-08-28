AM_CFLAGS = -Wall -Wextra -pedantic -g

HASH_SOURCES = \
    $(top_srcdir)/src/hash/int_htable.c \
    $(top_srcdir)/src/hash/int_htable.h \
    $(top_srcdir)/src/hash/htable.h \
    $(top_srcdir)/src/hash/htable_decl_stub.h \
    $(top_srcdir)/src/utils/utils.c \
    $(top_srcdir)/src/utils/utils.h \
    $(top_srcdir)/src/utils/types.h

check_PROGRAMS = \
    test_hash_int_basic \
    test_hash_int_delete \
    test_hash_int_grow \
    test_hash_int_free_cb \
    test_hash_int_churn

TESTS = $(check_PROGRAMS)

test_hash_int_basic_SOURCES = test_hash_int_basic.c test-common.h $(HASH_SOURCES)
test_hash_int_delete_SOURCES = test_hash_int_delete.c test-common.h $(HASH_SOURCES)
test_hash_int_grow_SOURCES = test_hash_int_grow.c test-common.h $(HASH_SOURCES)
test_hash_int_free_cb_SOURCES = test_hash_int_free_cb.c test-common.h $(HASH_SOURCES)
test_hash_int_churn_SOURCES = test_hash_int_churn.c test-common.h $(HASH_SOURCES)

VALGRIND_WRAPPER = $(top_srcdir)/build-aux/valgrind

check-valgrind:
	@if test -z "$(VALGRIND)"; then \
	    echo "valgrind not found; install it to use this target" >&2; \
	    exit 1; \
	fi
	VALGRIND="$(VALGRIND)" $(MAKE) $(AM_MAKEFLAGS) check LOG_COMPILER='$(VALGRIND_WRAPPER)'

.PHONY: check-valgrind
