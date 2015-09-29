#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS		+= -DUSE_CONDITION
#LIBS		+= /r4/mach3.0/release/pmax_mach/latest/lib/libthreads.a -lmach
LIBS		+= -lthreads -lmach

#include "/users/kupfer/lib/pmake/mach.mk"
