/* 
 * setjmp.s --
 *
 *	setjmp/longjmp routines for SUN4.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * static char rcsid[] = "$Header: machCCRegs.s,v 1.1 88/06/15 14:18:30 mendel E
xp $ SPRITE (Berkeley)";
 *
 */

 /*
  * Define offsets in the jmp_buf block.
  */

#define	SIGMASK_OFFSET	0
#define	RTNPC_OFFSET	4
#define	SP_OFFSET	8
/* 
 *----------------------------------------------------------------------
 *
 * setjmp/_setjmp --
 *
 *	setjmp and _setjmp routines for SUN4.
 *
 * Results:
 *	An integer 0.
 *
 * Side effects:
 *	None.
 *
 * Calling Sequence:
 *	int val = setjmp(env) or val = _setjmp(env)
 *	jmp_buf		env;
 *
 *----------------------------------------------------------------------
 * 
 */

.text
	.align 2
.globl _setjmp
.globl	__setjmp
_setjmp:
	save %sp, -104, %sp
	/*
	 * First call sigblock(0) to get the current signal mask and 
	 * save it in the jmp_env block.
	 */
	call	 _sigblock,1
	mov	0,%o0
	mov	%o0,%g1
	restore
	st      %g1, [%o0 + SIGMASK_OFFSET]
	/*
	 * _setjmp doesn't need the sigmask so it can start here.
	 */
__setjmp:
	/*
	 * Save our save pointer and return address in the jmp_buf for
	 * use by longjmp.
	 */
	st      %sp, [%o0 + SP_OFFSET]
	st      %o7, [%o0 + RTNPC_OFFSET]
	/*
	 * Return a 0 like a good setjmp should. 
	 */
	retl
	mov	0,%o0
/* 
 *----------------------------------------------------------------------
 *
 * longjmp/_longjmp --
 *
 *	longjmp and _longjmp routines for SUN4.
 *
 * Results:
 *	Doesn't return normally.
 *
 * Side effects:
 *	Returns to the specified setjmp/_setjmp call.
 * 	longjmp restores the signal mask.
 *
 * Calling Sequence:
 *	longjmp(env,val) or  _setjmp(env,val)
 *	jmp_buf		env;
 *	int	val;
 *
 *----------------------------------------------------------------------
 * 
 */



	.align 2
.globl _longjmp
.globl	__longjmp
_longjmp:
        save    %sp,-96,%sp
	/*
	 * Restore the signal mask to the saved value.
	 */
	call 	_sigsetmask,1
        ld      [%i0+SIGMASK_OFFSET],%o0
	restore
	/*
	 * _longjump doesn't restore the sigmask so it can start here.
	 */
__longjmp:
	/*
	 * Write out the register windows to memory.
	 */
	ta	5
	/*
	 * Fake togther a call frame that can "return" in to the
	 * setjmp call.
	 */
	ld	[%o0 + SP_OFFSET], %fp
	sub     %fp, 64, %sp
	ld	[%o0 + RTNPC_OFFSET], %o7
	mov     %o1,%i0
	retl
	restore
