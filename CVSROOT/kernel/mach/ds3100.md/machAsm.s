/*
 * machAsm.s --
 *
 *	Contains misc. assembler routines for the PMAX.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#include "machConst.h"
#include "machAsmDefs.h"
#include "vmPmaxConst.h"
#include <regdef.h>

/*----------------------------------------------------------------------------
 *
 * MachConfigCache --
 *
 *	Size the caches.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The size of the data cache stored into machDataCacheSize and the
 *	size of instruction cache stored into machInstCacheSize.
 *
 *----------------------------------------------------------------------------
 */
CONFIG_FRAME=	(4*4)+4+4		# 4 arg saves, ra, and a saved register
    .globl	MachConfigCache
MachConfigCache:
	subu	sp,CONFIG_FRAME
	sw	ra,CONFIG_FRAME-4(sp)		# Save return address.
	sw	s0,CONFIG_FRAME-8(sp)		# Save s0 on stack.
	mfc0	s0,MACH_COP_0_STATUS_REG	# Save status register.
	mtc0	zero,MACH_COP_0_STATUS_REG	# Disable interrupts.
	.set	noreorder
	la	v0,1f
	or	v0,VMMACH_PHYS_UNCACHED_START	# Run uncached.
	j	v0
	nop

1:	jal	SizeCache			# Get the size of the d-cache.
	nop
	sw	v0,machDataCacheSize
	nop					# Make sure sw out of pipe
	nop
	nop
	nop
	li	v0,MACH_SR_SWAP_CACHES		# Swap caches
	mtc0	v0,MACH_COP_0_STATUS_REG
	nop					# Insure caches stable
	nop
	nop
	nop
	jal	SizeCache			# Get the size of the i-cache.
	nop
	sw	v0,machInstCacheSize		
	nop					# Make sure sw out of pipe
	nop
	nop
	nop
	mtc0	zero, MACH_COP_0_STATUS_REG	# Swap back caches. 
	nop
	nop
	nop
	nop
	la	t0,1f
	j	t0				# Back to cached mode
	nop

1:	mtc0	s0,MACH_COP_0_STATUS_REG	# Restore status register.
	nop
	lw	s0,CONFIG_FRAME-8(sp)		# Restore old s0
	lw	ra,CONFIG_FRAME-4(sp)		# Restore return addr
	addu	sp,CONFIG_FRAME			# Restore sp.
	j	ra
	nop
	.set	reorder

/*----------------------------------------------------------------------------
 *
 * SizeCache --
 *
 *	Get the size of the cache.
 *
 * Results:
 *     	The size of the cache.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
SizeCache:
	.set	noreorder
	mfc0	t0,MACH_COP_0_STATUS_REG	# Save the current status reg.
	nop				
	or	v0,t0,MACH_SR_ISOL_CACHES	# Isolate the caches.
	nop					# Make sure no stores in pipe
	mtc0	v0,MACH_COP_0_STATUS_REG
	nop					# Make sure isolated
	nop
	nop
	/*
	 * Clear cache size boundaries.
	 */
	li	v0, MACH_MIN_CACHE_SIZE
1:
	sw	zero, VMMACH_PHYS_CACHED_START(v0)
	sll	v0,1
	ble	v0,+MACH_MAX_CACHE_SIZE,1b
	nop
	li	v0,-1
	sw	v0, VMMACH_PHYS_CACHED_START(zero)	# Store marker in cache
	li	v0, MACH_MIN_CACHE_SIZE

2:	lw	v1, VMMACH_PHYS_CACHED_START(v0)	# Look for marker
	nop			
	bne	v1,zero,3f				# Found marker.
	nop

	sll	v0,1			# cache size * 2
	ble	v0,+MACH_MAX_CACHE_SIZE,2b		# keep looking
	nop
	move	v0,zero			# must be no cache
	.set	reorder

3:	mtc0	t0,MACH_COP_0_STATUS_REG
	nop				# Make sure unisolated
	nop
	nop
	nop
	j	ra
	nop
.set reorder

/*----------------------------------------------------------------------------
 *
 * MachFlushCache --
 *
 *	Flush the caches.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The contents of the cache is flushed.
 *
 *----------------------------------------------------------------------------
 */
    .globl	MachFlushCache
MachFlushCache:
	lw	t1,machInstCacheSize		# Must load before isolating
	lw	t2,machDataCacheSize		# Must load before isolating
	mfc0	t3,MACH_COP_0_STATUS_REG 	# Save the status register.
	mtc0	zero,MACH_COP_0_STATUS_REG	# Disable interrupts.
	.set	noreorder
	la	v0,1f
	or	v0,VMMACH_PHYS_UNCACHED_START	# Run uncached.
	j	v0			
	nop

	/*
	 * flush text cache
	 */
1:	li	v0,MACH_SR_ISOL_CACHES|MACH_SR_SWAP_CACHES
	mtc0	v0,MACH_COP_0_STATUS_REG	# Isolate and swap caches.
	li	t0,VMMACH_PHYS_UNCACHED_START
	subu	t0,t1
	li	t1,VMMACH_PHYS_UNCACHED_START
	la	v0,1f				# Run cached
	j	v0
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,VMMACH_PHYS_UNCACHED_START
	j	v0				# Run uncached
	nop

	/*
	 * flush data cache
	 */
1:	li	v0,MACH_SR_ISOL_CACHES|MACH_SR_SWAP_CACHES
	mtc0	v0,MACH_COP_0_STATUS_REG	# Isolate and swap back caches
	li	t0,VMMACH_PHYS_UNCACHED_START
	subu	t0,t2
	la	v0,1f
	j	v0				# Back to cached mode
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b

	.set	noreorder
	nop					# Insure isolated stores 
	nop					#     out of pipe.
	nop
	mtc0	t3,MACH_COP_0_STATUS_REG	# Restore status reg.
	nop					# Insure cache unisolated.
	nop
	nop
	nop
	.set	reorder
	j	ra

/*----------------------------------------------------------------------------
 *
 * MachCleanICache --
 *
 *	MachCleanICache(addr, len)
 *
 *	Flush i cache for range ofaddr to addr + len - 1.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The contents of the cache is flushed.
 *
 *----------------------------------------------------------------------------
 */
    .globl MachCleanICache
MachCleanICache:
	lw	t1,machInstCacheSize
	mfc0	t3,MACH_COP_0_STATUS_REG	# Save SR
	mtc0	zero,MACH_COP_0_STATUS_REG	# Disable interrupts.

	.set	noreorder
	la	v0,1f
	or	v0,VMMACH_PHYS_UNCACHED_START	# Run uncached.
	j	v0
	nop

1:	li	v0,MACH_SR_ISOL_CACHES|MACH_SR_SWAP_CACHES
	mtc0	v0,MACH_COP_0_STATUS_REG
	bltu	t1,a1,1f		# cache is smaller than region
	nop
	move	t1,a1
1:	addu	t1,a0			# ending address + 1
	move	t0,a0
	la	v0,1f			# run cached
	j	v0
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,VMMACH_PHYS_UNCACHED_START
	j	v0			# Run uncached
	nop

1:	nop				# insure isolated stores out of pipe
	mtc0	zero,MACH_COP_0_STATUS_REG  # unisolate, unswap
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	mtc0	t3,MACH_COP_0_STATUS_REG # enable interrupts
	nop
	j	ra			# return and run cached
	nop
	.set	reorder

    .globl MachFetchICache
MachFetchICache:
	mfc0	t3,MACH_COP_0_STATUS_REG	# Save SR
	mtc0	zero,MACH_COP_0_STATUS_REG	# Disable interrupts.

	.set	noreorder
	la	v0,1f
	or	v0,VMMACH_PHYS_UNCACHED_START	# Run uncached.
	j	v0
	nop

