#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS          += -O -I../cc/sprite -I../cc/dist

#if !empty(TM:Msun4)
CFLAGS          += -DTARGET_SUN4 -D__sparc__
#endif

#
# Must add to OBJS for .c files generated below, since the .c files may
# not exist at the time mkmf is run to generate OBJS automatically.
#
OBJS		+= $(TM).md/insn-emit.o $(TM).md/insn-extract.o \
		$(TM).md/insn-output.o $(TM).md/insn-peep.o \
		$(TM).md/insn-recog.o
CLEANOBJS	+= $(TM).md/insn-emit.o $(TM).md/insn-extract.o \
		$(TM).md/insn-output.o $(TM).md/insn-peep.o \
		$(TM).md/insn-recog.o insn-emit.c insn-extract.c \
		insn-output.c insn-peep.c insn-recog.c \
		$(TM).md/gencodes $(TM).md/genconfig $(TM).md/genemit \
		$(TM).md/genextract $(TM).md/genflags $(TM).md/genoutput \
		$(TM).md/genpeep $(TM).md/genrecog

#
# Don't automatically back up Gcc stuff:  want to make sure the
# backup version is VERY reliable and don't want two quick buggy
# changes to result in unusable backup.
#
NOBACKUP	= true

#include	<$(SYSMAKEFILE)>


#
# Use headers from the main gcc area, including both stuff from
# the distribution and stuff that's been modified specially for Sprite.
#
.PATH.h		: ../cc/sprite ../cc/dist

#
# Target to make various utility programs.  Must run a separate make
# to do this, because the utilities must be compiled to run on $(MACHINE),
# the current machine, instead of $(TM), the machine that the new version
# of the compiler will run on.
#
# MACHINE		?= sun3

utils		: .MAKE .EXEC
	$(MAKE) TM=$(MACHINE) -l $(PASSVARS) -f utils.mk all

#
# Targets to create a bunch of source files from the machine
# description.
#
insn-codes.h : md $(MACHINE).md/gencodes
	rm -f insn-codes.h
	$(MACHINE).md/gencodes md > insn-codes.h

insn-config.h : md $(MACHINE).md/genconfig
	rm -f insn-config.h
	$(MACHINE).md/genconfig md > insn-config.h

insn-emit.c : md $(MACHINE).md/genemit
	rm -f insn-emit.c
	$(MACHINE).md/genemit md > insn-emit.c

insn-extract.c : md $(MACHINE).md/genextract
	rm -f insn-extract.c
	$(MACHINE).md/genextract md > insn-extract.c

insn-flags.h : md $(MACHINE).md/genflags
	rm -f insn-flags.h
	$(MACHINE).md/genflags md > insn-flags.h

insn-output.c: md $(MACHINE).md/genoutput
	rm -f insn-output.c
	$(MACHINE).md/genoutput md >insn-output.c

insn-peep.c : md $(MACHINE).md/genpeep
	rm -f insn-peep.c
	$(MACHINE).md/genpeep md > insn-peep.c

insn-recog.c : md $(MACHINE).md/genrecog
	rm -f insn-recog.c
	$(MACHINE).md/genrecog md > insn-recog.c

$(MACHINE).md/gencodes $(MACHINE).md/genconfig \
	  $(MACHINE).md/genemit $(MACHINE).md/genextract \
	  $(MACHINE).md/genflags $(MACHINE).md/genoutput \
	  $(MACHINE).md/genpeep $(MACHINE).md/genrecog: utils

#
# Must add to depend targets for above-generated .h files, since they
# may not exist when make depend is run for first time.
#

$(TM).md/dependencies.mk	! insn-config.h insn-codes.h insn-flags.h

TMINSTALLDIR	= /sprite/lib/gcc/$(TM).md

