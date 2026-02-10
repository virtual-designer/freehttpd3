SUBDIRS = core utils

bin_PROGRAMS = freehttpd
freehttpd_SOURCES = fhttpd.c
freehttpd_LDADD = core/libcore.a

export EXPORTED_AM_CFLAGS =
export EXPORTED_AM_LDFLAGS =
export EXPORTED_AM_CPPFLAGS = \
	-I$(abs_srcdir)/include \
	-I$(abs_srcdir)/utils \
	-I$(abs_srcdir) \
	-I$(top_builddir)/res \
	-DHAVE_RESOURCES \
	-DHAVE_CONFPATHS_H

AM_CFLAGS = $(EXPORTED_AM_CFLAGS)
AM_CPPFLAGS = $(EXPORTED_AM_CPPFLAGS)
AM_LDFLAGS = $(EXPORTED_AM_LDFLAGS)

clean-local:
	rm -rf .deps