1:	li	v0,MACH_SR_ISOL_CACHES|MACH_SR_SWAP_CACHES
	mtc0	v0,MACH_COP_0_STATUS_REG
	la	v0,1f			# run cached
	j	v0
	nop
1:	ld	v0, 0(a0)
	nop

	la	t0,1f
	or	t0,VMMACH_PHYS_UNCACHED_START
	j	t0			# Run uncached
	nop

1:	mtc0	zero, MACH_COP_0_STATUS_REG  # unisolate, unswap
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	mtc0	t3,MACH_COP_0_STATUS_REG # enable interrupts
	nop
	j	ra			# return and run cached
	nop
	.set	reorder

/*----------------------------------------------------------------------------
 *
 * MachRunUserProc --
 *
 *	MachRunUserProc(pc, sp)
 *		Address	pc;	* The program counter to execute at.
 *		Address sp;	* The stack pointer to start with.
 *
 *	Start a process running in user mode.  We are called with interrupts
 *	disabled.  
 *
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The status register and stack pointer are modified.
 *
 *----------------------------------------------------------------------------
 */
LEAF(MachRunUserProc)
.set noreorder
    subu	sp, sp, STAND_FRAME_SIZE
    sw		ra, STAND_RA_OFFSET(sp)
    sw		a0, STAND_FRAME_SIZE(sp)
    sw		a1, STAND_FRAME_SIZE + 4(sp)
    .mask	0x80000000, -4
    li		t0, (MACH_KERN_INT_MASK|MACH_SR_KU_PREV|MACH_SR_INT_ENA_PREV)
    mtc0	t0, MACH_COP_0_STATUS_REG
    lw		k0, machCurStatePtr
    add		k1, a0, zero
.set noat
    RESTORE_REGS(k0, MACH_TRAP_REGS_OFFSET)
.set at

    j		k1
    rfe
.set reorder
END(MachRunUserProc)

/*----------------------------------------------------------------------------
 *
 * MachException --
 *
 *	Handle a general exception.
 *
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl MachException
MachException:
.set noat
/*
 * Find out what mode we came from.
 */
    mfc0	k0, MACH_COP_0_STATUS_REG
    and		k0, k0, MACH_SR_KU_PREV
    bne		k0, zero, 1f
    j		MachKernException
1:  j		MachUserException
.set at
    .globl MachEndException
MachEndException:

/*----------------------------------------------------------------------------
 *
 * MachKernException --
 *
 *	Handle an exception from kernel mode.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl MachKernException
MachKernException:
/*
 * Determine the type of fault and jump to the appropriate routine.
 */
.set noreorder
.set noat
    mfc0	k0, MACH_COP_0_CAUSE_REG	# Get the cause register value.
    la		k1, machKernExcTable		# Load base of the func table.
    and		k0, k0, MACH_CR_EXC_CODE	# Mask out the cause bits. 
    add		k0, k0, k1			# Get the address of the
						#    function entry.  Note that
						#    the cause is already 
						#    shifted left by 2 bits so
						#    we don't have to shift.
    lw		k0, 0(k0)			# Get the function address
    nop
    j		k0				# Jump to the function.
    nop
.set at
.set reorder


/*----------------------------------------------------------------------------
 *
 * Mach_KernGenException --
 *
 *	Handle an exception from kernel mode.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */

/*
 * The kernel exception stack contains 28 saved general registers, the
 * status register and the cause register and the multiply lo and high 
 * registers.  In addition we need to set
 * this up for linkage conventions.
 */
#define	KERN_EXC_FRAME_SIZE	(STAND_FRAME_SIZE + 8 + SAVED_REG_SIZE + 8)
#define KERN_SR_OFFSET		(STAND_FRAME_SIZE)
#define CAUSE_OFFSET		(STAND_FRAME_SIZE + 4)
#define SAVED_REG_OFFSET	(STAND_FRAME_SIZE + 8)
#define KERN_MULT_LO_OFFSET	(STAND_FRAME_SIZE + 8 + SAVED_REG_SIZE)
#define KERN_MULT_HI_OFFSET	(STAND_FRAME_SIZE + 8 + SAVED_REG_SIZE + 4)

NON_LEAF(Mach_KernGenException,KERN_EXC_FRAME_SIZE,ra)
.set noreorder
.set noat
    subu	sp, sp, KERN_EXC_FRAME_SIZE
/*
 * Save kernel registers onto the stack.
 */
    SAVE_KERNEL_REGS(SAVED_REG_OFFSET)
    mflo	t0
    sw		t0, KERN_MULT_LO_OFFSET(sp)
    mfhi	t0
    sw		t0, KERN_MULT_HI_OFFSET(sp)
/*
 * Save the rest of the state.
 */
    mfc0	k0, MACH_COP_0_EXC_PC
    mfc0	k1, MACH_COP_0_STATUS_REG
    sw		k0, STAND_RA_OFFSET(sp)
    .mask	0x80000000, (STAND_RA_OFFSET - KERN_EXC_FRAME_SIZE)
    sw		k1, KERN_SR_OFFSET(sp)
    mfc0	k0, MACH_COP_0_CAUSE_REG
    nop
    sw		k0, CAUSE_OFFSET(sp)

/*
 * Call the exception handler.
 */
    mfc0	a0, MACH_COP_0_STATUS_REG	# First arg is the status reg.
    mfc0	a1, MACH_COP_0_CAUSE_REG	# Second arg is the cause reg.
    mfc0	a2, MACH_COP_0_BAD_VADDR	# Third arg is the fault addr.
    mfc0	a3, MACH_COP_0_EXC_PC		# Fourth arg is the pc.
    jal		MachKernelExceptionHandler
    nop
    mtc0	zero, MACH_COP_0_STATUS_REG	# Disable interrupts
    nop
/*
 * Check error code.
 */
    li		t0, MACH_OK
    li		t1, MACH_USER_ERROR
    beq		v0, t0, 9f
    nop
    beq		v0, t1, 8f
    nop

/*
 * We got a kernel error.  Save the special registers that we saved on 
 * the stack into the debug state struct.
 */
    lw		k0, machDebugStatePtr
    lw		k1, KERN_SR_OFFSET(sp)
    nop
    sw		k1, MACH_DEBUG_STATUS_REG_OFFSET(k0)
    lw		k1, STAND_RA_OFFSET(sp)
    nop
    sw		k1, MACH_DEBUG_EXC_PC_OFFSET(k0)
    lw		k1, CAUSE_OFFSET(sp)
    nop
    sw		k1, MACH_DEBUG_CAUSE_REG_OFFSET(k0)

/*
 * Restore kernel registers and pop the stack.
 */
    lw		t0, KERN_MULT_LO_OFFSET(sp)
    lw		t1, KERN_MULT_HI_OFFSET(sp)
    mtlo	t0
    mthi	t1
    RESTORE_KERNEL_REGS(SAVED_REG_OFFSET)
    addu	sp, sp, KERN_EXC_FRAME_SIZE

/*
 * Save the general registers into the debug state struct.
 */
    SAVE_REGS(k0, MACH_DEBUG_REGS_OFFSET)
    sw		gp, MACH_DEBUG_REGS_OFFSET +  (4 * GP)(k0)
    sw		sp, MACH_DEBUG_REGS_OFFSET +  (4 * SP)(k0)
    mflo	t0
    sw		t0, MACH_DEBUG_MULT_LO_OFFSET(k0)
    mfhi	t0
    sw		t0, MACH_DEBUG_MULT_HI_OFFSET(k0)

