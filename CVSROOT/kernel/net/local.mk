#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NAME = net

#
# Optimization breaks the ds3100.
#

#if !empty(TM:Mds3100) || !empty(TM:Mds5000)
NOOPTIMIZATION	= no -O please
#endif

#include	"kernel.mk"

