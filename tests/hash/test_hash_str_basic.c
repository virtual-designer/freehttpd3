/* Basic create/set/get/has/count behaviour of str_htable. */

#include <stdbool.h>
#include <stdlib.h>

#include "hash/str_htable.h"
#include "libtest.h"

static str_htable_t *table;

static char payload_a[] = "a";
static char payload_b[] = "b";
static char payload_c[] = "c";

static int
before_all (void)
{
    table = str_htable_create (8);

    return assert_true (table != NULL, "failed to create the table");
}

static int
after_all (void)
{
    str_htable_free (table);

    return ASSERT_OK;
}

/* Missing key on an empty table. */

static int
test_hash_str_empty_table (void)
{
    bool flag = true;

    check_equal (str_htable_count (table), 0);
    check_true (str_htable_get_with_flag (table, "missing", &flag) == NULL);
    check_false (flag);
    check_false (str_htable_has (table, "missing"));

    return ASSERT_OK;
}

/* Insert a handful of keys and read them back. */

static int
test_hash_str_set_and_get (void)
{
    check_true (str_htable_set (table, "one", payload_a));
    check_true (str_htable_set (table, "two", payload_b));
    check_true (str_htable_set (table, "three", payload_c));
    check_equal (str_htable_count (table), 3);

    check_true (str_htable_get (table, "one") == payload_a);
    check_true (str_htable_get (table, "two") == payload_b);
    check_true (str_htable_get (table, "three") == payload_c);
    check_true (str_htable_has (table, "one"));
    check_true (str_htable_has (table, "two"));
    check_true (str_htable_has (table, "three"));
    check_false (str_htable_has (table, "nine-hundred-ninety-nine"));

    return ASSERT_OK;
}

/* Setting an existing key overwrites the data without changing count. */

static int
test_hash_str_overwrite (void)
{
    check_true (str_htable_set (table, "two", payload_c));
    check_equal (str_htable_count (table), 3);
    check_true (str_htable_get (table, "two") == payload_c);

    return ASSERT_OK;
}

/* A key stored with NULL data is present (has() is true) even though
   get() alone cannot distinguish it from "missing"; only the _with_flag
   variant can. */

static int
test_hash_str_null_data (void)
{
    bool flag = false;

    check_true (str_htable_set (table, "four", NULL));
    check_equal (str_htable_count (table), 4);
    check_true (str_htable_has (table, "four"));
    check_true (str_htable_get (table, "four") == NULL);

    check_true (str_htable_get_with_flag (table, "four", &flag) == NULL);
    check_true (flag);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_str_empty_table),
        define_test_case(test_hash_str_set_and_get),
        define_test_case(test_hash_str_overwrite),
        define_test_case(test_hash_str_null_data),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
