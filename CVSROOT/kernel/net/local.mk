#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Optimization breaks the sun4 net modules currently.
#
#if !empty(TM:Msun4) || !empty(TM:Msun4c)
CFLAGS	+= -Dvolatile= -B/sprite/cmds/1.34/
NOOPTIMIZATION	= no -O please
#elif !empty(TM:Mds3100) || !empty(TM:Mcleands3100) || !empty(TM:Mjhh)
CFLAGS	+= -Dvolatile= 
NOOPTIMIZATION	= no -O please
#endif

#include	<$(SYSMAKEFILE)>
