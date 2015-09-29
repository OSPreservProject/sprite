#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

X=/X11/R4

LIBS	+= -lX11 -lm
SUBDIRS	= snf mi mfb cfb dix os Xsun extensions

#include	<$(SYSMAKEFILE)>

#
# Export the Xsun server under the name X.
#

SYNONYMS	= $(INSTALLDIR).$(TM)/X

install		:: $(SYNONYMS)

$(SYNONYMS)	:
	ln -s $(NAME) $(.TARGET)  

