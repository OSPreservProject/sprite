#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

MDPUBHDRS	=

#include	<$(SYSMAKEFILE)>

CFLAGS		+= -Dieee

# A lot of things are conditional on "national" because of the order of
# bytes in a double.  ds3100s use the same bizarre ordering.

#if !empty(TM:Mds3100)
CFLAGS += -Dnational
#endif

#
# Gcc seems to be screwing up on some files when optimization is enabled.
# So, for sincos.c, give explicit compilation commands here to avoid use
# of "-o" switch.  This problem occurred with version 1.25 of Gcc.  Retry
# in a while to see if the problem has gone away (cos(.785400) will screw
# up).
#

$(TM).md/sincos.go:	sincos.c
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS:N-O) -g -c sincos.c -o $(.TARGET)

$(TM).md/sincos.o:	sincos.c
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS:N-O) -c sincos.c -o $(.TARGET)
