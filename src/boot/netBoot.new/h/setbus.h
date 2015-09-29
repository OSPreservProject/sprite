
/*	@(#)setbus.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Declarations of bus-error-recovery structures.
 *
 * In your function, declare:
 *	foo()
 *	{
 *		auto bus_buf busbuf;
 *
 * Then call setbus with the buffer as argument.  Setbus will return
 * 0 the first time you call, and will return 1 later (from the same call)
 * if a bus error ever occurs before you call unsetbus().
 *
 * Note that, like setjmp, setbus trashes register variables in your
 * routine.
 */
struct busbuf {
	long	misc[3];
	long	regs[16];
};

typedef struct busbuf bus_buf[1];

int setbus();
void unsetbus();
