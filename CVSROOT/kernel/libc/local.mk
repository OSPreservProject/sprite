#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# When we take a snapshot follow the symbolic links in this module because
# they point off into various libraries and we want our snapshot to be
# self-contained.
#
SNAPFLAGS	+= -l

#include	<$(SYSMAKEFILE)>