/*
 * Now save the rest of the special registers.
 */
    mfc0	t0, MACH_COP_0_TLB_INDEX
    nop
    sw		t0, MACH_DEBUG_TLB_INDEX_OFFSET(k0)
    mfc0	t0, MACH_COP_0_TLB_RANDOM
    nop
    sw		t0, MACH_DEBUG_TLB_RANDOM_OFFSET(k0)
    mfc0	t0, MACH_COP_0_TLB_LOW
    nop
    sw		t0, MACH_DEBUG_TLB_LOW_OFFSET(k0)
    mfc0	t0, MACH_COP_0_TLB_CONTEXT
    nop
    sw		t0, MACH_DEBUG_TLB_CONTEXT_OFFSET(k0)
    mfc0	t0, MACH_COP_0_BAD_VADDR
    nop
    sw		t0, MACH_DEBUG_BAD_VADDR_OFFSET(k0)
    mfc0	t0, MACH_COP_0_TLB_HI
    nop
    sw		t0, MACH_DEBUG_TLB_HI_OFFSET(k0)
/*
 * Save the floating point state.
 */

.set at
    mfc0	t0, MACH_COP_0_STATUS_REG
    nop
    or		t0, t0, MACH_SR_COP_1_BIT
    mtc0	t0, MACH_COP_0_STATUS_REG
    nop
    nop
    cfc1	t1, MACH_FPC_CSR
    nop
    sw		t1, MACH_DEBUG_FPC_CSR_REG_OFFSET(k0)

#define SAVE_DEBUG_CP1_REG(reg) \
    swc1	$f/**/reg, MACH_DEBUG_FP_REGS_OFFSET+reg*4(k0)

    SAVE_DEBUG_CP1_REG(0);  SAVE_DEBUG_CP1_REG(1);  SAVE_DEBUG_CP1_REG(2)
    SAVE_DEBUG_CP1_REG(3);  SAVE_DEBUG_CP1_REG(4);  SAVE_DEBUG_CP1_REG(5)
    SAVE_DEBUG_CP1_REG(6);  SAVE_DEBUG_CP1_REG(7);  SAVE_DEBUG_CP1_REG(8)
    SAVE_DEBUG_CP1_REG(9);  SAVE_DEBUG_CP1_REG(10); SAVE_DEBUG_CP1_REG(11)
    SAVE_DEBUG_CP1_REG(12); SAVE_DEBUG_CP1_REG(13); SAVE_DEBUG_CP1_REG(14)
    SAVE_DEBUG_CP1_REG(15); SAVE_DEBUG_CP1_REG(16); SAVE_DEBUG_CP1_REG(17)
    SAVE_DEBUG_CP1_REG(18); SAVE_DEBUG_CP1_REG(19); SAVE_DEBUG_CP1_REG(20)
    SAVE_DEBUG_CP1_REG(21); SAVE_DEBUG_CP1_REG(22); SAVE_DEBUG_CP1_REG(23)
    SAVE_DEBUG_CP1_REG(24); SAVE_DEBUG_CP1_REG(25); SAVE_DEBUG_CP1_REG(26)
    SAVE_DEBUG_CP1_REG(27); SAVE_DEBUG_CP1_REG(28); SAVE_DEBUG_CP1_REG(29)
    SAVE_DEBUG_CP1_REG(30); SAVE_DEBUG_CP1_REG(31)
.set noat

/*
 * Switch to the debuggers stack and call the debugger.  The debuggers
 * stack starts at the base of the first kernel stack.
 */
    li		sp, MACH_STACK_BOTTOM - STAND_FRAME_SIZE
    jal		Dbg_Main
    nop
/*
 * The debugger returns the PC to continue at.
 */
    add		k1, v0, 0
    lw		k0, machDebugStatePtr
    nop

    lw		t0, MACH_TRAP_MULT_LO_OFFSET(k0)
    lw		t1, MACH_TRAP_MULT_HI_OFFSET(k0)
    mtlo	t0
    mthi	t1
    RESTORE_REGS(k0, MACH_DEBUG_REGS_OFFSET)
    lw		k0, MACH_DEBUG_STATUS_REG_OFFSET(k0)
    nop
    mtc0	k0, MACH_COP_0_STATUS_REG	
    nop
    j		k1
    rfe

8:
/*
 * We got an error on a cross address space copy.  All we have to do is
 * restore the stack pointer and the status register, set the return value
 * register and return.
 */
    lw		t0, KERN_MULT_LO_OFFSET(sp)
    lw		t1, KERN_MULT_HI_OFFSET(sp)
    mtlo	t0
    mthi	t1
    RESTORE_KERNEL_REGS(SAVED_REG_OFFSET)
    lui		v0, 0x2				# v0 <= SYS_ARG_NO_ACCESS
    lw		k0, KERN_SR_OFFSET(sp)		# Get the saved sp.
    addu	sp, sp, KERN_EXC_FRAME_SIZE	# Clear off the stack.
    mtc0	k0, MACH_COP_0_STATUS_REG	# Restore the status register.
    nop
    j		ra				# Now return to the caller
    rfe						#   who caused the error.


9:
/*
 * Restore registers and return from the exception.
 */
    lw		t0, KERN_MULT_LO_OFFSET(sp)
    lw		t1, KERN_MULT_HI_OFFSET(sp)
    mtlo	t0
    mthi	t1
    RESTORE_KERNEL_REGS(SAVED_REG_OFFSET)

    lw		k0, KERN_SR_OFFSET(sp)
    lw		k1, STAND_RA_OFFSET(sp)
    addu	sp, sp, KERN_EXC_FRAME_SIZE
    mtc0	k0, MACH_COP_0_STATUS_REG	# Restore the SR
    nop
    j		k1				# Now return from the
    rfe						#    exception.
END(Mach_KernGenException)
    .set at
    .set reorder

/*----------------------------------------------------------------------------
 *
 * MachUserException --
 *
 *	Handle an exception from user mode.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl MachUserException
MachUserException:
.set noreorder
.set noat
    mfc0	k0, MACH_COP_0_CAUSE_REG	# Get the cause register value.
    la		k1, machUserExcTable		# Load base of the func table.
    and		k0, k0, MACH_CR_EXC_CODE	# Mask out the cause bits. 
    add		k0, k0, k1			# Get the address of the
						#    function entry.  Note that
						#    the cause is already 
						#    shifted left by 2 bits so
						#    we don't have to shift.
    lw		k0, 0(k0)			# Get the function address
    nop
    j		k0				# Jump to the function.
    nop
.set at
.set reorder

/*----------------------------------------------------------------------------
 *
 * Mach_UserGenException --
 *
 *	Handle an exception from user mode.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
/*
 * The user exception stack contains the status register and the exception
 * PC.
 */
#define	USER_EXC_FRAME_SIZE	(4 + STAND_FRAME_SIZE)
#define USER_SR_OFFSET		(STAND_RA_OFFSET + 4)

NON_LEAF(Mach_UserGenException,USER_EXC_FRAME_SIZE,ra)
.set noreorder
.set noat
/*
 * First of all switch over to the kernel gp.
 */
    add		k1, gp, zero
    la		gp, _gp
    lw		k0, machCurStatePtr
    nop
/*
 * Save all registers.
 */
    sw		sp, MACH_TRAP_REGS_OFFSET + (SP * 4)(k0)
    sw		k1, MACH_TRAP_REGS_OFFSET + (GP * 4)(k0)
    SAVE_REGS(k0, MACH_TRAP_REGS_OFFSET)
    mflo	t0
    sw		t0, MACH_TRAP_MULT_LO_OFFSET(k0)
    mfhi	t0
    sw		t0, MACH_TRAP_MULT_HI_OFFSET(k0)
.set at

/*
 * Change to the kernel's stack.
 */
    lw		sp, MACH_KERN_STACK_END_OFFSET(k0)
