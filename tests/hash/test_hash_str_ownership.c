/* Key-ownership semantics that are unique to str_htable: unlike
   int_htable (whose HT_KEY_TYPE is a plain scalar), str_htable's keys
   are heap-duplicated on insert (HT_KEY_DUP_CB is strdup) and freed on
   removal (HT_KEY_FREE_CB is free). None of this is exercised by
   int_htable's tests at all, since its dup/free callbacks are no-ops,
   so this file is not a mirror of anything in the int suite. */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hash/str_htable.h"
#include "libtest.h"

static str_htable_t *table;

static char payload_a[] = "a";
static char payload_b[] = "b";
static char payload_c[] = "c";
static char payload_empty[] = "empty";

/* Keys spanning rapidhash's various internal length branches (empty,
   short, medium, and long-past-112-bytes). */

static const char *lengths[] = {
    "",
    "a",
    "abcd",
    "0123456789abcdef",                       /* 16 bytes */
    "0123456789abcdef0123456789abcdef012345", /* 39 bytes */
    /* 130 bytes, past the 112-byte long-input threshold */
    ("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
     "0123456789abcdef0123456789abcdef0123456789abcdef0123456789ab"),
};

static char length_payload[sizeof (lengths) / sizeof (lengths[0])];

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

/* The table must copy the key at insert time rather than alias the
   caller's buffer: mutating the buffer after set() must not change what
   is stored. */

static int
test_hash_str_key_is_copied (void)
{
    char mutable_key[16];

    strcpy (mutable_key, "original");
    check_true (str_htable_set (table, mutable_key, payload_a));

    strcpy (mutable_key, "mutated!");
    check_true (str_htable_has (table, "original"));
    check_true (str_htable_get (table, "original") == payload_a);
    check_false (str_htable_has (table, "mutated!"));

    return ASSERT_OK;
}

/* Two distinct allocations holding equal content must be treated as the
   same key (equality is by content via strcmp, not by pointer
   identity). */

static int
test_hash_str_equality_is_by_content (void)
{
    char *heap_key1 = strdup ("shared");
    char *heap_key2 = strdup ("shared");

    check_true (heap_key1 != NULL && heap_key2 != NULL);
    check_true (heap_key1 != heap_key2);

    check_true (str_htable_set (table, heap_key1, payload_b));
    check_equal (str_htable_count (table), 2);

    /* Setting via a different pointer with the same content overwrites
       the existing entry instead of creating a second one. */
    check_true (str_htable_set (table, heap_key2, payload_c));
    check_equal (str_htable_count (table), 2);
    check_true (str_htable_get (table, heap_key1) == payload_c);
    check_true (str_htable_get (table, "shared") == payload_c);

    /* The caller's own buffers were never taken ownership of; freeing
       them here must not corrupt the table's independent copy. */
    free (heap_key1);
    free (heap_key2);
    check_true (str_htable_get (table, "shared") == payload_c);

    return ASSERT_OK;
}

/* Deleting by a freshly-built key (yet another allocation, equal by
   content) must still find and remove the entry. */

static int
test_hash_str_delete_by_equal_key (void)
{
    char *heap_key3 = strdup ("shared");

    check_true (heap_key3 != NULL);
    check_true (str_htable_delete (table, heap_key3) == payload_c);
    free (heap_key3);
    check_false (str_htable_has (table, "shared"));

    return ASSERT_OK;
}

/* The empty string is a legal, distinct key. */

static int
test_hash_str_empty_string_key (void)
{
    check_false (str_htable_has (table, ""));
    check_true (str_htable_set (table, "", payload_empty));
    check_true (str_htable_has (table, ""));
    check_true (str_htable_get (table, "") == payload_empty);
    check_true (str_htable_delete (table, "") == payload_empty);
    check_false (str_htable_has (table, ""));

    return ASSERT_OK;
}

/* Keys of every rapidhash length branch must round-trip correctly once
   duplicated and hashed. */

static int
test_hash_str_key_length_branches (void)
{
    const size_t count = sizeof (lengths) / sizeof (lengths[0]);

    for (size_t i = 0; i < count; i++)
    {
        length_payload[i] = (char) ('A' + i);
        assert_true (str_htable_set (table, lengths[i], &length_payload[i]),
                     "set failed for length-%zu key", strlen (lengths[i]));
    }

    for (size_t i = 0; i < count; i++)
        check_true (str_htable_get (table, lengths[i]) == &length_payload[i]);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_str_key_is_copied),
        define_test_case(test_hash_str_equality_is_by_content),
        define_test_case(test_hash_str_delete_by_equal_key),
        define_test_case(test_hash_str_empty_string_key),
        define_test_case(test_hash_str_key_length_branches),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
