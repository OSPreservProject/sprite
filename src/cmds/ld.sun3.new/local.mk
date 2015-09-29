#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS +=  -DTARGET_MACHINE=TARGET_SUN3 \
           -I../ld/dist -I/sprite/lib/include/sun3.md

#include	<$(SYSMAKEFILE)>

.PATH.h : ../ld/dist /sprite/lib/include/sun3.md

#
# Arrange for programs to be installed in the library area instead of
# the normal commands area.
#

TMINSTALLDIR	= /sprite/lib/gcc/$(TM).md