/*
 * Set up the stack frame.
 */
    mfc0	a3, MACH_COP_0_EXC_PC		# The fourth arg is the PC
    subu	sp, sp, USER_EXC_FRAME_SIZE
    sw		a3, STAND_RA_OFFSET(sp)
    sw		a3, MACH_USER_PC_OFFSET(k0)
    .mask	0x80000000, (STAND_RA_OFFSET - USER_EXC_FRAME_SIZE)

    mfc0	a0, MACH_COP_0_STATUS_REG	# First arg is the status reg.
    nop
    and		k1, a0, ~MACH_SR_COP_1_BIT	# Turn off the FPU.
    mtc0	k1, MACH_COP_0_STATUS_REG
    sw		k1, USER_SR_OFFSET(sp)

/*
 * Call the handler.
 */
    mfc0	a1, MACH_COP_0_CAUSE_REG	# Second arg is the cause reg.
    mfc0	a2, MACH_COP_0_BAD_VADDR	# Third arg is the fault addr
    jal		MachUserExceptionHandler
    nop

/*
 * Restore user registers and return.  Interrupts are already disabled
 * when MachUserExceptionHandler returns.
 */
    lw		k0, USER_SR_OFFSET(sp)
    beq		v0, zero, 1f			# See if we are supposed to
    nop						#   to turn on the FPU.
    or		k0, k0, MACH_SR_COP_1_BIT
1:
    mtc0	k0, MACH_COP_0_STATUS_REG
    lw		k0, machCurStatePtr
    nop
.set noat
    lw		t0, MACH_TRAP_MULT_LO_OFFSET(k0)
    lw		t1, MACH_TRAP_MULT_HI_OFFSET(k0)
    mtlo	t0
    mthi	t1
    RESTORE_REGS(k0, MACH_TRAP_REGS_OFFSET)
    lw		k1, MACH_USER_PC_OFFSET(k0)
    nop
    j		k1
    rfe
END(Mach_UserGenException)
.set at
.set reorder


/*----------------------------------------------------------------------------
 *
 * Mach_EnableIntr --
 *
 *	Enable interrupts.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Interrupts enabled.
 *
 *----------------------------------------------------------------------------
 */
LEAF(Mach_EnableIntr)
.set noreorder
    mfc0	t0, MACH_COP_0_STATUS_REG
    nop
    or		t0, t0, MACH_KERN_INT_MASK | MACH_SR_INT_ENA_CUR
    mtc0	t0, MACH_COP_0_STATUS_REG
    nop
    j		ra
    nop
.set reorder
END(Mach_EnableIntr)

/*----------------------------------------------------------------------------
 *
 * Mach_DisableIntr --
 *
 *	Disable Interrupts.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Interrupts disabled.
 *
 *----------------------------------------------------------------------------
 */
LEAF(Mach_DisableIntr)
.set noreorder
    mfc0	t0, MACH_COP_0_STATUS_REG
    nop
    and		t0, t0, ~MACH_SR_INT_ENA_CUR
    mtc0	t0, MACH_COP_0_STATUS_REG
    nop
    j		ra
    nop
.set reorder
END(Mach_DisableIntr)

/*----------------------------------------------------------------------------
 *
 * Mach_ContextSwitch --
 *
 *	Mach_ContextSwitch(fromProcPtr, toProcPtr)
 *
 *	Perform a context switch.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The current process's state is saved into its machine specific
 *	process table entry and new state is loaded for the switched to
 *	process.  
 *
 *----------------------------------------------------------------------------
 */
.set noreorder

NON_LEAF(Mach_ContextSwitch,STAND_FRAME_SIZE + 8,ra)
    subu	sp, sp, STAND_FRAME_SIZE
    sw		ra, STAND_RA_OFFSET(sp)
    sw		a0, STAND_FRAME_SIZE(sp)
    sw		a1, STAND_FRAME_SIZE + 4(sp)
    .mask	0x80000000, -4
/*
 * Set up this processes context.
 */
    add		a0, a1, zero
    jal		VmMach_SetupContext
    nop
/*
 * Restore saved register values.
 */
    lw		ra, STAND_RA_OFFSET(sp)
    lw		a0, STAND_FRAME_SIZE(sp)
    lw		a1, STAND_FRAME_SIZE + 4(sp)
/*
 * Push the magic number and the status register onto the stack.
 */
    subu	sp, sp, 8
    li		t0, MAGIC
    sw		t0, 0(sp)
    mfc0	t0, MACH_COP_0_STATUS_REG # Save the status reg.
    nop
    sw		t0, 4(sp)

/*
 * Save the state of the current process.  We only have to save the saved
 * register registers (s0 through s8) and the stack pointer.
 */
    lw		t0, machCurStatePtr
    nop
    add		t0, t0, MACH_SWITCH_REGS_OFFSET
    sw		s0, S0 * 4(t0)
    sw		s1, S1 * 4(t0)
    sw		s2, S2 * 4(t0)
    sw		s3, S3 * 4(t0)
    sw		s4, S4 * 4(t0)
    sw		s5, S5 * 4(t0)
    sw		s6, S6 * 4(t0)
    sw		s7, S7 * 4(t0)
    sw		s8, S8 * 4(t0)
    sw		ra, RA * 4(t0)
    sw		sp, SP * 4(t0)
    .globl Mach_SwitchPoint
Mach_SwitchPoint:

/*
 * Restore the registers for the new process.
 */
    lw		t0, machStatePtrOffset
    nop
    add		t0, a1, t0
    lw		t0, 0(t0)
    nop
    sw		t0, machCurStatePtr
    add		t1, t0, MACH_SWITCH_REGS_OFFSET
    lw		a0, A0 * 4(t1)
    lw		s0, S0 * 4(t1)
    lw		s1, S1 * 4(t1)
    lw		s2, S2 * 4(t1)
    lw		s3, S3 * 4(t1)
    lw		s4, S4 * 4(t1)
    lw		s5, S5 * 4(t1)
    lw		s6, S6 * 4(t1)
    lw		s7, S7 * 4(t1)
    lw		s8, S8 * 4(t1)
    lw		ra, RA * 4(t1)
    lw		sp, SP * 4(t1)
/*
 * Set up the maximum stack addr for the debugger.
 */
    lw		t1, MACH_KERN_STACK_END_OFFSET(t0)
    nop
    sw		t1, dbgMaxStackAddr

/*
 * Wire down the current process's stack in the TLB.
 */
    mfc0	t1, VMMACH_TLB_HI

/*
 * Map the first entry.
 */
    lw		t2, MACH_TLB_HIGH_ENTRY_OFFSET(t0)
    lw		t3, MACH_TLB_LOW_ENTRY_1_OFFSET(t0)
    li		t4, MACH_STACK_TLB_INDEX_1
    mtc0	t2, VMMACH_TLB_HI
    mtc0	t3, VMMACH_TLB_LOW
    mtc0	t4, VMMACH_TLB_INDEX
    nop
    tlbwi

/*
 * Map the second entry.
 */
    addu	t2, t2, 1 << VMMACH_TLB_VIRT_PAGE_SHIFT
    lw		t3, MACH_TLB_LOW_ENTRY_2_OFFSET(t0)
    li		t4, MACH_STACK_TLB_INDEX_2
    mtc0	t2, VMMACH_TLB_HI
    mtc0	t3, VMMACH_TLB_LOW
    mtc0	t4, VMMACH_TLB_INDEX
    nop
    tlbwi

    mtc0	t1, VMMACH_TLB_HI

/*
 * Verify the magic number on the stack.
 */
    lw		t0, 0(sp)
    li		t1, MAGIC
    beq		t0, t1, 1f
    nop
    break	0

/*
 * Restore the status register and pop the stack.
 */
