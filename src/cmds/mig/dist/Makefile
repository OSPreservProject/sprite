#
# Mach Operating System
# Copyright (c) 1991,1990 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie the
# rights to redistribute these changes.
#
#
# HISTORY
# $Log:	Makefile,v $
# Revision 2.11  92/04/08  23:19:47  rpd
# 	Removed obsolete sun3 and mac2 compiler options.
# 	[92/03/23            rpd]
# 
# 	Added cross-compilation support.
# 	[92/03/18            jvh]
# 
# Revision 2.10  92/03/05  22:46:17  rpd
# 	Changed to use double-colon rules for top-level targets.
# 	[92/02/28            rpd]
# 
# Revision 2.9  92/01/23  15:21:21  rpd
# 	Revised for new Makefile organization.
# 	[92/01/16            rpd]
# 

include ${MAKETOP}Makefile-common

ITARGETS = ${HOST_INSTALLDIR}/bin/mig ${HOST_INSTALLDIR}/lib/migcom
TTARGETS = ${HOST_TRELEASEDIR}/bin/mig ${HOST_TRELEASEDIR}/lib/migcom

all :: mig.sh migcom

install :: ${ITARGETS}

release :: ${TTARGETS}

clean ::
	${RM} ${ITARGETS}

# installation rules

${HOST_INSTALLDIR}/bin/mig : mig.sh
	${RM} $@
	${CP} $? $@
	${CHMOD_BIN} $@

${HOST_INSTALLDIR}/lib/migcom : migcom
	${RM} $@
	${CP} $? $@
	${HOST_STRIP} $@
	${CHMOD_BIN} $@

# release rules

${HOST_TRELEASEDIR}/bin/mig : ${HOST_FRELEASEDIR}/bin/mig
	${RM} $@
	${CP} $? $@

${HOST_TRELEASEDIR}/lib/migcom : ${HOST_FRELEASEDIR}/lib/migcom
	${RM} $@
	${CP} $? $@

# build rules

OBJS = mig.o parser.o lexxer.o error.o string.o type.o routine.o \
	statement.o global.o header.o user.o server.o utils.o

PMAX_CCOPTS = -Dvoid=int
CCOPTS = ${${TARGET_MACHINE}_CCOPTS}

LIBS = ${LIBL}
DEPS = ${DEPL} ${DEPCRT0}

migcom : ${OBJS}
	${RM} $@ $@.out
	${USE_HOST_PATHS} ${DEPS/%/$@} \
	${HOST_CC} -o $@.out ${HOST_CFLAGS} ${OBJS} ${LIBS} && \
	${MV} $@.out $@

.c.o:
	${USE_HOST_PATHS} ${HOST_CC} -c ${HOST_CFLAGS} $*.c

lexxer.o : parser.h

parser.c parser.h : parser.y
	yacc -d parser.y && \
	mv y.tab.c parser.c && \
	mv y.tab.h parser.h

lexxer.c : lexxer.l
	lex lexxer.l && \
	mv lex.yy.c lexxer.c

-include Makedep
