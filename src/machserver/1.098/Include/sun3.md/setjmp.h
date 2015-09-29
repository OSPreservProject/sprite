/*	setjmp.h	4.1	83/05/03	*/
/* $Header$ */

#include <cfuncproto.h>


/*
 * Only 10 words are needed for the VAX, but 15 for the Sun.  Use the
 * larger of the two.
 */

typedef int jmp_buf[15];

extern int setjmp _ARGS_((jmp_buf));
extern void longjmp _ARGS_((jmp_buf, int));
