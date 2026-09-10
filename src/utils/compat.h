#ifndef FHTTPD_COMPAT_H
#define FHTTPD_COMPAT_H

#define macro_concat_(a, b) a##b
#define macro_concat(a, b) macro_concat_ (a, b)

#if defined(__GNUC__) || defined(__clang__)
    #define likely(x) (__builtin_expect ((x), 1))
    #define unlikely(x) (__builtin_expect ((x), 0))

    #define ATTRIBUTE_FORMAT_PRINTF(arg1, arg2)                                \
        __attribute__ ((format (printf, (arg1), (arg2))))

    #undef static_assert
    #define static_assert(test, msg)                                           \
        struct macro_concat (static_assert__dummy_symbol_, __LINE__)           \
        {                                                                      \
            int _ : (test) ? 1 : -1;                                           \
        }

    #define FH_INLINE inline __attribute__ ((always_inline))
#else
    #define likely(x) (x)
    #define unlikely(x) (x)

    #define ATTRIBUTE_FORMAT_PRINTF(arg1, arg2)
    #define FH_INLINE inline
    #define __attribute__(x)
    #define static_assert(test, msg)
#endif

#if defined(__linux__)
    #define FH_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define FH_PLATFORM_DARWIN 1
    #define FH_PLATFORM_BSDLIKE 1
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    #define FH_PLATFORM_BSD 1
    #define FH_PLATFORM_BSDLIKE 1
#else
    #define FH_PLATFORM_UNKNOWN 1
#endif

#endif /* FHTTPD_COMPAT_H */
