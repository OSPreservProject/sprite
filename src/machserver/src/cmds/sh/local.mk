#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# This program contains lots of gross code that generates zillions
# of compiler warnings.  Disable them.
#

CFLAGS		+= -w

MAKE_USER_PROGRAM	= sh

#include	"/users/kupfer/lib/pmake/spriteClient.mk"
