#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Must run set-user-id to root.
#
INSTALLFLAGS	+= -o root -g wheel -m 4774

#if empty(TM:Mds3100)
CFLAGS		+= -Wall
#endif

#include	<$(SYSMAKEFILE)>
