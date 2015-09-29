/*	setjmp.h	4.1	83/05/03	*/
/* $Header: /sprite/src/lib/include/sun3.md/RCS/setjmp.h,v 1.3 91/06/11 10:43:00 eklee Exp $ */

#ifndef _SETJMP
#define _SETJMP

#include <cfuncproto.h>


/*
 * Only 10 words are needed for the VAX, but 15 for the Sun.  Use the
 * larger of the two.
 */

typedef int jmp_buf[15];

extern int setjmp _ARGS_((jmp_buf));
extern void longjmp _ARGS_((jmp_buf, int));

#endif /* _SETJMP */
