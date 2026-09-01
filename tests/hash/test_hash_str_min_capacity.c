/* Behaviour at str_htable's minimum allowed initial capacity (the
   htable_create() contract asserts initial_cap >= 2), which packs the
   tightest possible table: index_capacity == 2 and entry_capacity ==
   1, so the very next insert must already force a grow() -- exercised
   together with key duplication, since even the single entry that
   fits before growth still goes through strdup/rehash. */

#include <stdbool.h>
#include <stdlib.h>

#include "hash/str_htable.h"
#include "test-common.h"

int
main (void)
{
    str_htable_t *table = str_htable_create (2);
    CHECK (table != NULL);
    CHECK (str_htable_count (table) == 0);

    bool flag = true;
    CHECK (str_htable_get_with_flag (table, "zero", &flag) == NULL);
    CHECK (flag == false);

    static char payload_zero[] = "zero-data";
    CHECK (str_htable_set (table, "zero", payload_zero));
    CHECK (str_htable_has (table, "zero"));
    CHECK (str_htable_get (table, "zero") == payload_zero);

    /* The table is already at its 1-entry capacity; this insert must
       force a grow() before it can succeed. */
    static char payload_one[] = "one-data";
    CHECK (str_htable_set (table, "one", payload_one));
    CHECK (str_htable_count (table) == 2);
    CHECK (str_htable_get (table, "zero") == payload_zero);
    CHECK (str_htable_get (table, "one") == payload_one);

    /* Explicit flag checks on a miss after real data exists, so a
       false positive can't hide behind an all-empty table. */
    flag = true;
    void *data = str_htable_get_with_flag (table, "missing", &flag);
    CHECK (data == NULL);
    CHECK (flag == false);

    flag = true;
    data = str_htable_delete_with_flag (table, "missing", &flag);
    CHECK (data == NULL);
    CHECK (flag == false);

    /* Delete both, forcing the table back down to empty from its
       smallest possible non-trivial size, and confirm both flags. */
    flag = false;
    CHECK (str_htable_delete_with_flag (table, "zero", &flag) == payload_zero);
    CHECK (flag == true);
    flag = false;
    CHECK (str_htable_delete_with_flag (table, "one", &flag) == payload_one);
    CHECK (flag == true);
    CHECK (str_htable_count (table) == 0);

    str_htable_free (table);

    return test_report ();
}
