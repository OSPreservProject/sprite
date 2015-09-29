#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

# No man pages defined; make installman be a no-op.
MANPAGES = NONE

#if !empty(TM:Mds3100)
CC=	gcc
#endif

#include	<$(SYSMAKEFILE)>
