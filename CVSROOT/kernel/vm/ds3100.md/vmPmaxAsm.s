/* vmPmaxAsm.s -
 *
 *	Subroutines to access PMAX virtual memory mapping hardware.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#include "vmPmaxConst.h"
#include "machAsmDefs.h"
#include <regdef.h>


/*
 *--------------------------------------------------------------------------
 *
 * VmMachWriteTLB --
 *
 *	Write the given entry into the TLB.
 *
 *	VmMachWriteTLB(lowEntry, highEntry)
 *	    unsigned	lowEntry;
 *	    unsigned	highEntry;
 *
 *	Results:
 *	    Returns the old index corresponding to the high register.
 *
 *	Side effects:
 *	    TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(VmMachWriteTLB)
.set noreorder
    mfc0	t0, VMMACH_TLB_HI	# Save the high register because this
    nop					#   contains the current PID.
    mtc0	a1, VMMACH_TLB_HI	# Store into the high register.
    nop
    tlbp				# Probe for value.
    mfc0	v0, VMMACH_TLB_INDEX	# See what index we got.  This value
					#   is returned as the result of this
					#   procedure.
    mtc0	a0, VMMACH_TLB_LOW	# Set the low register.
    nop
    bltz	v0, 1f			# index < 0 means not found
    nop
    tlbwi				# Reuse the entry that we found
    mtc0	t0, VMMACH_TLB_HI	# Restore the PID.
    j		ra			
    nop
1:  tlbwr				# Write a random entry.
    mtc0	t0, VMMACH_TLB_HI	# Restore the PID
    j		ra
    nop
.set reorder
END(VmMachWriteTLB)


/*
 *--------------------------------------------------------------------------
 *
 * VmMachWriteIndexedTLB --
 *
 *	Write the given entry into the TLB at the given index.
 *
 *	VmMachWriteTLB(index, lowEntry, highEntry)
 *	    unsigned	index;
 *	    unsigned	lowEntry;
 *	    unsigned	highEntry;
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	    TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(VmMachWriteIndexedTLB)
.set noreorder
    mfc0	t0, VMMACH_TLB_HI	# Save the high register because this
					#   contains the current PID.

    sll		a0, a0, VMMACH_TLB_INDEX_SHIFT
    mtc0	a0, VMMACH_TLB_INDEX	# Set the index.
    mtc0	a1, VMMACH_TLB_LOW	# Set up entry low.
    mtc0	a2, VMMACH_TLB_HI	# Set up entry high.
    nop
    tlbwi				# Write the TLB

    mtc0	t0, VMMACH_TLB_HI	# Restore the PID.

    j		ra
    nop
.set reorder
END(VmMachWriteIndexedTLB)

/* 
 *--------------------------------------------------------------------------
 *
 * VmMachFlushPIDFromTLB --
 *
 *	Flush all pages for the given PID from the TLB.
 *
 *	VmMachFlushPIDFromTLB(pid)
 *	    int		pid;
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	    All entries corresponding to this PID are flushed.
 *
 *--------------------------------------------------------------------------
 */
LEAF(VmMachFlushPIDFromTLB)
.set noreorder
    subu	sp, sp, STAND_FRAME_SIZE
    sw		ra, STAND_RA_OFFSET(sp)
    sw		a0, STAND_FRAME_SIZE(sp)
    .mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

    mfc0	t0, VMMACH_TLB_HI		# Save the PID
    sll		a0, a0, VMMACH_TLB_PID_SHIFT	# Align the pid to flush.

/*
 * Align the starting value (t1), the increment (t2) and the upper bound (t3).
 */
    li		t1, VMMACH_FIRST_RAND_ENTRY << VMMACH_TLB_INDEX_SHIFT
    li		t2, 1 << VMMACH_TLB_INDEX_SHIFT
    li		t3, VMMACH_NUM_TLB_ENTRIES << VMMACH_TLB_INDEX_SHIFT

