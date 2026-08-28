SUBDIRS = src tests
EXTRA_DIST = \
	README \
	NEWS \
	AUTHORS \
	bootstrap \
	ChangeLog \
	.tarball-version \
	build-aux/valgrind \
	build-aux/genchangelog \
	build-aux/update-license-comments \
	build-aux/git-version-gen

ACLOCAL_AMFLAGS = -I m4
GPG_LOCAL_USER ?=

vcs-clean: distclean
	for dir in $(SUBDIRS) .; do \
		rm -rf \
			$$dir/autom4te.cache \
			$$dir/config.log \
			$$dir/config.status \
			$$dir/Makefile \
			$$dir/Makefile.in \
			$$dir/aclocal.m4 \
			$$dir/configure \
			$$dir/config.h \
			$$dir/config.h.in \
			$$dir/stamp-h1 \
			$$dir/*~ \
			$$dir/.deps \
			$$dir/.dirstamp; \
	done

check-valgrind:
	$(MAKE) -C tests check-valgrind

.PHONY: check-valgrind vcs-clean dist-fhttpd dist-fhttpd-sign changelog

.tarball-version:
	echo "@VERSION@" > .tarball-version

distclean-local:
	rm -f .tarball-version
	rm -rf .deps

changelog:
	@echo "  UPDATE   ChangeLog" && \
		"$(top_srcdir)/build-aux/genchangelog" -f plain -o "$(top_srcdir)/ChangeLog"

dist-hook:
	@echo "  UPDATE   ChangeLog" && \
		chmod u+w $(distdir)/ChangeLog && \
		"$(distdir)/build-aux/genchangelog" -f plain -o "$(distdir)/ChangeLog" && \
		chmod u-w $(distdir)/ChangeLog

dist-fhttpd: dist
dist-fhttpd-sign: dist-fhttpd
	if test -z "$(GPG_LOCAL_USER)"; then \
		echo "The GPG_LOCAL_USER variable is not set - please set it to your GPG key ID."; \
		exit 1; \
	fi

	@for format in tar.gz tar.xz tar.bz2; do \
		echo "  SIGN     $(distdir).$$format"; \
		$(GPG) --batch --yes -a -b --local-user $(GPG_LOCAL_USER) "$(distdir).$$format"; \
	done
