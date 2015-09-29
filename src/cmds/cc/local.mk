#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Install as `gcc' instead of `cc' on decStations.
#
#if !empty(TM:Mds3100)
NAME=       gcc
#endif

#include	<$(SYSMAKEFILE)>

.PATH.h		: /sprite/src/cmds/cc/sprite /sprite/src/cmds/cc/dist

CFLAGS	+= -Isprite -Idist


DISTFILES   =   dist sprite

