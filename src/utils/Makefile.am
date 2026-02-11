noinst_LIBRARIES = libutils.a
libutils_a_SOURCES = \
	datetime.c \
	datetime.h \
	utils.c \
	utils.h

AM_CFLAGS = $(EXPORTED_AM_CFLAGS)
AM_CPPFLAGS = $(EXPORTED_AM_CPPFLAGS)
AM_LDFLAGS = $(EXPORTED_AM_LDFLAGS)

distclean-local:
	rm -rf .deps
