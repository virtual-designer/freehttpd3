noinst_LTLIBRARIES = libevent.la
libevent_la_SOURCES = \
    xpoll.c \
    xpoll.h

if FH_BUILD_LINUX_LIBURING
libevent_la_SOURCES += \
    xio.c \
    xio.h
endif

libevent_la_LIBADD = $(LIBURING_LIBS)

AM_CFLAGS = $(LIBURING_CFLAGS)
AM_CPPFLAGS = $(LIBURING_CPPFLAGS)
AM_LDFLAGS = $(LIBURING_LDFLAGS)
