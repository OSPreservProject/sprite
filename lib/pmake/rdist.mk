# For rdist, take the standard makefile and pass in the directory to rdist
# and the name of the binary/binaries not to rdist.
# Allow the user to specify extra flags (like -v) for the rdist via
# the DISTFLAGS variable.

DISTFILE 	= /sprite/lib/misc/distfile.command
DISTFLAGS 	+= 
#ifndef DIST_EXCEPT
# ifdef PROG
 DIST_EXCEPT 	= -d EXCEPT=\(${PROG}\)
# elifdef PROGRAM
 DIST_EXCEPT	= -d EXCEPT=\(${PROGRAM}\)
# else
 DIST_EXCEPT 	=
# endif
#endif

Rdist		:: .PRECIOUS .NOTMAIN
	rdist ${DISTFLAGS} -f ${DISTFILE} -d DIR=`pwd` ${DIST_EXCEPT}

