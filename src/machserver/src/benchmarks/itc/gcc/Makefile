#
# If your system has a system include file "alloca.h", make sure there
# is no file by that name in this directory (rename it to alloca.h.dummy).
# If your system doesn't define the include file "alloca.h", make sure a
# dummy version exists in this directory (raname alloca.h.dummy to
# alloca.h if alloca.h isn't here already).
#
# $Header: /user5/kupfer/spriteserver/src/benchmarks/itc/gcc/RCS/Makefile,v 1.2 92/05/12 14:29:10 kupfer Exp $

default: ../bin/gcc

OBJS =		gcc.o obstack.o version.o

TM=ds3100
NAME=gcc
#include "/users/kupfer/lib/pmake/spriteClient.mk"
SPRITEDINCLUDES = -I$(SPRITEINCLUDE)/user/$(TM).md -I$(SPRITEINCLUDE)/user

CFLAGS =	-O $(SPRITEDINCLUDES)

../bin/gcc:	$(OBJS)
	ld $(LOADFLAGS) $(INITOBJS) $(OBJS) -lc -lmach_sa -lc -o ../bin/gcc

clean:
	rm -f *.o ../bin/gcc
