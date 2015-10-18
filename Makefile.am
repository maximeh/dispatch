AUTOMAKE_OPTIONS = foreign subdir-objects
CFLAGS += -Wall -Wextra -Wpedantic \
	  -Wformat=2 -Wno-unused-parameter -Wshadow \
	  -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
	  -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
	  -Wjump-misses-init -Wlogical-op -Werror -g -O3 -I/usr/include/taglib

LDFLAGS = @LDFLAGS@
LIBS = @LIBS@ @TAGLIB_LIBS@ @TAGLIB_C_LIBS@
CPPFLAGS = -I/usr/local/include -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" @ARCH_DEFINES@
INSTALL = @INSTALL@
MKDIR_P = @MKDIR_P@
prefix = @prefix@
exec_prefix = @exec_prefix@
bindir = $(DESTDIR)$(exec_prefix)/bin

bin_PROGRAMS = dispatch
dispatch_SOURCES = src/dispatch.c \
		   src/files.c src/files.h \
		   src/tools.c src/tools.h

# Static analysis.
analyze_srcs = $(dispatch_SOURCES)

.PHONY: analyze

analyze: all
	@for f in $(analyze_srcs); do \
		if test "@AM_V@" = 1; then \
			echo clang --analyze -x c -I build-aux $$f -o /dev/null; \
		else \
			echo "  CCSA  " $$f; \
		fi; \
		clang --analyze -x c -I build-aux $$f -o /dev/null; \
	done