1:  mtc0	t1, VMMACH_TLB_INDEX		# Set the index register
    addu	t1, t1, t2			# Increment index.
    tlbr					# Read from the TLB	
    mfc0	t4, VMMACH_TLB_HI		# Fetch the hi register.
    nop
    and		t4, t4, a0			# See if the pids match.
    bne		t4, a0, 2f
    li		t4, VMMACH_PHYS_CACHED_START
    mtc0	t4, VMMACH_TLB_HI		# Mark entry high as invalid
    mtc0	zero, VMMACH_TLB_LOW		# Zero out entry low.
    nop
    tlbwi					# Write the entry.
2:
    bne		t1, t3, 1b
    nop

    mtc0	t0, VMMACH_TLB_HI
    addu	sp, sp, STAND_FRAME_SIZE

    j		ra
    nop
.set reorder
END(VmMachFlushPIDFromTLB)


/* 
 *--------------------------------------------------------------------------
 *
 * VmMachFlushTLB --
 *
 *	Flush the TLB.
 *
 *	VmMachFlushTLB()
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	   The TLB is flushed.
 *
 *--------------------------------------------------------------------------
 */
LEAF(VmMachFlushTLB)
.set noreorder
    subu	sp, sp, STAND_FRAME_SIZE
    sw		ra, STAND_RA_OFFSET(sp)
    .mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

    mfc0	t0, VMMACH_TLB_HI		# Save the PID
    li		t1, VMMACH_PHYS_CACHED_START
    mtc0	t1, VMMACH_TLB_HI		# Invalidate hi entry.
    mtc0	zero, VMMACH_TLB_LOW		# Zero out entry low.

/*
 * Align the starting value (t1), the increment (t2) and the upper bound (t3).
 */
    li		t1, VMMACH_FIRST_RAND_ENTRY << VMMACH_TLB_INDEX_SHIFT
    li		t2, 1 << VMMACH_TLB_INDEX_SHIFT
    li		t3, VMMACH_NUM_TLB_ENTRIES << VMMACH_TLB_INDEX_SHIFT

1:  mtc0	t1, VMMACH_TLB_INDEX		# Set the index register.
    addu	t1, t1, t2			# Increment index.
    tlbwi					# Write the TLB entry.
    bne		t1, t3, 1b
    nop

    mtc0	t0, VMMACH_TLB_HI		# Restore the PID
    addu	sp, sp, STAND_FRAME_SIZE

    j		ra
    nop
.set reorder
END(VmMachFlushTLB)


/* 
 *--------------------------------------------------------------------------
 *
 * VmMachFlushPageFromTLB --
 *
 *	Flush the given page from the TLB for the given process.
 *
 *	VmMachFlushPageFromTLB(pid, page)
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	   The process's page is flushed from the TLB.
 *
 *--------------------------------------------------------------------------
 */
LEAF(VmMachFlushPageFromTLB)
.set noreorder
    mfc0	t0, VMMACH_TLB_HI			# Save PID

    sll		a0, a0, VMMACH_TLB_PID_SHIFT		# Align pid
    sll		a1, a1, VMMACH_TLB_VIRT_PAGE_SHIFT	# Align virt page
    or		t1, a0, a1				# Set up entry.
    mtc0	t1, VMMACH_TLB_HI			# Put into high reg.
    nop
    tlbp						# Probe for the entry.
    mfc0	v0, VMMACH_TLB_INDEX			# See what we got
    li		t1, VMMACH_PHYS_CACHED_START		# Load invalid entry.
    bltz	v0, 1f					# index < 0 => !found
    nop
    mtc0	t1, VMMACH_TLB_HI			# Prepare index hi.
    mtc0	zero, VMMACH_TLB_LOW			# Prepare index lo.
    nop
    tlbwi

1:
    mtc0	t0, VMMACH_TLB_HI			# Restore the PID
    j		ra
    nop

.set reorder
END(VmMachFlushPageFromTLB)


/* 
 *--------------------------------------------------------------------------
 *
 * VmMachClearTLBModBit --
 *
 *	Clear the modified bit for the given <pid, page> pair.
 *
 *	VmMachClearTLBModBit(pid, page)
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	   The process's page is flushed from the TLB.
 *
 *--------------------------------------------------------------------------
 */
