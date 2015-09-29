#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

SUBDIRS		= lst src 

LIBS += -lc_g 

#include	<$(SYSMAKEFILE)>

#
# Add more install stuff in order to create a symbolic link from
# make to pmake.
#

install				:: $(INSTALLDIR).$(TM)/make
$(INSTALLDIR).$(TM)/make	:
	ln -s $(NAME) $(.TARGET)

#
# Ditto for a symlink for the documentation
#
install		:: /sprite/doc/pmake

/sprite/doc/pmake :
	ln -s /sprite/src/cmds/pmake/doc $(.TARGET)
