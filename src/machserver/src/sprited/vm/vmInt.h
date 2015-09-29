/*
 * vmInt.h --
 *
 *	Internal declarations for the Sprite VM module.
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
 * $Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmInt.h,v 1.13 92/07/16 18:04:52 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VMINT
#define _VMINT

#include <sprite.h>
#include <mach.h>
#include <fs.h>
#include <user/fs.h>
#include <procTypes.h>
#include <vmTypes.h>

extern Boolean vmRequestDebug;

extern ReturnStatus VmAddrParse _ARGS_((Proc_ControlBlock *procPtr,
			Address startAddr, int *numBytesPtr,
			Vm_Segment **segPtrPtr, vm_offset_t *offsetPtr));
extern void	VmCleanSegPages _ARGS_((Vm_Segment *segPtr,
			Boolean writeProtect, vm_size_t numBytes,
			Boolean fromFront));
extern ReturnStatus VmCmdInband _ARGS_((int command, int option, 
			int inBufLength, Address inBuf, int *outBufLengthPtr, 
			Address *outBufPtr, Boolean *outBufDeallocPtr));
extern void	VmFreeFileMap _ARGS_((VmFileMap *mapPtr));
extern ReturnStatus VmGetAttrStream _ARGS_((Fs_Stream *streamPtr, 
			Fs_Attributes *attrPtr));
extern char	*VmMakeSwapFileName _ARGS_((Vm_Segment *segPtr));
extern VmFileMap *VmNewFileMap _ARGS_((vm_size_t bytes));
extern ReturnStatus VmOpenSwapFile _ARGS_((Vm_Segment *segPtr));
extern void	VmPagerInit _ARGS_((void));
extern VmMappedSegment *VmSegByName _ARGS_((List_Links *segmentList, 
			mach_port_t segmentName));
extern char	*VmSegStateString _ARGS_((Vm_SegmentState state));
extern ReturnStatus VmSegmentCopy _ARGS_((VmMappedSegment *mappedSegPtr,
			Vm_Segment **copySegPtrPtr));
extern void	VmSegmentInit _ARGS_((void));
extern void	VmSegmentLock _ARGS_((Vm_Segment *segPtr));
extern void	VmSegmentUnlock _ARGS_((Vm_Segment *segPtr));

#endif /* _VMINT */
