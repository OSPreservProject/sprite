#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#if empty(TM:Mds3100)
CFLAGS += -Wall
#endif

MAKE_USER_PROGRAM	= netroute
LIBS		+= -lnet

#include "/users/kupfer/lib/pmake/spriteClient.mk"
