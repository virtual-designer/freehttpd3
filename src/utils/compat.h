#ifndef FHTTPD_COMPAT_H
#define FHTTPD_COMPAT_H

#if defined(__GNUC__) || defined(__clang__)
#    define likely(x) (__builtin_expect ((x), 1))
#    define unlikely(x) (__builtin_expect ((x), 0))
#else
#    define likely(x) (x)
#    define unlikely(x) (x)
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
