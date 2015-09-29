#ifndef lint
static char sccsid[] = "@(#)setbrk.c	4.2 8/11/83";
#endif

#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"

setbrk(incr)
{
	REG BYTPTR	a=sbrk(incr);
	if (a == -1) {
	    panic("out of memory\n");
	}
	brkend=a+incr;
	return(a);
}
