AM_CFLAGS = -std=gnu23 -Wall -Wextra -pedantic -g
AM_CPPFLAGS = -I$(LIBTEST_DIR) -I$(top_srcdir)/src
AM_LDFLAGS = $(LIBTEST_LA) \
    $(top_builddir)/src/hash/libhash.a \
    $(top_builddir)/src/utils/libutils.a

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

test_hash_int_basic_SOURCES = test_hash_int_basic.c
test_hash_int_delete_SOURCES = test_hash_int_delete.c
test_hash_int_grow_SOURCES = test_hash_int_grow.c
test_hash_int_free_cb_SOURCES = test_hash_int_free_cb.c
test_hash_int_churn_SOURCES = test_hash_int_churn.c
test_hash_int_min_capacity_SOURCES = test_hash_int_min_capacity.c
test_hash_int_iterate_SOURCES = test_hash_int_iterate.c
test_hash_int_iterate_delete_SOURCES = test_hash_int_iterate_delete.c

test_hash_str_basic_SOURCES = test_hash_str_basic.c
test_hash_str_delete_SOURCES = test_hash_str_delete.c
test_hash_str_grow_SOURCES = test_hash_str_grow.c
test_hash_str_free_cb_SOURCES = test_hash_str_free_cb.c
test_hash_str_churn_SOURCES = test_hash_str_churn.c
test_hash_str_ownership_SOURCES = test_hash_str_ownership.c
test_hash_str_min_capacity_SOURCES = test_hash_str_min_capacity.c
test_hash_str_iterate_SOURCES = test_hash_str_iterate.c
test_hash_str_iterate_delete_SOURCES = test_hash_str_iterate_delete.c

$(check_PROGRAMS): $(LIBTEST_LA)
check-local: $(check_PROGRAMS)
	$(SILENT_RULE)$(RUNTEST_EXEC) $(check_PROGRAMS)

clean-local:
	$(RM) -f vgcore.*
	$(SILENT_RULE)@RUNTEST_CLEANUP_EXEC@
