/* 
 * vmPager.c --
 *
 *	External pager interface routines for Sprite.
 *
 * Copyright 1991, 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmPager.c,v 1.27 92/07/16 18:05:21 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <bstring.h>
#include <status.h>
#include <stdlib.h>
#include <mach.h>
#include <mach_error.h>

#include <main.h>		/* for debug flag */
#include <user/fs.h>
#include <fs.h>
#include <fsutil.h>
#include <procMach.h>
#include <sync.h>
#include <sys.h>
#include <timer.h>
#include <utils.h>
#include <vm.h>
#include <vmInt.h>
#include <user/vmStat.h>

/* 
 * When the kernel asks for data, we read it into this buffer and 
 * pass the buffer to the kernel.
 * XXX Assume that the buffer is at most 1 page long.
 */
static Address pagingBuffer = NULL;

static Sync_Lock bufferLock = Sync_LockInitStatic("vmPager:bufferLock");

/* 
 * Some write errors may be temporary.  This is how long we wait 
 * before trying again.
 */
static Time retryTime = {30, 0}; /* thirty seconds */


/* forward references: */

static Boolean ControlPortOkay _ARGS_((Vm_Segment *segPtr,
		mach_port_t controlPort, char *functionName));
static ReturnStatus FillBuffer _ARGS_((Fs_Stream *filePtr, off_t fileSize,
		off_t offset, vm_size_t bytesWanted,
		Fs_PageType pageType, Vm_Segment *segPtr, Address buffer));
static Boolean IsFatalError _ARGS_((int errorCode));
static ReturnStatus WhichFile _ARGS_((Vm_Segment *segPtr,
		vm_offset_t segOffset, int readWrite,
		Fs_Stream **filePtrPtr, off_t *fileOffsetPtr,
		Fs_PageType *pageTypePtr, off_t *fileSizePtr));
static ReturnStatus WhichHeapFile _ARGS_((Vm_Segment *segPtr,
		vm_offset_t segOffset, int readWrite,
		Fs_Stream **filePtrPtr, off_t *fileOffsetPtr,
		Fs_PageType *pageTypePtr, off_t *fileSizePtr));
static ReturnStatus WhichStackFile _ARGS_((Vm_Segment *segPtr,
		vm_offset_t segOffset, int readWrite,
		Fs_Stream **filePtrPtr, off_t *fileOffsetPtr,
		Fs_PageType *pageTypePtr, off_t *fileSizePtr));
static ReturnStatus WriteBack _ARGS_((Vm_Segment *segPtr,
		Fs_Stream *filePtr, off_t fileOffset, Address buffer));


/*
 *----------------------------------------------------------------------
 *
 * memory_object_init --
 *
 *	Finish initialization for a segment (memory object) and tell 
 *	the kernel it's ready.
 *
 * Results:
 *	Mach status code.
 *
 * Side effects:
 *	Remembers the control port for the segment.  Allocates the 
 *	paging buffer if it doesn't already exist.
 *
 *----------------------------------------------------------------------
 */
    
kern_return_t
memory_object_init(objectPort, controlPort, namePort, size)
    mach_port_t objectPort;	/* request port for the segment */
    mach_port_t controlPort;	/* (new) control port for the segment */
    mach_port_t namePort;	/* (new) name for the segment that 
				 * vm_region will return */
    vm_size_t size;		/* page size associated with the segment */
{
    Vm_Segment *segPtr;
    kern_return_t kernStatus = KERN_SUCCESS;
    Time startTime, endTime;	/* instrumentation */

    vmStat.initCalls++;
    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /* 
     * We don't hand out ports for the memory objects, so only the local 
     * kernel should make memory_object_init requests.  Hence the page size 
     * should be constant.
     */
    if (size != vm_page_size) {
	panic("memory_object_init: different page size.\n");
    }

    segPtr = Vm_PortToSegment(objectPort);
    /* 
     * XXX If the port is bogus, how do we tell the kernel that?
     */
    if (segPtr == NULL) {
	printf("memory_object_init: bogus segment.\n");
	return KERN_INVALID_CAPABILITY;
    }

    VmSegmentLock(segPtr);

    /* 
     * If the segment is dead, record the control port anyway, so that when 
     * the object is destroyed we have our bookkeeping straight.
     */
    if (segPtr->state == VM_SEGMENT_DYING ||
		segPtr->state == VM_SEGMENT_DEAD ) {
	printf("%s: warning: trying to resurrect a dead segment (%s).\n",
	       "memory_object_init", Vm_SegmentName(segPtr));
    }

    /* 
     * XXX It appears that even if a vm_map call fails, the kernel might 
     * still issue memory object requests on that memory object.  For the 
     * time being, just kill the memory object.  We need a better long-term 
     * solution, though.
     */
    if (segPtr->controlPort == MACH_PORT_NULL) {
	printf("memory_object_init: segment ``%s'' has a null control port.\n",
	      Vm_SegmentName(segPtr));
	(void)memory_object_destroy(controlPort, 0);
    } else if (segPtr->controlPort != MACH_PORT_DEAD) {
	printf("%s: warning: overwriting control port for segment ``%s''.\n",
	       "memory_object_init", Vm_SegmentName(segPtr));
    }

    segPtr->controlPort = controlPort;
    segPtr->namePort = namePort;
    if (segPtr->state == VM_SEGMENT_OK) {
	kernStatus = memory_object_ready(controlPort, TRUE, 
					 MEMORY_OBJECT_COPY_DELAY);
	if (kernStatus != KERN_SUCCESS) {
	    printf("Can't enable segment: %s\n",
		   mach_error_string(kernStatus));
	}
    }

    VmSegmentUnlock(segPtr);
    Vm_SegmentRelease(segPtr);

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, vmStat.initTime, &vmStat.initTime);
    }

    return kernStatus;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_data_write --
 *
 *	Take dirty data back from the kernel (obsolete).
 *
 * Results:
 *	Mach status code.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
