/* Deletion, tombstone reuse, and count bookkeeping for int_htable. */

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

/* Deleting from an empty table is a well-defined no-op. */

static int
test_hash_int_delete_from_empty (void)
{
    bool flag = true;

    check_true (int_htable_delete_with_flag (table, 123, &flag) == NULL);
    check_false (flag);
    check_true (int_htable_delete (table, 123) == NULL);

    return ASSERT_OK;
}

/* Deleting an existing key returns its data, marks it absent, and
   decrements count. */

static int
test_hash_int_delete_existing (void)
{
    bool flag = false;

    check_true (int_htable_set (table, 10, payload_a));
    check_true (int_htable_set (table, 20, payload_b));
    check_equal (int_htable_count (table), 2);

    check_true (int_htable_delete_with_flag (table, 10, &flag) == payload_a);
    check_true (flag);
    check_equal (int_htable_count (table), 1);
    check_false (int_htable_has (table, 10));
    check_true (int_htable_get (table, 10) == NULL);

    /* The other key is unaffected. */
    check_true (int_htable_has (table, 20));
    check_true (int_htable_get (table, 20) == payload_b);

    return ASSERT_OK;
}

/* Deleting the same key twice is safe; the second call finds nothing. */

static int
test_hash_int_delete_twice (void)
{
    check_true (int_htable_delete (table, 10) == NULL);
    check_equal (int_htable_count (table), 1);

    return ASSERT_OK;
}

/* A deleted key can be reinserted (exercises tombstone reuse in
   htable_set) and is retrievable again afterwards. */

static int
test_hash_int_reinsert_deleted (void)
{
    check_true (int_htable_set (table, 10, payload_c));
    check_equal (int_htable_count (table), 2);
    check_true (int_htable_get (table, 10) == payload_c);

    check_true (int_htable_delete (table, 10) == payload_c);
    check_true (int_htable_delete (table, 20) == payload_b);
    check_equal (int_htable_count (table), 0);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_delete_from_empty),
        define_test_case(test_hash_int_delete_existing),
        define_test_case(test_hash_int_delete_twice),
        define_test_case(test_hash_int_reinsert_deleted),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
