#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Optimization breaks the sun4 net modules currently.
#
#if !empty(TM:Msun4) || !empty(TM:Mds3100) || !empty(TM:Mcleands3100) || !empty(TM:Mjhh) 
NOOPTIMIZATION	= no -O please
#endif

#include	<$(SYSMAKEFILE)>
