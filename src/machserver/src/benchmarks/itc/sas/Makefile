#
# Set the -DSUN preprocessor variable if this machine puts the
# most significant byte of a number first, and -DVAX if it puts
# the most significant byte last.
#
# $Header: /user5/kupfer/spriteserver/src/benchmarks/itc/sas/RCS/Makefile,v 1.2 92/05/12 14:30:05 kupfer Exp $
#

default: ../bin/sas

OBJS =		parser.o sas.o lexer.o

TM=ds3100
NAME=sas
#include "/users/kupfer/lib/pmake/spriteClient.mk"
SPRITEDINCLUDES = -I$(SPRITEINCLUDE)/user/$(TM).md -I$(SPRITEINCLUDE)/user

CFLAGS =	-O -DVAX -DLOADER=\"../../../bin/sld\" \
			-DPREPROCESSOR=\"../../../bin/cpp\" $(SPRITEDINCLUDES)

../bin/sas:		$(OBJS)
	ld $(LOADFLAGS) $(INITOBJS) $(OBJS) -lc -lmach_sa -lc -o ../bin/sas

clean:
	rm -f *.o ../bin/sas
