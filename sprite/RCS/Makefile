#
# $Header$ SPRITE (Berkeley)
#
# No mkmf
#
LD = /sprite/cmds.${MACHINE}/ld

#ifmake sun3 || profile3
TM = sun3
MTFLAGS = -msun3
#endif
#ifmake sun2 
TM = sun2
MTFLAGS = -msun2
#endif
#ifmake ds3100
TM = ds3100
MTFLAGS =
#endif
#ifmake sun4
TM = sun4
MTFLAGS = -msun4
#endif
#ifmake sun4c
TM = sun4c
MTFLAGS = -msun4
#endif

LIBC = -lnet -lc

MODULE = sprite
SPRITEDIR =	/sprite/src/kernel
USERH=		/sprite/lib/include
H =		${SPRITEDIR}/Include
INSTALLED =	{SPRITEDIR}/Installed/${MODULE}
LIB =		${SPRITEDIR}/${TM}.md
PROFD =	${SPRITEDIR}/Profiled/${TM}.md
ODIR =		${TM}.md
IFLAGS=	-I. -I${H} -I${USERH} -I${USERH}/${TM}.md -I${H}/${TM}.md
#PROFILE =	-p -DPROFILE
PROFILE =
CFLAGS =	${MTFLAGS} -g -DKERNEL ${PROFILE} ${IFLAGS}
LINTFLAGS =	-I. -I${H}
LINTLIBS =	${LINT}/*.ln

# symbol looked for by 'make nm'
NM = mcount

UPDATE = /sprite/cmds.${MACHINE}/update
MKVERSION = /sprite/cmds.${MACHINE}/mkversion
RELEASE = 1
NEWVERSION ?= TRUE

GINGER = ginger.Berkeley.EDU
ENVY = envy.Berkeley.EDU

#
#  Unprofiled object files
#

#
# For tracing undefined symbols
TRACE =

#
#  Uninstalled object files
#
TOP = ${SPRITEDIR}

MACH = ${TOP}/mach/${ODIR}
DBG = ${TOP}/dbg/${ODIR}
DEV = ${TOP}/dev/${ODIR}
FS = ${TOP}/fs/${ODIR}
FSNEW = ${TOP}/fs.new/${ODIR}
HOOK= mainHook.o
MAIN = ${TOP}/main/${ODIR}
NET = ${TOP}/net/${ODIR}
PROC = ${TOP}/proc/${ODIR}
PROF = ${TOP}/prof/${ODIR}
RAID = ${TOP}/raid.null/${ODIR}
RECOV = ${TOP}/recov/${ODIR}
RPC_O = ${TOP}/rpc/${ODIR}/rpc.o
RPC = ${TOP}/rpc/${ODIR}
SCHED = ${TOP}/sched/${ODIR}
SIG = ${TOP}/sig/${ODIR}
SYNC = ${TOP}/sync/${ODIR}
SYS = ${TOP}/sys/${ODIR}
TIMER = ${TOP}/timer/${ODIR}
UTILS = ${TOP}/utils/${ODIR}
VM = ${TOP}/vm/${ODIR}
MEM = ${TOP}/mem/${ODIR}


OBJS = 	${LIB}/mach.o \
	${HOOK} \
	${LIB}/dbg.o \
	${LIB}/dev.o \
	${LIB}/fs.o \
	${LIB}/main.o \
	${LIB}/mem.o \
	${LIB}/net.o \
	${LIB}/proc.o \
	${LIB}/prof.o \
	${LIB}/raid.null.o \
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

.BEGIN: 
	doversion ${RELEASE} ${NEWVERSION} > version.h.template

#sun2: ${OBJS} Makefile .PRECIOUS .NOEXPORT
#	rm -f $@ version.o
#	rm -f mainHook.o
#	cc -c ${CFLAGS} mainHook.c
#	${MKVERSION} -p "SPRITE VERSION 1.0 (Sun2)" > version.h
#	cc -c ${CFLAGS} version.c
#	${LD} -X -o $@ -e start -T 804000 ${OBJS} version.o \
#	    -L/sprite/lib/${TM}.md ${LIBOLD} ${LIBC}
#	@ls -l $@
#	@size $@
#	update -f -s $@ /sprite/boot/$@.sprite
#	rcp /sprite/boot/$@.sprite ${ENVY}:/bnf2/sprite/sun2.new
#	nm -n $@  > $@.nm

sun3: ${OBJS} Makefile .PRECIOUS .NOEXPORT
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	-Save $@
	rm -f mainHook.o
	cc -c ${CFLAGS} mainHook.c
	cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T e004000 ${OBJS} version.o \
	    -L/sprite/lib/${TM}.md ${LIBOLD} ${LIBC}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.md/sprite.new
	cp /sprite/boot/$@.md/sprite.new /vmsprite.new
	rcp /sprite/boot/$@.md/sprite.new ${GINGER}:/sprite3/$@.new
	rsh rosemary "(cd /tmp/sprite ; ./Save sun3.sprite)"
	rcp $@ rosemary:/tmp/sprite/$@.sprite
	nm -n  > $@.nm

sun4: ${OBJS} Makefile .PRECIOUS .NOEXPORT
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	-Save $@
	rm -f mainHook.o
	cc -c ${CFLAGS} -Dsun4 mainHook.c
	cc -c ${CFLAGS} -Dsun4 version.c
	${LD} -X -msun4 -o $@ -e start -T f6004020 ${OBJS} version.o \
	    -L/sprite/lib/${TM}.md ${LIBC}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.md/sprite.new
	nm -n $@ > $@.nm

sun4c: ${OBJS} Makefile .PRECIOUS .NOEXPORT
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	-Save $@
	rm -f mainHook.o
	cc -c ${CFLAGS} -Dsun4 -Dsun4c mainHook.c
	cc -c ${CFLAGS} -Dsun4 -Dsun4c version.c
	${LD} -X -msun4 -o $@ -e start -T f6004000 ${OBJS} version.o \
	    -L/sprite/lib/${TM}.md ${LIBC}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.md/sprite.new
	nm -n $@ > $@.nm

profile2: ${PROFOBJS} Makefile .PRECIOUS
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	rm -f $@
	rm -f mainHook.o
	cc -c ${CFLAGS} mainHook.c
	@cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T 804000 ${PROFOBJS} version.o \
	    -L/sprite/lib/${TM}.md ${LIBOLD} ${LIBC}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.sprite
	rcp /sprite/boot/$@.sprite ${ENVY}:/bnf2/sprite/sun2
	nm -n $@ > $@.nm

profile3: ${PROFOBJS} Makefile .PRECIOUS
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	rm -f $@
	rm -f mainHook.o
	cc -c ${CFLAGS} mainHook.c
	@cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T e004000 ${PROFOBJS} version.o \
	    -L/sprite/lib/${TM}.md ${LIBOLD} ${LIBC}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.sprite
	rcp /sprite/boot/$@.sprite ${GINGER}:/sprite3/tmp/$@
	nm -n $@ > $@.nm

ds3100: ${OBJS} .PRECIOUS
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	rm -f $@
	rm -f mainHook.o
	cc -c ${CFLAGS} mainHook.c
	@cc -c ${CFLAGS} version.c
	${LD} -N -o ds3100 -e start -T 80030000 ${OBJS} version.o \
		-L/sprite/lib/${TM}.md ${LIBC}
	@ls -l $@
	@size $@
	update -f $@ /sprite/boot/$@.md/$@.new
	strip /sprite/boot/$@.md/$@.new
	rm -f $@.nm
	nm -n $@ > $@.nm

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

mainHook.o: ${H}/proc.h ${H}/fs.h
# DO NOT DELETE THIS LINE -- make depend depends on it.
