#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Use the old compiler.  The newer compilers break the bit fields
# in the net module.  We need to rewrite all this stuff so it doesn't
# use bitfields.  They are not very portable.
#
#if !empty(TM:Msun4) || !empty(TM:Msun4c) || !empty(TM:Mcleansun4) || !empty(TM:Mcleansun4c)
CFLAGS	+= -B/sprite/cmds/1.34/
#elif !empty(TM:Msun3) || !empty(TM:Mcleansun3)
CFLAGS	+= -B/sprite/cmds/1.36/
#endif

CFLAGS	+= -Dvolatile=

#
# Optimization breaks just about everything.
#
NOOPTIMIZATION	= no -O please

#include	<$(SYSMAKEFILE)>

