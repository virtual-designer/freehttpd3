#include <stdarg.h>

#include "assert.h"

extern const char *libtest_suite_name;
extern const char *libtest_test_case_name;

static size_t assert_fail_count = 0;
static size_t assert_success_count = 0;

int
assert_internal (const char *filename, int line, const char *function_name,
                 bool value, const char *expr, const char *format, ...)

{
    if (value)
    {
        assert_success_count++;
        return ASSERT_OK;
    }

    assert_fail_count++;

    fprintf (stderr, "     [!] Assertion failed:\n");
    fprintf (stderr, "         expression: %s\n", expr);
    fprintf (stderr, "         test:       %s::%s\n", libtest_suite_name,
             libtest_test_case_name);
    fprintf (stderr, "         at:         %s:%i [%s]\n", filename, line,
             function_name);

    if (format && format[0] != 0)
    {
        fprintf (stderr, "         message:    ");
        va_list args;
        va_start (args, format);
        vfprintf (stderr, format, args);
        fputc ('\n', stderr);
        va_end (args);
    }

    return ASSERT_FAIL;
}

size_t
get_assert_fail_count (void)
{
    return assert_fail_count;
}

size_t
get_assert_success_count (void)
{
    return assert_success_count;
}

void
set_assert_fail_count (size_t value)
{
    assert_fail_count = value;
}

void
set_assert_success_count (size_t value)
{
    assert_success_count = value;
}
