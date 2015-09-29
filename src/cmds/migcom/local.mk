#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

YFLAGS	+= -d
LIBS	+= -ll

# migcom has macros that put arguments into strings, hence we need an
# old-style (traditional) cpp.
#if empty(TM:Mds3100) && empty(TM:Mds5000)
CFLAGS	+= -traditional
#endif

#include	<$(SYSMAKEFILE)>

.PATH.h :	/sprite/lib/include.mach

$(TM).md/lexxer.o : parser.h

parser.h: parser.c
	cp y.tab.h parser.h

clean::
	$(RM) -f parser.c parser.h y.tab.h
