#
# $Header$ SPRITE (Berkeley)
#
# No mkmf
#
LD = /sprite/cmds.${MACHINE}/ld

# don't use TM, because that is set explicitly by the environment in some cases
# and then we can't override it.  But set it by default to the current machine
# type.

TM          ?= $(MACHINE)

#if make(sun3) || make(profile3)
RTM = sun3
MTFLAGS = -msun3
#endif
#ifmake ds3100
RTM = ds3100
MTFLAGS =
#endif
#ifmake sun4
RTM = sun4
MTFLAGS = -msun4
#endif
#ifmake sun4c
RTM = sun4c
MTFLAGS = -msun4
#endif

#if !empty(TM)
RTM ?= $(TM)
#else
RTM ?= $(MACHINE)
#endif

LIBNET = -lnet

MODULE = sprite
SPRITEDIR =	/sprite/src/kernel
USERH=		/sprite/lib/include
H =		${SPRITEDIR}/Include
INSTALLED =	{SPRITEDIR}/Installed/${MODULE}
LIB =		${SPRITEDIR}/${RTM}.md
PROFD =	${SPRITEDIR}/Profiled/${RTM}.md
ODIR =		${RTM}.md
IFLAGS=	-I. -I${H} -I${USERH} -I${USERH}/${RTM}.md -I${H}/${RTM}.md
#PROFILE =	-p -DPROFILE
PROFILE =
CFLAGS =	${MTFLAGS} -g -DKERNEL ${PROFILE} ${IFLAGS}
LINTFLAGS =	-I. -I${H}
LINTLIBS =	${LINT}/*.ln
DISTDIR     ?= /dist/sprite/src/kernel/sprite

# symbol looked for by 'make nm'
NM = mcount

UPDATE = /sprite/cmds.${MACHINE}/update
TEST = /sprite/cmds.${MACHINE}/test
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
RAIDNULL = ${TOP}/raid/${ODIR}
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
LIBC = ${TOP}/libc/${ODIR}

# This is the list of objects excluding raid.

OBJS1 =	${LIB}/mach.o \
	${HOOK} \
	${LIB}/dbg.o \
	${LIB}/dev.o \
	${LIB}/fs.o \
        ${LIB}/fsdm.o  \
        ${LIB}/fscache.o  \
        ${LIB}/fsconsist.o \
        ${LIB}/fsio.o \
        ${LIB}/fslcl.o \
        ${LIB}/fspdev.o \
        ${LIB}/fsprefix.o \
        ${LIB}/fsrmt.o \
        ${LIB}/fsutil.o \
	${LIB}/libc.o \
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

# sun4 should use the raid module.  Others should use raid.null.

OBJS =	${OBJS1} \
	${LIB}/raid.null.o

OBJS_SUN4 =	${OBJS1} \
	${LIB}/raid.o

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
	${PROFD}/sun.o	${PROFD}/sys.o ${PROFD}/libc

###############################################################################

ALLSRCS= Makefile version.c

HDRS = sprite.h

DISTFILES ?= Save doversion mainHook.c porting_comments version.h.template

dist:
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in $(ALLSRCS) $(HDRS) $(DISTFILES)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i} ;else true; fi
	done
#endif

.BEGIN: 
	doversion ${RELEASE} ${NEWVERSION} version.h.template

#sun2: ${OBJS} Makefile .PRECIOUS .NOEXPORT
#	rm -f $@ version.o
#	rm -f mainHook.o
#	cc -c ${CFLAGS} mainHook.c
#	${MKVERSION} -p "SPRITE VERSION 1.0 (Sun2)" > version.h
#	cc -c ${CFLAGS} version.c
#	${LD} -X -o $@ -e start -T 804000 ${OBJS} version.o \
#	    -L/sprite/lib/${RTM}.md ${LIBOLD} ${LIBNET}
#	@ls -l $@
#	@size $@
#	update -f -s $@ /sprite/boot/$@.sprite
#	rcp /sprite/boot/$@.sprite ${ENVY}:/bnf2/sprite/sun2.new
#	nm -n $@  > $@.nm

default: ${MACHINE}

sun3: ${OBJS} Makefile .PRECIOUS .NOEXPORT
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	-Save $@
	rm -f mainHook.o
	cc -c ${CFLAGS} mainHook.c
	cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T e004000 ${OBJS} version.o \
	    -L/sprite/lib/${RTM}.md ${LIBOLD} ${LIBNET}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.md/new
	rcp /sprite/boot/$@.md/new ${GINGER}:/sprite3/$@.new
	cp $@ /rosemary/tmp/sprite/$@.sprite
	rsh rosemary -n "(cd /tmp/sprite ; ./Save sun3.sprite)"
	nm -n $@ > $@.nm
	-Save $@

sun4: ${OBJS_SUN4} Makefile .PRECIOUS .NOEXPORT
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	-Save $@
	rm -f mainHook.o
	cc -c ${CFLAGS} -Dsun4 mainHook.c
	cc -c ${CFLAGS} -Dsun4 version.c
	${LD} -X -msun4 -o $@ -e start -T f6004020 ${OBJS_SUN4} version.o \
	    -L/sprite/lib/${RTM}.md ${LIBNET}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.md/new
	cp $@ /rosemary/tmp/sprite/$@.sprite
	rsh rosemary -n "(cd /tmp/sprite ; ./Save sun4.sprite)"
	nm -n $@ > $@.nm
	-Save $@

sun4c: ${OBJS} Makefile .PRECIOUS .NOEXPORT
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	-Save $@
	rm -f mainHook.o
	cc -c ${CFLAGS} -Dsun4 -Dsun4c mainHook.c
	cc -c ${CFLAGS} -Dsun4 -Dsun4c version.c
	${LD} -X -msun4 -o $@ -e start -T f6004000 ${OBJS} version.o \
	    -L/sprite/lib/sun4.md ${LIBNET}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/$@.md/new
	cp $@ /rosemary/tmp/sprite/$@.sprite
	rsh rosemary -n "(cd /tmp/sprite ; ./Save sun4c.sprite)"
	nm -n $@ > $@.nm
	-Save $@

ds3100: ${OBJS} .PRECIOUS .NOEXPORT
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	-Save $@
	rm -f mainHook.o
	cc -c ${CFLAGS} mainHook.c
	@cc -c ${CFLAGS} version.c
	${LD} -N -o ds3100 -e start -T 80030000 ${OBJS} version.o \
		-L/sprite/lib/${RTM}.md ${LIBNET}
	@ls -l $@
	@size $@
	update -f $@ /sprite/boot/$@.md/new
	strip /sprite/boot/$@.md/new
	rm -f $@.nm
	nm -n $@ > $@.nm
	rsh thyme -n "rcp /sprite/src/kernel/sprite/$@ dill:/sprite/src/kernel/nelson/$@"
	rsh dill -n "(cd /sprite/src/kernel/nelson ; Save $@)"
	-Save $@

profile2: ${PROFOBJS} Makefile .PRECIOUS
	sed 's/MACHINE/$@/g' version.h.template > version.h
	cat version.h >> $@.versionLog
	rm -f $@
	rm -f mainHook.o
	cc -c ${CFLAGS} mainHook.c
	@cc -c ${CFLAGS} version.c
	${LD} -X -o $@ -e start -T 804000 ${PROFOBJS} version.o \
	    -L/sprite/lib/${RTM}.md ${LIBOLD} ${LIBNET}
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
	    -L/sprite/lib/${RTM}.md ${LIBOLD} ${LIBNET}
	@ls -l $@
	@size $@
	update -f -s $@ /sprite/boot/sun3.md/prof
	rcp /sprite/boot/sun3.md/prof ${GINGER}:/sprite3/tmp/$@
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

ALL_MODULES = \
dbg \
dev \
fs \
fscache \
fsconsist \
fsdm \
fsio \
fslcl \
fspdev \
fsprefix \
fsrmt \
libc \
main \
mem \
net \
proc \
prof \
rpc \
sched \
sig \
sync \
sys \
timer \
utils \
vm
#fsutil
#recov

All:
	for i in ${ALL_MODULES} ; do
	    cd ../$i ; make
	done

All-install:
	for i in ${ALL_MODULES} ; do
	    cd ../$i ; make install
	done

All-installhdrs:
	for i in ${ALL_MODULES} ; do
	    cd ../$i ; make installhdrs
	done

All-lint:
	for i in ${ALL_MODULES} ; do
	    cd ../$i ; make ../Lint/$i.ln
	done

All-depend:
	for i in ${ALL_MODULES} ; do
	    cd ../$i ; make depend
	done

rcsinfo:
	for i in ${ALL_MODULES} ; do
	    cd ../$i ;  rcsinfo
	done

mainHook.o: ${H}/proc.h ${H}/fs.h
# DO NOT DELETE THIS LINE -- make depend depends on it.
