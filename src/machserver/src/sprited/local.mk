#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS		+= -lmachid -lnetname -lthreads -lmach

use_version	= yes
VERSION		= sprited_version

#include "/users/kupfer/lib/pmake/spritedCommon.mk"

sprited_version	:
	./doversion FALSE
	sed "s/MACHINE/$(TM)/g" version.h.template > version.h
	$(RM) -f $(TM).md/version.o
	$(CC) $(CFLAGS) -c -o $(TM).md/version.o $(MISCLIBDIR)/version.c