LEAF(VmMachClearTLBModBit)
.set noreorder
    mfc0	t0, VMMACH_TLB_HI			# Save PID

    sll		a0, a0, VMMACH_TLB_PID_SHIFT		# Align pid
    sll		a1, a1, VMMACH_TLB_VIRT_PAGE_SHIFT	# Align virt page
    or		t1, a0, a1				# Set up entry.
    mtc0	t1, VMMACH_TLB_HI			# Put into high reg.
    nop
    tlbp						# Probe for the entry.
    mfc0	v0, VMMACH_TLB_INDEX			# See what we got
    nop
    bltz	v0, 1f					# index < 0 => !found
    nop
    tlbr
    mfc0	t1, VMMACH_TLB_LOW			# Read out low.
    nop
    and		t1, t1, ~VMMACH_TLB_MOD_BIT
    mtc0	t1, VMMACH_TLB_LOW			# Write index low.
    nop
    tlbwi						# Write the TLB entry.

1:
    mtc0	t0, VMMACH_TLB_HI			# Restore the PID
    j		ra
    nop

.set reorder
END(VmMachClearTLBModBit)

#ifdef notdef

/* 
 *--------------------------------------------------------------------------
 *
 * VmMachFlushGlobalPageFromTLB --
 *
 *	Flush the given page from the TLB for the given process.
 *
 *	VmMachFlushPageFromTLB(pid, page)
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	   The process's page is flushed from the TLB.
 *
 *--------------------------------------------------------------------------
 */
LEAF(VmMachFlushGlobalPageFromTLB)
.set noreorder
    subu	sp, sp, STAND_FRAME_SIZE
    sw		ra, STAND_RA_OFFSET(sp)
    .mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

    mfc0	t0, VMMACH_TLB_HI			# Save PID

    sll		t1, a0, VMMACH_TLB_VIRT_PAGE_SHIFT	# Align virt page

    addu	t2, zero, zero
    li		t3, VMMACH_NUM_TLB_ENTRIES << VMMACH_TLB_INDEX_SHIFT
    li		v0, 0x80000000

6:  mtc0	t2, VMMACH_TLB_INDEX			# Set index
    nop
    tlbr						# Read an entry.
    mfc0	t4, VMMACH_TLB_HI
    nop
    and		t4, t4, VMMACH_TLB_VIRT_PAGE_NUM
    bne		t4, t1, 2f				# test for pid/vpn match
    nop

    addu	v0, t2, zero
    li		t4, VMMACH_PHYS_CACHED_START
    mtc0	t4, VMMACH_TLB_HI			# Prepare index hi
    mtc0	zero, VMMACH_TLB_LOW			# Prepare index lo
    nop
    tlbwi						# Invalidate entry
2:
    addu	t2, t2, 1 << VMMACH_TLB_INDEX_SHIFT
    bne		t2, t3, 6b
    nop

4:  mtc0	t0, VMMACH_TLB_HI	
    addu	sp, sp, STAND_FRAME_SIZE
    j		ra
    nop

.set reorder
END(VmMachFlushGlobalPageFromTLB)
#endif

/* 
 *--------------------------------------------------------------------------
 *
 * VmMachDumpTLB --
 *
 *	Flush the TLB.
 *
 *	VmMachFlushTLB(tlbPtr)
 *	    unsigned *tlbPtr;
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	   The contents of the TLB are written to *tlbPtr.
 *
 *--------------------------------------------------------------------------
 */
LEAF(Vm_MachDumpTLB)
.set noreorder
    subu	sp, sp, STAND_FRAME_SIZE
    sw		ra, STAND_RA_OFFSET(sp)
    .mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

    mfc0	t0, VMMACH_TLB_HI		# Save the PID

/*
 * Align the starting value (t1), the increment (t2) and the upper bound (t3).
 */
    li		t1, 0
    li		t2, 1 << VMMACH_TLB_INDEX_SHIFT
    li		t3, VMMACH_NUM_TLB_ENTRIES << VMMACH_TLB_INDEX_SHIFT

1:  mtc0	t1, VMMACH_TLB_INDEX		# Set the index register.
    addu	t1, t1, t2			# Increment index.
    tlbr					# Read the TLB entry.
    mfc0	t4, VMMACH_TLB_LOW
    mfc0	t5, VMMACH_TLB_HI
    sw		t4, 0(a0)
    sw		t5, 4(a0)
    addu	a0, a0, 8
    bne		t1, t3, 1b
    nop

    mtc0	t0, VMMACH_TLB_HI		# Restore the PID
    addu	sp, sp, STAND_FRAME_SIZE

    j		ra
    nop
