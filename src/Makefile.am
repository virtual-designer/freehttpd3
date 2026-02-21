SUBDIRS = include core hash http mm log utils

bin_PROGRAMS = freehttpd
freehttpd_SOURCES = fhttpd.c
freehttpd_LDADD = \
	core/libcore.a \
	hash/libhash.a \
	http/libhttp.a \
	log/liblog.a \
	utils/libutils.a \
	mm/libmm.a

AM_CFLAGS = $(EXPORTED_AM_CFLAGS)
AM_CPPFLAGS = $(EXPORTED_AM_CPPFLAGS)
AM_LDFLAGS = $(EXPORTED_AM_LDFLAGS)

distclean-local:
	rm -rf .deps
