#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Must run set-user-id to root.
#
INSTALLFLAGS	+= -o sprite -m 4775

#include	<$(SYSMAKEFILE)>
