# $Id: Makefile.in 62 2005-03-09 21:11:53Z gyunaev $
#AUTOMAKE_OPTIONS = foreign subdir-objects no-define no-installinfo no-installman no-texinfo.tex
ACLOCAL_AMFLAGS=-I $(top_srcdir)/m4
CC = @CC@
CFLAGS = -pthread -Wall -DIN_BUILDING_LIBIRC @CFLAGS@ 

INCLUDES=-I../include

lib_LTLIBRARIES = libircclient.la

libircclient_la_SOURCES = libircclient.c
libircclient_la_LDFLAGS = -version-info 1:3:0 -pthread

distclean: clean
	-rm -f Makefile

FORCE:

