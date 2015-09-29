# 
# The FIRST_BYTE preprocessor variable should be set to HIGH if the
# host machine puts the most significant byte of a number first (e.g.
# 68000s), and otherwise should be set to LOW (e.g. VAXes).
# 
# The FLOATING_FORM variable should be set to IEEE if the host machine
# uses IEEE format, or else to OTHER.
#
# $Header: /sprite/src/benchmarks/itc/sld/RCS/Makefile,v 1.2 92/05/12 14:59:24 kupfer Exp $
#

OBJS		= sld.o bsearch.o 

CFLAGS		= -O -DFIRST_BYTE=LOW -DFLOATING_FORM=IEEE \
			-DTARGET=HOST -I../sas

../bin/sld:		$(OBJS)
	$(CC) $(OBJS) -o ../bin/sld

clean:
	rm -f *.o ../bin/sld
