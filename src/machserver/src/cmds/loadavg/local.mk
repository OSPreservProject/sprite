#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

MAKE_USER_PROGRAM	= loadavg

# Use the uninstalled C library, in general.
LIBS += -lc_g

#include	"/users/kupfer/lib/pmake/spriteClient.mk"

#
# Add more install stuff in order to create symbolic links for other
# aliases.
#

SYNONYMS	= $(INSTALLDIR).$(TM)/la \
		  $(INSTALLDIR).$(TM)/evict \
		  $(INSTALLDIR).$(TM)/uptime
install		:: $(SYNONYMS)
$(SYNONYMS)	:
	ln -s $(NAME) $(.TARGET)
