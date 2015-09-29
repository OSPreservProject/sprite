#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

MAKE_USER_PROGRAM	= pmake

LIBS += -lc_g 

#include	"/users/kupfer/lib/pmake/spriteClient.mk"

#
# Add more install stuff in order to create a symbolic link from
# make to pmake.
#

install				:: $(INSTALLDIR).$(TM)/make
$(INSTALLDIR).$(TM)/make	:
	ln -s $(NAME) $(.TARGET)

