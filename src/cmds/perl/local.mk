#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS +=		-lm 

#include	<$(SYSMAKEFILE)>

#
# A couple of files can't be compiled with -O on a mips.
#
#if !empty(TM:Mds3100)
$(TM).md/cmd.o: cmd.c
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS:N-O) -c $(.IMPSRC) -o $(.TARGET)

$(TM).md/perl.o: perl.c
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS:N-O) -c $(.IMPSRC) -o $(.TARGET)

$(TM).md/cmd.po: cmd.c
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS:N-O) -DPROFILE -c $(.IMPSRC) -o $(.TARGET)

$(TM).md/perl.po: perl.c
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS:N-O) -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
#endif

perly.h: perly.c
	@ echo Dummy dependency for dumb parallel make
	touch perly.h

perly.c: perly.y perly.fixer
	@ echo 'Expect either' 29 shift/reduce and 59 reduce/reduce conflicts...
	@ echo '           or' 27 shift/reduce and 61 reduce/reduce conflicts...
	$(YACC) -d perly.y
	sh ./perly.fixer y.tab.c perly.c
	mv y.tab.h perly.h
	echo 'extern YYSTYPE yylval;' >>perly.h

