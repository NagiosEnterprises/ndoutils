###############################
# Makefile for NDO
#
# Last Modified: 10-03-2007
###############################


# Source code directories
SRC_BASE=./src/
SRC_INCLUDE=./include/

all:
	cd $(SRC_BASE); $(MAKE) ; cd ..

clean:
	cd $(SRC_BASE); $(MAKE) $@ ; cd ..
	rm -f core
	rm -f *~ */*~ include/nagios-*/*~

distclean: clean
	cd $(SRC_BASE); $(MAKE) $@ ; cd ..
	rm -f config.log config.status config.cache $(SRC_INCLUDE)/config.h
	rm -f Makefile
	rm -f init-script.suse subst

devclean: distclean

