/* memAsm.s --
 *
 *	Routine to return callers PC.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcs = $Header$ SPRITE (Berkeley)
 */

    .globl _Mem_CallerPC
_Mem_CallerPC:
    /*
     * Put our return address into r12 so that we can to it as r28 after we
     * return.  We can't leave it in r10 because r26 can get trashed when
     * an interrupt comes in after we do the return.
     */
    add_nt	r12, r10, $0
    rd_special	r16, pc
    return	r16, $12
    nop
    add_nt	r27, r10, $0
    jump_reg	r28, $8
    nop
