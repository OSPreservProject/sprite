/* 
 * machCCRegs.s --
 *
 *	Routines for reading and writing SPUR Cache Controller Registers
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
 * static char rcsid[] = "$Header$ SPRITE (Berkeley)";
 *
 * The information about the cache controller registers can be found in 
 *  Wood, Eggers, Gibson : "SPUR  Memory System Architecture" 
 *	UCB/CSD 87/394 for details [Wood87]). 
 * 
 */

/*
 * Constant for ld/st_external byte addresses
 */
#define	BYTE7_0	0x00
#define	BYTE15_8 0x20
#define	BYTE23_16 0x40
#define	BYTE31_24 0x60


/*
 *----------------------------------------------------------------------
 *
 * Mach_Read32bitCCReg --
 *
 *	Read the value of a 32 bit Spur Cache controller register starting
 *	at the specified address.  This routine assumes that the register
 *	is fixed and hence interupts should be disabled and the timer/counter
 *	being read should be stopped.
 *
 * Results:
 *	The Value of the specified register.
 *
 * Side effects:
 *	None.
 *
 * Calling Sequence:
 *	unsigned int CounterValue = Mach_Read32bitCCReg(register_address)
 *
 *	Where register_address is an integer with <12:8> specifying
 *	the register to read. Possble register_address are:
 *	T1, T2, IStatus, IMask, FEStatus, C0, C1, ..., C15
 *----------------------------------------------------------------------
 * 
 */
	 .text
	 .align 2
	.globl _Mach_Read32bitCCReg
_Mach_Read32bitCCReg:
 /*
  * Register assigment: 
  * 	Local registers:
  *		VALUE	- Value of register 
  *		TEMP    - Temp register 
  *	Inputs:
  *		ADDR  - Address of registers lower byte.
  *	Outputs:
  *		RESULT	- Function result return register.
  */
#define	ADDR	r11
#define	VALUE	r12
#define	TEMP	r13
#define	RESULT	r11
	/*
	 * Set the cacheSubOp to read.
	 */
	or		ADDR, ADDR, $RDREG
	ld_external	VALUE, ADDR, $BYTE7_0
	ld_external	TEMP, ADDR, $BYTE15_8
	wr_insert	$1
	insert		VALUE, VALUE, TEMP
	ld_external	TEMP, ADDR, $BYTE23_16
	wr_insert	$2
	insert		VALUE, VALUE, TEMP
	ld_external	TEMP, ADDR,  $BYTE31_24
	wr_insert	$3
	insert		VALUE, VALUE, TEMP
	add_nt		RESULT, r0, VALUE
	return		r10,$8
	nop

#undef	ADDR	
#undef	VALUE	
#undef	TEMP	
#undef	RESULT	

/*
 *----------------------------------------------------------------------
 *
 * Mach_Write32bitCCReg --
 *
 *	Write the value of a 32 bit Spur Cache controller register starting
 *	at the specified address.  This routine assumes that the register
 *	is fixed and hence interupts should be disabled and the timer/counter
 *	being read should be stopped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified register is updated. 
 *
 * Calling Sequence:
 *	Mach_Write32bitCCReg(register_address,CounterValue)
 *		unsigned int CounterValue;
 *	Where register_address is an integer with <12:8> specifying
 *	the register to read. Possble register_address are:
 *	T1, T2, IStatus, IMask, FEStatus, C0, C1, ..., C15
 *----------------------------------------------------------------------
 * 
 */
	 .text
	 .align 2
	.globl _Mach_Write32bitCCReg
_Mach_Write32bitCCReg:
 /*
  * Register assigment: 
  * 	Local registers:
  *		TEMP    - Temp register 
  *	Inputs:
  *		ADDR  - Address of register's lower byte.
  *		VALUE - Value to write
  */
#define	ADDR	r11
#define	VALUE	r12
#define	TEMP	r13
	/*
	 * Set the cacheSubOp to read.
	 */
	or		ADDR, ADDR, $WRREG
	/*
	 * Disable interrupts so the st_external wont store garbage.
	 */
	rd_kpsw		r14
	and		r15,r14,$~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		r15, $0
	st_external	VALUE, ADDR, $BYTE7_0
	extract		TEMP, VALUE, $1
	st_external	TEMP, ADDR, $BYTE15_8
	extract		TEMP, VALUE, $2
	st_external	TEMP, ADDR, $BYTE23_16
	extract		TEMP, VALUE, $3
	st_external	TEMP, ADDR,  $BYTE31_24
	wr_kpsw		r14,$0
	return		r10,$8
	nop


#undef	ADDR	
#undef	VALUE	
#undef	TEMP	

/*
 *----------------------------------------------------------------------
 *
 * Mach_Read8bitCCReg --
 *
 *	Read the value of a 8 bit Spur Cache controller register starting
 *	at the specified address.  This routine assumes that the register
 *	is fixed and hence interupts should be disabled and the timer/counter
 *	being read should be stopped.
 *
 * Results:
 *	The Value of the specified register.
 *
 * Side effects:
 *	None.
 *
 * Calling Sequence:
 *	unsigned int CounterValue = Mach_Read8bitCCReg(register_address)
 *
 *	Where register_address is an integer with <12:8> specifying
 *	the register to read. Possble register_address are:
 *	Mode, SlotId, or one byte of number of registers.
 *----------------------------------------------------------------------
 * 
 */
	 .text
	 .align 2
	.globl _Mach_Read8bitCCReg
_Mach_Read8bitCCReg:
 /*
  * Register assigment: 
  *	Inputs:
  *		ADDR  - Address of register's lower byte.
  *	Outputs:
  *		RESULT	- Function result return register.
  */
#define	ADDR	r11
#define	RESULT	r11
	/*
	 * Set the cacheSubOp to read.
	 */
	or		ADDR, ADDR, $RDREG
	ld_external	RESULT, ADDR, $BYTE7_0
	return		r10,$8
	nop

#undef	ADDR	
#undef	RESULT	

/*
 *----------------------------------------------------------------------
 *
 * Mach_Write8bitCCReg --
 *
 *	Write the value of a 8 bit Spur Cache controller register starting
 *	at the specified address. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified register is updated. 
 *
 * Calling Sequence:
 *	Mach_Write8bitCCReg(register_address,CounterValue)
 *		unsigned int CounterValue;
 *	Where register_address is an integer with <12:8> specifying
 *	the register to read. Possble register_address are:
 *	SlotID, Mode, GSN*, etc..
 *----------------------------------------------------------------------
 * 
 */
	 .text
	 .align 2
	.globl _Mach_Write8bitCCReg
_Mach_Write8bitCCReg:
 /*
  * Register assigment: 
  *	Inputs:
  *		ADDR  - Address of register's lower byte.
  *		VALUE - Value to write
  */
#define	ADDR	r11
#define	VALUE	r12
	/*
	 * Set the cacheSubOp to read.
	 */
	or		ADDR, ADDR, $WRREG
	/*
	 * Disable interrupts so the st_external wont store garbage.
	 */
	rd_kpsw		r14
	and		r15,r14,$~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		r15, $0
	st_external	VALUE, ADDR, $BYTE7_0
	wr_kpsw		r14,$0
	return		r10,$8
	nop
#undef ADDR
#undef VALUE
