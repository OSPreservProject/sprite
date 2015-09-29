#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff. Mklfs uses
# this file to invoke the C compiler gcc with the -Wall option. The -Wall
# option tells gcc to do lint-like checking while compiling. 
#

LIBS +=	-ldisk
#include	<$(SYSMAKEFILE)>

#if empty(TM:Mds3100)
CFLAGS +=  -Wall 
#endif

