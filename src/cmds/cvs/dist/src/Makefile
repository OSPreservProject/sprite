#
#	$Id: Makefile,v 1.20.1.1 91/01/18 12:04:00 berliner Exp $
#
# Makefile for CVS
#
COMMONSRCS=	subr.c
COMMONOBJS=	${COMMONSRCS:.c=.o}

CVS=		cvs
CVSSRCS=	add.c build_entry.c checkin.c checkout.c collect_sets.c\
		commit.c create_admin.c diff.c entries_file.c\
		find_names.c join.c locate_rcs.c log.c main.c\
		maketime.c modules.c name_repository.c no_difference.c\
		options.c partime.c patch.c register.c remove.c\
		scratch_entry.c set_lock.c status.c tag.c update.c\
		version_number.c version_ts.c
CVSOBJS=	${CVSSRCS:.c=.o} ${COMMONOBJS}

MOD=		mkmodules
MODSRCS=	mkmodules.c
MODOBJS=	${MODSRCS:.c=.o} ${COMMONOBJS}

CSHEXT=		.csh
CSHELLS=	checkin${CSHEXT}

HEADERS=	cvs.h patchlevel.h
ALLOBJS=	${COMMONOBJS} ${CVSOBJS} ${MODOBJS}
DESTDIR=	/usr/local/bin

ALLSRCS=	${COMMONSRCS} ${CVSSRCS} ${MODSRCS}
FILES=		Makefile ${HEADERS} ${ALLSRCS} ${CSHELLS}

PROGS=		${CVS} ${MOD}
CFLAGS=		-O # -g
LDFLAGS=	# -Bstatic

ETAGS=		/usr/local/bin/etags
CTAGS=		ctags

all: ${PROGS}

${CVS}: ${CVSOBJS}
	${CC} -o ${CVS} ${CFLAGS} ${CVSOBJS} ${LDFLAGS}

${MOD}: ${MODOBJS}
	${CC} -o ${MOD} ${CFLAGS} ${MODOBJS} ${LDFLAGS}

install: ${PROGS}
	install -m 755 ${PROGS} ${DESTDIR}
	for i in ${CSHELLS}; do\
		install -m 755 $$i ${DESTDIR}/`basename $$i ${CSHEXT}`;\
	done

clean:
	rm -f ${PROGS} *.o core Makefile.bak

lint:
	lint ${CVSSRCS} ${COMMONSRCS}
	lint -u ${MODSRCS} ${COMMONSRCS}

TAGS: ${ALLSRCS}
	${ETAGS} ${ALLSRCS}

tags: ${ALLSRCS}
	${CTAGS} ${ALLSRCS}

list:
	@for i in ${FILES}; do\
		echo $$i;\
	done

depend:
	@exit 0				# done by hand now

#
# Dependencies -- done by hand since some systems don't have cc -M!
#
${ALLOBJS}:	${HEADERS}
