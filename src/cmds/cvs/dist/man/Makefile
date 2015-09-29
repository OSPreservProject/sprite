#
#	$Id: Makefile,v 1.1 89/11/19 23:17:41 berliner Exp $
#
# Makefile for CVS Manual Pages
#
MANSEC=		l
MANEXT=		.man
MANDIR=		/usr/man/man${MANSEC}
MANSRCS=	cvs${MANEXT} mkmodules${MANEXT} checkin${MANEXT}

FILES=		Makefile ${MANSRCS}

all:

install:
	for i in ${MANSRCS}; do\
		install -m 644 $$i\
			${MANDIR}/`basename $$i ${MANEXT}`.${MANSEC};\
	done

clean:
	rm -f *.o core

list:
	@for i in ${FILES}; do\
		echo $$i;\
	done

depend:
