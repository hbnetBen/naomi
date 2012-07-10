## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src 

daserfostscandocdir = ${prefix}/doc/daserfostscan
daserfostscandoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(daserfostscandoc_DATA)


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(daserfostscandocdir)
