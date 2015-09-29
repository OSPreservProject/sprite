#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS		= -lc -lmx -lsx -lcmd -ltcl5 -lX11
LINTFLAGS	= -S -u -lsx -lmx

#include	<$(SYSMAKEFILE)>

#
# Add more install stuff in order to create a "mxsync" synonym for
# mx using a symbolic link.
#

SYNONYMS	= $(INSTALLDIR).$(TM)/mxsync
install		:: $(SYNONYMS)
$(SYNONYMS)	:
	ln -s $(NAME) $(.TARGET)
