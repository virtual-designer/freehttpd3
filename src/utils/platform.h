#ifndef FHTTPD_PLATFORM_H
#define FHTTPD_PLATFORM_H

#define PLATFORM_LINUX 0
#define PLATFORM_DARWIN 0
#define PLATFORM_UNKNOWN 0

#if defined(__linux__)
#   undef PLATFORM_LINUX
#   define PLATFORM_LINUX 1
#   define PLATFORM "GNU/Linux"
#elif defined(__APPLE__)
#   undef PLATFORM_DARWIN
#   define PLATFORM_DARWIN 1
#   define PLATFORM "Darwin"
#else /* not defined(__linux__) */
#   undef PLATFORM_UNKNOWN
#   define PLATFORM_UNKNOWN 1
#   define PLATFORM "Unknown"
#endif /* defined(__linux__) */

#endif /* FHTTPD_PLATFORM_H */