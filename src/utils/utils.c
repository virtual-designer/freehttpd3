#include <fcntl.h>

#include "utils.h"

bool
util_fd_set_nonblocking (fd_t fd)
{
    int flags = fcntl (fd, F_GETFL);

    if (flags < 0)
        return false;

    return fcntl (fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

uint64_t
util_round_ceil2_ull (uint64_t value)
{
    if (value <= 2)
        return value;

#if defined(__GNUC__) || defined(__clang__)
    return 1ULL << (64ULL - __builtin_clzll (value - 1));
#else
    value--;
    value |= value >> 1ULL;
    value |= value >> 2ULL;
    value |= value >> 4ULL;
    value |= value >> 8ULL;
    value |= value >> 16ULL;
    value |= value >> 32ULL;
    return value + 1;
#endif /* defined(__GNUC__) || defined(__clang__) */
}
