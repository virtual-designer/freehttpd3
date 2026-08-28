/* htable_free() must invoke the data_free_cb exactly once for every
   entry still live in the table, and must not invoke it for entries
   that were already removed via htable_delete() (delete() hands
   ownership of the data back to the caller instead). */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hash/int_htable.h"
#include "test-common.h"

#define M 64

static int free_counts[M];

static void
on_free (void *data)
{
    int *slot = data;
    (*slot)++;
}

int
main (void)
{
    memset (free_counts, 0, sizeof (free_counts));

    int_htable_t *table = int_htable_create (8, on_free);
    CHECK (table != NULL);

    for (int i = 0; i < M; i++)
        CHECK (int_htable_set (table, (uint64_t) i, &free_counts[i]));

    /* Remove a subset via delete() before freeing the table; their
       free callback must never fire since delete() returns ownership
       of the data pointer to the caller. */
    for (int i = 0; i < M; i += 2)
        CHECK (int_htable_delete (table, (uint64_t) i) == &free_counts[i]);

    int_htable_free (table);

    for (int i = 0; i < M; i++)
    {
        if (i % 2 == 0)
            CHECK_MSG (free_counts[i] == 0,
                       "deleted entry %d's free callback fired %d time(s)", i,
                       free_counts[i]);
        else
            CHECK_MSG (free_counts[i] == 1,
                       "live entry %d's free callback fired %d time(s), "
                       "expected exactly 1",
                       i, free_counts[i]);
    }

    /* A NULL data_free_cb must be tolerated (no crash on free). */
    int_htable_t *table2 = int_htable_create (4, NULL);
    CHECK (table2 != NULL);
    CHECK (int_htable_set (table2, 1, &free_counts[0]));
    int_htable_free (table2);

    return test_report ();
}
