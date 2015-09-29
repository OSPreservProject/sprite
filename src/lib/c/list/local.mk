#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

MANPAGES	+= List.man

#include	<$(SYSMAKEFILE)>

List.man	: List.g
	grn List.g > List.man
