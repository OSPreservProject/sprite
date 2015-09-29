#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NAME		= kgdb.sun4

LIBS		+= -ltermlib 

#include	<$(SYSMAKEFILE)>

.PATH.h		: . gdb gdb/sun4.md gdb/sprite gdb/dist gdb/dist/readline \
		    /sprite/src/lib/include/sun4.md

CFLAGS          += -DKGDB -I. -Igdb -Igdb/sun4.md -Igdb/sprite \
                    -Igdb/dist -Igdb/dist/readline \
		    -I/sprite/src/lib/include/sun4.md

