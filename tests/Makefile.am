SUBDIRS = event hash
noinst_HEADERS = test-common.h

check-valgrind:
	@if test -z "$(VALGRIND)"; then \
	    echo "valgrind not found; install it to use this target" >&2; \
	    exit 1; \
	fi
	VALGRIND="$(VALGRIND)" $(MAKE) $(AM_MAKEFLAGS) check LOG_COMPILER='$(VALGRIND_WRAPPER)'

check-xpoll-generic-backend:
	@cd event && $(MAKE) $(AM_MAKEFLAGS) check-xpoll-generic-backend

clean-local:
	$(RM) -f vgcore.*

.PHONY: check-valgrind check-xpoll-generic-backend
