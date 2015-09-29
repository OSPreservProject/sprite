#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#


#include	<$(SYSMAKEFILE)>

.PATH.h		: /sprite/src/cmds/gcc-2.1/sprite /sprite/src/cmds/gcc-2.1/dist

CFLAGS	+= -Isprite -Idist


DISTFILES   =   dist sprite

