#
# This Makefile is for use when distributing Tcl to the outside world.
# It is normally set up by running the "config" script.  Before modifying
# this file by hand, you should read through the "config" script to see
# what it does.
#
# Some changes you may wish to make here:
#
# 1. To compile for non-UNIX systems (so that only the non-UNIX-specific
# commands are available), change the OBJS line below so it doesn't
# include ${UNIX_OBJS}.  Also, add the switch "-DTCL_GENERIC_ONLY" to
# CFLAGS.  Lastly, you'll have to provide your own replacement for the
# "panic" procedure (see panic.c for what the current one does).
#
# 2. ANSI-C procedure prototypes are turned on by default if supported
# by the compiler.  To turn them off, add "-DNO_PROTOTYPE" to CFLAGS
# below.
#
# 3. If you've put the Tcl script library in a non-standard place, change
# the definition of TCL_LIBRARY to correspond to its location on your
# system.
#

TCL_LIBRARY =	/usr/local/lib/tcl

CC =		cc
CFLAGS =	-g -I. -DTCL_LIBRARY=\"${TCL_LIBRARY}\"

GENERIC_OBJS =	regexp.o tclAssem.o tclBasic.o tclCkalloc.o \
	tclCmdAH.o tclCmdIL.o tclCmdMZ.o tclExpr.o tclGet.o \
	tclHash.o tclHistory.o tclParse.o tclProc.o tclUtil.o \
	tclVar.o

UNIX_OBJS = panic.o tclEnv.o tclGlob.o tclUnixAZ.o tclUnixStr.o \
	tclUnixUtil.o 

COMPAT_OBJS =

OBJS = ${GENERIC_OBJS} ${UNIX_OBJS} ${COMPAT_OBJS}

libtcl.a: ${OBJS}
	rm -f libtcl.a
	ar cr libtcl.a ${OBJS}
	ranlib libtcl.a

tclTest: tclTest.o libtcl.a
	${CC} ${CFLAGS} tclTest.o libtcl.a -o tclTest

test: tclTest
	( echo cd tests ; echo source all ) | ./tclTest

clean:
	rm -f ${OBJS} libtcl.a tclTest.o tclTest

# The following target is used during configuration to compile
# a test program to see if certain facilities are available on
# the system.

configtest:
	${CC} ${CFLAGS} test.c

${OBJS}: tcl.h tclHash.h tclInt.h
${UNIX_OBJS}: tclUnix.h
