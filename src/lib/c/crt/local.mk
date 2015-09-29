#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
#   This directory contains the low level profiling routines.
#   They need to be included in the profiling library, but they
#   are not themselves profiled.  So they are compiled without
#   the -p flag when `make profile' is run.
#
NOPROFILE=TRUE

#include	<$(SYSMAKEFILE)>

