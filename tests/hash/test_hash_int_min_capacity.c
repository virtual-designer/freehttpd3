/* Behaviour at int_htable's minimum allowed initial capacity (the
   htable_create() contract asserts initial_cap >= 2), which packs the
   tightest possible table: index_capacity == 2 and entry_capacity ==
   1, so the very next insert must already force a grow(). Also
   exercises the key value 0, a legitimate uint64_t key distinct from
   the HT_EMPTY(0) sentinel that the index list uses internally for
   empty slots -- none of the other int tests use key 0. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "libtest.h"

static int_htable_t *table;

static char payload_zero[] = "zero";
static char payload_one[] = "one";

static int
before_all (void)
{
    table = int_htable_create (2);
    assert_true (table != NULL, "failed to create the table");

    return check_equal (int_htable_count (table), 0);
}

static int
after_all (void)
{
    int_htable_free (table);

    return ASSERT_OK;
}

/* Key 0 must be handled like any other key, not confused with the
   HT_EMPTY sentinel used for empty index-list slots. */

static int
test_hash_int_key_zero (void)
{
    bool flag = true;

    check_true (int_htable_get_with_flag (table, 0, &flag) == NULL);
    check_false (flag);
    check_true (int_htable_set (table, 0, payload_zero));
    check_true (int_htable_has (table, 0));
    check_true (int_htable_get (table, 0) == payload_zero);

    return ASSERT_OK;
}

/* The table is already at its 1-entry capacity; this insert must force a
   grow() before it can succeed. */

static int
test_hash_int_grow_from_min_capacity (void)
{
    check_true (int_htable_set (table, 1, payload_one));
    check_equal (int_htable_count (table), 2);
    check_true (int_htable_get (table, 0) == payload_zero);
    check_true (int_htable_get (table, 1) == payload_one);

    return ASSERT_OK;
}

/* Explicit flag checks on a miss after real data exists, so a false
   positive can't hide behind an all-empty table. */

static int
test_hash_int_flags_on_miss (void)
{
    bool flag = true;

    check_true (int_htable_get_with_flag (table, 42, &flag) == NULL);
    check_false (flag);

    flag = true;
    check_true (int_htable_delete_with_flag (table, 42, &flag) == NULL);
    check_false (flag);

    return ASSERT_OK;
}

/* Delete both, forcing the table back down to empty from its smallest
   possible non-trivial size, and confirm both flags. */

static int
test_hash_int_delete_back_to_empty (void)
{
    bool flag = false;

    check_true (int_htable_delete_with_flag (table, 0, &flag) == payload_zero);
    check_true (flag);

    flag = false;
    check_true (int_htable_delete_with_flag (table, 1, &flag) == payload_one);
    check_true (flag);
    check_equal (int_htable_count (table), 0);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_key_zero),
        define_test_case(test_hash_int_grow_from_min_capacity),
        define_test_case(test_hash_int_flags_on_miss),
        define_test_case(test_hash_int_delete_back_to_empty),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
