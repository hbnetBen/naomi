## Process this file with automake to produce Makefile.in

## Created by Anjuta

uidir = $(datadir)/daserfostscan/ui
ui_DATA = daserfostscan.ui

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(DASERFOSTSCAN_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = daserfostscan

daserfostscan_SOURCES = \
	callbacks.c \
	callbacks.h \
	main.c \
	actions.c \
	database.c \
	database.h \
	DPcommon.h \
	FPentrypoint.c

daserfostscan_LDFLAGS = \
	-Wl,--export-dynamic

daserfostscan_LDADD = $(DASERFOSTSCAN_LIBS)

EXTRA_DIST = $(ui_DATA)

SUBDIRS = \
	data

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/daserfostscan
