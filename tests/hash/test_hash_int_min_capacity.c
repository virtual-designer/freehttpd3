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
#include "test-common.h"

int
main (void)
{
    int_htable_t *table = int_htable_create (2);
    CHECK (table != NULL);
    CHECK (int_htable_count (table) == 0);

    /* Key 0 must be handled like any other key, not confused with the
       HT_EMPTY sentinel used for empty index-list slots. */
    static char payload_zero[] = "zero";
    bool flag = true;
    CHECK (int_htable_get_with_flag (table, 0, &flag) == NULL);
    CHECK (flag == false);
    CHECK (int_htable_set (table, 0, payload_zero));
    CHECK (int_htable_has (table, 0));
    CHECK (int_htable_get (table, 0) == payload_zero);

    /* The table is already at its 1-entry capacity; this insert must
       force a grow() before it can succeed. */
    static char payload_one[] = "one";
    CHECK (int_htable_set (table, 1, payload_one));
    CHECK (int_htable_count (table) == 2);
    CHECK (int_htable_get (table, 0) == payload_zero);
    CHECK (int_htable_get (table, 1) == payload_one);

    /* Explicit flag checks on a miss after real data exists, so a
       false positive can't hide behind an all-empty table. */
    flag = true;
    void *data = int_htable_get_with_flag (table, 42, &flag);
    CHECK (data == NULL);
    CHECK (flag == false);

    flag = true;
    data = int_htable_delete_with_flag (table, 42, &flag);
    CHECK (data == NULL);
    CHECK (flag == false);

    /* Delete both, forcing the table back down to empty from its
       smallest possible non-trivial size, and confirm both flags. */
    flag = false;
    CHECK (int_htable_delete_with_flag (table, 0, &flag) == payload_zero);
    CHECK (flag == true);
    flag = false;
    CHECK (int_htable_delete_with_flag (table, 1, &flag) == payload_one);
    CHECK (flag == true);
    CHECK (int_htable_count (table) == 0);

    int_htable_free (table);

    return test_report ();
}
