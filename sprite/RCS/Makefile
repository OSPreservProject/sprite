#
# $Header$ SPRITE (Berkeley)
#
# No makemake
#
LD = /sprite/bin/ld

#ifmake sun3 || profile3
MACHINE = sun3
#else
MACHINE = sun2
#endif

CLIB = /sprite/lib/libc.a
MODULE = sprite
SPRITEDIR =	..
USERH=		/sprite/lib/include
H =		${SPRITEDIR}/Include
INSTALLED =	{SPRITEDIR}/Installed/${MODULE}
LIB =		${SPRITEDIR}/Object/${MACHINE}
PROFD =	${SPRITEDIR}/Profiled/${MACHINE}
ODIR =		Object.${MACHINE}
IFLAGS=	-I. -I${H} -I${USERH}
#PROFILE =	-p -DPROFILE
PROFILE =
CFLAGS =	-m68010 -g ${PROFILE} ${IFLAGS}
LINTFLAGS =	-I. -I${H}
LINTLIBS =	${LINT}/*.ln

# symbol looked for by 'make nm'
NM = mcount

UPDATE = /sprite/bin/update
DEPEND = /sprite/bin/depend.csh
DEPENDSCRIPT = /sprite/lib/ex.depend
MKVERSION = /sprite/bin/mkversion

ENDOBJ=${LIB}/end.o

ALLSPICE = allspice.Berkeley.EDU
GINGER = ginger.Berkeley.EDU
ENVY = envy.Berkeley.EDU

#
#  Unprofiled object files
#

DBG = ../dbg/${ODIR}
DEV = ../dev/${ODIR}
FS = ../fs/${ODIR}
FSNEW = ../fs.new/${ODIR}
HOOK= mainHook.o
MAIN = ../main/${ODIR}
NET = ../net/${ODIR}
PROC = ../proc/${ODIR}
PROF = ../prof/${ODIR}
RECOV = ../recov/${ODIR}
RPC = ../rpc/${ODIR}
SCHED = ../sched/${ODIR}
SIG = ../sig/${ODIR}
SUN = ../sun/${ODIR}
SYNC = ../sync/${ODIR}
SYS = ../sys/${ODIR}
TIMER = ../timer/${ODIR}
UTILS = ../utils/${ODIR}
VM = ../vm/${ODIR}

OBJS = 	${LIB}/sun.o \
	${HOOK} \
	${LIB}/dbg.o \
	${LIB}/dev.o \
	${LIB}/fs.o \
	${LIB}/main.o \
	${LIB}/mem.o \
	${LIB}/net.o \
	${LIB}/proc.o \
	${LIB}/prof.o \
	${LIB}/recov.o \
	${LIB}/rpc.o \
	${LIB}/sched.o \
	${LIB}/sig.o \
	${LIB}/sync.o \
	${LIB}/sys.o \
	${LIB}/timer.o \
	${LIB}/utils.o \
	${LIB}/vm.o

#
# Profiled (perhaps) object files	####################################
#

PROFOBJS= ${LIB}/dbg.o	${PROFD}/fs.o	${LIB}/prof.o \
	${PROFD}/dev.o	${PROFD}/net.o	${PROFD}/sig.o \
	${PROFD}/main.o	${HOOK}	\
	${PROFD}/mem.o	${PROFD}/proc.o	${PROFD}/rpc.o \
	${PROFD}/sched.o \
	${PROFD}/timer.o ${PROFD}/sync.o	${PROFD}/utils.o \
	${PROFD}/vm.o \
	${PROFD}/sun.o	${PROFD}/sys.o
###############################################################################

ALLSRCS= Makefile version.c

HDRS = sprite.h

sun2: ${OBJS} ${ENDOBJ} Makefile
	rm -f $@ version.o
	@${MKVERSION} -p "SPRITE VERSION 1.0 (Sun2)" > version.h
	 cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T 804000 ${OBJS} version.o \
	    ${CLIB} ${ENDOBJ} -lg
	@ls -l $@
	@size $@
	cpstrip $@ /sprite/kernels/$@.sprite
	rcp /sprite/kernels/$@.sprite ${ENVY}:/bnf2/sprite/sun2
	nm $@ | sort > $@.nm

sun3: ${OBJS} ${ENDOBJ} Makefile
	rm -f $@
	@${MKVERSION} -p "SPRITE VERSION 1.0 (sun3)" > version.h
	@cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T f004000 ${OBJS} version.o \
	    ${CLIB} ${ENDOBJ} -lg
	@ls -l $@
	@size $@
	cpstrip $@ /sprite/kernels/$@.sprite
	rcp /sprite/kernels/$@.sprite allspice:/sprite/$@
	nm $@ | sort > $@.nm

profile2: ${PROFOBJS} ${ENDOBJ} Makefile
	rm -f $@
	@${MKVERSION} -p "SPRITE VERSION 1.0 (profile2)" > version.h
	@cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T 804000 ${PROFOBJS} version.o \
	    ${CLIB} ${ENDOBJ} -lg
	@ls -l $@
	@size $@
	cpstrip $@ /sprite/kernels/$@.sprite
	rcp /sprite/kernels/$@.sprite ${ENVY}:/bnf2/sprite/sun2
	nm $@ | sort > $@.nm

profile3: ${PROFOBJS} ${ENDOBJ} Makefile
	rm -f $@
	@${MKVERSION} -p "SPRITE VERSION 1.0 (profile3)" > version.h
	@cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T f004000 ${PROFOBJS} version.o \
	    ${CLIB} ${ENDOBJ} -lg
	@ls -l $@
	@size $@
	cpstrip $@ /sprite/kernels/$@.sprite
	rcp /sprite/kernels/$@.sprite allspice:/sprite/tmp/$@
	nm $@ | sort > $@.nm


nm: ${OBJS}
	./NM ${NM} ${OBJS}

clean:
	rm *.o sun2 sun3

# install: installhdrs /bnf/sprite/sprite

# installhdrs:
# 	@${UPDATE} ${HDRS} ${H}
# 	@${UPDATE} ${HDRS} ${INSTALLED}

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

All:
	cd ../dbg; make
	cd ../dev; make
	cd ../fs; make
	cd ../main; make
	cd ../mem; make
	cd ../net; make
	cd ../proc; make
	cd ../prof; make
	cd ../rpc; make
	cd ../sched; make
	cd ../sig; make
	cd ../sun; make
	cd ../sync; make
	cd ../sys; make
	cd ../timer; make
	cd ../utils; make
	cd ../vm; make

All-install:
	cd ../dbg; make install
	cd ../dev; make install
	cd ../fs; make install
	cd ../main; make install
	cd ../mem; make install
	cd ../net; make install
	cd ../proc; make install
	cd ../prof; make install
	cd ../rpc; make install
	cd ../sched; make install
	cd ../sig; make install
	cd ../sun; make install
	cd ../sync; make install
	cd ../sys; make install
	cd ../timer; make install
	cd ../utils; make install
	cd ../vm; make install

All-installhdrs:
	cd ../dbg; make installhdrs
	cd ../dev; make installhdrs
	cd ../fs; make installhdrs
	cd ../main; make installhdrs
	cd ../mem; make installhdrs
	cd ../net; make installhdrs
	cd ../proc; make installhdrs
	cd ../prof; make installhdrs
	cd ../rpc; make installhdrs
	cd ../sched; make installhdrs
	cd ../sig; make installhdrs
	cd ../sun; make installhdrs
	cd ../sync; make installhdrs
	cd ../sys; make installhdrs
	cd ../timer; make installhdrs
	cd ../utils; make installhdrs
	cd ../vm; make installhdrs


All-lint:
	cd ../dbg; make ../Lint/dbg.ln
	cd ../dev; make ../Lint/dev.ln
	cd ../fs; make ../Lint/fs.ln
	cd ../main; make ../Lint/main.ln
	cd ../mem; make ../Lint/mem.ln
	cd ../net; make ../Lint/net.ln
	cd ../proc; make ../Lint/proc.ln
	cd ../prof; make ../Lint/prof.ln
	cd ../rpc; make ../Lint/rpc.ln
	cd ../sched; make ../Lint/sched.ln
	cd ../sig; make ../Lint/sig.ln
	cd ../sun; make ../Lint/sun.ln
	cd ../sync; make ../Lint/sync.ln
	cd ../sys; make ../Lint/sys.ln
	cd ../timer; make ../Lint/timer.ln
	cd ../utils; make ../Lint/utils.ln
	cd ../vm; make ../Lint/vm.ln

All-depend:
	cd ../dbg; make depend
	cd ../dev; make depend
	cd ../fs; make depend
	cd ../main; make depend
	cd ../mem; make depend
	cd ../net; make depend
	cd ../proc; make depend
	cd ../prof; make depend
	cd ../rpc; make depend
	cd ../sched; make depend
	cd ../sig; make depend
	cd ../sun; make depend
	cd ../sync; make depend
	cd ../sys; make depend
	cd ../timer; make depend
	cd ../utils; make depend
	cd ../vm; make depend

rcsinfo:
	cd ../dbg; rcsinfo
	cd ../dev; rcsinfo
	cd ../fs; rcsinfo
	cd ../main; rcsinfo
	cd ../mem; rcsinfo
	cd ../net; rcsinfo
	cd ../proc; rcsinfo
	cd ../prof; rcsinfo
	cd ../rpc; rcsinfo
	cd ../sched; rcsinfo
	cd ../sig; rcsinfo
	cd ../sun; rcsinfo
	cd ../sync; rcsinfo
	cd ../sys; rcsinfo
	cd ../timer; rcsinfo
	cd ../utils; rcsinfo
	cd ../vm; rcsinfo

mainHook.o: ${H}/proc.h ${H}/fs.h ${H}/mem.h
# DO NOT DELETE THIS LINE -- make depend depends on it.
