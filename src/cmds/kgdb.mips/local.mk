#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NAME		= kgdb.mips

NOOPTIMIZATION=true

LIBS		+= -ltermlib 

#
# If the kernel debugger stub supports kdbx then KDBX must be defined when
# compiling both the kernel debugger stub and kgdb.
#

CFLAGS += -DKDBX

#include	<$(SYSMAKEFILE)>

.PATH.h		:  . gdb gdb/ds3100.md \
		    gdb/sprite gdb/dist gdb/dist/readline 

CFLAGS          += -DKGDB -I. -Igdb \
		    -Igdb/ds3100.md -Igdb/sprite \
                    -Igdb/dist -Igdb/dist/readline 