.set reorder
END(Vm_MachDumpTLB)

/* 
 *--------------------------------------------------------------------------
 *
 * VmMachSetPID --
 *
 *	Write the given pid into the TLB pid reg.
 *
 *	VmMachWritePID(pid)
 *	    int		pid;
 *
 *	Results:
 *	    None.
 *
 *	Side effects:
 *	    PID set in the entry hi register.
 *
 *--------------------------------------------------------------------------
 */
    .globl VmMachSetPID
VmMachSetPID:
    sll		a0, a0, 6		# Reg 4 <<= 6 to put PID in right spot
    mtc0	a0, VMMACH_TLB_HI	# Write the hi reg value
    j		ra


/*
 * ----------------------------------------------------------------------
 *
 * Vm_Copy{In,Out}
 *
 *	Copy numBytes from *sourcePtr in to *destPtr.
 *	This routine is optimized to do transfers when sourcePtr and 
 *	destPtr are both 4-byte aligned.
 *
 *	ReturnStatus
 *	Vm_Copy{In,Out}(numBytes, sourcePtr, destPtr)
 *	    register int numBytes;      * The number of bytes to copy *
 *	    Address sourcePtr;          * Where to copy from. *
 *	    Address destPtr;            * Where to copy to. *
 *
 *	NOTE: The trap handler assumes that this routine does not push anything
 *	      onto the stack.  It uses this fact to allow it to return to the
 *	      caller of this routine upon an address fault.  If you must push
 *	      something onto the stack then you had better go and modify 
 *	      "CallTrapHandler" in asmDefs.h appropriately.
 *
 * Results:
 *	Returns SUCCESS if the copy went OK (which is almost always).  If
 *	a bus error (other than a page fault) occurred while reading or
 *	writing user memory, then SYS_ARG_NO_ACCESS is returned (this return
 *	occurs from the trap handler, rather than from this procedure).
 *
 * Side effects:
 *	The area that destPtr points to is modified.
 *
 * ----------------------------------------------------------------------
 */
    .globl VmMachDoCopy
    .globl Vm_CopyIn
VmMachDoCopy:
Vm_CopyIn:
/*
 * The number of bytes was passed in in r4, the source in r5, and the dest
 * in r6.
 *
 * If the source or dest are not 4 byte aligned then everything must be
 * done as byte copies.
 */

gotArgs:
    or		t0, a1, a2
    andi	t0, t0, 3
    blez	t0, 1f
    j		3f

/*
 * Do as many 64-byte copies as possible.
 */

1:
    subu	t0, a0, 64
    bltz	t0, 2f
    lw		t0, 0(a1)
    lw		t1, 4(a1)
    sw		t0, 0(a2)
    sw		t1, 4(a2)
    lw		t0, 8(a1)
    lw		t1, 12(a1)
    sw		t0, 8(a2)
    sw		t1, 12(a2)
    lw		t0, 16(a1)
    lw		t1, 20(a1)
    sw		t0, 16(a2)
    sw		t1, 20(a2)
    lw		t0, 24(a1)
    lw		t1, 28(a1)
    sw		t0, 24(a2)
    sw		t1, 28(a2)
    lw		t0, 32(a1)
    lw		t1, 36(a1)
    sw		t0, 32(a2)
    sw		t1, 36(a2)
    lw		t0, 40(a1)
    lw		t1, 44(a1)
    sw		t0, 40(a2)
    sw		t1, 44(a2)
    lw		t0, 48(a1)
    lw		t1, 52(a1)
    sw		t0, 48(a2)
    sw		t1, 52(a2)
    lw		t0, 56(a1)
    lw		t1, 60(a1)
    sw		t0, 56(a2)
    sw		t1, 60(a2)
    subu	a0, a0, 64
    addu	a1, a1, 64
    addu	a2, a2, 64
    j		1b

/*
 * Do 4 byte copies.
 */
2:
    subu	t0, a0, 4
    bltz	t0, 3f
    lw		t0, 0(a1)
    sw		t0, 0(a2)
    subu	a0, a0, 4
    addu	a1, a1, 4
    addu	a2, a2, 4
    j		2b

