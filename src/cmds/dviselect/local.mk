#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS		+= '-DCONFFILE="/sprite/lib/fonts/pk"' -DHAVE_VPRINTF -fwritable-strings

#include	<$(SYSMAKEFILE)>