1:
    lw		t0, 4(sp)
    nop
    mtc0	t0, MACH_COP_0_STATUS_REG
    add		sp, sp, STAND_FRAME_SIZE + 8
/*
 * Return 
 */
    j		ra
    nop

END(Mach_ContextSwitch)
.set reorder

/*----------------------------------------------------------------------------
 *
 * Mach_GetPC --
 *
 *	Mach_GetPC()
 *
 *	Return the caller's caller's PC.
 *
 * Results:
 *     	0 for now.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl Mach_GetPC
Mach_GetPC:
    add		v0, zero, zero
    j		ra

/*----------------------------------------------------------------------------
 *
 * Mach_TestAndSet --
 *
 *	Mach_TestAndSet(intPtr)
 *
 *	Return the caller's caller's PC.
 *
 * Results:
 *     	0 for now.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(Mach_TestAndSet)
    mfc0	t0, MACH_COP_0_STATUS_REG
    mtc0	zero, MACH_COP_0_STATUS_REG	# Disable interrupts
    lw		v0, 0(a0)			# Read out old value
    li		t1, 1
    sw		t1, 0(a0)			# Set value.
    mtc0	t0, MACH_COP_0_STATUS_REG	# Restore interrupts.
    j		ra
END(Mach_TestAndSet)

/*----------------------------------------------------------------------------
 *
 * Mach_EmptyWriteBuffer --
 *
 *	Mach_EmptyWriteBuffer()
 *
 *	Return when the write buffer is empty.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(Mach_EmptyWriteBuffer)
	nop
	nop
	nop
	nop
1:	bc0f	1b
	j	ra
END(Mach_EmptyWriteBuffer)

#define SAVE_CP1_REG(reg) \
	swc1	$f/**/reg, MACH_FP_REGS_OFFSET+reg*4(a0)

#define REST_CP1_REG(reg) \
	lwc1	$f/**/reg, MACH_FP_REGS_OFFSET+reg*4(a1)

/*----------------------------------------------------------------------------
 *
 * MachSwitchFPState --
 *
 *	MachSwitchFPState(fromFPStatePtr, toFPStatePtr)
 *
 *	Save the current state into fromFPStatePtrs state and restore it
 *	from toFPStatePtr.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(MachSwitchFPState)
	subu	sp, sp, STAND_FRAME_SIZE
	sw	ra, STAND_RA_OFFSET(sp)
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

	mfc0	t1, MACH_COP_0_STATUS_REG	# Disable interrupts and
	li	t0, MACH_SR_COP_1_BIT		#    enable the coprocessor
	mtc0	t0, MACH_COP_0_STATUS_REG

	add	t0, a0, 1	# If fromFPStatePtr is NIL then it will equal
	beq	t0, zero, 1f	#    zero if we add one to it.

.set noreorder
/*
 * First read out the status register to make sure that all FP operations
 * have completed.
 */
	cfc1	t0, MACH_FPC_CSR
	nop
	sw	t0, MACH_FP_SR_OFFSET(a0)
/* 
 * Save the floating point registers.
 */
	SAVE_CP1_REG(0); SAVE_CP1_REG(1); SAVE_CP1_REG(2); SAVE_CP1_REG(3)
	SAVE_CP1_REG(4); SAVE_CP1_REG(5); SAVE_CP1_REG(6); SAVE_CP1_REG(7)
	SAVE_CP1_REG(8); SAVE_CP1_REG(9); SAVE_CP1_REG(10); SAVE_CP1_REG(11)
	SAVE_CP1_REG(12); SAVE_CP1_REG(13); SAVE_CP1_REG(14); SAVE_CP1_REG(15)
	SAVE_CP1_REG(16); SAVE_CP1_REG(17); SAVE_CP1_REG(18); SAVE_CP1_REG(19)
	SAVE_CP1_REG(20); SAVE_CP1_REG(21); SAVE_CP1_REG(22); SAVE_CP1_REG(23)
	SAVE_CP1_REG(24); SAVE_CP1_REG(25); SAVE_CP1_REG(26); SAVE_CP1_REG(27)
	SAVE_CP1_REG(28); SAVE_CP1_REG(29); SAVE_CP1_REG(30); SAVE_CP1_REG(31)

1:	
/*
 * Restore the floating point registers.
 */
	REST_CP1_REG(0); REST_CP1_REG(1); REST_CP1_REG(2); REST_CP1_REG(3)
	REST_CP1_REG(4); REST_CP1_REG(5); REST_CP1_REG(6); REST_CP1_REG(7)
	REST_CP1_REG(8); REST_CP1_REG(9); REST_CP1_REG(10); REST_CP1_REG(11)
	REST_CP1_REG(12); REST_CP1_REG(13); REST_CP1_REG(14); REST_CP1_REG(15)
	REST_CP1_REG(16); REST_CP1_REG(17); REST_CP1_REG(18); REST_CP1_REG(19)
	REST_CP1_REG(20); REST_CP1_REG(21); REST_CP1_REG(22); REST_CP1_REG(23)
	REST_CP1_REG(24); REST_CP1_REG(25); REST_CP1_REG(26); REST_CP1_REG(27)
	REST_CP1_REG(28); REST_CP1_REG(29); REST_CP1_REG(30); REST_CP1_REG(31)

	lw	t0, MACH_FP_SR_OFFSET(a1)
	nop
	and	t0, t0, ~MACH_FPC_EXCEPTION_BITS
	ctc1	t0, MACH_FPC_CSR
	nop
	mtc0	t1, MACH_COP_0_STATUS_REG	# Restore the status register.

	addu	sp, sp, STAND_FRAME_SIZE

	j	ra
	nop

.set reorder
END(MachSwitchFPState)

/*----------------------------------------------------------------------------
 *
 * MachGetCurFPState --
 *
 *	MachGetCurFPState(statePtr)
 *
 *	Save the current state into *statePtr.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(MachGetCurFPState)
	subu	sp, sp, STAND_FRAME_SIZE
	sw	ra, STAND_RA_OFFSET(sp)
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

	mfc0	t1, MACH_COP_0_STATUS_REG	# Disable interrupts and
	li	t0, MACH_SR_COP_1_BIT		#    enable the coprocessor
	mtc0	t0, MACH_COP_0_STATUS_REG

.set noreorder
	/*
	 * First read out the status register to make sure that all FP
	 * operations have completed.
	 */
	cfc1	t0, MACH_FPC_CSR
	nop
	sw	t0, MACH_FP_SR_OFFSET(a0)
	/* 
	 * Save the floating point registers.
	 */
	SAVE_CP1_REG(0); SAVE_CP1_REG(1); SAVE_CP1_REG(2); SAVE_CP1_REG(3)
	SAVE_CP1_REG(4); SAVE_CP1_REG(5); SAVE_CP1_REG(6); SAVE_CP1_REG(7)
	SAVE_CP1_REG(8); SAVE_CP1_REG(9); SAVE_CP1_REG(10); SAVE_CP1_REG(11)
	SAVE_CP1_REG(12); SAVE_CP1_REG(13); SAVE_CP1_REG(14); SAVE_CP1_REG(15)
	SAVE_CP1_REG(16); SAVE_CP1_REG(17); SAVE_CP1_REG(18); SAVE_CP1_REG(19)
	SAVE_CP1_REG(20); SAVE_CP1_REG(21); SAVE_CP1_REG(22); SAVE_CP1_REG(23)
	SAVE_CP1_REG(24); SAVE_CP1_REG(25); SAVE_CP1_REG(26); SAVE_CP1_REG(27)
	SAVE_CP1_REG(28); SAVE_CP1_REG(29); SAVE_CP1_REG(30); SAVE_CP1_REG(31)

	mtc0	t1, MACH_COP_0_STATUS_REG	# Restore the status register.

	addu	sp, sp, STAND_FRAME_SIZE

	j	ra
	nop

