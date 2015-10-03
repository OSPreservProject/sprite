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
    .text


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
	invalidate_ib
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
	rd_kpsw		SAFE_TEMP1
	and		VOL_TEMP2, SAFE_TEMP1, $~(MACH_KPSW_INTR_TRAP_ENA);
	wr_kpsw		VOL_TEMP2, $0
	st_external	VOL_TEMP1, INPUT_REG1, $MACH_CO_FLUSH
	wr_kpsw		SAFE_TEMP1, $0
	invalidate_ib
	return		RETURN_ADDR_REG, $8
	Nop


/*
 *----------------------------------------------------------------------
 *
 * VmMachFlushCache --
 *
 *	void VmMachFlushCache()
 *
 *	Flush the entire cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entire cache is flushed.
 *
 *----------------------------------------------------------------------
 */
	.globl _VmMachFlushCache
_VmMachFlushCache:
	add_nt		SAFE_TEMP1, r0, $0
	LD_CONSTANT(SAFE_TEMP2, VMMACH_CACHE_SIZE)
1:	st_external	r0, SAFE_TEMP1, $MACH_CO_FLUSH
	add_nt		SAFE_TEMP1, SAFE_TEMP1, $VMMACH_CACHE_LINE_SIZE
	cmp_br_delayed	lt, SAFE_TEMP1, SAFE_TEMP2, 1b
	Nop
	invalidate_ib
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
	rd_kpsw		SAFE_TEMP3
	and		VOL_TEMP2, SAFE_TEMP3, $~(MACH_KPSW_INTR_TRAP_ENA);
	wr_kpsw		VOL_TEMP2, $0
#ifdef set_kern_rptm
	ld_32		SAFE_TEMP1, INPUT_REG1, $0
	ld_32		SAFE_TEMP2, INPUT_REG2, $0
	ST_GSN(SAFE_TEMP1, MACH_GSN_0)
	ST_RPTM_PAGE(SAFE_TEMP2, MACH_RPTM_0)
#endif
	ld_32		SAFE_TEMP1, INPUT_REG1, $4
	ld_32		SAFE_TEMP2, INPUT_REG2, $4
	ST_GSN(SAFE_TEMP1, MACH_GSN_1)
	ST_RPTM_PAGE(SAFE_TEMP2, MACH_RPTM_1)
	ld_32		SAFE_TEMP1, INPUT_REG1, $8
	ld_32		SAFE_TEMP2, INPUT_REG2, $8
	ST_GSN(SAFE_TEMP1, MACH_GSN_2)
	ST_RPTM_PAGE(SAFE_TEMP2, MACH_RPTM_2)
	ld_32		SAFE_TEMP1, INPUT_REG1, $12
	ld_32		SAFE_TEMP2, INPUT_REG2, $12
	ST_GSN(SAFE_TEMP1, MACH_GSN_3)
	ST_RPTM_PAGE(SAFE_TEMP2, MACH_RPTM_3)

	wr_kpsw		SAFE_TEMP3, $0
	invalidate_ib
	return		RETURN_ADDR_REG, $8
	Nop


/*
 *----------------------------------------------------------------------
 *
 * VmMachSetSegReg1 --
 *
 *	unsigned VmMachSetSegReg1(segNum, RPTPM)
 *	    int	segNum;
 *	    int RPTM;
 *
 *	Set the global segment register 1.
 *
 * Results:
 *	The previous GSN1.
 *
 * Side effects:
 *	Set segment register 1.
 *
 *----------------------------------------------------------------------
 */
	.globl _VmMachSetSegReg1
_VmMachSetSegReg1:
	rd_kpsw		SAFE_TEMP3
	and		VOL_TEMP2, SAFE_TEMP3, $~(MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP2, $0
	LD_GSN(SAFE_TEMP1, MACH_GSN_1)
	ST_GSN(INPUT_REG1, MACH_GSN_1)
	ST_RPTM_PAGE(INPUT_REG2, MACH_RPTM_1)
	add_nt		RETURN_VAL_REG_CHILD, SAFE_TEMP1, $0
	wr_kpsw		SAFE_TEMP3, $0
	invalidate_ib
	return		RETURN_ADDR_REG, $8
	Nop


/*
 *----------------------------------------------------------------------
 *
 * VmMachSetSegReg2 --
 *
 *	unsigned  VmMachSetSegReg2(segNum, RPTPM)
 *	    int	segNum;
 *	    int RPTM;
 *
 *	Set the global segment register 1.
 *
 * Results:
 *	The previous GSN2.
 *
 * Side effects:
 *	Set segment register 1.
 *
 *----------------------------------------------------------------------
 */
	.globl _VmMachSetSegReg2
_VmMachSetSegReg2:
	rd_kpsw		SAFE_TEMP3
	and		VOL_TEMP2, SAFE_TEMP3, $~(MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP2, $0
	LD_GSN(SAFE_TEMP1, MACH_GSN_2)
	ST_GSN(INPUT_REG1, MACH_GSN_2)
	ST_RPTM_PAGE(INPUT_REG2, MACH_RPTM_2)
	add_nt		RETURN_VAL_REG_CHILD, SAFE_TEMP1, $0
	wr_kpsw		SAFE_TEMP3, $0
	invalidate_ib
	return		RETURN_ADDR_REG, $8
	Nop


/*
 *----------------------------------------------------------------------
 *
 * VmMachSetSegReg3 --
 *
 *	unsigned VmMachSetSegReg3(segNum, RPTPM)
 *	    int	segNum;
 *	    int RPTM;
 *
 *	Set the global segment register 1.
 *
 * Results:
 *	The previous GSN3.
 *
 * Side effects:
 *	Set segment register 1.
 *
 *----------------------------------------------------------------------
 */
	.globl _VmMachSetSegReg3
_VmMachSetSegReg3:
	rd_kpsw		SAFE_TEMP3
	and		VOL_TEMP2, SAFE_TEMP3, $~(MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP2, $0
	LD_GSN(SAFE_TEMP1, MACH_GSN_3)
	ST_GSN(INPUT_REG1, MACH_GSN_3)
	ST_RPTM_PAGE(INPUT_REG2, MACH_RPTM_3)
	add_nt		RETURN_VAL_REG_CHILD, SAFE_TEMP1, $0
	wr_kpsw		SAFE_TEMP3, $0
	invalidate_ib
	return		RETURN_ADDR_REG, $8
	Nop


