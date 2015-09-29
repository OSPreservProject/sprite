/* 
 * vmSegment.c --
 *
 *	Code to create, destroy, find, etc. VM segment structures.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmSegment.c,v 1.25 92/06/25 15:55:36 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <ckalloc.h>
#include <list.h>
#include <mach.h>
#include <mach_error.h>
#include <status.h>
#include <string.h>

#include <user/fs.h>
#include <fs.h>
#include <fsio.h>
#include <fsutil.h>
#include <sync.h>
#include <sys.h>
#include <timer.h>
#include <utils.h>
#include <vm.h>
#include <vmInt.h>
#include <user/vmStat.h>
#include <vmSwapDir.h>

/* 
 * This type is used to make a list of segments to clean.
 */
typedef struct {
    List_Links link;
    Vm_Segment *segPtr;
} VmCleaningElement;

/* 
 * List of all segments known to the system.  The list is protected by
 * a monitor lock, which should be obtained before any of the segments
 * themselves are locked.  Membership in this list does not count as a
 * reference to the segment.
 * 
 * Notes about segment deletion: because the segment data structures
 * are dynamically freed, we must avoid a situation where process A
 * frees the segment while process B is waiting to obtain the
 * segment's lock.  There are three mechanisms to prevent this
 * situation.  
 * 
 * The first is the use of reference counts.  This handles the case
 * where process B already has a segment handle.
 * 
 * The second mechanism is the monitor lock for the list of segments.
 * This handles the case where process B has done a lookup on the
 * segment (given a file ID) and wants to increase the reference count
 * for the segment.
 * 
 * The third mechanism is to ensure that the segment's control port is
 * null before freeing it.  This handles the case where Sprite has
 * decided that it's done with the segment, but the kernel is still
 * using it (i.e., memory_object_terminate hasn't been called for the
 * segment yet).  (For the window between the vm_map call and the
 * memory_object_init call, the segment's control port is assigned to
 * MACH_PORT_DEAD.)
 */
    
static List_Links allSegListHdr;
#define	allSegList	(&allSegListHdr)

/* 
 * This monitor lock protects the "all segments" list.  To avoid deadlock, 
 * if both the monitor lock and a segment lock are needed, the monitor lock 
 * should be obtained first.  The lock is also used to synchronize VM 
 * segment deletion with FS notification that the backing file for a code 
 * segment has changed.
 */
static Sync_Lock segmentLock = Sync_LockInitStatic("VM:allSegLock");
#define LOCKPTR (&segmentLock)

/* 
 * If a process is looking for a segment and finds it in the middle of 
 * being destroyed, it can wait on this condition variable to recheck the 
 * segment list (at which time the segment is hopefully gone).
 */
static Sync_Condition segmentRetry;

/* 
 * This flag controls whether a code segment will be kept around after all 
 * its references go away.
 */
Boolean vm_StickySegments = TRUE;


/* Forward references */

static void FixFsSegPtr _ARGS_((Vm_Segment *segPtr));
static ReturnStatus VmSegFindHelper _ARGS_((Fs_Attributes *attrPtr,
					    Vm_SegmentType type,
					    Vm_Segment **segPtrPtr));
static void VmSegmentCleanup _ARGS_((Vm_Segment *segPtr));
static void VmSegmentDestroy _ARGS_((Vm_Segment *segPtr));
static ReturnStatus VmSegmentFind _ARGS_((Fs_Stream *filePtr,
		Vm_SegmentType type, Vm_Segment **segPtrPtr));
static ReturnStatus VmSegmentNew _ARGS_((Vm_SegmentType type,
		Fs_Stream *filePtr, char *fileName,
		Vm_Segment **segPtrPtr));
static void VmSegmentReleaseInt _ARGS_((Vm_Segment *segPtr));


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentInit --
 *
 *	Initialization for the code that manages VM segments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the global list of segments and "retry" condition 
 *	variable.
 *
 *----------------------------------------------------------------------
 */
    