/*
 * Do one byte copies until done.
 */

3:
    beq		a0, zero, 4f
    lb		t0, 0(a1)
    sb		t0, 0(a2)
    subu	a0, a0, 1
    addu	a1, a1, 1
    addu	a2, a2, 1
    j		3b

/* 
 * Return.
 */
4: 
    li		v0, 0
    j		ra

/* 
 * Vm_CopyOut is just like Vm_CopyIn except that it checks to make sure
 * that the destination is in the user area (otherwise this would be a
 * trap door to write to kernel space).
 */

    .globl Vm_CopyOut
.ent Vm_CopyOut
Vm_CopyOut:
/*
 * If a2 is < 0 then it is has the sign bit set which means its in the
 * kernel's VAS.
 */
    bltz	a2, 5f
    j		gotArgs

/*
 * User address out of range.  Check for a zero byte count before
 * returning an error, though;  there appear to be kernel routines
 * that call Vm_CopyOut with a zero count but bogus other arguments.
 */

5:
    blez	a0, 6f
    lui		v0, 2
    j		ra
6:  li		v0, 0
    j		ra
.end


/*
 * ----------------------------------------------------------------------
 *
 * Vm_StringNCopy
 *
 *	Copy the NULL terminated string from *sourcePtr to *destPtr up
 *	numBytes worth of bytes.
 *
 *	void
 *	Vm_StringNCopy(numBytes, sourcePtr, destPtr, bytesCopiedPtr)
 *	    register int numBytes;      * The number of bytes to copy *
 *	    Address sourcePtr;          * Where to copy from. *
 *	    Address destPtr;            * Where to copy to. *
 *	    int	*bytesCopiedPtr;	* Number of bytes copied. *
 *
 *	NOTE: The trap handler assumes that this routine does not push anything
 *	      onto the stack.  It uses this fact to allow it to return to the
 *	      caller of this routine upon an address fault.  If you must push
 *	      something onto the stack then you had better go and modify 
 *	      "CallTrapHandler" in asmDefs.h appropriately.
 *
 * Results:
 *	Normally returns SUCCESS.  If a non-recoverable bus error occurs,
 *	then the trap handler fakes up a SYS_ARG_NO_ACCESS return from
 *	this procedure.
 *
 * Side effects:
 *	The area that destPtr points to is modified and *bytesCopiedPtr 
 *	contains the number of bytes copied.
 *
 * ----------------------------------------------------------------------
 */
    .globl  Vm_StringNCopy
Vm_StringNCopy:
    addu	t2, a0, 0		# Save the number of bytes
1:
    lb		t0, 0(a1)
    sb		t0, 0(a2)
    beq		t0, zero, 2f
    addu	a1, a1, 1
    addu	a2, a2, 1
    subu	a0, a0, 1
    bne		a0, zero, 1b
2: 
    subu	a0, t2, a0
    sw		a0, 0(a3)
    li		v0, 0
    j		ra


/*
 * ----------------------------------------------------------------------
 *
 * Vm_TouchPages --
 *
 *	Touch the range of pages.
 *
 *	void
 *	Vm_TouchPages(firstPage, numPages)
 *	    int	firstPage;	* First page to touch. *
 *	    int	numPages;	* Number of pages to touch. *
 *
 *	NOTE: The trap handler assumes that this routine does not push anything
 *	      onto the stack.  It uses this fact to allow it to return to the
 *	      caller of this routine upon an address fault.
 *
 * Results:
 *	Returns SUCCESS if were able to touch the page (which is almost
 *	always).  If a bus error (other than a page fault) occurred while 
 *	reading user memory, then SYS_ARG_NO_ACCESS is returned (this return
 *	occurs from the trap handler, rather than from this procedure).
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
    .globl Vm_TouchPages
Vm_TouchPages:
    sll		a0, a0, VMMACH_PAGE_SHIFT
1:
    beq		a1, zero, 2f
    lw		t0, 0(a0)
    subu	a1, a1, 1
    addu	a0, a0, VMMACH_PAGE_SIZE
    j		1b
2:
    j		ra

    .globl VmMachCopyEnd
VmMachCopyEnd:

/*----------------------------------------------------------------------------
 *
 * VmMach_UTLBMiss --
 *
 *	Handle a user TLB miss.
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
    .globl VmMach_UTLBMiss
    .ent VmMach_UTLBMiss
VmMach_UTLBMiss:
.set noat
.set noreorder
    mfc0	k0, VMMACH_TLB_HI			# Get the high val.
    lui		k1, 0x8000			
    sw		AT, VMMACH_SAVED_AT_OFFSET(k1)
#ifndef NO_COUNTERS
    lw		AT, VMMACH_UTLB_COUNT_OFFSET(k1)
    nop	
    addu	AT, AT, 1
    sw		AT, VMMACH_UTLB_COUNT_OFFSET(k1)
#endif

/*
 * Make the hash key.
 */
    srl		k1, k0, VMMACH_PAGE_HASH_SHIFT
    sll		AT, k0, VMMACH_PID_HASH_SHIFT
    xor		k1, k1, AT
