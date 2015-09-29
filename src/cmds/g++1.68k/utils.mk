#
# This Makefile is a special one for the cc1 program (the main portion of
# the GNU C compiler).  The purpose of this Makefile is to generate
# executable copies of several utility programs, which are then used
# to generate source files for the compiler from a description of the
# machine for which the compiler is to generate code.  This Makefile
# must be separate from the main Makefile, and must be processed in
# a separate invocation of Pmake, because its target machine must be
# the machine on which we're executing now, not the machine on which
# the compiler is to execute.
#
# $Header: /sprite/src/cmds/g++1.68k/RCS/utils.mk,v 1.2 91/05/23 18:10:41 kupfer Exp $
#

MACHINES 	= $(TM)
SRCDIR		?= ../cc/dist
TM		?= sun3
CC		= cc
LIBS		?=
XCFLAGS		?=

.PATH.h		: ../cc/sprite $(SRCDIR) /sprite/lib/include \
			/sprite/lib/include/$(TM).md

#include	<tm.mk>
CFLAGS		 = -g -O $(TMCFLAGS) -I. $(.INCLUDES) $(XCFLAGS)

all		: $(TM).md/genconfig $(TM).md/genflags $(TM).md/gencodes \
			$(TM).md/genemit $(TM).md/genrecog \
			$(TM).md/genextract $(TM).md/genpeep \
			$(TM).md/genoutput
#
# The targets below are for a bunch of utility programs used to generate
# C files for machine-dependent aspects of the compiler.  Things are a
# little tricky here:  these programs have to be generated to run on
# the current machine ($MACHINE).
#

MAKEGEN:	.USE $(TM).md/rtl.o $(TM).md/obstack.o $(LIBS)
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -o $(.TARGET) $(.ALLSRC:N*.h:N*.def)

$(TM).md/genconfig		: $(SRCDIR)/genconfig.c MAKEGEN
$(TM).md/genflags		: $(SRCDIR)/genflags.c MAKEGEN
$(TM).md/gencodes		: $(SRCDIR)/gencodes.c MAKEGEN
$(TM).md/genemit		: $(SRCDIR)/genemit.c MAKEGEN
$(TM).md/genrecog		: $(SRCDIR)/genrecog.c MAKEGEN
$(TM).md/genextract		: $(SRCDIR)/genextract.c MAKEGEN
$(TM).md/genpeep		: $(SRCDIR)/genpeep.c MAKEGEN
$(TM).md/genoutput		: $(SRCDIR)/genoutput.c MAKEGEN

#
# Targets to generate the .o files that must be linked with the files
# above.

MAKEOBJ:	.USE
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -c -o $(.TARGET) $(.ALLSRC:N*.h:N*.def)

$(TM).md/obstack.o		: obstack.c MAKEOBJ
$(TM).md/rtl.o			: rtl.c MAKEOBJ

#if exists($(TM).md/dependencies.mk)
#include	"$(TM).md/dependencies.mk"
#endif
