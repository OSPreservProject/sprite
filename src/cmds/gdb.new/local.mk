#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS		+= readline/$(TM).md/libreadline.a bfd/$(TM).md/libbfd.a libiberty/$(TM).md/liblibiberty.a -ltermlib

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

CFLAGS		+= -DNO_MALLOC_CHECK -DNO_SIGINTERRUPT -Dputenv=setenv -DHAVE_STRSTR

#if   !empty(TM:Mds3100)
#  The dist ds3100 for Ultrix doesn't ATTACH_DETACH but on Sprite it does.
CFLAGS	+=  -DATTACH_DETACH
#endif 
#include	<$(SYSMAKEFILE)> 

#
# Set the include search path to look in the following directory 
# for the include files.

.PATH.h		: ./gnu_include ./readline ./bfd ./libiberty

