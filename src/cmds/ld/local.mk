#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#


#
# Install as `gld' instead of `ld' on decStations.
#
#if   !empty(TM:Mds3100)
NAME            =   gld
#endif

#include	<$(SYSMAKEFILE)>

