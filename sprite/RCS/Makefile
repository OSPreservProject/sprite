# 
# $Header$ SPRITE (Berkeley)
#
ROOT = /sprite
SPRITESRC = ..
SPRITEHOME = /sprite2/users/sprite
H =       ${SPRITESRC}/Include
LIB =     ${SPRITESRC}/Object
LINT =    ${SPRITESRC}/Lint
USERLINT =${ROOT}/lib/lint
USERINCLUDE=${ROOT}/lib/include
INSTALLED=${SPRITESRC}/Installed/${MODULE}
PROFDIR=  ${SPRITESRC}/Profiled
ALL=      ${SPRITESRC}/All
IFLAGS =  -I. -I${H} -I${USERINCLUDE}
DBX =     -g
PROFILE = -p -DPROFILE
DFLAGS =
CFLAGS =  ${DBX} ${IFLAGS} ${DFLAGS}
PCFLAGS = ${DBX} ${IFLAGS} ${DFLAGS} ${PROFILE}
CTFLAGS = -t
PPFLAGS = -E ${IFLAGS} ${DFLAGS}
CC = cc -m68010
AS = as -m68010
LD = ld

UPDATE = /usr/local/update
DEPEND = ${SPRITEHOME}/bin/depend.csh
DEPENDSCRIPT = ${SPRITEHOME}/lib/ex.depend
MKVERSION = ${SPRITEHOME}/bin/mkversion
LIBC = /sprite/lib/libc.a /sprite/lib/crt.a
SPECIAL=${LIB}/bootSys.o
ENDOBJ=${LIB}/end.o

OBJS= 	${LIB}/crt.o \
	${LIB}/dbg.o \
	${LIB}/dev.o \
	${LIB}/fs.o \
	${LIB}/main.o \
	mainHook.o \
	${LIB}/mem.o \
	${LIB}/net.o \
	${LIB}/proc.o \
	${LIB}/prof.o \
	${LIB}/rpc.o \
	${LIB}/sched.o \
	${LIB}/sig.o \
	${LIB}/sun.o \
	${LIB}/sys.o \
	${LIB}/sync.o \
	${LIB}/timer.o \
	${LIB}/utils.o \
	${LIB}/vm.o \
	${LIB}/win.o

ALLSRCS= Makefile version.c

HDRS = sprite.h

sprite: ${SPECIAL} ${OBJS} ${ENDOBJ}
	rm -f $@ version.o
	@${MKVERSION} > version.h
	${CC} -c ${CFLAGS} version.c
	${LD} -X -o sprite -e start -T a04000 ${SPECIAL} version.o ${OBJS} \
	    ${LIBC} ${ENDOBJ} -lg
	modAout sprite 4000
	cp sprite sprite.strip
	strip sprite.strip
	rcp sprite.strip allspice:/pub.MC68010/sprite/sprite
	rcp sprite.strip envy:/pub.MC68010/sprite/sprite
	rcp sprite.strip ginger:/pub.MC68010/sprite/sprite
	rm sprite.strip

install: sprite installhdrs

installhdrs:
	@${UPDATE} ${HDRS} ${H}
	@${UPDATE} ${HDRS} ${INSTALLED}

/pub/sprite/sprite: sprite
	rm -f /pub/sprite/sprite
	rcp sprite envy:/pub/sprite/sprite

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
	cd ../win; make 

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
	cd ../win; make install

All-all-install:
	cd ../dbg; make all-install
	cd ../dev; make all-install
	cd ../fs; make all-install
	cd ../main; make all-install
	cd ../mem; make all-install
	cd ../net; make all-install
	cd ../proc; make all-install
	cd ../prof; make all-install
	cd ../rpc; make all-install
	cd ../sched; make all-install
	cd ../sig; make all-install
	cd ../sun; make all-install
	cd ../sync; make all-install
	cd ../sys; make all-install
	cd ../timer; make all-install
	cd ../utils; make all-install
	cd ../vm; make all-install
	cd ../win; make all-install

#:.,.+15s,^\(	cd ../\)\([^;]*\); make install,\1\2; make ../Lint/\2.ln,

All-lint:
	cd ../dbg; make ../Lint/dbg.ln
	cd ../dev; make ../Lint/dev.ln
	cd ../fs; make ../Lint/fs.ln
	cd ../main; make ../Lint/main.ln
	cd ../mem; make ../Lint/mem.ln
	cd ../net; make ../Lint/net.ln
	cd ../proc; make ../Lint/sched.ln
	cd ../prof; make ../Lint/sched.ln
	cd ../rpc; make ../Lint/sched.ln
	cd ../sched; make ../Lint/sched.ln
	cd ../sig; make ../Lint/sched.ln
	cd ../sun; make ../Lint/sun.ln
	cd ../sync; make ../Lint/sync.ln
	cd ../sys; make ../Lint/rpc.ln
	cd ../timer; make ../Lint/timer.ln
	cd ../utils; make ../Lint/utils.ln
	cd ../vm; make ../Lint/vm.ln
	cd ../win; make ../Lint/win.ln

All: All-install sprite

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
	cd ../win; make depend

All-makemake:
	cd ../dbg; makemake
#	cd ../dev; makemake
#	cd ../fs; makemake
	cd ../main; makemake
	cd ../mem; makemake
	cd ../net; makemake
	cd ../proc; makemake
	cd ../prof; makemake
	cd ../rpc; makemake
	cd ../sched; makemake
	cd ../sig; makemake
	cd ../sun; makemake
	cd ../sync; makemake
	cd ../sys; makemake
	cd ../timer; makemake
	cd ../utils; makemake
	cd ../vm; makemake
	cd ../win; makemake