.set at
    and		k1, k1, VMMACH_HASH_MASK
.set noat
/*
 * Load the hash table key.
 */
    la		AT, vmMachTLBHashTable
    addu	AT, k1, AT
    lw		k1, VMMACH_HASH_KEY_OFFSET(AT)
    nop

/*
 * Check for match.
 */
    beq		k1, k0, 1f
    lw		k1, VMMACH_LOW_REG_OFFSET(AT)
    j		SlowUTLBFault
/*
 * We have a match.  Restore AT and write the TLB entry.
 */
1:
    lui		AT, 0x8000
    mtc0	k1, VMMACH_TLB_LOW
    mfc0	k0, MACH_COP_0_EXC_PC
    tlbwr
#ifndef NO_COUNTERS
    lw		k1, VMMACH_UTLB_HIT_OFFSET(AT)
    nop
    addu	k1, k1, 1
    sw		k1, VMMACH_UTLB_HIT_OFFSET(AT)
#endif
    lw		AT, VMMACH_SAVED_AT_OFFSET(AT)
    j		k0
    rfe
    .end VmMach_UTLBMiss

    .globl VmMach_EndUTLBMiss
VmMach_EndUTLBMiss:
.set reorder

/*
 * We couldn't find a TLB entry.  Find out what mode we came from and call
 * the appropriate handler.
 */
SlowUTLBFault:
    lui		AT, 0x8000
    lw		AT, VMMACH_SAVED_AT_OFFSET(AT)
.set reorder
    mfc0	k0, MACH_COP_0_STATUS_REG
    nop
    and		k0, k0, MACH_SR_KU_PREV
    bne		k0, zero, 1f
    j		Mach_KernGenException
1:  j		Mach_UserGenException

.set at

/*----------------------------------------------------------------------------
 *
 * VmMach_KernTLBException --
 *
 *	Handle a kernel TLB fault.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl VmMach_KernTLBException
    .ent VmMach_KernTLBException, 0
VmMach_KernTLBException:
.set noreorder
.set noat
    mfc0	k0, MACH_COP_0_BAD_VADDR	# Get the faulting address
    addu	k1, sp, zero
    srl		k1, k1, VMMACH_PAGE_SHIFT
    srl		k0, k0, VMMACH_PAGE_SHIFT
    bne		k0, k1, 1f
    nop
    jal		PrintError
    nop

1:
    mfc0	k0, MACH_COP_0_BAD_VADDR	# Get the faulting address
    li		k1, VMMACH_VIRT_CACHED_START	# Get the lower bound
    sltu	k1, k0, k1			# Compare to the lower bound
    bne		k1, zero, KernGenException	#    and take a normal exception
    nop						#    if we are lower
    li		k1, MACH_KERN_END		# Get the upper bound
    sltu	k1, k0, k1			# Compare to the upper bound
    beq		k1, zero, KernGenException	#    and take a normal exception
						#    if we are higher.
    srl		k0, VMMACH_PAGE_SHIFT		# Get the virtual page number.
    li		k1, VMMACH_VIRT_CACHED_START_PAGE
    subu	k0, k0, k1
    lw		k1, vmMach_KernelTLBMap		# Get to map that contains
						#     the TLB entry.
    sll		k0, k0, 2			# Each entry is four bytes
    addu	k0, k0, k1			# Get pointer to entry.
    lw		k0, 0(k0)			# Get entry
    nop
    beq		k0, zero, KernGenException	# If entry is zero then take
						#    a normal exception.
    mtc0	k0, VMMACH_TLB_LOW		# Set the low entry.
    mfc0	k1, MACH_COP_0_EXC_PC		# Get the exception PC
    tlbwr					# Write the entry out.
    j		k1
    rfe

KernGenException:
    j		Mach_KernGenException
    nop

.end VmMach_KernTLBException
.set at
.set reorder

/*----------------------------------------------------------------------------
 *
 * VmMach_TLBModException --
 *
 *	Handle a modified exception.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl VmMach_TLBModException
    .ent VmMach_TLBModException, 0
VmMach_TLBModException:
.set noat
.set noreorder
    mfc0	k0, VMMACH_TLB_HI			# Get the high val.
    lui		k1, 0x8000
    sw		AT, VMMACH_SAVED_AT_OFFSET(k1)
#ifndef NO_COUNTERS
    lw		AT, VMMACH_MOD_COUNT_OFFSET(k1)
    nop	
    addu	AT, AT, 1
    sw		AT, VMMACH_MOD_COUNT_OFFSET(k1)
#endif

/*
 * Make the hash key.
 */
    srl		k1, k0, VMMACH_PAGE_HASH_SHIFT
    sll		AT, k0, VMMACH_PID_HASH_SHIFT
    xor		k1, k1, AT
.set at
    and		k1, k1, VMMACH_HASH_MASK
.set noat
/*
 * Load the hash table key.
 */
    la		AT, vmMachTLBHashTable
    addu	AT, k1, AT
    lw		k1, VMMACH_HASH_KEY_OFFSET(AT)
    nop

/*
 * Check for match.
 */
    beq		k1, k0, 1f
    nop
    j		SlowModFault
    nop
/*
 * We have a match.  See if we can write this page.
 */
1:  
#ifndef NO_COUNTERS
    lui		k0, 0x8000
    lw		k1, VMMACH_MOD_HIT_OFFSET(k0)
    nop
    addu	k1, k1, 1
    sw		k1, VMMACH_MOD_HIT_OFFSET(k0)
#endif

    lw		k1, VMMACH_LOW_REG_OFFSET(AT)
    nop
    and		k0, k1, VMMACH_TLB_ENTRY_WRITEABLE
    beq		k0, zero, SlowModFault
    nop
/*
 * Find the entry in the TLB.  It has to be there since we took a mod fault
 * on it.  While were at it set the mod bit in the hash table entry.
 */
    tlbp
    mfc0	k0, VMMACH_TLB_INDEX
    or		k1, k1, VMMACH_TLB_MOD_BIT
    sw		k1, VMMACH_LOW_REG_OFFSET(AT)
    bltz	k0, 1f
    nop
/*
 * Write the TLB.
 */
    mtc0	k1, VMMACH_TLB_LOW
    nop
    tlbwi
/*
 * Set the modified bit in the physical page array.
 */
.set at
    lw		k0, vmMachPhysPageArr
    srl		k1, k1, VMMACH_TLB_PHYS_PAGE_SHIFT
    sll		k1, k1, 2
    addu	k0, k0, k1
    lw		k1, 0(k0)
    nop
    or		k1, k1, 4
    sw		k1, 0(k0)
.set noat
/* 
 * Restore AT and return.
 */
    mfc0	k0, MACH_COP_0_EXC_PC
    lui		AT, 0x8000
    lw		AT, VMMACH_SAVED_AT_OFFSET(AT)
    j		k0
    rfe
1:
    break	0
.set reorder

/*
 * We couldn't find a TLB entry.  Find out what mode we came from and call
 * the appropriate handler.
 */
SlowModFault:
    lui		AT, 0x8000
    lw		AT, VMMACH_SAVED_AT_OFFSET(AT)
.set reorder
    mfc0	k0, MACH_COP_0_STATUS_REG
    nop
    and		k0, k0, MACH_SR_KU_PREV
    bne		k0, zero, 1f
    j		Mach_KernGenException
1:  j		Mach_UserGenException

.end VmMach_TLBModException
.set at