memory_object_data_write(objectPort, controlPort, segStart, data, dataCount)
    mach_port_t objectPort;	/* the object being cleaned */
    mach_port_t controlPort;	/* its control port */
    vm_offset_t segStart;	/* where in segment to start writing back */
    pointer_t data;		/* the bytes */
    mach_msg_type_number_t dataCount; /* number of bytes to write back */
{
#ifdef lint
    objectPort = objectPort;
    controlPort = controlPort;
    segStart = segStart;
    data = data;
    dataCount = dataCount;
#endif

    panic("memory_object_data_write called.\n");
    return KERN_SUCCESS;	/* lint */
}


/*
 *----------------------------------------------------------------------
 *
 * WriteBack --
 *
 *	Write a page of memory to the given file at the given offset.  
 *	Retries if there is a non-fatal error.
 *
 * Results:
 *	Returns a Sprite status code.
 *
 * Side effects:
 *	Updates the swap file size for the segment.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
WriteBack(segPtr, filePtr, fileOffset, buffer)
    Vm_Segment *segPtr;		/* segment being cleaned */
    Fs_Stream *filePtr;		/* file to write back */
    off_t fileOffset;		/* where in the file */
    Address buffer;		/* page of data to write back */
{
    ReturnStatus status;
    Boolean retry;

    /* 
     * If there is a fatal I/O error, bail out.  If the server seems to be 
     * dead, wait for it to come back.  If there is some non-fatal I/O
     * error, complain, sleep for an interval, and try again.
     * XXX Should probably check whether some other process has marked the 
     * segment as dead (or dying).  Need to lock the segment to do that, of 
     * course. 
     */
    do {
	retry = FALSE;
	status = Fs_PageWrite(filePtr, (Address)buffer, (int)fileOffset,
			      (int)vm_page_size, FALSE);
	if (status == SUCCESS) {
	    vmStat.pagesWritten[segPtr->type]++;
	    if (segPtr->flags & VM_CLEANING_SEGMENT) {
		vmStat.pagesCleaned[segPtr->type]++;
	    }
	} else {
	    printf("WriteBack: can't write back %s: %s\n",
		   Fsutil_GetFileName(filePtr),
		   Stat_GetMsg(status));
	    if (!sys_ShuttingDown && !IsFatalError(status)) {
		if (Fsutil_RecoverableError(status)) {
		    status = Fsutil_WaitForHost(filePtr, 0, status);
		    if (status == SUCCESS) {
			retry = TRUE;
		    } else {
			printf("WriteBack: recovery failed for %s: %s\n",
			       Fsutil_GetFileName(filePtr),
			       Stat_GetMsg(status));
		    }
		} else {
		    retry = TRUE;
		    (void)Sync_WaitTime(retryTime);
		}
	    }
	}
    } while (retry);

#if VM_KNOWS_SWAP_FILE_SIZE
    if (status == SUCCESS) {
	if (segPtr->swapFileSize < fileOffset + vm_page_size) {
	    segPtr->swapFileSize = fileOffset + vm_page_size;
	}
    }
#endif

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_data_request --
 *
 *	Supply bytes to the kernel.
 *	
 *	All pages given to the kernel are marked precious.  Code pages must 
 *	be precious so that we can clean up sticky segments after they've 
 *	been paged out.  Heap and stack pages must be precious so that our 
 *	copy-on-write scheme will work.  Might as well make shared (mapped 
 *	file) pages precious, too, especially since they're not used much 
 *	in Sprite.
 *
 * Results:
 *	Returns KERN_SUCCESS.  (See memory_object_data_provided.)
 *
 * Side effects:
 *	Overwrites the paging buffer.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
memory_object_data_request(objectPort, controlPort, segOffset, bytesWanted,
			   desiredAccess)
    mach_port_t objectPort;	/* the segment to read from */
    mach_port_t controlPort;	/* its control port */
    vm_offset_t segOffset;	/* where in the segment to start reading */
    vm_size_t bytesWanted;	/* number of bytes to read */
    vm_prot_t desiredAccess;	/* requested permissions for the data; 
				 * unused */
{
    Vm_Segment *segPtr;
    ReturnStatus status = SUCCESS;
    Fs_Stream *filePtr;		/* file to actually read from */
    off_t fileOffset;		/* where in the file to read from */
    off_t fileSize;		/* size of the file */
    Fs_PageType pageType;	/* heap, swap, etc. */
    Time startTime;		/* instrumentation */
    Time endTime;		/* ditto */
    Boolean isCopy = FALSE;	/* is this for a segment copy operation? 
				 * (instrumentation) */

    if (bytesWanted > vm_page_size) {
	panic("memory_object_data_request: kernel wants more than a page\n");
    }

    /* 
     * Get the segment that's to be read from and do some sanity checks. 
     */
    segPtr = Vm_PortToSegment(objectPort);
    if (segPtr == NULL) {
	status = FAILURE;	/* XXX - be more specific? */
    } else {
	VmSegmentLock(segPtr);
	if (!ControlPortOkay(segPtr, controlPort,
			     "memory_object_data_request")) {
	    status = FAILURE;	/* XXX - be more specific? */
	}
	if (segPtr->state == VM_SEGMENT_DYING ||
	    	segPtr->state == VM_SEGMENT_DEAD) {
	    printf("memory_object_data_request: dead segment.\n");
	    status = VM_SWAP_ERROR;
	}
    }

    if (status == SUCCESS && segOffset + bytesWanted > segPtr->size) {
	printf("memory_object_data_read: kernel requested 0x%x bytes",
	       bytesWanted);
	printf(" starting at 0x%x\n", segOffset);
	printf("(segment size is 0x%x\n", segPtr->size);
	status = FS_INVALID_ARG; /* XXX be more specific */
    }
    isCopy = (segPtr->flags & (VM_COPY_SOURCE | VM_COPY_TARGET));
    if (segPtr != NULL) {
	VmSegmentUnlock(segPtr);
    }

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /* 
     * Figure out which file to read from and read in the bytes.
     */

    Sync_GetLock(&bufferLock);
    if (status == SUCCESS) {
	status = WhichFile(segPtr, segOffset, FS_READ, &filePtr, &fileOffset,
			   &pageType, &fileSize);
	if (status == SUCCESS) {
	    status = FillBuffer(filePtr, fileSize, fileOffset, bytesWanted,
				pageType, segPtr, pagingBuffer);
	}
    }

    if (status != SUCCESS) {
	printf("Can't satisfy page-in request for segment %s ",
	       Vm_SegmentName(segPtr));
	printf("(0x%x bytes starting at segment offset 0x%x):\n", 
	       bytesWanted, segOffset);
	printf("%s\n", Stat_GetMsg(status));
	(void)memory_object_data_error(controlPort, segOffset, bytesWanted,
				       status);
    } else {
	/* 
	 * Give the bytes to the kernel.  Write-protect code pages.
	 * Provide no special protection for other pages.
	 */
	(void)memory_object_data_supply(controlPort, segOffset,
					(pointer_t)pagingBuffer,
					bytesWanted, FALSE,
					(segPtr->type == VM_CODE
					 ? VM_PROT_WRITE
					 : VM_PROT_NONE),
					TRUE, MACH_PORT_NULL);
	VmSegmentLock(segPtr);
	segPtr->residentPages++;
	VmSegmentUnlock(segPtr);
    }

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(vmStat.readTime[segPtr->type], endTime,
		 &vmStat.readTime[segPtr->type]);
	if (isCopy) {
	    Time_Add(vmStat.readCopyTime[segPtr->type], endTime,
		     &vmStat.readCopyTime[segPtr->type]);
	}
    }

    Sync_Unlock(&bufferLock);

    if (segPtr != NULL) {
	Vm_SegmentRelease(segPtr);
    } 
    (void)mach_port_deallocate(mach_task_self(), controlPort);

    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_terminate --
 *
 *	Clean up for an unused segment.
 *
 * Results:
 *	Returns KERN_SUCCESS.
 *
 * Side effects:
 *	Destroys the control and name ports.  If there are no references 
 *	left, the segment is destroyed.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
memory_object_terminate(objectPort, controlPort, namePort)
    memory_object_t	objectPort; /* the memory object */
    memory_object_control_t	controlPort;
    memory_object_name_t	namePort;
{
    kern_return_t kernStatus;
    Vm_Segment *segPtr;
    Time startTime, endTime;	/* instrumentation */

    vmStat.terminateCalls++;
    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /* 
     * Figure out which segment is supposed to go away and do some sanity 
     * checks. 
     */
    segPtr = Vm_PortToSegment(objectPort);
    if (segPtr == NULL) {
	printf("memory_object_terminate: unknown segment.\n");
	(void)mach_port_destroy(mach_task_self(), controlPort);
	(void)mach_port_destroy(mach_task_self(), namePort);
	return KERN_INVALID_CAPABILITY;
    }

    VmSegmentLock(segPtr);
    (void)ControlPortOkay(segPtr, controlPort,
			  "memory_object_terminate");

    if (segPtr->flags & VM_CLEANING_SEGMENT) {
	/* 
	 * Panic, so that we can poke around and see what's happening.
	 */
	panic("memory_object_terminate: segment %s is still being cleaned?\n",
	      Vm_SegmentName(segPtr));
	segPtr->flags &= ~VM_CLEANING_SEGMENT;
	/* 
	 * XXX Should we just always do this broadcast, whether or not the
	 * "cleaning" flag is set?
	 */
	Sync_Broadcast(&segPtr->condition);
    }

    /* 
     * Check for a possible race condition where a new init request 
     * arrived before the terminate request.  If that happened, the 
     * control port will be different.  If there was a new init request but 
     * the segment is being shut down, complain, but wait for a second 
     * terminate request before nulling out the control port.
     */
    if (segPtr->controlPort == controlPort) {
	segPtr->controlPort = MACH_PORT_NULL;
    } else {
	if (segPtr->state != VM_SEGMENT_OK) {
	    printf("%s: re-initialized a dead segment (%s).\n",
		   "memory_object_terminate", Vm_SegmentName(segPtr));
	}
    }

    /* 
     * If the segment is dying, the "terminate" request means that 
     * there are no dirty pages to write back, so mark the segment as 
     * really dead. 
     */
    if (segPtr->state != VM_SEGMENT_DEAD) {
	segPtr->state = VM_SEGMENT_DEAD;
	Sync_Broadcast(&segPtr->condition);
    }

    /* 
     * The other routines (data_request, etc.) should have been 
     * freeing their references to the control port as the references
     * were obtained.  So we could theoretically get rid of the
     * control and name ports by deallocating the one or two remaining
     * send references and the receive reference.  But rather than
     * fiddling around with trying to get the reference counts right,
     * let's just blow the ports out of the water and be done with it.
     */

    kernStatus = mach_port_destroy(mach_task_self(), controlPort);
    if (kernStatus != KERN_SUCCESS) {
	printf("Can't destroy control port for segment %s: %s\n",
	       Vm_SegmentName(segPtr), mach_error_string(kernStatus));
    }
    kernStatus = mach_port_destroy(mach_task_self(), namePort);
    if (kernStatus != KERN_SUCCESS) {
	printf("Can't destroy name port for segment %s: %s\n",
	       Vm_SegmentName(segPtr), mach_error_string(kernStatus));
    }

    VmSegmentUnlock(segPtr);
    Vm_SegmentRelease(segPtr);

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, vmStat.terminateTime, &vmStat.terminateTime);
    }

    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_copy --
 *
 *	Handle notification that an object has been copied.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	panics--shouldn't ever get called.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
