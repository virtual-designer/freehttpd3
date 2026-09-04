/* Basic create/set/get/has/count behaviour of int_htable. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "libtest.h"

static int_htable_t *table;

static char payload_a[] = "a";
static char payload_b[] = "b";
static char payload_c[] = "c";

static int
before_all (void)
{
    table = int_htable_create (8);

    return assert_true (table != NULL, "failed to create the table");
}

static int
after_all (void)
{
    int_htable_free (table);

    return ASSERT_OK;
}

/* Missing key on an empty table. */

static int
test_hash_int_empty_table (void)
{
    bool flag = true;

    check_equal (int_htable_count (table), 0);
    check_true (int_htable_get_with_flag (table, 42, &flag) == NULL);
    check_false (flag);
    check_false (int_htable_has (table, 42));

    return ASSERT_OK;
}

/* Insert a handful of keys and read them back. */

static int
test_hash_int_set_and_get (void)
{
    check_true (int_htable_set (table, 1, payload_a));
    check_true (int_htable_set (table, 2, payload_b));
    check_true (int_htable_set (table, 3, payload_c));
    check_equal (int_htable_count (table), 3);

    check_true (int_htable_get (table, 1) == payload_a);
    check_true (int_htable_get (table, 2) == payload_b);
    check_true (int_htable_get (table, 3) == payload_c);
    check_true (int_htable_has (table, 1));
    check_true (int_htable_has (table, 2));
    check_true (int_htable_has (table, 3));
    check_false (int_htable_has (table, 999));

    return ASSERT_OK;
}

/* Setting an existing key overwrites the data without changing count. */

static int
test_hash_int_overwrite (void)
{
    check_true (int_htable_set (table, 2, payload_c));
    check_equal (int_htable_count (table), 3);
    check_true (int_htable_get (table, 2) == payload_c);

    return ASSERT_OK;
}

/* A key stored with NULL data is present (has() is true) even though
   get() alone cannot distinguish it from "missing"; only the _with_flag
   variant can. */

static int
test_hash_int_null_data (void)
{
    bool flag = false;

    check_true (int_htable_set (table, 4, NULL));
    check_equal (int_htable_count (table), 4);
    check_true (int_htable_has (table, 4));
    check_true (int_htable_get (table, 4) == NULL);

    check_true (int_htable_get_with_flag (table, 4, &flag) == NULL);
    check_true (flag);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_empty_table),
        define_test_case(test_hash_int_set_and_get),
        define_test_case(test_hash_int_overwrite),
        define_test_case(test_hash_int_null_data),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
