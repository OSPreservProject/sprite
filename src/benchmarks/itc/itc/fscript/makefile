#
# Set the BINDIR variable to contain the relative name of the
# directory containing all the compiler binaries.
#
BINDIR = ../../../bin

CC = $(BINDIR)/gcc
RANLIB = $(BINDIR)/sranlib
CFLAGS = -B$(BINDIR)/ -mspur -mlong-jumps -I./include -g -DDSHR ${XCFLAGS}
.SUFFIXES:
.SUFFIXES:	.out .o .tmp .c .e .r .f .y .l .s .p
HOST=father
AWK=awk
WM=wm
XLIBS=
WMOBJS=wm.o arbitrator.o  menu.o window.o mouse.o windowops.o towindow.o profile.o putenv.o

DSPOBJS = sun1bw.o sun1color.o font.o fontnaming.o
DSPLIB=libdsp.a

ULIBOBJS=usergraphics.o UserDrawS.o profile.o ProgramName.o fontmanip.o fontnaming.o font.o
ULIB=usergraphics.a

CFILES=wm.c arbitrator.c menu.c window.c mouse.c windowops.c towindow.c profile.c \
	rasterop.c vec.c gxfind.c font.c DrawString.c usergraphics.c UserDrawS.c \
	profile.c ProgramName.c	fontmanip.c cvtfont.c

all:	${WM} fontmanip.o usergraphics.a

${WM}:	${WMOBJS} ${DSPLIB}
#	rm -f ${WM}; ${CC} ${CFLAGS} ${WMOBJS} ${DSPLIB} /user/jag/libcmu/libcmu.a -o ${WM}
	rm -f ${WM}; ${BINDIR}/sld -r ${WMOBJS} ${DSPLIB} -o ${WM}


cvtfont:	cvtfont.o fontmanip.o font.o ${ULIB}
	${CC} ${CFLAGS} cvtfont.o fontmanip.o font.o ${ULIB} -o cvtfont

${ULIB}:	${ULIBOBJS}
	-ar uv ${ULIB} $?; ar d ${ULIB} __.SYMDEF; $(RANLIB) ${ULIB}

DrawString.o:	DrawString.c 
	${CC} ${CFLAGS} -O -c DrawString.c

${DSPLIB}:	$(DSPOBJS)
	-ar uv ${DSPLIB} $?; ar d ${DSPLIB} __.SYMDEF; $(RANLIB) ${DSPLIB}

install:	all
	install wm /usr/local/bin
	install -c usergraphics.h /usr/local/include
	install -c font.h /usr/local/include
	ar uv /usr/local/lib/libitc.a ${ULIBOBJS}
	$(RANLIB) /usr/local/lib/libitc.a

depend:
	grep '^#include' ${CFILES} | grep -v '<' | \
	      sed 's/:[^"]*"\([^"]*\)".*/:	\1/' | \
	      sed 's/\.c/.o/' | sed 's,../[a-z]*/,,' | \
	${AWK} ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp makefile makefile.bak
	ed - makefile < eddep
	rm eddep makedep

# stuff that isn't part of the distribution

cvtalto.o:	cvtalto.c font.h fontmanip.h
cvtalto:	cvtalto.o fontmanip.o font.o ${ULIB}
	${CC} ${CFLAGS} cvtalto.o fontmanip.o font.o ${ULIB} -o cvtalto
test.o:	usergraphics.h
test:	test.o ${ULIB}
	${CC} ${CFLAGS} test.o ${ULIB} -o test
logo:	logo.o ${DSPLIB}
	${CC} ${CFLAGS} logo.o ${DSPLIB} -o logo
mkfont:	mkfont.o ${DSPLIB}
	${CC} ${CFLAGS} mkfont.o -o mkfont ${DSPLIB}
mkfont.o:	font.h
# DO NOT DELETE THIS LINE

wm.o:	framebuf.h font.h menu.h window.h display.h
arbitrator.o:	window.h font.h menu.h usergraphics.h display.h keymap.h
menu.o:	font.h menu.h window.h display.h util.h
window.o:	window.h font.h menu.h display.h
mouse.o:	framebuf.h window.h menu.h display.h usergraphics.h
windowops.o:	menu.h window.h display.h
towindow.o:	window.h font.h menu.h usergraphics.h graphicops.c display.h
profile.o:	util.h
rasterop.o:	framebuf.h window.h
vec.o:	framebuf.h window.h
gxfind.o:	framebuf.h
font.o:	font.h window.h
DrawString.o:	font.h window.h framebuf.h
usergraphics.o:	usergraphics.h graphicops.c
UserDrawS.o:	font.h usergraphics.h
profile.o:	util.h
fontmanip.o:	font.h font.v0.h fontmanip.h
cvtfont.o:	font.h font.v0.h
