AM_CFLAGS = -Wall -Wextra -pedantic -g
AM_CPPFLAGS = -I$(top_srcdir)/tests

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

check_PROGRAMS = \
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

test_hash_int_basic_SOURCES = test_hash_int_basic.c $(HASH_SOURCES)
test_hash_int_delete_SOURCES = test_hash_int_delete.c $(HASH_SOURCES)
test_hash_int_grow_SOURCES = test_hash_int_grow.c $(HASH_SOURCES)
test_hash_int_free_cb_SOURCES = test_hash_int_free_cb.c $(HASH_SOURCES)
test_hash_int_churn_SOURCES = test_hash_int_churn.c $(HASH_SOURCES)
test_hash_int_min_capacity_SOURCES = test_hash_int_min_capacity.c $(HASH_SOURCES)
test_hash_int_iterate_SOURCES = test_hash_int_iterate.c $(HASH_SOURCES)
test_hash_int_iterate_delete_SOURCES = test_hash_int_iterate_delete.c $(HASH_SOURCES)

test_hash_str_basic_SOURCES = test_hash_str_basic.c $(STR_HASH_SOURCES)
test_hash_str_delete_SOURCES = test_hash_str_delete.c $(STR_HASH_SOURCES)
test_hash_str_grow_SOURCES = test_hash_str_grow.c $(STR_HASH_SOURCES)
test_hash_str_free_cb_SOURCES = test_hash_str_free_cb.c $(STR_HASH_SOURCES)
test_hash_str_churn_SOURCES = test_hash_str_churn.c $(STR_HASH_SOURCES)
test_hash_str_ownership_SOURCES = test_hash_str_ownership.c $(STR_HASH_SOURCES)
test_hash_str_min_capacity_SOURCES = test_hash_str_min_capacity.c $(STR_HASH_SOURCES)
test_hash_str_iterate_SOURCES = test_hash_str_iterate.c $(STR_HASH_SOURCES)
test_hash_str_iterate_delete_SOURCES = test_hash_str_iterate_delete.c $(STR_HASH_SOURCES)

clean-local:
	$(RM) -f vgcore.*
