/* vmSysCall.c -
 *
 * 	This file contains routines that handle virtual memory system
 *	calls.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */


#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vm.h"
#include "vmInt.h"
#include "user/vm.h"
#include "sync.h"
#include "sys.h"
#include "machineConst.h"


/*
 *----------------------------------------------------------------------
 *
 * Vm_PageSize --
 *
 *	Return the hardware page size.
 *
 * Results:
 *	The size of a hardware page.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_PageSize(pageSizePtr)
    int	*pageSizePtr;
{
    int			pageSize = VM_PAGE_SIZE;

    if (Vm_CopyOut(4, (Address) &pageSize, (Address) pageSizePtr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    return(SUCCESS);
}


/***************************************************************************
 *
 *	The following two routines, Vm_CreateVA and Vm_DestroyVA, are used
 *	to allow users to add or delete ranges of valid virtual addresses
 *	from their virtual address space.  Since neither of these routiens
 *	are monitored (although they call monitored routines), there are 
 *	potential synchronization problems for users sharing memory who
 *	expand their heap segment.  The problems are caused if two or more
 *	users attempt to simultaneously change the size of the heap segment 
 *	to different sizes. The virtual memory system will not have any 
 *	problems handling the conflicting requests, however the actual range 
 *	of valid virtual addresses is unpredictable.  It is up to the user
 *	to synchronize when he is expanding his HEAP segment.
 */


/*
 *----------------------------------------------------------------------
 *
 * Vm_CreateVA --
 *
 *	Validate the given range of addresses.  If necessary the segment
 *	is expanded to make room.
 *
 * Results:
 *	VM_WRONG_SEG_TYPE if tried to validate addresses for a stack or
 *	code segment and VM_SEG_TOO_LARGE if the segment cannot be
 *	expanded to fill the size.
 *
 * Side effects:
 *	The page tables are modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_CreateVA(address, size)
    Address address;	/* Address to validate from. */
    int	    size;	/* Number of bytes to validate. */
{
    register Vm_Segment *segPtr;
    int			firstPage, lastPage;
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    firstPage = (unsigned) (address) >> VM_PAGE_SHIFT;
    lastPage = (unsigned) ((int) address + size - 1) >> VM_PAGE_SHIFT;

    segPtr = (Vm_Segment *) procPtr->vmPtr->segPtrArray[VM_HEAP];

    /*
     * Make sure that the beginning address falls into the heap segment and
     * not the code segment.
     */
    if (firstPage < segPtr->offset) {
	return(VM_WRONG_SEG_TYPE);
    }

    /*
     * Make sure that the ending page is not greater than the highest
     * possible page.  Since there must be one stack page and one page
     * between the stack and the heap, the highest possible heap page is
     * MACH_LAST_USER_STACK_PAGE - 2.
     */
    if (lastPage > MACH_LAST_USER_STACK_PAGE - 2) {
	return(VM_SEG_TOO_LARGE);
    }

    /*
     * Make pages between firstPage and lastPage part of the heap segment's
     * virtual address space.
     */
    return(VmAddToSeg(segPtr, firstPage, lastPage));
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_DestroyVA --
 *
 *	Invalidate the given range of addresses.  If the starting address
 *	is too low then an error message is returned.
 *
 * Results:
 *	VM_WRONG_SEG_TYPE if tried to invalidate addresses for a code.
 *
 * Side effects:
 *	The page tables are modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Vm_DestroyVA(address, size)
    Address address;	/* Address to validate from. */
    int	    size;	/* Number of bytes to validate. */
{
    register Vm_Segment *segPtr;
    int			firstPage, lastPage;
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    firstPage = (unsigned) (address) >> VM_PAGE_SHIFT;
    lastPage = (unsigned) ((int) address + size - 1) >> VM_PAGE_SHIFT;

    segPtr = (Vm_Segment *) procPtr->vmPtr->segPtrArray[VM_HEAP];

    /*
     * Make sure that the beginning address falls into the 
     * heap segment.
     */
    if (firstPage < segPtr->offset) {
	return(VM_WRONG_SEG_TYPE);
    }

    /*
     * Make pages between firstPage and lastPage not members of the segment's
     * virtual address spac.e
     */
    VmDeleteFromSeg(segPtr, firstPage, lastPage);

    return(SUCCESS);
}
