/*
 * test.s --
 *
 *	Program for testing floating point instruction problems from user mode.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

.seg	"data"
.asciz	"$Header: /sprite/lib/forms/sun4.md/RCS/proto.s,v 1.1 89/01/16 20:38:17 mgbaker Exp Locker: mgbaker $ SPRITE (Berkeley)"
.align	8
.seg	"text"

/*
 * ----------------------------------------------------------------------
 *
 * TestFp --
 *
 *	Test whatever floating point instructions from user mode.  The
 *	contents of this routine depend upon whatever I'm testing at the
 *	moment.  This is a temporary tester for the sun4 port.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_TestFp
_TestFp:
	set	_testValue, %o0
	ld	[%o0], %f0
	retl
	nop
