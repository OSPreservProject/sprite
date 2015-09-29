#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Optimization is turned off because the current compiler (1.37.1)
# for the sun3 does not produce correct code for Mach_GetCallerPC().
# The register that the pc was stored into, and the one which it was


NOOPTIMIZATION = yes		

#include	<$(SYSMAKEFILE)>

