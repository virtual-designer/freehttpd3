/* Deletion, tombstone reuse, and count bookkeeping for str_htable. */

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

/* Deleting from an empty table is a well-defined no-op. */

static int
test_hash_str_delete_from_empty (void)
{
    bool flag = true;

    check_true (str_htable_delete_with_flag (table, "missing", &flag) == NULL);
    check_false (flag);
    check_true (str_htable_delete (table, "missing") == NULL);

    return ASSERT_OK;
}

/* Deleting an existing key returns its data, marks it absent, and
   decrements count. */

static int
test_hash_str_delete_existing (void)
{
    bool flag = false;

    check_true (str_htable_set (table, "alpha", payload_a));
    check_true (str_htable_set (table, "beta", payload_b));
    check_equal (str_htable_count (table), 2);

    check_true (str_htable_delete_with_flag (table, "alpha", &flag)
                == payload_a);
    check_true (flag);
    check_equal (str_htable_count (table), 1);
    check_false (str_htable_has (table, "alpha"));
    check_true (str_htable_get (table, "alpha") == NULL);

    /* The other key is unaffected. */
    check_true (str_htable_has (table, "beta"));
    check_true (str_htable_get (table, "beta") == payload_b);

    return ASSERT_OK;
}

/* Deleting the same key twice is safe; the second call finds nothing. */

static int
test_hash_str_delete_twice (void)
{
    check_true (str_htable_delete (table, "alpha") == NULL);
    check_equal (str_htable_count (table), 1);

    return ASSERT_OK;
}

/* A deleted key can be reinserted (exercises tombstone reuse in
   htable_set) and is retrievable again afterwards. */

static int
test_hash_str_reinsert_deleted (void)
{
    check_true (str_htable_set (table, "alpha", payload_c));
    check_equal (str_htable_count (table), 2);
    check_true (str_htable_get (table, "alpha") == payload_c);

    check_true (str_htable_delete (table, "alpha") == payload_c);
    check_true (str_htable_delete (table, "beta") == payload_b);
    check_equal (str_htable_count (table), 0);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_str_delete_from_empty),
        define_test_case(test_hash_str_delete_existing),
        define_test_case(test_hash_str_delete_twice),
        define_test_case(test_hash_str_reinsert_deleted),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
