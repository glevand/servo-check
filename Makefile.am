AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = $(DEFAULT_CPPFLAGS)
AM_CFLAGS = $(DEFAULT_CFLAGS)

EXTRA_DIST = bootstrap configure.ac $(srcdir)/m4 $(noinst_SCRIPTS) mit-plus-license.txt \
 README.md notes.txt

MAINTAINERCLEANFILES = autom4te.cache aclocal.m4 compile config.* configure \
 depcomp install-sh ltmain.sh Makefile.in missing $(PACKAGE)-*.gz

maintainer-clean-local:
	rm -rf m4

bin_PROGRAMS = servo-check

servo_check_DEPENDENCIES = Makefile
servo_check_LDADD = -lm
servo_check_SOURCES = \
 filter.c filter.h \
 util.c util.h \
 servo-check.c

noinst_SCRIPTS = check-servo-check.sh

.PHONY: help

help:
	@echo "Targets:"
	@echo "  make install"
	@echo "  make dist"
	@echo "  make distcheck"
	@echo "  make distclean"
	@echo "  make maintainer-clean"
