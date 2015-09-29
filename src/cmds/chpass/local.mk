#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Must run set-user-id to root.
#
INSTALLFLAGS	+= -o root -m 4775

#include	<$(SYSMAKEFILE)>

#
# Add more install stuff in order to create symbolic links for other
# versions of chpass like chfn, chsh.
#

SYNONYMS	= $(INSTALLDIR).$(TM)/chfn $(INSTALLDIR).$(TM)/chsh
install		:: $(SYNONYMS)
$(SYNONYMS)	:
	ln -s $(NAME) $(.TARGET)
