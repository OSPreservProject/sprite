#
#	$Id: Makefile,v 1.1 89/11/19 23:15:10 berliner Exp $
#
# Master Makefile for CVS
#

DESTDIR=	/usr/local/bin
CFLAGS=		-O # -g
LDFLAGS=	# -Bstatic
SUBDIRS=	doc examples man src

FILES=		README Makefile COPYING
SHAR=		shar

all:
	@for i in ${SUBDIRS}; do\
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR="${DESTDIR}"\
			CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}");\
	done

install:
	@for i in ${SUBDIRS}; do\
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR="${DESTDIR}"\
			CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" install);\
	done

clean:
	@for i in ${SUBDIRS}; do\
		echo $$i:;\
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR="${DESTDIR}"\
			CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" clean);\
	done

shar:
	@rm -f ${SHAR}.[0-9]*
	makekit -p -s45k -n${SHAR}. `make -s list`

list:
	@for i in ${FILES}; do\
		echo $$i;\
	done
	@for i in ${SUBDIRS}; do\
		echo $$i;\
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR="${DESTDIR}"\
			CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" list) |\
		sed -e "s1^1$$i/1";\
	done

depend:
	@for i in ${SUBDIRS}; do\
		(cd $$i; ${MAKE} ${MFLAGS} DESTDIR="${DESTDIR}"\
			CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" depend);\
	done
