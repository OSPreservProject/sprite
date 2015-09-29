#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS		+= -DDEBUG
LIBS		+= -lm 

# Release of gawk.  There can be no leading or trailing white space here!
REL=2.11

SRCS		+= version.c
OBJS		+= $(TM).md/version.o

INFOFILES= gawk-info gawk-info-1 gawk-info-2 gawk-info-3 gawk-info-4 \
	   gawk-info-5 gawk-info-6 gawk.aux gawk.cp gawk.cps gawk.fn \
	   gawk.fns gawk.ky gawk.kys gawk.pg gawk.pgs gawk.toc \
	   gawk.tp gawk.tps gawk.vr gawk.vrs

CLEANOBJS += $(INFOFILES)


#include	<$(SYSMAKEFILE)>

version.c: version.sh
	sh version.sh $(REL) > version.c

gawk.dvi: gawk.texinfo
	tex gawk.texinfo ; support/$MACHINE.md/support gawk.??
	tex gawk.texinfo ; support/$MACHINE.md/support gawk.??
	tex gawk.texinfo

$(INFOFILES): gawk.texinfo
	makeinfo gawk.texinfo


#
# Install gawk under the more standard names of awk and nawk.
#

#SYNONYMS	= $(INSTALLDIR).$(TM)/awk $(INSTALLDIR).$(TM)/nawk 

#install		:: $(SYNONYMS)
#$(SYNONYMS)	:
#	ln -s $(NAME) $(.TARGET)

#MAN_SYNONYMS	= $(INSTALLMAN)/awk.man $(INSTALLMAN)/nawk.man

#installman	:: $(MAN_SYNONYMS)
#$(MAN_SYNONYMS) :
#	ln -s $(NAME).man $(.TARGET)


