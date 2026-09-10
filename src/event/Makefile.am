AM_CFLAGS =
AM_CPPFLAGS =
AM_LDFLAGS =

noinst_LTLIBRARIES = libevent.la
libevent_la_SOURCES = \
    xpoll.c \
    xpoll.h \
    xio.h

if FH_BUILD_LINUX_LIBURING
libevent_la_SOURCES += \
    xio_uring.c

libevent_la_LIBADD = $(LIBURING_LIBS)

AM_CFLAGS += $(LIBURING_CFLAGS)
AM_CPPFLAGS += $(LIBURING_CPPFLAGS)
AM_LDFLAGS += $(LIBURING_LDFLAGS)
else # not FH_BUILD_LINUX_LIBURING
libevent_la_SOURCES += \
    xio_xpoll.c
endif # FH_BUILD_LINUX_LIBURING
