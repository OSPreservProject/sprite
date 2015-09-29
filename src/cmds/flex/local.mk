#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LEX = $MACHINE.md/flex
CFLAGS += -DDEFAULT_SKELETON_FILE=\"/sprite/lib/flex/flex.skel\"


#include	<$(SYSMAKEFILE)>

parse.c: parse.y
	yacc -vd parse.y
	mv y.tab.c parse.c

parse.h: parse.c
	- if test ! -f y.tab.h; then \
	    yacc -d parse.y; \
	    rm -f y.tab.c; \
	fi
	- if test -f parse.h; then \
	   if cmp parse.h y.tab.h; then \
	     echo parse.h unchanged; touch parse.h; rm y.tab.h; \
	   else mv y.tab.h parse.h; \
	   fi; \
	else mv y.tab.h parse.h; \
	fi

#
# Extra stuff to install information in /sprite/lib/flex.
#
install		:: installlib
installlib	:
	$(UPDATE) -m 664 -l $(INSTALLFLAGS) flex.skel /sprite/lib/flex

