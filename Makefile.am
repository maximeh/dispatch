AUTOMAKE_OPTIONS = foreign subdir-objects
CFLAGS=-Wall -g -O3 -I/usr/include/taglib
LDFLAGS = @LDFLAGS@
LIBS = @LIBS@ @TAGLIB_LIBS@ @TAGLIB_C_LIBS@
CPPFLAGS=-I/usr/local/include -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" @ARCH_DEFINES@
INSTALL = @INSTALL@
MKDIR_P = @MKDIR_P@
prefix = @prefix@
exec_prefix = @exec_prefix@
bindir = $(DESTDIR)$(exec_prefix)/bin

bin_PROGRAMS = dispatch
dispatch_SOURCES = src/dispatch.c src/dispatch.h

