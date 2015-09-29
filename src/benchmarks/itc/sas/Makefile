#
# Set the -DSUN preprocessor variable if this machine puts the
# most significant byte of a number first, and -DVAX if it puts
# the most significant byte last.
#

CFLAGS =	-O -DVAX -DLOADER=\"../../../bin/sld\" \
			-DPREPROCESSOR=\"../../../bin/cpp\"

../bin/sas:		parser.o sas.o lexer.o
	$(CC) parser.o sas.o lexer.o -o ../bin/sas

clean:
	rm -f *.o ../bin/sas
