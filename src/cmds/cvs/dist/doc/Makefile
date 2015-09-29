#
#	$Id: Makefile,v 1.1 89/11/19 23:16:41 berliner Exp $
#
# Makefile for CVS document for the Winter 1990 USENIX Conference,
# Washington D.C.
#

TROFF=		nitroff
TROFFOPTS=	-ms
PIC=		pic
TBL=		tbl

CVSDOC=		cvs.ms
FILES=		Makefile ${CVSDOC}

all:

print:
	${PIC} ${CVSDOC} | ${TBL} | ${TROFF} ${TROFFOPTS}

install:

clean:
	rm -f *.o core

list:
	@for i in ${FILES}; do\
		echo $$i;\
	done

depend:
