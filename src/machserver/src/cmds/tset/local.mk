#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#if empty(TM:Mds3100)
CFLAGS		+= -fwritable-strings
#endif
LIBS		+= -ltermcap

MAKE_USER_PROGRAM	= tset

#include	"/users/kupfer/lib/pmake/spriteClient.mk"

#
# Extend the install target to create a symbolic link for reset.
#

SYNONYMS	= $(INSTALLDIR).$(TM)/reset
install		:: $(SYNONYMS)
$(SYNONYMS)	:
	ln -s $(NAME) $(.TARGET)
