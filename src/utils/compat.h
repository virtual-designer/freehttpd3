#ifndef FHTTPD_COMPAT_H
#define FHTTPD_COMPAT_H

#if defined(__GNUC__) || defined(__clang__)
#    define likely(x) (__builtin_expect ((x), 1))
#    define unlikely(x) (__builtin_unexpect ((x), 0))
#else
#    define likely(x) (x)
#    define unlikely(x) (x)
#endif

#endif /* FHTTPD_COMPAT_H */
