#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NOOPTIMIZATION	= no -O please

#
# Define this flag until kdbx is no longer needed.
#

#if !empty(TM:Mds5000) || !empty(TM:Mds3100)
CFLAGS += -DKDBX
#endif

#include	<$(SYSMAKEFILE)>
