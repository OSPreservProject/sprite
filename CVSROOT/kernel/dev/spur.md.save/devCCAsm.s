/* DevCCAsm.s --
 *
 * 	Routines for reading and writing bytes to and from the registers 
 *	on the Spur's Cache controller chip. 
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
 *
 * rcs = $Header$ SPRITE (Berkeley)
 *
 *
 * The routines in this file are callable from C. The C definitions are:
 *
 * extern void DevCCReadBytesFromRegs(startOffset, bytesToRead, bufPtr) 
 *		unsigned int startOffset;	 Starting address of read.
 *		int	bytesToRead;		 Count of bytes to read.
 *		char	*bufPtr;		 Buffer to read to. 
 * extern void DevCCWriteBytesFromRegs(startOffset, bufSize, bufPtr)
 *		unsigned int startOffset;	 Starting address of write. 
 *		int	bytesToRead;		 Count of bytes to write. 
 *		char	*bufPtr;		 Data to write 
 *
 */

#include "machConst.h"
#include "machConst.h"

/*----------------------------------------------------------------------
 * _DevCCReadBytesFromRegs --
 *
 *	Read the specified byte range from the SPUR Cache Controller registers.
 *
 *	Results:
 *		None 
 *
 *	Side effects:
 *		Node.
 *
 *----------------------------------------------------------------------
 */
	.text
	.align 2
	.globl _DevCCReadBytesFromRegs


_DevCCReadBytesFromRegs:
	/*
	 * Disable all interrupts.
	 */
        rd_kpsw         r17
        and             r18, r17, $~(MACH_KPSW_INTR_TRAP_ENA);
        wr_kpsw         r18, $0
	/*
	 * Convert byte offset into a register address. 
	 */
	sll		r11,r11,$3
	sll		r11,r11,$2
	/*
	 * Convert the number bytes into register address range.
	 */
	sll		r12,r12,$3
	sll		r12,r12,$2
	/*
	 * Compute register offset to stop on.
	 */
	add_nt		r14, r11, r12
	cmp_br_delayed	uge, r11, r14, 2f
1:	
	/*
	 * Fetch byte from CC reg.
	 */
	ld_external	r15, r11, $RDREG
	/*
	 * Insert it in the buffer.
	 */
	ld_32		r16, r13, $0
	wr_insert	r13
	insert		r16, r16, r15
	st_32		r16, r13, $0
	/*
	 * Increment the buffer pointer and the register offset.
	 */
	add_nt		r13, r13, $1
	add_nt		r11, r11, $(1 << 5)
	cmp_br_delayed  lt,r11,r14,1b
	Nop
2:
	wr_kpsw		r17, $0
        return		r10,$8
	nop
/*----------------------------------------------------------------------
 * _DevCCReadWriteFromRegs --
 *
 *	Write the specified byte range to the SPUR Cache Controller registers.
 *
 *	Results:
 *		None 
 *
 *	Side effects:
 *		Node.
 *
 *----------------------------------------------------------------------
 */
	.text
	.align 2
	.globl _DevCCWriteBytesFromRegs


_DevCCWriteBytesFromRegs:
	/*
	 * Disable all interrupts.
	 */
        rd_kpsw         r17
        and             r18, r17, $~(MACH_KPSW_INTR_TRAP_ENA);
        wr_kpsw         r18, $0
	/*
	 * Convert byte offset into a register address. 
	 */
	sll		r11,r11,$3
	sll		r11,r11,$2
	/*
	 * Convert the number bytes into register address range.
	 */
	sll		r12,r12,$3
	sll		r12,r12,$2
	/*
	 * Compute register offset to stop on.
	 */
	add_nt		r14, r11, r12
	cmp_br_delayed	uge, r11, r14,1f
2:	
	/*
	 * Fetch byte from buffer and increment buffer pointer.
	 */
	ld_32		r16, r13, $0
	add_nt		r13, r13, $1
	extract		r15, r16, r13
	st_external	r15, r11, $WRREG
	/*
	 * Increment the  register offset.
	 */
	add_nt		r11, r11, $(1 << 5)
	cmp_br_delayed  lt,r11,r14,2b
	Nop
1:
        wr_kpsw         r17, $0
        return		r10,$8
	nop

