/*
 * vm.h --
 *
 *	Public declarations for Sprite VM module.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vm.h,v 1.17 92/06/04 14:10:55 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VM
#define _VM

#include <sprite.h>
#include <ckalloc.h>
#include <mach.h>

#include <fs.h>
#include <vmTypes.h>
#include <proc.h>

extern int vm_MaxPendingRequests;
extern Boolean vm_StickySegments;


/*
 *----------------------------------------------------------------------
 *
 * Vm_ByteToPage --
 *
 *	Convert a byte address or offset to a page number.
 *	XXX This should probably be VmMach_ByteToPage, and it should 
 *	probably be defined to do a right shift, using the constants 
 *	from <mach/machine/vm_param.h>.
 *
 * Results:
 *	Returns the given number, divided by the page size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define Vm_ByteToPage(num)	((num)/vm_page_size)


/*
 *----------------------------------------------------------------------
 *
 * Vm_PageToByte --
 *
 *	Convert a page number to a byte address or offset.
 *	XXX This should probably be VmMach_ByteToPage, and it should 
 *	probably be defined to do a left shift, using the constants 
 *	from <mach/machine/vm_param.h>.
 *
 * Results:
 *	Returns the given number, multiplied by the page size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define Vm_PageToByte(num)	((num)*vm_page_size)


/*
 *----------------------------------------------------------------------
 *
 * Vm_RoundPage --
 *
 *	Round an address to the next page boundary.
 *
 * Results:
 *	Returns the given address, rounded up to the start of a page 
 *	boundary.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define Vm_RoundPage(addr)	((Address)round_page((vm_address_t)(addr)))



/*
 *----------------------------------------------------------------------
 *
 * Vm_TruncPage --
 *
 *	Return the starting address of the page that contains the 
 *	given address.
 *
 * Results:
 *	Returns the given address, truncated to the start of a page 
 *	boundary.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define Vm_TruncPage(addr)	((Address)trunc_page((vm_address_t)(addr)))


/* 
 * Aliases for compatibility with old code.
 */

#define Vm_BootAlloc(howMuch)	ckalloc(howMuch)
#define Vm_RawAlloc(howMuch)	ckalloc(howMuch)

/*
 * Maximum number of processes to handle paging requests.  This information
 * is needed in order to configure the correct number of Proc_ServerProcs.
 * Native Sprite allows for 3 page-out procs.  We'll allow for the same 
 * number of page-in procs, plus another 3 procs for handling miscellaneous 
 * paging requests.
 */
#define VM_MAX_PAGE_OUT_PROCS	9


#ifdef SPRITED

extern	void		Vm_AddHeapInfo _ARGS_((Vm_Segment *segPtr,
				Fs_Stream *initFilePtr, char *initFileName,
				Proc_ObjInfo *execInfoPtr));
extern	void		Vm_CleanupSharedFile _ARGS_((
				Proc_ControlBlock *procPtr, 
				Fs_Stream *streamPtr));
extern	void		Vm_CleanupTask _ARGS_((Proc_LockedPCB *procPtr));
extern	ReturnStatus	Vm_Cmd _ARGS_((int command, vm_size_t length,
				       Address arg));
extern	ReturnStatus	Vm_CreateVA _ARGS_((Address address,
				vm_size_t bytes));
extern	ReturnStatus	Vm_CopyIn _ARGS_((int numBytes,
				Address sourcePtr, Address destPtr));
extern	ReturnStatus	Vm_CopyOut _ARGS_((int numBytes,
				Address sourcePtr, Address destPtr));
extern	ReturnStatus	Vm_CopyInProc _ARGS_((int numBytes,
				Proc_LockedPCB *fromProcPtr, 
				Address fromAddr, Address toAddr,
				Boolean toKernel));
extern	ReturnStatus	Vm_CopyOutProc _ARGS_((int numBytes, Address fromAddr,
				Boolean fromKernel,
				Proc_LockedPCB *toProcPtr,
				Address toAddr));
extern	void		Vm_EnqueueRequest _ARGS_((Vm_Segment *segPtr,
				Sys_MsgBuffer *requestPtr,
				Sys_MsgBuffer *replyPtr));
extern	ReturnStatus	Vm_ExtendStack _ARGS_((Proc_LockedPCB *procPtr,
				Address address));
extern	void		Vm_FileChanged _ARGS_((Vm_Segment **segPtrPtr));
extern	ReturnStatus	Vm_Fork _ARGS_ ((Proc_ControlBlock *procPtr,
				Proc_ControlBlock *parentProcPtr));
extern	void		Vm_FsCacheSize _ARGS_((Address *startAddrPtr,
				Address *endAddrPtr));
extern	ReturnStatus	Vm_GetCodeSegment _ARGS_((Fs_Stream *filePtr,
				char *fileName, Proc_ObjInfo *execInfoPtr,
				Boolean dontCreate, Vm_Segment **segPtrPtr));
extern	int		Vm_GetPageSize _ARGS_((void));
extern	int		Vm_GetRefTime _ARGS_((void));
extern	ReturnStatus	Vm_GetSharedSegment _ARGS_((char *fileName,
				Vm_Segment **segmentPtrPtr));
extern	ReturnStatus	Vm_GetSwapSegment _ARGS_((Vm_SegmentType type,
				vm_size_t size, Vm_Segment **segPtrPtr));
extern	void		Vm_Init _ARGS_((void));
extern	int		Vm_MapBlock _ARGS_((Address addr));
extern	kern_return_t	Vm_MapFile _ARGS_((Proc_ControlBlock *procPtr,
				char *fileName, boolean_t readOnly,
				off_t offset, vm_size_t length, 
				ReturnStatus *statusPtr, 
				Address *startAddrPtr));
extern	kern_return_t	Vm_MapSegment _ARGS_((Proc_LockedPCB *procPtr,
				Vm_Segment *segPtr, boolean_t readOnly,
				boolean_t anywhere, vm_offset_t offset,
				vm_size_t length, Address *startAddrPtr,
				ReturnStatus *statusPtr));
extern	void		Vm_MakeAccessible _ARGS_((int accessType, int numBytes,
				Address startAddr, int *retBytesPtr,
				Address *retAddrPtr));
extern	void		Vm_MakeUnaccessible _ARGS_((Address addr,
				int numBytes));
extern	void		Vm_NewProcess _ARGS_((Vm_TaskInfo *vmPtr));
extern	Vm_Segment	*Vm_PortToSegment _ARGS_((mach_port_t port));
extern	void		Vm_Recovery _ARGS_((void));
extern	void		Vm_ReleaseMappedSegments _ARGS_((
				Proc_LockedPCB *procPtr));
extern	void		Vm_SegmentAddRef _ARGS_((Vm_Segment *segmentPtr));
extern	char		*Vm_SegmentName _ARGS_((Vm_Segment *segPtr));
extern	void		Vm_SegmentRelease _ARGS_((Vm_Segment *segmentPtr));
extern	int		Vm_Shutdown _ARGS_((void));
extern	ReturnStatus	Vm_StringNCopy _ARGS_((int numBytes,
				Address sourcePtr, Address destPtr,
				int *bytesCopiedPtr));
extern	void		Vm_SyncAll _ARGS_((void));
extern	int		Vm_UnmapBlock _ARGS_((Address addr));

#endif /* SPRITED */

#endif /* _VM */
