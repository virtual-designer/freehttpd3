/* Deletion, tombstone reuse, and count bookkeeping for int_htable. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "test-common.h"

int
main (void)
{
    int_htable_t *table = int_htable_create (8);
    CHECK (table != NULL);

    static char payload_a[] = "a";
    static char payload_b[] = "b";

    /* Deleting from an empty table is a well-defined no-op. */
    bool flag = true;
    void *data = int_htable_delete_with_flag (table, 123, &flag);
    CHECK (data == NULL);
    CHECK (flag == false);
    CHECK (int_htable_delete (table, 123) == NULL);

    CHECK (int_htable_set (table, 10, payload_a));
    CHECK (int_htable_set (table, 20, payload_b));
    CHECK (int_htable_count (table) == 2);

    /* Deleting an existing key returns its data, marks it absent,
       and decrements count. */
    flag = false;
    data = int_htable_delete_with_flag (table, 10, &flag);
    CHECK (data == payload_a);
    CHECK (flag == true);
    CHECK (int_htable_count (table) == 1);
    CHECK (int_htable_has (table, 10) == false);
    CHECK (int_htable_get (table, 10) == NULL);

    /* The other key is unaffected. */
    CHECK (int_htable_has (table, 20));
    CHECK (int_htable_get (table, 20) == payload_b);

    /* Deleting the same key twice is safe; the second call finds
       nothing. */
    CHECK (int_htable_delete (table, 10) == NULL);
    CHECK (int_htable_count (table) == 1);

    /* A deleted key can be reinserted (exercises tombstone reuse in
       htable_set) and is retrievable again afterwards. */
    static char payload_c[] = "c";
    CHECK (int_htable_set (table, 10, payload_c));
    CHECK (int_htable_count (table) == 2);
    CHECK (int_htable_get (table, 10) == payload_c);

    CHECK (int_htable_delete (table, 10) == payload_c);
    CHECK (int_htable_delete (table, 20) == payload_b);
    CHECK (int_htable_count (table) == 0);

    int_htable_free (table);

    return test_report ();
}
