/* htable_free() must invoke the data_free_cb exactly once for every
   entry still live in the table, and must not invoke it for entries
   that were already removed via htable_delete() (delete() hands
   ownership of the data back to the caller instead). */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hash/int_htable.h"
#include "libtest.h"

#define M 64

static int free_counts[M];

static void
on_free (void *data)
{
    int *slot = data;
    (*slot)++;
}

static int
test_hash_int_free_cb_fires_once_per_live_entry (void)
{
    memset (free_counts, 0, sizeof (free_counts));

    int_htable_t *table = int_htable_create (8);
    check_true (table != NULL);

    for (int i = 0; i < M; i++)
        check_true (int_htable_set (table, (uint64_t) i, &free_counts[i]));

    /* Remove a subset via delete() before freeing the table; their free
       callback must never fire since delete() returns ownership of the
       data pointer to the caller. */
    for (int i = 0; i < M; i += 2)
        check_true (int_htable_delete (table, (uint64_t) i)
                    == &free_counts[i]);

    int_htable_free_with_cleanup (table, &on_free);

    for (int i = 0; i < M; i++)
    {
        if (i % 2 == 0)
            assert_equal (free_counts[i], 0,
                          "deleted entry %d's free callback fired %d time(s)",
                          i, free_counts[i]);
        else
            assert_equal (free_counts[i], 1,
                          "live entry %d's free callback fired %d time(s), "
                          "expected exactly 1",
                          i, free_counts[i]);
    }

    return ASSERT_OK;
}

/* A NULL data_free_cb must be tolerated (no crash on free). */

static int
test_hash_int_null_free_cb (void)
{
    int_htable_t *table = int_htable_create (4);

    check_true (table != NULL);
    check_true (int_htable_set (table, 1, &free_counts[0]));
    int_htable_free (table);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_free_cb_fires_once_per_live_entry),
        define_test_case(test_hash_int_null_free_cb),
        NULL,
    },
};
