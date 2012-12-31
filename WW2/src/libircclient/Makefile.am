SUBDIRS = src examples

ACLOCAL_AMFLAGS=-I m4

install-data-local:
	mkdir -p /playnet/wwiiol/include/libircclient
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=install $(INSTALL_PROGRAM) include/*.h '/playnet/wwiiol/include/libircclient'

