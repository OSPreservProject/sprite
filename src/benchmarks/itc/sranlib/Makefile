# 
# The FIRST_BYTE preprocessor variable should be set to HIGH if the
# host machine puts the most significant byte of a number first (e.g.
# 68000s), and otherwise should be set to LOW (e.g. VAXes).
# 
# The FLOATING_FORM variable should be set to IEEE if the host machine
# uses IEEE format, or else to OTHER.
#

CFLAGS		= -O -DFIRST_BYTE=LOW -DFLOATING_FORM=IEEE \
			-DTARGET=HOST -I../sas

../bin/sranlib:		sranlib.o
	$(CC) sranlib.o -o ../bin/sranlib

clean:
	rm -f *.o ../bin/sranlib
