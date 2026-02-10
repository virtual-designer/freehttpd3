#           -*- mode: makefile-automake; -*-
# Process this file with automake to produce Makefile.in.
#
# This file is part of OSN freehttpd.
#
# Copyright (C) 2025-2026  OSN Developers.
#
# OSN freehttpd is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OSN freehttpd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with OSN freehttpd.  If not, see <https://www.gnu.org/licenses/>.

RESOURCES = html/error.html

EXTRA_DIST = \
	$(RESOURCES) \
	logos/freehttpd.png \
	logos/freehttpd_pb.png \
	logos/freehttpd_web.png \
	logos/freehttpd_http.png \
	icons/file.png \
	icons/folder.png \
	icons/image.png \
	icons/text.png \
	NOTICE

noinst_LIBRARIES = libresources.a
nodist_libresources_a_SOURCES = resources.c resources.h
BUILT_SOURCES = $(nodist_libresources_a_SOURCES)
CLEANFILES = $(BUILT_SOURCES)

resources.c: $(RESOURCES) resources.h
	@echo "  GEN      resources.c" && echo '#include "resources.h"' > $@; \
	echo '#include <stddef.h>' >> $@; \
	echo '#include <stdint.h>' >> $@; \
	for res in $(RESOURCES); do \
	    name=$$(basename $$res | sed "s/\./_/g"); \
		xxd -n "resource_$$name" -i "$(srcdir)/$$res" \
			| sed 's/unsigned int/const size_t/g' \
			| sed 's/unsigned char/const char/g' \
			| sed 's/};/, 0x00 };/g' >> $@; \
	done; \
	echo "const struct resource_file global_resources[] = {" >> $@; \
	for res in $(RESOURCES); do \
	    name=$$(basename $$res | sed "s/\./_/g"); \
		name_len=$$(echo "$$name" | wc -c | sed 's/ //g'); \
		size=$$(wc -c < "$(srcdir)/$$res"); \
		echo "  { .name = \"resource_$$name\", .name_len = $$name_len, .data = resource_$$name, .data_len = $$size, }," >> $@; \
	done; \
	echo "};" >> $@; \
	echo 'const size_t global_resources_size = sizeof(global_resources) / sizeof(global_resources[0]);' >> $@;

resources.h: $(RESOURCES)
	@echo "  GEN      resources.h" && echo '#ifndef GENERATED_RESOURCE_H' > $@; \
	echo '#define GENERATED_RESOURCE_H' >> $@; \
	echo '#include <stddef.h>' >> $@; \
	echo 'struct resource_file {' >> $@; \
	echo '  const char *name;' >> $@; \
	echo '  size_t name_len;' >> $@; \
	echo '  const char *data;' >> $@; \
	echo '  size_t data_len;' >> $@; \
	echo '};' >> $@; \
	for res in $(RESOURCES); do \
	    name=$$(basename $$res | sed "s/\./_/g"); \
		echo 'extern const size_t resource_'$$name'_len;' >> $@; \
		echo 'extern const char resource_'$$name'[];' >> $@; \
	done; \
	echo 'extern const struct resource_file global_resources[];' >> $@; \
	echo 'extern const size_t global_resources_size;' >> $@; \
	echo '#endif /* GENERATED_RESOURCE_H */' >> $@;

distclean-local:
	rm -rf .deps
