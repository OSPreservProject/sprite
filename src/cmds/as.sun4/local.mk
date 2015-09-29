#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS += -I../as/sprite -I../as/dist -Derror=as_fatal -DSPARC

#include	<$(SYSMAKEFILE)>

.PATH.h : ../as/sprite ../as/dist

#
# Arrange for programs to be installed in the library area instead of
# the normal commands area.
#

TMINSTALLDIR	= /sprite/lib/gcc/$(TM).md