.set reorder
END(MachGetCurFPState)

/*----------------------------------------------------------------------------
 *
 * MachFPInterrupt --
 *
 *	Handle a floating point interrupt.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NON_LEAF(MachFPInterrupt,STAND_FRAME_SIZE,ra)
	subu	sp, sp, STAND_FRAME_SIZE
	sw	ra, STAND_RA_OFFSET(sp)

	mfc0	t0, MACH_COP_0_STATUS_REG
	and	t1, t0, MACH_SR_KU_PREV
	bne	t1, zero, 1f
	/*
	 * We got an FPU interrupt in kernel mode.  This is panic time.
	 */
	PANIC("FPU Interrupt in Kernel mode\012")
1:
	/*
	 * Turn on the floating point coprocessor.
	 */
	or	t1, t0, MACH_SR_COP_1_BIT
	mtc0	t1, MACH_COP_0_STATUS_REG
	/* 
	 * Check for a stray interrupt.
	 */
	lw	t1, machFPCurStatePtr
	lw	t2, machCurStatePtr
	beq	t1, t2, 1f
	/*
	 * We got an interrupt and no one was using the coprocessor.  Clear
	 * the interrupt and complain.
	 */
	PRINTF("Stray FPU interrupt\012")
	ctc1	zero, MACH_FPC_CSR
	j	FPReturn
1:
	/*
	 * Fetch the instruction.
	 */
	mfc0	a3, MACH_COP_0_EXC_PC
	mfc0	v0, MACH_COP_0_CAUSE_REG
	bltz	v0, 3f				# Check the branch delay bit.
	/*
	 * This is not in the branch delay slot so calculate the resulting
	 * PC (epc + 4) into v0 and continue to softfp().
	 */
	lw	a1, 0(a3)
	addu	v0, a3, 4
	lw	t0, machCurStatePtr
	sw	v0, MACH_USER_PC_OFFSET(t0)
	b	4f
3:
	/*
	 * This is in the branch delay slot so the branch will have to
	 * be emulated to get the resulting PC.
	 */
	lw	a0, machCurStatePtr
	add	a0, a0, MACH_TRAP_REGS_OFFSET
	add	a1, a3, zero
	cfc1	a2, MACH_FPC_CSR
	add	a3, zero, zero
	jal	MachEmulateBranch	# MachEmulateBranch(regsPtr,instPC,csr,
					#		    FALSE)
	lw	t0, machCurStatePtr
	sw	v0, MACH_USER_PC_OFFSET(t0)
	/*
	 * Now load the floating-point instruction in the branch delay slot
	 * to be emulated by softfp().
	 */
	mfc0	a3, MACH_COP_0_EXC_PC
	lw	a1, 4(a3)
4:
	/*
	 * Check to see if the instruction to be emulated is a floating-point
	 * instruction.
	 */
	srl	a3, a1, MACH_OPCODE_SHIFT
	beq	a3, MACH_OPCODE_C1, 5f
	/*
	 * Send a floating point exception signal to the current process.
	 */
	li	a0, MACH_SIGFPE
	jal	Mach_SendSignal
	j	FPReturn

5:
	/*
	 * Finally we can call softfp() where a1 has the instruction to
	 * emulate.
	 */
	jal	softfp

FPReturn:
	/*
	 * Turn off the floating point coprocessor.
	 */
	mfc0	t0, MACH_COP_0_STATUS_REG
	and	t0, t0, ~MACH_SR_COP_1_BIT
	mtc0	t0, MACH_COP_0_STATUS_REG
	/*
	 * Return to our caller.
	 */
	lw	ra, STAND_RA_OFFSET(sp)
	addu	sp, sp, STAND_FRAME_SIZE
	j	ra
END(MachFPInterrupt)

/*----------------------------------------------------------------------------
 *
 * MachSysCall --
 *
 *	MachSysCall --
 *
 *	Handle a system call.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
.set noreorder
    .globl MachSysCall
    .ent MachSysCall, 0
MachSysCall:
/*
 * Check the magic number.
 */
    li		k0, MACH_SYSCALL_MAGIC
    bne		t1, k0, UNIXSyscall
    nop
1:
    add		t7, gp, zero			# Save the user's gp in t7
    la		gp, _gp				# Switch to the kernel's gp
/*
 * See if this system call is valid.
 */
    lw		t2, machMaxSysCall		# t2 <= Maximum sys call value.
    nop
    add		t2, t2, 1			
    sltu	t2, t0, t2			# Is t0 < t2 ?	
    bne		t2, zero, 1f			# If so then continue on.
    nop
/*
 * System call number is too big.  Return SYS_INVALID_SYSTEM_CALL to
 * the user.
 */
    mfc0	t3, MACH_COP_0_EXC_PC
    add		gp, t7, zero
    li		v0, 0x20002
    add		t3, t3, 4
    j		t3
    rfe
/* 
 * Now we know that we have a good system call number so go ahead and
 * save state and switch to the kernel's stack.
 */
1:
    lw		t1, machCurStatePtr
    add		t2, sp, zero
    mfc0	t3, MACH_COP_0_EXC_PC
    sw		sp, MACH_TRAP_REGS_OFFSET + (SP * 4)(t1)
    sw		t7, MACH_TRAP_REGS_OFFSET + (GP * 4)(t1)
    sw		s0, MACH_TRAP_REGS_OFFSET + (S0 * 4)(t1)
    sw		s1, MACH_TRAP_REGS_OFFSET + (S1 * 4)(t1)
    sw		s2, MACH_TRAP_REGS_OFFSET + (S2 * 4)(t1)
    sw		s3, MACH_TRAP_REGS_OFFSET + (S3 * 4)(t1)
    sw		s4, MACH_TRAP_REGS_OFFSET + (S4 * 4)(t1)
    sw		s5, MACH_TRAP_REGS_OFFSET + (S5 * 4)(t1)
    sw		s6, MACH_TRAP_REGS_OFFSET + (S6 * 4)(t1)
    sw		s7, MACH_TRAP_REGS_OFFSET + (S7 * 4)(t1)
    sw		s8, MACH_TRAP_REGS_OFFSET + (S8 * 4)(t1)
    sw		ra, MACH_TRAP_REGS_OFFSET + (RA * 4)(t1)
    sw		t0, MACH_TRAP_REGS_OFFSET + (T0 * 4)(t1)
    sw		t3, MACH_USER_PC_OFFSET(t1)
/*
 * Change to the kernel's stack, enable interrupts and turn off the
 * floating point coprocessor.
 */
    mfc0	s8, MACH_COP_0_STATUS_REG
    lw		sp, MACH_KERN_STACK_END_OFFSET(t1)
    and		s8, s8, ~MACH_SR_COP_1_BIT
    or		t3, s8, MACH_SR_INT_ENA_CUR
    mtc0	t3, MACH_COP_0_STATUS_REG
/*
 * Now fetch the args.  The user's stack pointer is in t2.
 */
    sll		t0, t0, 2
    la		t3, machArgDispatch
    add		t3, t0, t3
    lw		t3, 0(t3)
    nop
    jal		t3
    add		v0, zero, zero
    bne		v0, zero, sysCallReturn
    add		s0, t1, zero			# Save pointer to current state
						#    in s0
