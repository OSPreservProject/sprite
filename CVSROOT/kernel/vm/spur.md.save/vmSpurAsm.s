/* vmSpurAsm.s -
 *
 *	Subroutines to access the SPUR virtual memory system.
 *
 * Copyright (C) 1988 Regents of the University of California
 * All rights reserved.
 */

#include "reg.h"
#include "vmSpurConst.h"
#include "machAsmDefs.h"
#include "machConst.h"

    .data
    .asciz "$Header$ SPRITE (Berkeley)"


/*
 *----------------------------------------------------------------------
 *
 * VmMachReadAnyways --
 *
 *	void VmMachReadAnyways(addr)

 *	Perform the "read anyways" instruction to get exclusive access of
 *	the cache block at the given address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Gets exclusive access of the cache block.
 *
 *----------------------------------------------------------------------
 */
	.globl _VmMachReadAnyways
_VmMachReadAnyways:
	ld_32_ri	VOL_TEMP1, INPUT_REG1, $0
	Nop
	return		RETURN_ADDR_REG, $8
	Nop


/*
 *----------------------------------------------------------------------
 *
 * VmMachFlushBlock --
 *
 *	void VmMachFlushBlock(addr)
 *
 *	Flush the block from the cache at the given address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Flushes the block at the given address from the cache.
 *
 *----------------------------------------------------------------------
 */
	.globl _VmMachFlushBlock
_VmMachFlushBlock:
	st_external	VOL_TEMP1, INPUT_REG1, $MACH_CO_FLUSH
	return		RETURN_ADDR_REG, $8
	Nop


/*
 *----------------------------------------------------------------------
 *
 * VmMachSetSegRegisters --
 *
 *	void VmMachSetSegRegisters(segNums, RPTPMs)
 *	    int	segNums[4];
 *	    int RPTMs[4];
 *
 *	Set the 4 global segment and RPTPM registers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	4 global segment and RPTPM registers set.
 *
 *----------------------------------------------------------------------
 */
	.globl _VmMachSetSegRegisters
_VmMachSetSegRegisters:
	ld_32		SAFE_TEMP1, INPUT_REG1, $0
	ld_32		SAFE_TEMP2, INPUT_REG2, $0
	ST_GSN(SAFE_TEMP1, MACH_GSN_0)
	ST_RPTM(SAFE_TEMP2, MACH_RPTM_0)
	ld_32		SAFE_TEMP1, INPUT_REG1, $4
	ld_32		SAFE_TEMP2, INPUT_REG2, $4
	ST_GSN(SAFE_TEMP1, MACH_GSN_1)
	ST_RPTM(SAFE_TEMP2, MACH_RPTM_1)
	ld_32		SAFE_TEMP1, INPUT_REG1, $8
	ld_32		SAFE_TEMP2, INPUT_REG2, $8
	ST_GSN(SAFE_TEMP1, MACH_GSN_2)
	ST_RPTM(SAFE_TEMP2, MACH_RPTM_2)
	ld_32		SAFE_TEMP1, INPUT_REG1, $12
	ld_32		SAFE_TEMP2, INPUT_REG2, $12
	ST_GSN(SAFE_TEMP1, MACH_GSN_3)
	ST_RPTM(SAFE_TEMP2, MACH_RPTM_3)

	return		RETURN_ADDR_REG, $8
	Nop

