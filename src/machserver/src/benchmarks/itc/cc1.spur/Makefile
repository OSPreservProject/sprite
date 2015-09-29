# $Header: /user5/kupfer/spriteserver/src/benchmarks/itc/cc1.spur/RCS/Makefile,v 1.2 92/05/12 14:28:00 kupfer Exp $

default: ../bin/cc1.spur

OBJS =		combine.o cse.o dbxout.o decl.o emit-rtl.o explow.o \
		expmed.o expr.o final.o flow.o fold-const.o global-alloc.o \
		insn-emit.o insn-extract.o insn-output.o insn-peep.o \
		insn-recog.o integrate.o jump.o local-alloc.o loop.o \
		obstack.o optabs.o parse.tab.o print-tree.o recog.o \
		regclass.o reload.o reload1.o rtl.o stmt.o stor-layout.o \
		stupid.o symout.o toplev.o tree.o typecheck.o varasm.o \
		version.o

TM=ds3100
NAME=cc1.spur
#include "/users/kupfer/lib/pmake/spriteClient.mk"
SPRITEDINCLUDES = -I$(SPRITEINCLUDE)/user/$(TM).md -I$(SPRITEINCLUDE)/user

MACHINEFLAGS =
CFLAGS =	-O -I. -I../gcc $(MACHINEFLAGS) $(SPRITEDINCLUDES)

../bin/cc1.spur: $(OBJS)
	ld $(LOADFLAGS) $(INITOBJS) $(MACHINEFLAGS) $(OBJS) -lc -lmach_sa -lc \
	-o ../bin/cc1.spur

#
# Special case because C compiler for DS3100 generates bad
# code with optimization on:
#

dbxout.o: dbxout.c
	cc -g -I. -I../gcc $(SPRITEDINCLUDES) -c $(MACHINEFLAGS) dbxout.c

#
# Special case because C compiler for Sun4 generates bad
# code without debugging on:
#

expr.o: expr.c
	cc -g -O -I. -I../gcc $(SPRITEDINCLUDES) -c $(MACHINEFLAGS) expr.c

clean:
	rm -f *.o ../bin/cc1.spur