memory_object_copy(oldObjectPort, controlPort, offset, length,
		   newObjectPort)
    memory_object_t	oldObjectPort;
    memory_object_control_t	controlPort;
    vm_offset_t		offset;
    vm_size_t		length;
    memory_object_t	newObjectPort;
{
    panic("memory_object_copy called.\n");
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_data_unlock --
 *
 *	Handle a request from the kernel to change the protection of some
 *	memory.  We normally use permissions at the level of vm_map and
 *	vm_protect to control access to memory.  The only exception so far
 *	is when we're flushing a dirty segment, in which case we wait for 
 *	the flush operation to complete before re-enabling permissions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
memory_object_data_unlock(objectPort, controlPort, offset, length,
			  desiredAccess)
    memory_object_t	objectPort;
    memory_object_control_t	controlPort;
    vm_offset_t		offset;
    vm_size_t		length;
    vm_prot_t		desiredAccess;
{
    Vm_Segment *segPtr;
    char *segName;

    /* 
     * Print a debugging message with the name of the segment.
     */
    segPtr = Vm_PortToSegment(objectPort);
    segName = (segPtr == NULL) ? "<bogus segment>" : Vm_SegmentName(segPtr);
    printf("memory_object_data_unlock request for %s.\n", segName);

    /* 
     * Drop the request on the floor.
     */
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_lock_completed --
 *
 *	Handle notification from the server that a 
 *	memory_object_lock_request call has been completed.  Currently 
 *	this is used only to indicate that a segment's dirty pages 
 *	have all been flushed to disk.  (Because the operation is on the 
 *	entire segment, the "offset" and "length" parameters are ignored.)
 *
 * Results:
 *	Returns KERN_SUCCESS, which is ignored.
 *
 * Side effects:
 *	Clears the flag telling that the segment is being cleaned.  If 
 *	the segment is being destroyed, tell the kernel to kill it.
 *	Does NOT unprotect the segment--that's up to the code that 
 *	originally invoked the cleaning operation.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
memory_object_lock_completed(objectPort, controlPort, offset, length)
    memory_object_t	objectPort;
    memory_object_control_t	controlPort;
    vm_offset_t		offset;
    vm_size_t		length;
{
    Vm_Segment *segPtr;
    Time startTime, endTime;	/* instrumentation */

    vmStat.lockCompletedCalls++;
    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /* 
     * Figure out which segment is referred to, and do some sanity checks. 
     */
    segPtr = Vm_PortToSegment(objectPort);
    if (segPtr == NULL) {
	printf("memory_object_lock_completed: bogus segment.\n");
	goto punt;
    }
    VmSegmentLock(segPtr);
    if (!ControlPortOkay(segPtr, controlPort,
			 "memory_object_lock_completed")) {
	goto punt;
    }

    /* 
     * If there was an I/O error flushing the segment, its "cleaning" flag 
     * might have gotten turned off.
     */
    if (!(segPtr->flags & VM_CLEANING_SEGMENT)
	    && segPtr->state == VM_SEGMENT_OK) {
	printf("memory_object_lock_completed: wasn't cleaning %s?",
	       Vm_SegmentName(segPtr));
    }
    segPtr->flags &= ~VM_CLEANING_SEGMENT;
    Sync_Broadcast(&segPtr->condition);

    /* 
     * If the segment is being destroyed and all the dirty pages have 
     * been flushed to disk, then it's time to kill the memory object.  
     * Final cleanup will be done when the kernel notifies us that 
     * it's done with the memory object.
     */
    if (segPtr->state == VM_SEGMENT_DYING) {
	segPtr->state = VM_SEGMENT_DEAD;
    }
    if (segPtr->state == VM_SEGMENT_DEAD) {
	(void)memory_object_destroy(segPtr->controlPort, 0);
    } 

 punt:
    if (segPtr != NULL) {
	VmSegmentUnlock(segPtr);
	Vm_SegmentRelease(segPtr);
    }
    (void)mach_port_deallocate(mach_task_self(), controlPort);

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, vmStat.lockCompletedTime,
		 &vmStat.lockCompletedTime);
    }

    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * FillBuffer --
 *
 *	Set up a page to pass to the kernel.
 *
 * Results:
 *	Returns a Sprite status code and fills in the given buffer (usually 
 *	zero-fill or read a page from the backing stream).
 *
 * Side effects:
 *	Might reposition the backing stream.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
FillBuffer(filePtr, fileLength, offset, bytesToRead, pageType, segPtr,
	   buffer)
    Fs_Stream *filePtr;		/* the file to read from; possibly nil */
    off_t fileLength;		/* number of bytes in the file */
    off_t offset;		/* where in the file to start reading */
    vm_size_t bytesToRead;	/* number of bytes to read */
    Fs_PageType pageType;	/* CODE, HEAP, or SWAP */
    Vm_Segment *segPtr;		/* the segment that needs bytes */
    Address buffer;		/* where to put the characters; should 
				 * be protected against concurrent access */
{
    ReturnStatus status;
    int bufferLength;		/* number of bytes in the buffer */
#if !VM_KNOWS_SWAP_FILE_SIZE
    Fs_Attributes fileAttributes;
#endif
    
    bufferLength = bytesToRead;

#if VM_KNOWS_SWAP_FILE_SIZE
    if (fileLength == -1) {
	panic("FillBuffer: file size was never initialized.\n");
    }
#else
    if (filePtr != NULL) {
	status = VmGetAttrStream(filePtr, &fileAttributes);
	if (status != SUCCESS) {
	    return status;
	}
	fileLength = fileAttributes.size;
    }
#endif

    /* 
     * There are generally three possibilities: (a) the requested page does
     * not intersect the backing file, so no read is necessary; (b) the
     * requested page is the last page of the file, but the file is not an 
     * integral number of pages, so a partial read is necessary; (c) the
     * requested page can be satisfied completely from the backing file, so
     * a whole-page read is necessary.
     * 
     * The exception to the above rule is a performance hack.  When a swap
     * segment is being copied, there's no need to get the correct page for 
     * the target (new) segment, as it will be overwritten via vm_write.  
     * Unfortunately, it's not clear whether this hack is actually a win.
     */
    if (offset >= fileLength || (segPtr->flags & VM_COPY_TARGET)) {
	bytesToRead = 0;				/* (a) */
	vmStat.pagesZeroed[segPtr->type]++;
    } else if (offset + bytesToRead > fileLength) {
	bytesToRead = fileLength - offset;		/* (b) */
	vmStat.partialPagesRead[segPtr->type]++;
    } else {
	vmStat.pagesRead[segPtr->type]++;		/* (c) */
    }

    if (bytesToRead == 0) {
	if (segPtr->flags & VM_COPY_SOURCE) {
	    vmStat.sourceCopyZeroed++;
	} else if (segPtr->flags & VM_COPY_TARGET) {
	    vmStat.targetCopyZeroed++;
	}
    } else {
	if (segPtr->flags & VM_COPY_SOURCE) {
	    vmStat.sourceCopyRead++;
	} else if (segPtr->flags & VM_COPY_TARGET) {
	    vmStat.targetCopyRead++;
	}
    }

    if (bytesToRead != 0) {
	if (filePtr == NULL) {
	    panic("FillBuffer: nil file handle.\n");
	}
	status = Fs_PageRead(filePtr, buffer, (int)offset,
			     (int)bytesToRead, pageType);
	if (status != SUCCESS) {
	    return status;
	}
    }

    if (bytesToRead < bufferLength) {
	bzero(buffer+bytesToRead, (size_t)(bufferLength - bytesToRead));
    }

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * VmCleanSegPages --
 *
 *	Ask the kernel to clean any dirty pages belonging to the given 
 *	locked segment.  
 *
 * Results:
 *	None.  If the caller needs to wait for the writebacks, it should
 *	wait until the VM_CLEANING_SEGMENT flag is clear.
 *
 * Side effects:
 *	Can start write operations on the dirty pages.  Write-protects the 
 *	segment if requested.
 *
 *----------------------------------------------------------------------
 */

void
VmCleanSegPages(segPtr, writeProtect, numBytes, fromFront)
    Vm_Segment *segPtr;
    Boolean writeProtect;	/* prevent user processes from dirtying the 
				 * segment */
    vm_size_t numBytes;		/* number of bytes to clean; 0 means entire
				 * segment */
    Boolean fromFront;		/* clean bytes at front of segment or at 
				 * end? */ 
{
    kern_return_t kernStatus;
    vm_offset_t firstClean;	/* first byte in segment to clean */
    vm_size_t bytesClean;	/* bytes to clean */

    /* 
     * If we've already decided that writebacks won't work, don't bother.
     */
    if (segPtr->state == VM_SEGMENT_DEAD) {
	return;
    }

    /* 
     * If the segment's control port is "dead", it means the segment 
     * has been registered with the kernel, but the kernel hasn't done 
     * anything with it.  Thus there can't be any pages to flush, so 
     * we can quit now.
     */
    if (segPtr->controlPort == MACH_PORT_DEAD) {
	return;
    }

    /* 
     * If the control port is null, it means the segment hasn't been 
     * registered yet.  This is a bad sign and deserves investigation. 
     */
    if (segPtr->controlPort == MACH_PORT_NULL) {
	panic("VmCleanSegPages: unregistered segment.\n");
    }

    segPtr->flags |= VM_CLEANING_SEGMENT;

    if (numBytes == 0) {
	firstClean = 0;
	bytesClean = segPtr->size;
    } else {
	firstClean = (fromFront ? 0 : segPtr->size - numBytes);
	bytesClean = numBytes;
    }
    kernStatus = memory_object_lock_request(segPtr->controlPort,
					    trunc_page(firstClean),
					    round_page(bytesClean), TRUE,
					    FALSE,
					    (writeProtect
					     ? VM_PROT_WRITE
					     : VM_PROT_NO_CHANGE),
					    segPtr->requestPort);
    if (kernStatus != KERN_SUCCESS) {
	printf("VmCleanSegPages: lock request failed: %s\n",
	       mach_error_string(kernStatus));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * IsFatalError --
 *
 *	Decide whether an operation should be retried, given an error 
 *	code.
 *
 * Results:
 *	TRUE if the error is fatal (don't retry); FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
IsFatalError(errorCode)
    int errorCode;		/* Sprite error code */
{
    /* 
     * XXX there are probably a bunch more recoverable errors; talk to 
     * the other Spriters to find out which ones.
     */
    return (errorCode != FS_NO_DISK_SPACE && errorCode != SUCCESS &&
	    !Fsutil_RecoverableError(errorCode));
}


/*
 *----------------------------------------------------------------------
 *
 * VmPagerInit --
 *
 *	Initialization for the pager routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates the page-in buffer.
 *
 *----------------------------------------------------------------------
 */

void
VmPagerInit()
{
    vm_allocate(mach_task_self(), (vm_address_t *)&pagingBuffer,
		vm_page_size, TRUE);
    if (pagingBuffer == NULL) {
	panic("Can't create paging buffer.\n");
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ControlPortOkay --
 *
 *	Paranoia checks on a segment's control ports.
 *
 * Results:
 *	Returns TRUE if everything looks good.  Returns FALSE if the 
 *	segment's control port doesn't match the given control port.
 *
 * Side effects:
 *	Panics if the segment's control port don't indicate an 
 *	initialized memory object.
 *
 *----------------------------------------------------------------------
 */

static Boolean
ControlPortOkay(segPtr, givenControlPort, functionName)
    Vm_Segment *segPtr;		/* the segment to check, locked */
    mach_port_t givenControlPort;
    char *functionName;		/* name of the calling function */
{
    /* 
     * The check against MACH_PORT_NULL is especially important, 
     * because if the registered control port is null, that increases 
     * the likelihood that the segment will get incorrectly freed.  
     * The check against MACH_PORT_DEAD is just extra paranoia.
     */
    if (segPtr->controlPort == MACH_PORT_NULL
		|| segPtr->controlPort == MACH_PORT_DEAD) {
	panic("ControlPortOkay: %s got bogus control port.\n", functionName);
    }
    if (segPtr->controlPort != givenControlPort) {
	printf("%s: control port mismatch.\n", functionName);
	return FALSE;
    }

    return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * WhichFile --
 *
 *	Given a segment and an offset, figure out which backing file 
 *	to use and where in the file to go.
 *
 * Results:
 *	Returns a Sprite status code.  If successful, fills in the file 
 *	pointer, file offset, FS page type, and file size.  If the file 
 *	doesn't exist, the pointer is set to nil and the size is set to 0.
 *
 * Side effects:
 *	Opens the swap file if writing to a heap or stack segment for the 
 *	first time. 
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WhichFile(segPtr, segOffset, readWrite, filePtrPtr, fileOffsetPtr,
	  pageTypePtr, fileSizePtr)
    Vm_Segment *segPtr;		/* the segment we're doing I/O for; 
				 * should be locked */
    vm_offset_t segOffset;	/* where in the segment */
    int readWrite;		/* FS_READ or FS_WRITE */
    Fs_Stream **filePtrPtr;	/* OUT: which backing file to use */
    off_t *fileOffsetPtr;	/* OUT: where in the file to go */
    Fs_PageType *pageTypePtr;	/* OUT: FS type for the page */
    off_t *fileSizePtr;		/* OUT: size of the backing file */
{
    ReturnStatus status = SUCCESS;

    switch (segPtr->type) {
    case VM_CODE:
	*filePtrPtr = segPtr->swapFilePtr;
	*fileOffsetPtr = segOffset;
	*pageTypePtr = FS_CODE_PAGE;
	*fileSizePtr = segPtr->swapFileSize;
	break;
    case VM_HEAP:
	if (segOffset != trunc_page(segOffset)) {
	    panic("WhichFile: heap segment offset not page-aligned.\n");
	}
	status = WhichHeapFile(segPtr, segOffset, readWrite, filePtrPtr,
			       fileOffsetPtr, pageTypePtr, fileSizePtr);
	break;
    case VM_STACK:
	if (segOffset != trunc_page(segOffset)) {
	    panic("WhichFile: stack segment offset not page-aligned.\n");
	}
	status = WhichStackFile(segPtr, segOffset, readWrite, filePtrPtr,
			       fileOffsetPtr, pageTypePtr, fileSizePtr);
	break;
    case VM_SHARED:
	*filePtrPtr = segPtr->swapFilePtr;
	*fileOffsetPtr = segOffset;
	*pageTypePtr = FS_SHARED_PAGE;
	*fileSizePtr = segPtr->swapFileSize;
	break;
    default:
	panic("WhichFile: bogus segment type.\n");
	break;
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * WhichHeapFile --
 * 
 *	Given a heap segment and an offset, figure out which backing 
 *	file to use and where in the file to go.
 *
 * Results:
 *	Returns a Sprite status code.  If successful, fills in the file 
 *	pointer (possibly nil), file offset, FS page type, and file size.
 *
 * Side effects:
 *	For write operations, might update the mapping telling whether 
 *	to use the initialization file, and might open the swap file.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
WhichHeapFile(segPtr, segOffset, readWrite, filePtrPtr, fileOffsetPtr,
	      pageTypePtr, fileSizePtr)
    Vm_Segment *segPtr;		/* the heap segment we're doing I/O for; 
				 * should be locked */
    vm_offset_t segOffset;	/* where in the segment */
    int readWrite;		/* FS_READ or FS_WRITE */
    Fs_Stream **filePtrPtr;	/* OUT: which backing file to use */
    off_t *fileOffsetPtr;	/* OUT: where in the file to go */
    Fs_PageType *pageTypePtr;	/* OUT: HEAP or SWAP */
    off_t *fileSizePtr;		/* OUT: size of the backing file */
{
    VmFileMap *mapPtr;		/* the map for the segment, telling 
				 * which file to use */
    char *useInitFilePtr;	/* ptr to the entry for the given offset */
    int pageNum;		/* page number for segment offset */
    Boolean useInitFile = FALSE;
    ReturnStatus status = SUCCESS;
#if !VM_KNOWS_SWAP_FILE_SIZE
    Fs_Attributes fileAttributes;
#endif

    /* 
     * Figure out whether to use the initialization (object) file or the 
     * swap file.
     */
    mapPtr = segPtr->typeInfo.heapInfo.mapPtr;
    pageNum = Vm_ByteToPage(segOffset);
    if (pageNum >= mapPtr->arraySize) {
	useInitFile = FALSE;
    } else {
	useInitFilePtr = mapPtr->useInitPtr + pageNum;
	if (readWrite == FS_WRITE) {
	    *useInitFilePtr = FALSE;
	}
	useInitFile = *useInitFilePtr;
    }

    /* 
     * Open the swap file if necessary.
     */
    if (readWrite == FS_WRITE && segPtr->swapFilePtr == NULL) {
	status = VmOpenSwapFile(segPtr);
	if (status != SUCCESS) {
	    return status;
	}
    }

    /*
     * Get the results for whichever file we decided to use.
     */
    if (useInitFile) {
	*filePtrPtr = segPtr->typeInfo.heapInfo.initFilePtr;
	*fileOffsetPtr = segPtr->typeInfo.heapInfo.initStart + segOffset;
	*pageTypePtr = FS_HEAP_PAGE;
	*fileSizePtr = segPtr->typeInfo.heapInfo.initStart +
	    		segPtr->typeInfo.heapInfo.initLength;
    } else {
	*filePtrPtr = segPtr->swapFilePtr;
	*pageTypePtr = FS_SWAP_PAGE;
	if (segPtr->swapFilePtr == NULL) {
	    *fileOffsetPtr = 0;
	    *fileSizePtr = 0;
	} else {
	    *fileOffsetPtr = (off_t)segOffset;
#if VM_KNOWS_SWAP_FILE_SIZE
	    *fileSizePtr = segPtr->swapFileSize;
#else
	    status = VmGetAttrStream(segPtr->swapFilePtr, &fileAttributes);
	    if (status != SUCCESS) {
		return status;
	    }
	    *fileSizePtr = fileAttributes.size;
#endif
	}
    }

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * WhichStackFile --
 *
 *	Given a stack segment and an offset, get the backing file and the 
 *	corresponding offset in it.
 *
 * Results:
 *	Returns a Sprite status code.  If successful, fills in the file 
 *	pointer (possibly nil), file offset, FS page type, and file size.
 *
 * Side effects:
 *	Opens the swap file if writing to it for the first time.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
WhichStackFile(segPtr, segOffset, readWrite, filePtrPtr, fileOffsetPtr,
	      pageTypePtr, fileSizePtr)
    Vm_Segment *segPtr;		/* the heap segment we're doing I/O for; 
				 * should be locked */
    vm_offset_t segOffset;	/* where in the segment */
    int readWrite;		/* FS_READ or FS_WRITE */
    Fs_Stream **filePtrPtr;	/* OUT: which backing file to use */
    off_t *fileOffsetPtr;	/* OUT: where in the file to go */
    Fs_PageType *pageTypePtr;	/* OUT: SWAP */
    off_t *fileSizePtr;		/* OUT: size of the backing file */
{
    ReturnStatus status = SUCCESS;
#if !VM_KNOWS_SWAP_FILE_SIZE
    Fs_Attributes fileAttributes;
#endif

    /* 
     * Open the swap file if necessary.  If there's an error, the 
     * segment's swap file pointer will remain nil, so there's no need 
     * for an explicit error check here.
     */
    if (readWrite == FS_WRITE && segPtr->swapFilePtr == NULL) {
	status = VmOpenSwapFile(segPtr);
    }
    *filePtrPtr = segPtr->swapFilePtr;
    *pageTypePtr = FS_SWAP_PAGE;
    if (segPtr->swapFilePtr == NULL) {
	*fileOffsetPtr = 0;
	*fileSizePtr = 0;
    } else {
	*fileOffsetPtr = (PROCMACH_STACK_GROWS_DOWN
			  ? segPtr->size - vm_page_size - segOffset
			  : segOffset);
#if VM_KNOWS_SWAP_FILE_SIZE
	*fileSizePtr = segPtr->swapFileSize;
#else
	status = VmGetAttrStream(segPtr->swapFilePtr, &fileAttributes);
	if (status != SUCCESS) {
	    return status;
	}
	*fileSizePtr = fileAttributes.size;
#endif
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_supply_completed --
 *
 *	Called by kernel to indicate that a previous 
 *	memory_object_data_supply call has completed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
memory_object_supply_completed(objectPort, controlPort, offset, length,
			       result, error_offset)
    memory_object_t	objectPort;
    memory_object_control_t controlPort;
    vm_offset_t		offset;
    vm_size_t		length;
    kern_return_t	result;
    vm_offset_t		error_offset;
{
#ifdef	lint
    objectPort++; controlPort++; offset++;
    length++; result++; error_offset++;
#endif	lint
    panic("memory_object_supply_completed called.\n");
    return KERN_SUCCESS;	/* lint */
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_data_return --
 *
 *	Take back or clean pages from the kernel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Dirty pages are cleaned.  The resident page count for the segment 
 *	is updated.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
memory_object_data_return(objectPort, controlPort, segStart, data, dataCount,
			  dirty, kernelCopy)
    mach_port_t objectPort;	/* the object being cleaned */
    mach_port_t controlPort;	/* its control port */
    vm_offset_t segStart;	/* where in segment to start writing back */
    pointer_t data;		/* the bytes to write or deallocate */
    vm_size_t dataCount;	/* how many bytes */
    boolean_t dirty;		/* is write-back needed? */
    boolean_t kernelCopy;	/* did the kernel keep a copy? */
{
    Vm_Segment *segPtr;
    int bytesLeft;		/* number of bytes left to write */
    vm_offset_t segOffset;	/* current offset in the segment */
    ReturnStatus status = FAILURE;
    Fs_Stream *filePtr;		/* backing file to write to */
    off_t fileOffset;		/* where in the file to write to */
    off_t fileSize;		/* dummy size for WhichFile */
    Fs_PageType pageType;	/* type of page to write (ignored) */
    Time startTime;		/* instrumentation */
    Time endTime;		/* ditto */

    vmStat.returnCalls++;
    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    segPtr = Vm_PortToSegment(objectPort);
    if (segPtr == NULL) {
	printf("memory_object_data_return: bogus segment.\n");
	goto punt;
    }
    VmSegmentLock(segPtr);

    /* 
     * Sanity checks.
     */
    if (!ControlPortOkay(segPtr, controlPort, "memory_object_data_return")) {
	goto punt;
    }
    if (dirty && segPtr->state == VM_SEGMENT_DEAD) {
	printf("%s: trying to write back dead segment %s.\n",
	       "memory_object_data_return", Vm_SegmentName(segPtr));
	goto punt;
    }
    if (segStart + dataCount > segPtr->size) {
	panic("memory_object_data_return: writing off end of segment.\n");
    }
    if (dirty && segPtr->type == VM_CODE) {
	panic("memory_object_data_return: dirty code pages.\n");
    }
    if (dataCount != trunc_page(dataCount)) {
	panic("memory_object_data_return: writing partial page.\n");
    }
    if (!dirty && kernelCopy) {
	printf("memory_object_data_return: pointless call for %s\n",
	       Vm_SegmentName(segPtr));
    }

    status = SUCCESS;

    /* 
     * Write back each dirty page that was given to us.  To do this, 
     * first figure out which backing file to use and where in the file to
     * go.
     */
    for (segOffset = segStart, bytesLeft = dataCount;
		bytesLeft > 0;
		segOffset += vm_page_size, bytesLeft -= vm_page_size) {
	if (!kernelCopy) {
	    segPtr->residentPages--;
	    if (segPtr->residentPages < 0) {
		panic("memory_object_data_return: page count botch.\n");
	    }
	}
	if (!dirty) {
	    continue;
	}
	status = WhichFile(segPtr, segOffset, FS_WRITE, &filePtr, &fileOffset,
			   &pageType, &fileSize);
	if (status == SUCCESS) {
	    /* 
	     * Don't keep the segment locked while doing I/O.
	     */
	    VmSegmentUnlock(segPtr);
	    status = WriteBack(segPtr, filePtr, fileOffset,
			       (Address)(data + segOffset - segStart));
	    VmSegmentLock(segPtr);
	}
	if (status != SUCCESS) {
	    printf("memory_object_data_return: segment %s marked dead: %s\n",
		   Vm_SegmentName(segPtr), Stat_GetMsg(status));
	    segPtr->state = VM_SEGMENT_DEAD;
	    Sync_Broadcast(&segPtr->condition);
	    memory_object_destroy(controlPort, status);
	    break;
	}
    }
    
 punt:
    if (segPtr != NULL) {
	VmSegmentUnlock(segPtr);
	Vm_SegmentRelease(segPtr);
    }
    (void)vm_deallocate(mach_task_self(), data, (vm_size_t)dataCount);
    (void)mach_port_deallocate(mach_task_self(), controlPort);

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, vmStat.writeTime[segPtr->type],
		 &vmStat.writeTime[segPtr->type]);
    }

    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * memory_object_change_completed --
 *
 *	yet another unused routine we have to provide a stub for.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
memory_object_change_completed(objectPort, machCache, copyStrategy)
    memory_object_t	objectPort;
    boolean_t		machCache;
    int			copyStrategy;
{
#ifdef	lint
    objectPort++; machCache++; copyStrategy++;
#endif	lint
    panic("(inode_pager)change_completed: called");
    return KERN_SUCCESS;	/* lint */
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_Recovery --
 *
 *	The swap area has just come back up.  Wake up anyone waiting for it to
 *	come back and start up page cleaners if there are dirty pages to be
 *	written out.
 *	
 *	Currently a no-op.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Vm_Recovery()
{
}
