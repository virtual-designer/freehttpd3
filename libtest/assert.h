#ifndef LIBTEST_ASSERT_H
#define LIBTEST_ASSERT_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum assert_code
{
    ASSERT_OK,
    ASSERT_FAIL
};

/* Two families, differing only in whether they carry an explanation:
   the assert_*() macros take a printf-style message, the check_*()
   ones report the failing expression alone.

   They are spelled separately rather than made variadic-with-default
   because omitting a variadic macro's arguments entirely is a C23
   extension, and the tree is still compiled as gnu99 in maintainer
   mode.  check_*() therefore passes a NULL format explicitly, which
   assert_internal() reads as "no message". */

#define assert_true(value, ...)                                                \
    assert_internal (__FILE__, __LINE__, __func__, (value), #value, __VA_ARGS__)
#define assert_false(value, ...)                                               \
    assert_internal (__FILE__, __LINE__, __func__, !(value), "!" #value,       \
                     __VA_ARGS__)

#define assert_equal(value1, value2, ...)                                      \
    assert_internal (__FILE__, __LINE__, __func__, (value1) == (value2),       \
                     "(" #value1 ") == (" #value2 ")", __VA_ARGS__)

#define assert_not_equal(value1, value2, ...)                                  \
    assert_internal (__FILE__, __LINE__, __func__, (value1) != (value2),       \
                     "(" #value1 ") != (" #value2 ")", __VA_ARGS__)

#define check_true(value) assert_true (value, NULL)
#define check_false(value) assert_false (value, NULL)
#define check_equal(value1, value2) assert_equal (value1, value2, NULL)
#define check_not_equal(value1, value2)                                        \
    assert_not_equal (value1, value2, NULL)

int assert_internal (const char *filename, int line, const char *function_name,
                     bool value, const char *expr, const char *format, ...)
    __attribute__ ((format (printf, 6, 7)));
size_t get_assert_fail_count (void);
size_t get_assert_success_count (void);
void set_assert_fail_count (size_t value);
void set_assert_success_count (size_t value);

#endif /* LIBTEST_ASSERT_H */
