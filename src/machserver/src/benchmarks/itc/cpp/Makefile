# $Header: /user5/kupfer/spriteserver/src/benchmarks/itc/cpp/RCS/Makefile,v 1.2 92/05/12 14:28:41 kupfer Exp $

default: ../bin/cpp

OBJS =		cccp.o cexp.o version.o

TM=ds3100
NAME=cpp
#include "/users/kupfer/lib/pmake/spriteClient.mk"
SPRITEDINCLUDES = -I$(SPRITEINCLUDE)/user/$(TM).md -I$(SPRITEINCLUDE)/user

CFLAGS =	-O -I. -I../gcc $(SPRITEDINCLUDES)

../bin/cpp: $(OBJS)
	ld $(LOADFLAGS) $(INITOBJS) $(OBJS) -lc -lmach_sa -lc -o ../bin/cpp

clean:
	rm -f *.o ../bin/cpp
