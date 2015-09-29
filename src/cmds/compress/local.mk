#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

# this first way doesn't really work.
#CFLAGS		+= -DBSD4_2 -O -DSACREDMEM=256000 -DUSERMEM=`cat USERMEM`
CFLAGS		+= -DBSD4_2 -O -DSACREDMEM=256000 -DUSERMEM=1170000

#include	<$(SYSMAKEFILE)>

#
# Add more install stuff in order to create symbolic links for other
# incarnations of the program.
#

SYNONYMS	= $(INSTALLDIR).$(TM)/uncompress $(INSTALLDIR).$(TM)/zcat

install		:: $(SYNONYMS)
$(SYNONYMS)	:
	ln -s $(NAME) $(.TARGET)