/* 
 * We got the args now call the routine.
 */
    lw		s2, proc_RunningProcesses	# s2 <= pointer to running
						#       processes array.
    lw		s1, machKcallTableOffset	# s1 <= Offset of kcall table
						#       in proc table entry.
    lw		s2, 0(s2)			# s2 <= pointer to currently
    nop						#       running process
    add		s3, s2, s1			# s3 <= pointer to kcall table
						#       pointer for currently
						#       running	process
    add		s1, s3, 4			# Special handling flag follows
						# kcallTable field.  Save a 
						# pointer to it in s1.
    lw		s3, 0(s3)			# s3 <= pointer to kcall table
    nop
    add		s3, s3, t0			# s3 <= pointer to pointer to
						#       function to call.
    lw		s3, 0(s3)			# s3 <= pointer to function.
    nop
    jal		s3				# Call the function
    nop
/*
 * Return to the user.  We have following saved information:
 *
 *	s0:	machCurStatePtr
 *	s1:	procPtr->specialHandling
 *	s2:	procPtr
 *	s8:	status register
 */
sysCallReturn:
    mtc0	s8, MACH_COP_0_STATUS_REG	# Disable interrupts.
    nop
    lw		t0, 0(s1)			# Get special handling flag.
    nop
    beq		t0, zero, checkFP		# See if special handling 
    nop						#    required
/*
 * Need some special handling.
 */
    lw		t1, MACH_USER_PC_OFFSET(s0)	# Fetch return PC.
    or		t0, s8, MACH_SR_INT_ENA_CUR	# Prepare to enable interrupts.
    add		t1, t1, 4			# Increment return PC by 4 to
						#    get past the syscall inst.
    sw		t1, MACH_USER_PC_OFFSET(s0)	# Write back the return PC.

    mtc0	t0, MACH_COP_0_STATUS_REG	# Enable interrupts.
    sw		v0, MACH_TRAP_REGS_OFFSET + (V0 * 4)(s0)

    add		a0, s2, zero
    jal		MachUserReturn			# Call MachUserReturn(procPtr)
    nop
/*
 * Restore A0, A1 and A2 because these will get changed if a signal handler
 * is to be called.
 */
    lw		k0, MACH_USER_PC_OFFSET(s0)
    lw		a0, MACH_TRAP_REGS_OFFSET + (A0 * 4)(s0)
    lw		a1, MACH_TRAP_REGS_OFFSET + (A1 * 4)(s0)
    lw		a2, MACH_TRAP_REGS_OFFSET + (A2 * 4)(s0)
/*
 * V1 and A3 are restored for UNIX binary compatibility.
 */
    lw		v1, MACH_TRAP_REGS_OFFSET + (V1 * 4)(s0)
    lw		a3, MACH_TRAP_REGS_OFFSET + (A3 * 4)(s0)

    beq		v0, zero, sysCallRestore
    lw		v0, MACH_TRAP_REGS_OFFSET + (V0 * 4)(s0)
    or		s8, s8, MACH_SR_COP_1_BIT
    j		sysCallRestore
    nop

checkFP:
    lw		k0, MACH_USER_PC_OFFSET(s0)
    lw		t0, machFPCurStatePtr
    add		k0, k0, 4
    bne		t0, s0, sysCallRestore
    nop
    or		s8, s8, MACH_SR_COP_1_BIT

/*
 * Restore the registers.
 */

sysCallRestore:
    mtc0	s8, MACH_COP_0_STATUS_REG
    lw		sp, MACH_TRAP_REGS_OFFSET + (SP * 4)(s0)
    lw		gp, MACH_TRAP_REGS_OFFSET + (GP * 4)(s0)
    lw		s1, MACH_TRAP_REGS_OFFSET + (S1 * 4)(s0)
    lw		s2, MACH_TRAP_REGS_OFFSET + (S2 * 4)(s0)
    lw		s3, MACH_TRAP_REGS_OFFSET + (S3 * 4)(s0)
    lw		s4, MACH_TRAP_REGS_OFFSET + (S4 * 4)(s0)
    lw		s5, MACH_TRAP_REGS_OFFSET + (S5 * 4)(s0)
    lw		s6, MACH_TRAP_REGS_OFFSET + (S6 * 4)(s0)
    lw		s7, MACH_TRAP_REGS_OFFSET + (S7 * 4)(s0)
    lw		s8, MACH_TRAP_REGS_OFFSET + (S8 * 4)(s0)
    lw		ra, MACH_TRAP_REGS_OFFSET + (RA * 4)(s0)
    lw		s0, MACH_TRAP_REGS_OFFSET + (S0 * 4)(s0)
/*
 * Return.
 */
    j		k0
    rfe

.end MachSysCall
.set reorder

/*----------------------------------------------------------------------------
 *
 * UNIXSyscall --
 *
 *	Handle a UNIX system call.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
.set noreorder
    .globl UNIXSyscall
    .ent UNIXSyscall, 0
UNIXSyscall:
/*
 * If we are tracing system calls are we have a signal or long jump return
 * do it the slow way.  Signal and long jump returns are done the slow way
 * because they have to do a full restore.
 */
    lw		k0, machUNIXSyscallTrace
    beq		v0, MACH_UNIX_LONG_JUMP_RETURN, Mach_UserGenException
    nop
    beq		v0, MACH_UNIX_SIG_RETURN, Mach_UserGenException
    nop
    bne		k0, zero, Mach_UserGenException
    nop

    add		t7, gp, zero			# Save the user's gp in t7
    la		gp, _gp				# Switch to the kernel's gp
/*
 * See if this system call is valid.
 */
    lw		t0, machNumUNIXSyscalls		# t0 <= Maximum sys call value.
    nop
    add		t0, t0, 1			
    sltu	t0, v0, t0			# Is v0 < t0 ?	
    bne		t0, zero, 1f			# If so then continue on.
    nop
/*
 * System call number is too big.  Return EINVAL to
 * the user.
 */
    mfc0	t0, MACH_COP_0_EXC_PC
    add		gp, t7, zero
    li		v0, 22
    li		a3, 1
    add		t0, t0, 4
    j		t0
    rfe
/* 
 * Now we know that we have a good system call number so go ahead and
 * save state and switch to the kernel's stack.  Note that we save 
 * a0 - a2 and v1 because UNIX system call stubs assume that these
 * won't get modified unless a value is returned in v1.
 */
1:
    lw		t1, machCurStatePtr
    add		t2, sp, zero
    mfc0	t3, MACH_COP_0_EXC_PC
    sw		sp, MACH_TRAP_REGS_OFFSET + (SP * 4)(t1)
    sw		t7, MACH_TRAP_REGS_OFFSET + (GP * 4)(t1)
    sw		s0, MACH_TRAP_REGS_OFFSET + (S0 * 4)(t1)
    sw		s1, MACH_TRAP_REGS_OFFSET + (S1 * 4)(t1)
    sw		s2, MACH_TRAP_REGS_OFFSET + (S2 * 4)(t1)
    sw		s3, MACH_TRAP_REGS_OFFSET + (S3 * 4)(t1)
    sw		s4, MACH_TRAP_REGS_OFFSET + (S4 * 4)(t1)
    sw		s5, MACH_TRAP_REGS_OFFSET + (S5 * 4)(t1)
    sw		s6, MACH_TRAP_REGS_OFFSET + (S6 * 4)(t1)
    sw		s7, MACH_TRAP_REGS_OFFSET + (S7 * 4)(t1)
    sw		s8, MACH_TRAP_REGS_OFFSET + (S8 * 4)(t1)
    sw		ra, MACH_TRAP_REGS_OFFSET + (RA * 4)(t1)
    sw		a0, MACH_TRAP_REGS_OFFSET + (A0 * 4)(t1)
    sw		a1, MACH_TRAP_REGS_OFFSET + (A1 * 4)(t1)
    sw		a2, MACH_TRAP_REGS_OFFSET + (A2 * 4)(t1)
    sw		v1, MACH_TRAP_REGS_OFFSET + (V1 * 4)(t1)
    sw		t3, MACH_USER_PC_OFFSET(t1)