void
VmSegmentInit()
{
    List_Init(allSegList);
    Sync_ConditionInit(&segmentRetry, "VM:segmentRetry", TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_Shutdown --
 *
 *	Flush all the segments in the system.
 *
 * Results:
 *	Returns the number of segments that still exist.
 *
 * Side effects:
 *	All segments are write-protected and marked for write-back and 
 *	destruction.
 *
 *----------------------------------------------------------------------
 */

ENTRY int
Vm_Shutdown()
{
    Vm_Segment *segPtr;		/* a segment to kill */
    List_Links *itemPtr;
    int numSegments = 0;

    LOCK_MONITOR;
    LIST_FORALL(allSegList, itemPtr) {
	numSegments++;
	segPtr = (Vm_Segment *)itemPtr;
	VmSegmentLock(segPtr);
	if (segPtr->state == VM_SEGMENT_OK) {
	    segPtr->state = VM_SEGMENT_DYING;
	}
	if (segPtr->flags & VM_CLEANING_SEGMENT) {
	    printf("Warning: still cleaning segment %s.\n",
		   Vm_SegmentName(segPtr));
	} else {
	    VmCleanSegPages(segPtr, TRUE, 0, TRUE);
	}
	VmSegmentUnlock(segPtr);
    }
    UNLOCK_MONITOR;

    return numSegments;
}


/*
 *----------------------------------------------------------------------
 *
 * VmGetSharedSegment --
 *
 *	Get the shared segment corresponding to the given file.
 *
 * Results:
 * 	Fills in the handle to the segment and returns SUCCESS.  The 
 * 	caller is responsible for eventually freeing the reference to 
 * 	the segment.  If the segment doesn't already exist and
 * 	couldn't be created (or already exists but has an incompatible 
 * 	type), an error code is returned.
 *
 * Side effects:
 *	Adds the segment to the global list, if necessary.  
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
Vm_GetSharedSegment(fileName, segPtrPtr)
    char *fileName;
    Vm_Segment **segPtrPtr;	/* OUT: the resulting segment */
{
    ReturnStatus status = SUCCESS;
    Fs_Stream *filePtr;		/* handle for the named file */

    /* 
     * Try to find the file in the list of segments.  If that fails,
     * create a new segment for it.
     */
    
    /* 
     * Assumes that permissions checking is done at a higher 
     * level.
     */
    status = Fs_Open(fileName, FS_READ|FS_WRITE|FS_CREATE, FS_FILE,
		     0600, &filePtr);
    if (status != SUCCESS) {
	return status;
    }

    LOCK_MONITOR;
    status = VmSegmentFind(filePtr, VM_SHARED, segPtrPtr);
    if (status == SUCCESS || status == FS_FILE_BUSY) {
	(void)Fs_Close(filePtr);
	goto done;
    }

    status = VmSegmentNew(VM_SHARED, filePtr, fileName, segPtrPtr);
    (void)Fs_Close(filePtr);

 done:
    UNLOCK_MONITOR;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentNew --
 *
 *	Create a new segment, backed by the given file (which may be 
 *	NULL).
 *	XXX Maybe it would be cleaner to not give file information to 
 *	VmSegmentNew, instead having the caller responsible for 
 *	filling in the file information.
 *
 * Results:
 *	Returns a Sprite status code, and fills in the pointer to a
 *	new segment, which has a reference count of 1.  Only the 
 *	fields of the segment that are common to all segment types are 
 *	initialized; the caller is responsible for other fields.  The 
 *	caller may also need to reset the segment size, which is set 
 *	to the length of the file (0, if no file).
 *
 * Side effects:
 *	Adds the request port for the segment to the system request 
 *	port set.  Adds the segment to the list of segments.  If a 
 *	file is provided, its reference count is incremented (so the 
 *	caller is responsible for freeing its copy).
 *
 *----------------------------------------------------------------------
 */

static INTERNAL ReturnStatus
VmSegmentNew(type, filePtr, fileName, segPtrPtr)
    Vm_SegmentType type;	/* The type of segment that this is */
    Fs_Stream *filePtr;		/* The backing file */
    char *fileName;		/* and its name */
    Vm_Segment **segPtrPtr;	/* resulting segment pointer */
{
    Vm_Segment *segmentPtr;	/* local copy of the segment handle */
    kern_return_t kernStatus;	/* Mach return status */
    ReturnStatus status = SUCCESS; /* Sprite return status */
    Fs_Attributes fileAttributes;

    segmentPtr = (Vm_Segment *)ckalloc(sizeof(Vm_Segment));
    if (segmentPtr == NULL) {
	/* 
	 * XXX If we have a list of unused, cached segments, now would 
	 * be a good time to reclaim some of them and try again.
	 */
	panic("VmSegmentNew: no memory.\n");
    }

    /* 
     * Zero out all the fields.  This will (a) hopefully make it easier to 
     * detect initialization botches and (b) make routines like 
     * Sync_ConditionInit always do the right thing.
     */
    bzero((_VoidPtr)segmentPtr, (size_t)sizeof(Vm_Segment));
    List_InitElement((List_Links *)segmentPtr);
    segmentPtr->magic = VM_SEGMENT_MAGIC_NUMBER;
    segmentPtr->refCount = 1;
    segmentPtr->residentPages = 0;
    Sync_LockInitDynamic(&segmentPtr->lock, "VM:segmentLock");
    Sync_ConditionInit(&segmentPtr->condition, "VM:segmentCondition",
		       TRUE);
    segmentPtr->flags = 0;
    segmentPtr->requestPort = MACH_PORT_NULL;
    segmentPtr->controlPort = MACH_PORT_NULL;
    segmentPtr->namePort = MACH_PORT_NULL;
    segmentPtr->type = type;
    segmentPtr->state = VM_SEGMENT_OK;
    if (filePtr != NULL) {
	Fsio_StreamCopy(filePtr, &segmentPtr->swapFilePtr);
	segmentPtr->swapFileHandle = Fs_GetFileHandle(filePtr);
	status = VmGetAttrStream(segmentPtr->swapFilePtr,
				  &fileAttributes);
	if (status != SUCCESS) {
	    goto bailOut;
	}
	segmentPtr->size = round_page(fileAttributes.size);
	segmentPtr->swapFileServer = fileAttributes.serverID;
	segmentPtr->swapFileDomain = fileAttributes.domain;
	segmentPtr->swapFileNumber = fileAttributes.fileNumber;
#if VM_KNOWS_SWAP_FILE_SIZE
	segmentPtr->swapFileSize = fileAttributes.size;
#endif
    } else {
	segmentPtr->swapFilePtr = NULL;
	segmentPtr->size = 0;
	segmentPtr->swapFileHandle = NULL;
	/* 
	 * XXX rather than using -1 to indicate "not initialized", we 
	 * should probably set a flag in the segment.
	 */
	segmentPtr->swapFileServer = -1;
	segmentPtr->swapFileDomain = -1;
	segmentPtr->swapFileNumber = -1;
#if VM_KNOWS_SWAP_FILE_SIZE
	segmentPtr->swapFileSize = 0;
#endif
    }
    if (fileName != NULL) {
	segmentPtr->swapFileName = ckstrdup(fileName);
    } else {
	segmentPtr->swapFileName = NULL;
    }
    segmentPtr->requestList = &segmentPtr->requestHdr;
    List_Init(segmentPtr->requestList);
    segmentPtr->queueSize = 0;

    /* 
     * Allocate the request port now.  The control port will be
     * assigned to the segment when the kernel calls 
     * memory_object_init. 
     */
    kernStatus = mach_port_allocate_name(mach_task_self(),
					 MACH_PORT_RIGHT_RECEIVE,
					 (mach_port_t)segmentPtr);
    if (kernStatus != KERN_SUCCESS) {
	printf("VmSegmentNew: can't allocate segment request port: %s\n",
	       mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }
    segmentPtr->requestPort = (mach_port_t)segmentPtr;

    /* 
     * Create a send right for the port (e.g., so that it can be used 
     * in a vm_map call).
     */
    kernStatus = mach_port_insert_right(mach_task_self(),
					segmentPtr->requestPort,
					segmentPtr->requestPort,
					MACH_MSG_TYPE_MAKE_SEND);
    if (kernStatus != KERN_SUCCESS) {
	printf("VmSegmentNew: can't create send right for port: %s\n",
	       mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }

    /* 
     * Add the request port for the segment to the port set for the system.
     */
    kernStatus = mach_port_move_member(mach_task_self(),
				       segmentPtr->requestPort,
				       sys_RequestPort);
    if (kernStatus != KERN_SUCCESS) {
	printf("VmSegmentNew: can't move segment %s to port set: %s\n",
	       Vm_SegmentName(segmentPtr), mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto bailOut;
    }

    if (filePtr != NULL) {
	status = Fs_FileBeingMapped(segmentPtr->swapFilePtr, 1);
    }
    if (status != SUCCESS) {
	goto bailOut;
    }

    /* 
     * Add the segment to the list of segments.
     */
    List_Insert((List_Links *)segmentPtr, LIST_ATREAR(allSegList));

 bailOut:
    if (status == SUCCESS) {
	vmStat.segmentsCreated++;
    } else {
	if (segmentPtr->swapFileName != NULL) {
	    ckfree(segmentPtr->swapFileName);
	}
	if (segmentPtr->swapFilePtr != NULL) {
	    (void)Fs_Close(segmentPtr->swapFilePtr);
	}
	(void)mach_port_destroy(mach_task_self(), (mach_port_t)segmentPtr);
	ckfree(segmentPtr);
	segmentPtr = (Vm_Segment *)NIL;
    }
    *segPtrPtr = segmentPtr;

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentDestroy --
 *
 *	Destroy a segment, or at least mark it for destruction.  The caller
 *	should assume that the segment was destroyed and not try to use the
 *	segment handle anymore.  Note that the segment might already be in
 *	the process of being destroyed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the segment can be destroyed immediately, all the resources
 *	it holds (ports, memory) are freed and the segment is removed 
 *	from the list of segments.  If the segment can't be destroyed
 *	immediately, cleanup is initiated for the segment, and the
 *	segment is unlocked.
 *
 *----------------------------------------------------------------------
 */

static INTERNAL void
VmSegmentDestroy(segPtr)
    Vm_Segment *segPtr;		/* should already be locked */
{
    if (segPtr->refCount != 0) {
	panic("VmSegmentDestroy: segment has references.\n");
    }
    
    if (segPtr->state != VM_SEGMENT_DEAD) {
	segPtr->state = VM_SEGMENT_DYING;
    }

    /* 
     * If the kernel doesn't have a reference to the segment, get rid of it 
     * now. 
     */
    if (segPtr->controlPort == MACH_PORT_NULL) {
	VmSegmentCleanup(segPtr);
	segPtr = NULL;
    } else {
	/* 
	 * The kernel still has a reference to the segment, so there might
	 * still be dirty pages in memory.  For "anonymous" segments (swap
	 * files), we don't care; tell the kernel to destroy the segment.
	 * For other segments, flush any dirty pages; when that's done
	 * we'll tell the kernel to blow the segment away.  In either case,
	 * our segment data structure isn't free until the kernel says it's
	 * okay.
	 */
	if (!(segPtr->flags & VM_CLEANING_SEGMENT)) {
	    if (segPtr->type == VM_STACK || segPtr->type == VM_HEAP) {
		segPtr->state = VM_SEGMENT_DEAD;
		(void)memory_object_destroy(segPtr->controlPort, 0);
	    } else {
		VmCleanSegPages(segPtr, TRUE, 0, TRUE);
	    }
	}
	VmSegmentUnlock(segPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentCleanup --
 *
 *	Clean up a dead locked segment, which is not referred to 
 *	either by Sprite code or by the kernel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The segment's request port is deallocated, and all associated 
 *	storage is freed.  The segment is removed from the "all 
 *	segments" list.
 *
 *----------------------------------------------------------------------
 */

static INTERNAL void
VmSegmentCleanup(segPtr)
    Vm_Segment *segPtr;		/* the segment that's going away */
{
    kern_return_t kernStatus;

    /* 
     * Sanity checks.
     */
    if (segPtr->refCount != 0) {
	panic("VmSegmentCleanup: non-zero reference count.\n");
    }
    if (segPtr->controlPort != MACH_PORT_NULL) {
	panic("VmSegmentCleanup: control port not nil.\n");
    }
    if (segPtr->state != VM_SEGMENT_DEAD &&
		segPtr->state != VM_SEGMENT_DYING) {
	panic("VmSegmentCleanup: segment not dead.\n");
    }
    if (!List_IsEmpty(segPtr->requestList)) {
	panic("VmSegmentCleanup: there are pending requests.\n");
    }

    List_Remove((List_Links *)segPtr);
    segPtr->magic = 0;
    kernStatus = mach_port_destroy(mach_task_self(), segPtr->requestPort);
    if (kernStatus != KERN_SUCCESS) {
	printf("%s: problem destroying port for segment %s: %s\n",
	       "VmSegmentCleanup", Vm_SegmentName(segPtr),
	       mach_error_string(kernStatus));
    }

    /* 
     * Notify the file system that the backing file is no longer 
     * being used.  XXX Will it be a performance problem to do the 
     * notification here?  There aren't that many places where it can 
     * be done, given that we have little control over the kernel's 
     * use of the pages.
     */
    if (segPtr->swapFilePtr != NULL) {
	(void)Fs_FileBeingMapped(segPtr->swapFilePtr, 0);
	(void)Fs_Close(segPtr->swapFilePtr);
	if ((segPtr->type == VM_STACK || segPtr->type == VM_HEAP)
	    && segPtr->swapFileName != NULL) {
	    VmSwapFileRemove(segPtr->swapFileName);
	}
	segPtr->swapFilePtr = NULL;
    }
    if (segPtr->swapFileHandle != NULL) {
	FixFsSegPtr(segPtr);
	segPtr->swapFileHandle = NULL;
    }
    if (segPtr->swapFileName != NULL) {
	ckfree(segPtr->swapFileName);
	segPtr->swapFileName = NULL;
    }
    if (segPtr->type == VM_HEAP) {
	if (segPtr->typeInfo.heapInfo.initFilePtr != NULL) {
	    (void)Fs_Close(segPtr->typeInfo.heapInfo.initFilePtr);
	}
	if (segPtr->typeInfo.heapInfo.initFileName != NULL) {
	    ckfree(segPtr->typeInfo.heapInfo.initFileName);
	}
	if (segPtr->typeInfo.heapInfo.mapPtr != NULL) {
	    VmFreeFileMap(segPtr->typeInfo.heapInfo.mapPtr);
	}
    } else if (segPtr->type == VM_CODE) {
	if (segPtr->typeInfo.codeInfo.execInfoPtr != NULL) {
	    ckfree(segPtr->typeInfo.codeInfo.execInfoPtr);
	}
    }

    Sync_ConditionFree(&segPtr->condition);

    /* 
     * Make the lock debugging code happy.
     */
    VmSegmentUnlock(segPtr);

    ckfree(segPtr);
    segPtr = NULL;
    vmStat.segmentsDestroyed++;

    /* 
     * Notify any interested processes that a segment has been removed from 
     * the global list.
     */
    Sync_Broadcast(&segmentRetry);
}


/*
 *----------------------------------------------------------------------
 *
 * FixFsSegPtr --
 *
 *	Get rid of the pointer that the given code segment's swap file has 
 *	back to the segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	NILs out the FS handle's segment pointer.
 *
 *----------------------------------------------------------------------
 */

static INTERNAL void
FixFsSegPtr(segPtr)
    Vm_Segment *segPtr;		/* a locked code segment */
{
    Vm_Segment **segPtrPtr;	/* ptr into swap file handle */

    segPtrPtr = Fs_GetSegPtr(segPtr->swapFileHandle);
    if (*segPtrPtr != (Vm_Segment *)NIL && *segPtrPtr != segPtr) {
	panic("FixFsSegPtr: wrong segment pointer in file handle.\n");
    }

    *segPtrPtr = (Vm_Segment *)NIL;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentFind --
 *
 *	Try to find the segment for the given file in the existing 
 *	pool of segments.
 *
 * Results:
 *	Returns a Sprite status code.  If successful, fills in the 
 *	handle for the segment.  
 *	VM_SEG_NOT_FOUND: didn't find a segment for the file. 
 *	FS_FILE_BUSY: found the segment, but it has a type that
 *	  conflicts with the requested type.
 *	VM_SEGMENT_DESTROYED: found the segment, but it's being destroyed.
 *
 * Side effects:
 *	Increments the reference count of the segment (only if successful).
 *
 *----------------------------------------------------------------------
 */

static INTERNAL ReturnStatus
VmSegmentFind(filePtr, type, segPtrPtr)
    Fs_Stream *filePtr;		/* the desired swap file */
    Vm_SegmentType type;	/* desired type for the segment */
    Vm_Segment **segPtrPtr;	/* OUT: handle to found segment */
{
    ReturnStatus status;
    Fs_Attributes fileAttributes;
    Boolean sigPending;
    Boolean retry;

    status = VmGetAttrStream(filePtr, &fileAttributes);
    if (status != SUCCESS) {
	printf("VmSegmentFind: couldn't get file attributes for %s: %s\n",
	       Fsutil_GetFileName(filePtr), Stat_GetMsg(status));
	return status;
    }

    do {
	retry = FALSE;
	status = VmSegFindHelper(&fileAttributes, type, segPtrPtr);
	if (status == VM_SEGMENT_DESTROYED) {
	    printf("VmSegmentFind: waiting for cleanup of segment %s.\n",
		   Vm_SegmentName(*segPtrPtr));
	    sigPending = Sync_Wait(&segmentRetry, TRUE);
	    if (sigPending) {
		status = VM_SEGMENT_DESTROYED;
	    } else {
		retry = TRUE;
	    }
	}
    } while(retry);

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegFindHelper --
 *
 *	Helper routine for VmSegmentFind.
 *
 * Results:
 *	Returns a Sprite status code, the same as VmSegmentFind.  Fills in 
 *	the matching segment handle (if any), even if the segment isn't 
 *	usable for the given request.
 *
 * Side effects:
 *	Same as VmSegmentFind (only increments the segment's reference 
 *	count if successful).
 *
 *----------------------------------------------------------------------
 */

static INTERNAL ReturnStatus
VmSegFindHelper(attrPtr, type, segPtrPtr)
    Fs_Attributes *attrPtr;	/* attributes for the file we want */
    Vm_SegmentType type;	/* desired type for the segment */
    Vm_Segment **segPtrPtr;	/* OUT: handle to found segment */
{
    List_Links *itemPtr;
    Vm_Segment *segPtr = (Vm_Segment *)NIL; /* lint */
    ReturnStatus status;

    status = VM_SEG_NOT_FOUND;
    *segPtrPtr = (Vm_Segment *)NIL;
    vmStat.segmentLookups++;

    LIST_FORALL(allSegList, itemPtr) {
	vmStat.segmentsLookedAt++;
	if (vmStat.segmentsLookedAt < 0) {
	    printf("VmSegFindHelper: counter overflow.\n");
	    vmStat.segmentsLookedAt = 0;
	}
	segPtr = (Vm_Segment *)itemPtr;
	VmSegmentLock(segPtr);
	if (attrPtr->fileNumber == segPtr->swapFileNumber
		&& attrPtr->domain == segPtr->swapFileDomain
		&& attrPtr->serverID == segPtr->swapFileServer) {
	    *segPtrPtr = segPtr;
	    if (segPtr->state != VM_SEGMENT_OK) {
		status = VM_SEGMENT_DESTROYED;
	    } else if (segPtr->type != type) {
		/* 
		 * For future reference: if the segment is unused (cached),
		 * then we could just change the type here.
		 */
		status = FS_FILE_BUSY;
	    } else {
		status = SUCCESS;
		segPtr->refCount++;
	    }
	    VmSegmentUnlock(segPtr);
	    break;
	}
	VmSegmentUnlock(segPtr);
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_PortToSegment --
 *
 *	Given a Mach request port, return the corresponding segment 
 *	handle. 
 *
 * Results:
 *	Returns a segment handle, NULL if the port doesn't refer to a 
 *	segment. 
 *
 * Side effects:
 *	Increments the segment's reference count.
 *
 *----------------------------------------------------------------------
 */

Vm_Segment *
Vm_PortToSegment(port)
    mach_port_t port;
{
    Vm_Segment *segPtr;

    segPtr = (Vm_Segment *)port;
    if (segPtr->magic != VM_SEGMENT_MAGIC_NUMBER) {
	segPtr = NULL;
    }
    if (segPtr != NULL) {
	VmSegmentLock(segPtr);
	if (segPtr->refCount < 0) {
	    panic("Vm_PortToSegment: negative reference count.\n");
	}
	segPtr->refCount++;
	VmSegmentUnlock(segPtr);
    }

    return segPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentAddRef --
 *
 *	Add a reference to a segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The segment's reference count is incremented.
 *
 *----------------------------------------------------------------------
 */

void
Vm_SegmentAddRef(segPtr)
    Vm_Segment *segPtr;
{
    VmSegmentLock(segPtr);
    if (segPtr->refCount < 0
	|| (segPtr->refCount == 0 && segPtr->type != VM_CODE)) {
	panic("Vm_SegmentAddRef: bogus reference count.\n");
    }
    segPtr->refCount++;
    VmSegmentUnlock(segPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_SegmentRelease --
 *
 *	Decrement the reference count on a segment, and free it if this is
 *	the last reference.  (Exception: code segments will be kept around, 
 *	if sticky segments are enabled and the segment is still registered 
 *	with the kernel.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See VmSegmentReleaseInt.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Vm_SegmentRelease(segPtr)
    Vm_Segment *segPtr;
{
    /* 
     * Obtain the monitor lock now because we might need it when we call 
     * VmSegmentDestroy. 
     */
    LOCK_MONITOR;
    VmSegmentReleaseInt(segPtr);
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentReleaseInt --
 *
 *	Decrement the reference count on a segment, and free it if it meets 
 *	some slightly complicated requirements.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Decrements the reference count and destroys the segment if 
 *	necessary. 
 *
 *----------------------------------------------------------------------
 */

static INTERNAL void
VmSegmentReleaseInt(segPtr)
    Vm_Segment *segPtr;
{
    Boolean nukeIt = TRUE;	/* deallocate it, or leave it inactive? */

    VmSegmentLock(segPtr);
    if (segPtr->refCount == 0) {
	panic("VmSegmentReleaseInt: refCount is already zero.\n");
    }
    if (segPtr->magic != VM_SEGMENT_MAGIC_NUMBER) {
	panic("VmSegmentReleaseInt: bad magic.\n");
    }
    segPtr->refCount--;
    if (segPtr->refCount > 0) {
	nukeIt = FALSE;
    } else if (segPtr->type == VM_CODE && vm_StickySegments) {
	if (segPtr->state == VM_SEGMENT_OK
	    && segPtr->controlPort != MACH_PORT_NULL
	    && segPtr->residentPages > 0) {
	    nukeIt = FALSE;
	}
    }

    /* 
     * If a code segment has become inactive, close the swap file, so that 
     * the file server doesn't think it's still in use.
     */
    if (segPtr->refCount == 0 && !nukeIt && segPtr->swapFilePtr != NULL) {
	(void)Fs_Close(segPtr->swapFilePtr);
	segPtr->swapFilePtr = NULL;
    }

    if (nukeIt) {
	VmSegmentDestroy(segPtr);
	segPtr = NULL;
    } else {
	VmSegmentUnlock(segPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentUnlock --
 *
 *	Release the lock on a segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The segment is unlocked.
 *
 *----------------------------------------------------------------------
 */

void
VmSegmentUnlock(segPtr)
    Vm_Segment *segPtr;
{
    Sync_Unlock(&segPtr->lock);
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentLock --
 *
 *	Obtain exclusive access to a segment structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The segment is locked.
 *
 *----------------------------------------------------------------------
 */

void
VmSegmentLock(segPtr)
    Vm_Segment *segPtr;
{
    Sync_GetLock(&segPtr->lock);
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegStateString --
 *
 *	Return a printable name for a segment state.
 *
 * Results:
 *	Returns a pointer to a read-only string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
VmSegStateString(state)
    Vm_SegmentState state;
{
    char *result;

    switch (state) {
    case VM_SEGMENT_OK: 
	result = "okay";
	break;
    case VM_SEGMENT_DYING:
	result = "dying";
	break;
    case VM_SEGMENT_DEAD:
	result = "dead";
	break;
    default:
	result = "<bogus state>";
	break;
    }

    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * VmGetCodeSegment --
 *
 *	Get a reference for the code segment corresponding to the given
 *	file.
 *
 * Results:
 * 	Fills in the handle to the segment and returns SUCCESS.  The 
 * 	caller is responsible for eventually freeing the reference to 
 * 	the segment.  If the segment doesn't already exist and
 * 	couldn't be created (or already exists but has an incompatible 
 * 	type), an error code is returned.
 *
 * Side effects:
 *	Adds the segment to the global list, if necessary.  Gives the file 
 *	a non-reference counted pointer to the segment.  Puts back the 
 *	segment's swap file handle if the segment was inactive.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
Vm_GetCodeSegment(filePtr, fileName, execInfoPtr, dontCreate, segPtrPtr)
    Fs_Stream *filePtr;		/* the object file to use */
    char *fileName;		/* name of the object file */
    Proc_ObjInfo *execInfoPtr;	/* a.out header information; may be NULL if 
				 * dontCreate is TRUE */
    Boolean dontCreate;		/* if the segment can't be found, tells
				 * whether or not to create it */
    Vm_Segment **segPtrPtr;	/* OUT: the resulting segment */
{
    ReturnStatus status = FAILURE;
    Vm_Segment *segPtr = NULL;	/* local copy of resulting segment */
    ClientData fileHandle;	/* handle for filePtr */
    Vm_Segment **cachedSegPtrPtr; /* from filePtr */

    LOCK_MONITOR;

    /* 
     * First check if the segment is directly associated with a file
     * handle.  If that fails, try to find the file in the list of
     * segments.  If that fails and it's okay to create a new segment, do 
     * it.
     */
    
    fileHandle = Fs_GetFileHandle(filePtr);
    cachedSegPtrPtr = Fs_GetSegPtr(fileHandle);
    segPtr = *cachedSegPtrPtr;
    if (segPtr != (Vm_Segment *)NIL) {
	VmSegmentLock(segPtr);
	if (segPtr->magic != VM_SEGMENT_MAGIC_NUMBER) {
	    panic("Vm_GetCodeSegment: bad magic (segment %s).\n",
		  Vm_SegmentName(segPtr));
	}
	if (segPtr->state == VM_SEGMENT_OK) {
	    *segPtrPtr = *cachedSegPtrPtr;
	    segPtr->refCount++;
	    status = SUCCESS;
	}
	VmSegmentUnlock(segPtr);
    }

    if (status != SUCCESS) {
	segPtr = NULL;
	status = VmSegmentFind(filePtr, VM_CODE, segPtrPtr);
	if (status == SUCCESS) {
	    segPtr = *cachedSegPtrPtr = *segPtrPtr;
	    if (vm_StickySegments) {
		printf("%s: had to find segment %s in the global list.\n",
		       "Vm_GetCodeSegment", Vm_SegmentName(segPtr));
	    }
	}
    }

    if (status != SUCCESS && status != FS_FILE_BUSY && !dontCreate) {
	status = VmSegmentNew(VM_CODE, filePtr, fileName, segPtrPtr);
	if (status == SUCCESS) {
	    segPtr = *segPtrPtr;
	    VmSegmentLock(segPtr);
	    segPtr->size = round_page(execInfoPtr->codeFileOffset +
				      execInfoPtr->codeSize);
	    segPtr->typeInfo.codeInfo.execInfoPtr = 
		(Proc_ObjInfo *)ckalloc(sizeof(Proc_ObjInfo));
	    bcopy((_VoidPtr)execInfoPtr,
		  (_VoidPtr)(segPtr->typeInfo.codeInfo.execInfoPtr),
		  sizeof(Proc_ObjInfo));
	    *cachedSegPtrPtr = segPtr;
	    VmSegmentUnlock(segPtr);
	}
    }

    /* 
     * If the segment was inactive, reactivate it by getting a reference to 
     * the open stream.
     */
    if (status == SUCCESS && segPtr->swapFilePtr == NULL) {
	VmSegmentLock(segPtr);
	Fsio_StreamCopy(filePtr, &segPtr->swapFilePtr);
	if (segPtr->swapFileHandle != Fs_GetFileHandle(segPtr->swapFilePtr)) {
	    /* 
	     * I don't think this can happen, but then I don't completely 
	     * understand how the FS handle stuff works.
	     */
	    panic("Vm_GetCodeSegment: handle/stream mismatch.\n");
	}
	VmSegmentUnlock(segPtr);
    }
    UNLOCK_MONITOR;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_GetSwapSegment --
 *
 *	Create a segment backed by a swap file.
 *
 * Results:
 *	Returns a Sprite status code.  Fills in the handle to a new 
 *	segment. 
 *
 * Side effects:
 *	Adds the segment to the global list.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
Vm_GetSwapSegment(type, size, segPtrPtr)
    Vm_SegmentType type;	/* VM_STACK or VM_HEAP */
    vm_size_t size;		/* size to make the segment */ 
    Vm_Segment **segPtrPtr;	/* OUT: handle for the new segment */
{
    ReturnStatus status = SUCCESS;
    Vm_Segment *segPtr = NULL;

    /* 
     * We ask for a segment without a backing file, then we use the 
     * segment handle to generate a unique file name.
     */
    LOCK_MONITOR;
    status = VmSegmentNew(type, (Fs_Stream *)NULL, (char *)NULL,
			  &segPtr);
    if (status != SUCCESS) {
	goto done;
    }
    segPtr->swapFileName = VmMakeSwapFileName(segPtr);
    segPtr->size = size;

 done:
    UNLOCK_MONITOR;
    *segPtrPtr = segPtr;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * VmNewFileMap --
 *
 *	Create a map telling which backing file to get a page from.
 *
 * Results:
 *	Returns a map with all entries set to get the page from the 
 *	initialization file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

VmFileMap *
VmNewFileMap(bytes)
    vm_size_t bytes;		/* number of bytes the map is 
				 * responsible for */
{
    VmFileMap *mapPtr;		/* the result */
    char *arrayPtr;		/* the array of flags */
    char *elementPtr;		/* element in the array */
    unsigned int pages;		/* number of pages the map is for */

    if (bytes != trunc_page(bytes)) {
	panic("VmNewFileMap: partial page.\n");
    }
    pages = Vm_ByteToPage(bytes);
    arrayPtr = ckalloc(pages * sizeof(char));
    mapPtr = (VmFileMap *)ckalloc(sizeof(VmFileMap));
    if (arrayPtr == NULL || mapPtr == NULL) {
	panic("VmFileMap: out of memory.\n");
    }
    for (elementPtr = arrayPtr; elementPtr < arrayPtr + pages;
	 elementPtr++) {
	*elementPtr = TRUE;
    }
    mapPtr->arraySize = pages;
    mapPtr->useInitPtr = arrayPtr;

    return mapPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * VmFreeFileMap --
 *
 *	Release the object file/swap file mapping for a heap segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the memory for the map.
 *
 *----------------------------------------------------------------------
 */

void
VmFreeFileMap(mapPtr)
    VmFileMap *mapPtr;		/* the map to free */
{
    ckfree(mapPtr->useInitPtr);
    ckfree(mapPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * VmCopyFileMap --
 *
 *	Make a new file map equivalent to the given map.
 *
 * Results:
 *	Returns a new file map.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

VmFileMap *
VmCopyFileMap(mapPtr)
    VmFileMap *mapPtr;		/* map to copy */
{
    VmFileMap *newMapPtr;
    int index;

    newMapPtr = VmNewFileMap(Vm_PageToByte(mapPtr->arraySize));
    for (index = 0; index < mapPtr->arraySize; index++) {
	newMapPtr->useInitPtr[index] = mapPtr->useInitPtr[index];
    }

    return newMapPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_AddHeapInfo --
 *
 *	Fill in heap-specific information for a segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in information about the segment's initialization file.
 *
 *----------------------------------------------------------------------
 */

void
Vm_AddHeapInfo(segPtr, initFilePtr, initFileName, execInfoPtr)
    Vm_Segment *segPtr;		/* the segment to fill in */
    Fs_Stream *initFilePtr;	/* the segment's initialization file */
    char *initFileName;		/* name of the initialization file */
    Proc_ObjInfo *execInfoPtr;	/* where to find things in the init. file */
{
    VmSegmentLock(segPtr);
    segPtr->typeInfo.heapInfo.initStart = execInfoPtr->heapFileOffset;
    segPtr->typeInfo.heapInfo.initLength = execInfoPtr->heapSize;
    Fsio_StreamCopy(initFilePtr, &segPtr->typeInfo.heapInfo.initFilePtr);
    segPtr->typeInfo.heapInfo.initFileName = ckstrdup(initFileName);
    segPtr->typeInfo.heapInfo.mapPtr = 
		VmNewFileMap((vm_size_t)execInfoPtr->heapSize);
    VmSegmentUnlock(segPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_SegmentName --
 *
 *	Get the printable name for a segment.  The segment should be 
 *	locked, to ensure that the returned string doesn't disappear 
 *	suddenly. 
 *
 * Results:
 *	Returns the segment's (unique) swap file name, or a placeholder if 
 *	the swap file name isn't set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Vm_SegmentName(segPtr)
    Vm_Segment *segPtr;
{
    if (segPtr->swapFileName != NULL) {
	return segPtr->swapFileName;
    } else {
	return "<no name>";
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegmentCopy --
 *
 *	Create a new segment to copy into from the given segment.
 *
 * Results:
 *	Returns a status code.  If successful, fills in the new segment
 *	handle.
 *
 * Side effects:
 *	The source and destination segments are marked as being involved in 
 *	a copy operation.  Note: the actual contents of the segment are NOT 
 *	copied.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
VmSegmentCopy(mappedSegPtr, copySegPtrPtr)
    VmMappedSegment *mappedSegPtr; /* the segment to copy */
    Vm_Segment **copySegPtrPtr;	/* OUT: the copy */
{
    Vm_Segment *segPtr = mappedSegPtr->segPtr;
    Boolean fromSegmentLocked = FALSE;
    ReturnStatus status = SUCCESS;
    Vm_Segment *copySegPtr = NULL; /* *copySegPtrPtr */
    Boolean toSegmentLocked = FALSE;
    int swapSize = 0;		/* size of swap file (instrumentation) */
#if !VM_KNOWS_SWAP_FILE_SIZE
    Fs_Attributes fileAttributes;
#endif

    if (segPtr->type != VM_STACK && segPtr->type != VM_HEAP) {
	panic("VmSegmentCopy: unexpected segment type.\n");
    }

    /* 
     * Get a new segment to copy to.  This involves acquiring the monitor 
     * lock, so do it before locking the source segment.
     */
    status = Vm_GetSwapSegment(segPtr->type, segPtr->size, &copySegPtr);
    if (status != SUCCESS) {
	goto bailOut;
    }

    /* 
     * Do some sanity checks.  The original segment should be fully
     * initialized (otherwise how did we get here?).  This is potentially
     * important, since otherwise we couldn't always lock the segment
     * against writes.
     */
    VmSegmentLock(segPtr);
    segPtr->flags |= VM_COPY_SOURCE;
    fromSegmentLocked = TRUE;
    if (segPtr->state != VM_SEGMENT_OK) {
	status = VM_SEGMENT_DESTROYED;
	goto bailOut;
    }
    if (segPtr->controlPort == MACH_PORT_NULL ||
	    segPtr->controlPort == MACH_PORT_DEAD) {
	panic("VmCopySegment: bogus control port.\n");
    }

    VmSegmentLock(copySegPtr);
    toSegmentLocked = TRUE;

    /* 
     * Fill in the missing fields of the new segment.
     */
    if (segPtr->type == VM_HEAP) {
	copySegPtr->typeInfo.heapInfo.initStart =
	    segPtr->typeInfo.heapInfo.initStart;
	copySegPtr->typeInfo.heapInfo.initLength =
	    segPtr->typeInfo.heapInfo.initLength;
	Fsio_StreamCopy(segPtr->typeInfo.heapInfo.initFilePtr,
			&copySegPtr->typeInfo.heapInfo.initFilePtr);
	copySegPtr->typeInfo.heapInfo.initFileName =
	    ckstrdup(segPtr->typeInfo.heapInfo.initFileName);
	copySegPtr->typeInfo.heapInfo.mapPtr =
	    VmCopyFileMap(segPtr->typeInfo.heapInfo.mapPtr);
	vmStat.objPagesCopied += segPtr->typeInfo.heapInfo.mapPtr->arraySize;
    } else {
	copySegPtr->typeInfo.stackInfo.baseAddr = 
	    segPtr->typeInfo.stackInfo.baseAddr;
    }
    copySegPtr->flags |= VM_COPY_TARGET;

 bailOut:
    if (status == SUCCESS) {
#if VM_KNOWS_SWAP_FILE_SIZE
	swapSize = segPtr->swapFileSize;
#else
	if (segPtr->swapFilePtr == NULL) {
	    swapSize = 0;
	} else {
	    status = VmGetAttrStream(segPtr->swapFilePtr, &fileAttributes);
	    swapSize = fileAttributes.size;
	}
#endif
    }
    if (status == SUCCESS) {
	vmStat.segmentCopies++;
	vmStat.swapPagesCopied += swapSize / vm_page_size;
    }
    if (fromSegmentLocked) {
	if (status != SUCCESS) {
	    segPtr->flags &= ~VM_COPY_SOURCE;
	}
	VmSegmentUnlock(segPtr);
    }
    if (toSegmentLocked) {
	if (status != SUCCESS) {
	    copySegPtr->flags &= ~VM_COPY_TARGET;
	}
	VmSegmentUnlock(copySegPtr);
    }
    if (status == SUCCESS) {
	*copySegPtrPtr = copySegPtr;
    } else {
	if (copySegPtr != NULL) {
	    Vm_SegmentRelease(copySegPtr);
	}
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_FileChanged --
 *
 *	This routine is called by the file system when it detects that a
 *	file has been opened for writing.  If the file corresponds to
 *	an unused sticky code segment, the segment will be deleted.
 *	
 * Results:
 *	Nils out the given segment handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Vm_FileChanged(segPtrPtr)
    Vm_Segment **segPtrPtr;
{
    Vm_Segment *segPtr;

    LOCK_MONITOR;
    segPtr = *segPtrPtr;
    /* 
     * There are a couple reasons that the segment handle might be nil.  
     * First, maybe the file was never associated with a segment.  Second, 
     * maybe the file was once associated with a segment, but the segment 
     * got destroyed.
     */
    if (segPtr != (Vm_Segment *)NIL) {
	VmSegmentLock(segPtr);
	if (segPtr->refCount != 0) {
	    panic("Vm_FileChanged: segment %s is still in use.\n",
		  Vm_SegmentName(segPtr));
	}
	if (segPtr->type != VM_CODE) {
	    printf("%s: warning: segment %s is not code (type %d).\n",
		   "Vm_FileChanged", Vm_SegmentName(segPtr), segPtr->type);
	}
	
	VmSegmentDestroy(segPtr);
	*segPtrPtr = (Vm_Segment *) NIL;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_SyncAll --
 *
 *	Flush any dirty VM pages.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Vm_SyncAll()
{
    List_Links cleaningListHdr;	/* list of segments to be cleaned */
    List_Links *itemPtr;	/* element in a list */
    VmCleaningElement *cleanEltPtr;
    Vm_Segment *segPtr;

    vmStat.syncCalls++;

    /* 
     * Make a copy of the current list of alive segments.  This is to keep
     * segments from going away and to protect us from possible changes to
     * the master segment list.
     */
    
    List_Init(&cleaningListHdr);
    LOCK_MONITOR;
    LIST_FORALL(allSegList, itemPtr) {
	segPtr = (Vm_Segment *)itemPtr;
	if (segPtr->state == VM_SEGMENT_DEAD) {
	    continue;
	}
	Vm_SegmentAddRef(segPtr);
	cleanEltPtr = (VmCleaningElement *)ckalloc(sizeof(VmCleaningElement));
	List_InitElement((List_Links *)cleanEltPtr);
	cleanEltPtr->segPtr = segPtr;
	List_Insert((List_Links *)cleanEltPtr,
		    LIST_ATREAR(&cleaningListHdr));
    }
    UNLOCK_MONITOR;

    /* 
     * Run through the list, making sure that all dirty pages in each 
     * segment get flushed back.
     */

    LIST_FORALL(&cleaningListHdr, itemPtr) {
	cleanEltPtr = (VmCleaningElement *)itemPtr;
	segPtr = cleanEltPtr->segPtr;
	VmSegmentLock(segPtr);
	if (!(segPtr->flags & VM_CLEANING_SEGMENT)) {
	    VmCleanSegPages(segPtr, FALSE, 0, TRUE);
	}
	while (segPtr->state != VM_SEGMENT_DEAD &&
	       (segPtr->flags & VM_CLEANING_SEGMENT)) {
	    (void)Sync_SlowWait(&segPtr->condition, &segPtr->lock, FALSE);
	}
	if (segPtr->state == VM_SEGMENT_DEAD) {
	    segPtr->flags &= ~VM_CLEANING_SEGMENT;
	}
	VmSegmentUnlock(segPtr);
    }

    /* 
     * Free up the cleaning list.
     */

    while (!List_IsEmpty(&cleaningListHdr)) {
	cleanEltPtr = (VmCleaningElement *)List_First(&cleaningListHdr);
	segPtr = cleanEltPtr->segPtr;
	List_Remove((List_Links *)cleanEltPtr);
	Vm_SegmentRelease(segPtr);
	ckfree(cleanEltPtr);
    }
}
