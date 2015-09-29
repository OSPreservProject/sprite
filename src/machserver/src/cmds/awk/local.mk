#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#


MAKE_USER_PROGRAM	= awk

#
# Things are tricky for awk because a) some of the source don't pertain
# directly to awk:  they are used to generate the program "proc", which
# in turn is used to generate one of the "real" awk source files;  and b)
# some of the awk source files are derived using yacc.
#

SRCS		= awk.def awk.g.y awk.lx.l b.c lib.c main.c parse.c proc.c \
		freeze.c run.c token.c tran.c
OBJS		= $(TM).md/awk.g.o $(TM).md/awk.lx.o $(TM).md/b.o \
		$(TM).md/main.o $(TM).md/token.o $(TM).md/tran.o \
		$(TM).md/lib.o $(TM).md/run.o $(TM).md/parse.o \
		$(TM).md/proctab.o $(TM).md/freeze.o
CLEANOBJS	+= $(MACHINES:S|$|.md/proc|g)
LIBS		= -lm

#include	"/users/kupfer/lib/pmake/spriteClient.mk"

awk.g.o			:
	$(YACC) $(YFLAGS) $(.IMPSRC)
	$(CC) -c $(CFLAGS) -o $(.TARGET) y.tab.c
	-cmp -s y.tab.h awk.h || cp y.tab.h awk.h
	rm -f y.tab.c

awk.h			: awk.g.y
	$(YACC) -d awk.g.y
	mv y.tab.h awk.h
	rm -f y.tab.c

token.c			: awk.h
	ex -h <tokenscript

#if !empty(MACHINE:Mds5000)
BUILD_MACHINE	= ds3100
#else
BUILD_MACHINE	= $(MACHINE)
#endif

proctab.c		: $(BUILD_MACHINE).md/proc
	$(BUILD_MACHINE).md/proc >proctab.c

$(BUILD_MACHINE).md/proc	: utils

utils			: .MAKE .EXEC
	$(MAKE) -l $(PASSVARS) TM=$(BUILD_MACHINE) -f utils.mk
