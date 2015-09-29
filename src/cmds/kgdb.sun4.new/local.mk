#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS		+= gdb/readline/$(TM).md/libreadline.a gdb/bfd/$(TM).md/libbfd.a gdb/libiberty/$(TM).md/liblibiberty.a -ltermlib

#
# NO_MALLOC_CHECK - Sprite doesn't appear to have the mcheck() stuff that
#		    gdb wants.  Defining NO_MALLOC_CHECK causes gdb to 
#		    be happy without.
# putenv=setenv	  - Gdb calls a routine putenv that doesn't exist on Sprite.
#		    Sprite does have a routine setenv that appears to do
#		    the same thing.
# NO_SIGINTERRUPT - Sprite doesn't have a siginterrupt library routine.
# HAVE_STRSTR	  - Sprite has the strstr function and gets upset if gdb
#		    trys to define it.

CFLAGS		+= -DNO_MALLOC_CHECK -DNO_SIGINTERRUPT -Dputenv=setenv -DHAVE_STRSTR -DKGDB


#include	<$(SYSMAKEFILE)>

.PATH.h		: . gdb gdb/sun4.md /usr/include /usr/include/sun4.md ./gdb/gnu_include ./gdb/readline ./gdb/bfd ./gdb/libiberty   

