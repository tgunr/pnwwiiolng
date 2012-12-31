# $Id: Makefile.in 42 2004-10-10 16:16:15Z gyunaev $

CFLAGS = -I../include -Wall @CFLAGS@
CXXFLAGS = -I../include -Wall @CFLAGS@
LDFLAGS = ../src/libircclient.la -lpthread @LIBS@

noinst_PROGRAMS = spammer colors irctest censor ircftp

spammer_SOURCES = spammer.c
spammer_LDADD = ../src/libircclient.la -lpthread @LIBS@

colors_SOURCES = colors.cpp
colors_LDADD = ../src/libircclient.la -lpthread @LIBS@

irctest_SOURCES = irctest.c
irctest_LDADD = ../src/libircclient.la -lpthread @LIBS@

censor_SOURCES = censor.cpp
censor_LDADD = ../src/libircclient.la -lpthread @LIBS@

ircftp_SOURCES = ircftp.cpp
ircftp_LDADD = ../src/libircclient.la -lpthread @LIBS@

FORCE:

