/* 
 * machPhys.s --
 *
 *	Routines for reading and writing SPUR addresses in physical mode.
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
 * The information about the physical/virtual mode addressing can be found in 
 *  Wood, Eggers, Gibson : "SPUR  Memory System Architecture" 
 *	UCB/CSD 87/394 for details [Wood87]). 
 * 
 */

#include "machConst.h"


/*
 *----------------------------------------------------------------------
 *
 * Mach_ReadPhysicalWord --
 *
 *	Read a word from the physical address space.
 *
 * Results:
 *	The value of the specified address.
 *
 * Side effects:
 *	Processor is temporarily switched into physical mode if not
 *	already in physical mode.
 *
 * Calling Sequence:
 *	unsigned int value = Mach_ReadPhysicalWord(address)
 *
 *----------------------------------------------------------------------
 * 
 */
	 .text
	 .align 2
	.globl _Mach_ReadPhysicalWord
_Mach_ReadPhysicalWord:
 /*
  * Register assigment: 
  * 	Local registers:
  *		TEMP	  - Temp register 
  *		KPSW_REG  - Temp register to hold original value of KPSW.
  *	Inputs:
  *		ADDR      - Address to read.
  *	Outputs:
  *		RESULT	  - Function result return register.
  */
#define	ADDR	  r11
#define	TEMP	  r27
#define	KPSW_REG  r28
#define	RESULT	  r11

	/*
	 * Go to physical mode to talk.
	 */
	rd_kpsw TEMP                     
	add_nt  KPSW_REG,TEMP,r0
	and	TEMP,TEMP,$~(MACH_KPSW_VIRT_DFETCH_ENA|MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw TEMP,r0

 	/*
	 * Read the data and restore previous mode.
	 */
	ld_32   RESULT,ADDR,$0
	wr_kpsw  KPSW_REG,r0 
	invalidate_ib
	return	r10, $8
	nop				

#undef	ADDR	
#undef	TEMP	
#undef	KPSW_REG
#undef	RESULT	

/*
 *----------------------------------------------------------------------
 *
 * Mach_WritePhysicalWord --
 *
 *	Write a word into the physical address space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Processor is temporarily switched into physical mode if not
 *	already in physical mode.
 *
 * Calling Sequence:
 *	(void) Mach_WritePhysicalWord(address, data)
 *
 *----------------------------------------------------------------------
 * 
 */
	 .text
	 .align 2
	.globl _Mach_WritePhysicalWord
_Mach_WritePhysicalWord:
 /*
  * Register assigment: 
  * 	Local registers:
  *		TEMP	  - Temp register 
  *		KPSW_REG  - Temp register to hold original value of KPSW.
  *	Inputs:
  *		ADDR      - Address where to write.
  *		DATA      - Data to write.
  *	Outputs:
  *		RESULT	  - Function result return register.
  */
#define	ADDR	  r11
#define	DATA	  r12
#define	TEMP	  r27
#define	KPSW_REG  r28
#define	RESULT	  r11

	/*
	 * Go to physical mode to talk.
	 */
	rd_kpsw TEMP                     
	add_nt  KPSW_REG,TEMP,r0
	and	TEMP,TEMP,$~(MACH_KPSW_VIRT_DFETCH_ENA|MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw TEMP,r0

 	/*
	 * Write the data and restore previous mode.
	 */
	st_32   DATA,ADDR,$0
	wr_kpsw KPSW_REG,r0 
	invalidate_ib
	return	r10, $8
	nop				

#undef	ADDR	
#undef	DATA	
#undef	TEMP	
#undef	KPSW_REG
#undef	RESULT	
