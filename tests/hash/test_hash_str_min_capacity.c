/* Behaviour at str_htable's minimum allowed initial capacity (the
   htable_create() contract asserts initial_cap >= 2), which packs the
   tightest possible table: index_capacity == 2 and entry_capacity ==
   1, so the very next insert must already force a grow() -- exercised
   together with key duplication, since even the single entry that
   fits before growth still goes through strdup/rehash. */

#include <stdbool.h>
#include <stdlib.h>

#include "hash/str_htable.h"
#include "libtest.h"

static str_htable_t *table;

static char payload_zero[] = "zero-data";
static char payload_one[] = "one-data";

static int
before_all (void)
{
    table = str_htable_create (2);
    assert_true (table != NULL, "failed to create the table");

    return check_equal (str_htable_count (table), 0);
}

static int
after_all (void)
{
    str_htable_free (table);

    return ASSERT_OK;
}

static int
test_hash_str_first_entry (void)
{
    bool flag = true;

    check_true (str_htable_get_with_flag (table, "zero", &flag) == NULL);
    check_false (flag);

    check_true (str_htable_set (table, "zero", payload_zero));
    check_true (str_htable_has (table, "zero"));
    check_true (str_htable_get (table, "zero") == payload_zero);

    return ASSERT_OK;
}

/* The table is already at its 1-entry capacity; this insert must force a
   grow() before it can succeed. */

static int
test_hash_str_grow_from_min_capacity (void)
{
    check_true (str_htable_set (table, "one", payload_one));
    check_equal (str_htable_count (table), 2);
    check_true (str_htable_get (table, "zero") == payload_zero);
    check_true (str_htable_get (table, "one") == payload_one);

    return ASSERT_OK;
}

/* Explicit flag checks on a miss after real data exists, so a false
   positive can't hide behind an all-empty table. */

static int
test_hash_str_flags_on_miss (void)
{
    bool flag = true;

    check_true (str_htable_get_with_flag (table, "missing", &flag) == NULL);
    check_false (flag);

    flag = true;
    check_true (str_htable_delete_with_flag (table, "missing", &flag) == NULL);
    check_false (flag);

    return ASSERT_OK;
}

/* Delete both, forcing the table back down to empty from its smallest
   possible non-trivial size, and confirm both flags. */

static int
test_hash_str_delete_back_to_empty (void)
{
    bool flag = false;

    check_true (str_htable_delete_with_flag (table, "zero", &flag)
                == payload_zero);
    check_true (flag);

    flag = false;
    check_true (str_htable_delete_with_flag (table, "one", &flag)
                == payload_one);
    check_true (flag);
    check_equal (str_htable_count (table), 0);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_str_first_entry),
        define_test_case(test_hash_str_grow_from_min_capacity),
        define_test_case(test_hash_str_flags_on_miss),
        define_test_case(test_hash_str_delete_back_to_empty),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
