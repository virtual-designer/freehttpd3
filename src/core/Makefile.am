noinst_LIBRARIES = libcore.a
libcore_a_SOURCES = \
	xpoll.c \
	xpoll.h \
	server.c \
	server.h \
	config.c \
	config.h \
	worker.c \
	worker.h

AM_CFLAGS = $(EXPORTED_AM_CFLAGS)
AM_CPPFLAGS = $(EXPORTED_AM_CPPFLAGS)
AM_LDFLAGS = $(EXPORTED_AM_LDFLAGS)

distclean-local:
	rm -rf .deps
