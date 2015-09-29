#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS		+= -g -I/sprite/src/kernel/dev -I/sprite/src/lib/disk -L/sprite/src/lib/disk/$MACHINE.md
LIBS		+=  -ldisk
LINTFLAGS	+= -S
#include	<$(SYSMAKEFILE)>

