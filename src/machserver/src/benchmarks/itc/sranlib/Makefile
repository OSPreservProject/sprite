# 
# The FIRST_BYTE preprocessor variable should be set to HIGH if the
# host machine puts the most significant byte of a number first (e.g.
# 68000s), and otherwise should be set to LOW (e.g. VAXes).
# 
# The FLOATING_FORM variable should be set to IEEE if the host machine
# uses IEEE format, or else to OTHER.
#
# $Header: /user5/kupfer/spriteserver/src/benchmarks/itc/sranlib/RCS/Makefile,v 1.2 92/05/12 14:31:03 kupfer Exp $
#

default: ../bin/sranlib

TM=ds3100
NAME=cpp
#include "/users/kupfer/lib/pmake/spriteClient.mk"
SPRITEDINCLUDES = -I$(SPRITEINCLUDE)/user/$(TM).md -I$(SPRITEINCLUDE)/user

CFLAGS		= -O -DFIRST_BYTE=LOW -DFLOATING_FORM=IEEE \
			-DTARGET=HOST -I../sas $(SPRITEDINCLUDES)

../bin/sranlib:		sranlib.o
	ld $(LOADFLAGS) $(INITOBJS) sranlib.o -lc -lmach_sa -lc -o ../bin/sranlib

clean:
	rm -f *.o ../bin/sranlib