/*
 * Change to the kernel's stack, enable interrupts and turn off the
 * floating point coprocessor.
 */
    mfc0	s8, MACH_COP_0_STATUS_REG
    lw		sp, MACH_KERN_STACK_END_OFFSET(t1)
    and		s8, s8, ~MACH_SR_COP_1_BIT
    or		t3, s8, MACH_SR_INT_ENA_CUR
    mtc0	t3, MACH_COP_0_STATUS_REG
/*
 * Now fetch the args.  The user's stack pointer is in t2 and the 
 * current state pointer in t1.
 */
    sll		t0, v0, 2	# t0 <= v0 * 4
    sll		t3, v0, 3	# t3 <= v0 * 8
    add		t0, t0, t3	# t0 <= v0 * 12
    la		t3, machUNIXSysCallTable
    add		t0, t0, t3
    lw		t3, 4(t0)	# t3 <= number of arguments.
    add		s3, v0, zero	# Save syscall type in s3.
    sll		t3, t3, 2
    la		t4, machArgDispatchTable
    add		t3, t3, t4
    lw		t3, 0(t3)	# t3 <= pointer to arg fetch routine.
    nop
    jal		t3
    add		v0, zero, zero
    bne		v0, zero, unixSyscallReturn
    add		s0, t1, zero			# Save pointer to current state
						#    in s0

/* 
 * We got the args now call the routine.
 */
    lw		t3, 8(t0)	# t3 <= routine to call.
    sw		zero, MACH_TRAP_UNIX_RET_VAL_OFFSET(s0)
    jal		t3		# Call the routine.
    nop

/*
 * Return to the user.  We have the following saved information:
 *	s0:	machCurStatePtr
 *	s3:	syscall type.
 *	s8:	status register.
 */
unixSyscallReturn:
    lw		s2, proc_RunningProcesses	# s2 <= pointer to running
						#       processes array.
    lw		s1, machKcallTableOffset	# s1 <= Offset of kcall table
						#       in proc table entry.
    lw		s2, 0(s2)			# s2 <= pointer to currently
						#       running process
    add		s1, s1, 4			# Special handling flag follows
						# kcallTable field. 
    add		s1, s2, s1			# s1 <= pointer to special
						#       handling flag.
/*
 * We now have the following saved information:
 *
 *	s0:	machCurStatePtr
 *	s1:	procPtr->specialHandling
 *	s2:	procPtr
 *	s3:	syscall type.
 *	s8:	status register
 */
/*
 * Set up the registers correctly:
 *
 *	1) Restore a0, a1, a2 and v1
 *	2) If status == 0 then regs[a3] <= 0 and v0 <= return value.
 *	   Else regs[A3] <= 1 and v0 <= Compat_MapCode(status).
 */
1:
    bne		v0, zero, 1f
    nop
    lw		v0, MACH_TRAP_UNIX_RET_VAL_OFFSET(s0)
    lw		v1, MACH_TRAP_REGS_OFFSET + (V1 * 4)(s0)
    lw		a0, MACH_TRAP_REGS_OFFSET + (A0 * 4)(s0)
    lw		a1, MACH_TRAP_REGS_OFFSET + (A1 * 4)(s0)
    lw		a2, MACH_TRAP_REGS_OFFSET + (A2 * 4)(s0)
    add		a3, zero, zero
    sw		v0, MACH_TRAP_REGS_OFFSET + (V0 * 4)(s0)
    sw		a3, MACH_TRAP_REGS_OFFSET + (A3 * 4)(s0)
    j		sysCallReturn
    nop
1:
    jal		Compat_MapCode
    add		a0, v0, zero
    sw		v0, MACH_TRAP_UNIX_RET_VAL_OFFSET(s0)
    lw		v1, MACH_TRAP_REGS_OFFSET + (V1 * 4)(s0)
    lw		a0, MACH_TRAP_REGS_OFFSET + (A0 * 4)(s0)
    lw		a1, MACH_TRAP_REGS_OFFSET + (A1 * 4)(s0)
    lw		a2, MACH_TRAP_REGS_OFFSET + (A2 * 4)(s0)
    li		a3, 1
    sw		a3, MACH_TRAP_REGS_OFFSET + (A3 * 4)(s0)
    j		sysCallReturn
    nop

.end MachSysCall
.set reorder

    .globl MachFetchArgs
MachFetchArgs:
/*----------------------------------------------------------------------------
 *
 * MachFetch?Args --
 *
 *	Fetch the given number of arguments from the user's stack and put
 *	them onto the kernel's stack.  The user's stack pointer is in t2.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
.set noreorder
    .globl MachFetch0Args
MachFetch0Args:
    j		ra
    subu	sp, sp, MACH_STAND_FRAME_SIZE

    .globl MachFetch1Arg
MachFetch1Arg:
    lw		s0, 16(t2)	
    subu	sp, sp, MACH_STAND_FRAME_SIZE + 4
    j		ra
    sw		s0, 16(sp)

    .globl MachFetch2Args
MachFetch2Args:
    lw		s0, 16(t2)	
    lw		s1, 20(t2)
    subu	sp, sp, MACH_STAND_FRAME_SIZE + 8
    sw		s0, 16(sp)
    j		ra
    sw		s1, 20(sp)

    .globl MachFetch3Args
MachFetch3Args:
    lw		s0, 16(t2)	
    lw		s1, 20(t2)
    lw		s2, 24(t2)
    subu	sp, sp, MACH_STAND_FRAME_SIZE + 12
    sw		s0, 16(sp)
    sw		s1, 20(sp)
    j		ra
    sw		s2, 24(sp)

    .globl MachFetch4Args
MachFetch4Args:
    lw		s0, 16(t2)	
    lw		s1, 20(t2)
    lw		s2, 24(t2)
    lw		s3, 28(t2)
    subu	sp, sp, MACH_STAND_FRAME_SIZE + 16
    sw		s0, 16(sp)
    sw		s1, 20(sp)
    sw		s2, 24(sp)
    j		ra
    sw		s3, 28(sp)

    .globl MachFetch5Args
MachFetch5Args:
    lw		s0, 16(t2)	
    lw		s1, 20(t2)
    lw		s2, 24(t2)
    lw		s3, 28(t2)
    lw		s4, 32(t2)
    subu	sp, sp, MACH_STAND_FRAME_SIZE + 20
    sw		s0, 16(sp)
    sw		s1, 20(sp)
    sw		s2, 24(sp)
    sw		s3, 28(sp)
    j		ra
    sw		s4, 32(sp)

    .globl MachFetch6Args
MachFetch6Args:
    lw		s0, 16(t2)	
    lw		s1, 20(t2)
    lw		s2, 24(t2)
    lw		s3, 28(t2)
    lw		s4, 32(t2)
    lw		s5, 36(t2)
    subu	sp, sp, MACH_STAND_FRAME_SIZE + 24
    sw		s0, 16(sp)
    sw		s1, 20(sp)
    sw		s2, 24(sp)
    sw		s3, 28(sp)
    sw		s4, 32(sp)
    j		ra
    sw		s5, 36(sp)

.set reorder

    .globl MachFetchArgsEnd
MachFetchArgsEnd:

/*----------------------------------------------------------------------------
 *
 * MachProbeAddr --
 *
 *	Fetch the given number of arguments from the user's stack and put
 *	them onto the kernel's stack.  The user's stack pointer is in t2.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl Mach_ProbeAddr
Mach_ProbeAddr:
.set noreorder
    sw		zero, 0(a0)
    lw		t0, 0(a0)
    nop
    j		ra
    add		v0, zero, zero
.set reorder

    .globl MachProbeAddrEnd
MachProbeAddrEnd:
