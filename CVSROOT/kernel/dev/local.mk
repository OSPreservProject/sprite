#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NAME = dev

#if !empty(TM:Mds3100) || !empty(TM:Mjhh) || !empty(TM:Mcleands3100) || !empty(TM:Mds5000)
NOOPTIMIZATION	= no -O please
#endif

CFLAGS	+= -DPROM_1_4

#include	<$(SYSMAKEFILE)>
