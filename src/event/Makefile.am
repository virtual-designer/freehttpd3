noinst_LTLIBRARIES = libevent.la
libevent_la_SOURCES = \
    xpoll.c \
    xpoll.h \
    xio.c \
    xio.h

libevent_la_LIBADD = $(LIBURING_LIBS)

AM_CFLAGS = $(LIBURING_CFLAGS)
AM_CPPFLAGS = $(LIBURING_CPPFLAGS)
AM_LDFLAGS = $(LIBURING_LDFLAGS)