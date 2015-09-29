/* 
 * vmMsgQueue.c --
 *
 *	Routines for queueing and initiating processing of VM request 
 *	messages. 
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmMsgQueue.c,v 1.8 92/07/07 15:56:42 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <mach_error.h>
#include <spriteTime.h>

#include <proc.h>
#include <sync.h>
#include <sys.h>
#include <vm.h>
#include <vmInt.h>
#include <user/vmStat.h>

/* 
 * This flag enables some debug printf's in the code path for enqueueing VM 
 * requests. 
 */
Boolean vmRequestDebug = FALSE;

/* 
 * Keep track of how many threads are currently processing memory object 
 * requests.
 */
static Sync_Lock requestCountLock = Sync_LockInitStatic("vm:requestCountLock");
static int vmThreadsActive = 0;

/* 
 * This is the maximum number of requests that we will queue up for a 
 * single segment.  Once this limit is reached, the segment is removed from
 * the system request port set until all the segment's requests are 
 * processed. 
 */
int vm_MaxPendingRequests = 5;	/* untuned */

/* 
 * This declaration really oughta be gotten from a Mach header file 
 * somewhere... 
 */
extern boolean_t memory_object_server _ARGS_((mach_msg_header_t *requestPtr,
			mach_msg_header_t *replyPtr));

/* Forward declarations: */
static ReturnStatus DequeueBuffers _ARGS_((Vm_Segment *segPtr, 
			Sys_MsgBuffer **requestPtrPtr, 
			Sys_MsgBuffer **replyPtrPtr));
static void EnqueueBuffers _ARGS_((Vm_Segment *segPtr,
			Sys_MsgBuffer *requestPtr,
			Sys_MsgBuffer *replyPtr));
extern void VmDoRequests _ARGS_((ClientData clientData,
			Proc_CallInfo *callInfoPtr));


/*
 *----------------------------------------------------------------------
 *
 * Vm_EnqueueRequest --
 *
 *	Enqueue a request for the given segment, and hand it off to a 
 *	server process, starting a new process if necessary.  The server
 * 	process eventually sends a reply and frees the message buffers for 
 * 	the request.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	Removes the segment from the system request port set if it has too
 * 	many pending requests.
 *
 *----------------------------------------------------------------------
 */
    
void
Vm_EnqueueRequest(segPtr, requestPtr, replyPtr)
    Vm_Segment *segPtr;		/* the segment that the request is for */
    Sys_MsgBuffer *requestPtr;	/* the request buffer */
    Sys_MsgBuffer *replyPtr;	/* the reply buffer */
{
    kern_return_t kernStatus;

    if (vmRequestDebug) {
	printf("Enqueue %s...", Vm_SegmentName(segPtr));
    }
    VmSegmentLock(segPtr);
    EnqueueBuffers(segPtr, requestPtr, replyPtr);
    if (segPtr->queueSize > vm_MaxPendingRequests) {
	if (vmRequestDebug) {
	    printf("Queue overflow for %s...", Vm_SegmentName(segPtr));
	}
	vmStat.queueOverflows++;
	kernStatus = mach_port_move_member(mach_task_self(),
					   segPtr->requestPort,
					   MACH_PORT_NULL);
	if (kernStatus == KERN_SUCCESS) {
	    segPtr->flags |= VM_SEGMENT_NOT_IN_SET;
	} else {
	    printf("%s: couldn't remove segment %s from request set: %s.\n",
		   "Vm_EnqueueRequest", Vm_SegmentName(segPtr),
		   mach_error_string(kernStatus));
	}
    }
    if (!(segPtr->flags & VM_SEGMENT_ACTIVE)) {
	segPtr->flags |= VM_SEGMENT_ACTIVE;
	segPtr->refCount++;
	if (vmRequestDebug) {
	    printf("starting thread...");
	}
	Proc_CallFunc(VmDoRequests, (ClientData)segPtr, time_ZeroSeconds);
    }
    VmSegmentUnlock(segPtr);
    if (vmRequestDebug) {
	printf("enqueued\n");
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmDoRequests --
 *
 *	Run a segment's requests through the VM code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sends the replies and frees the message buffers.  When there are no 
 *	more messages, clears the "active" flag in the segment and frees 
 *	its reference to the segment.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
/* static */ void
VmDoRequests(clientData, callInfoPtr)
    ClientData clientData;	/* which segment */
    Proc_CallInfo *callInfoPtr;	/* unused */
{
    Boolean msgAccepted;	/* did the server code accept the msg */
    Vm_Segment *segPtr = (Vm_Segment *)clientData;
    Sys_MsgBuffer *requestPtr;	/* the buffer with the request message */
    Sys_MsgBuffer *replyPtr;	/* the buffer to hold the reply */
    kern_return_t kernStatus;

    Sync_GetLock(&requestCountLock);
    vmThreadsActive++;
    Sync_Unlock(&requestCountLock);

    for (;;) {
	VmSegmentLock(segPtr);
	if (DequeueBuffers(segPtr, &requestPtr, &replyPtr) != SUCCESS) {
	    segPtr->flags &= ~VM_SEGMENT_ACTIVE;

	    Sync_GetLock(&requestCountLock);
	    vmThreadsActive--;
	    if (vmRequestDebug) {
		printf("done processing %s; %d VM threads active\n",
		       Vm_SegmentName(segPtr), vmThreadsActive);
	    }
	    Sync_Unlock(&requestCountLock);

	    if (segPtr->flags & VM_SEGMENT_NOT_IN_SET) {
		if (vmRequestDebug) {
		    printf("re-enabling requests for segment %s\n",
			   Vm_SegmentName(segPtr));
		}
		kernStatus = mach_port_move_member(mach_task_self(),
						   segPtr->requestPort,
						   sys_RequestPort);
		if (kernStatus == KERN_SUCCESS) {
		    segPtr->flags &= ~VM_SEGMENT_NOT_IN_SET;
		} else {
		    printf("%s: can't move segment %s to port set: %s\n",
			   "VmDoRequests", Vm_SegmentName(segPtr), 
			   mach_error_string(kernStatus));
		}
	    }
	    VmSegmentUnlock(segPtr);
	    Vm_SegmentRelease(segPtr);
	    return;
	}
	VmSegmentUnlock(segPtr);
	msgAccepted = memory_object_server(&requestPtr->bufPtr->Head,
					   &replyPtr->bufPtr->Head);
	if (!msgAccepted) {
	    printf("VmDoRequests: bogus pager request.\n");
	}
	Sys_ReplyAndFree(requestPtr, replyPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * EnqueueBuffers --
 *
 *	Put the message buffers on the segment's queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The message buffers are put at the send of the segment's request 
 *	list. 
 *
 *----------------------------------------------------------------------
 */

static void
EnqueueBuffers(segPtr, requestPtr, replyPtr)
    Vm_Segment *segPtr;		/* segment whose queue to used; locked */
    Sys_MsgBuffer *requestPtr;	/* request buffer */
    Sys_MsgBuffer *replyPtr;	/* reply buffer */
{
    List_Insert((List_Links *)requestPtr, LIST_ATREAR(segPtr->requestList));
    List_Insert((List_Links *)replyPtr, LIST_ATREAR(segPtr->requestList));
    segPtr->queueSize++;
}


/*
 *----------------------------------------------------------------------
 *
 * DequeueBuffers --
 *
 *	Get the request and reply buffers for the next VM request.
 *
 * Results:
 *	If there messages in the queue, fills in the pointers to the 
 *	request and reply buffers and returns SUCCESS.  Otherwise returns
 *	FAILURE.
 *
 * Side effects:
 *	The message are removed from the segment's queue.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DequeueBuffers(segPtr, requestPtrPtr, replyPtrPtr)
    Vm_Segment *segPtr;		/* segment whose queue to check; locked */
    Sys_MsgBuffer **requestPtrPtr; /* OUT: request buffer */
    Sys_MsgBuffer **replyPtrPtr; /* OUT: reply buffer */
{
    if (List_IsEmpty(segPtr->requestList)) {
	return FAILURE;
    }

    *requestPtrPtr = (Sys_MsgBuffer *)
		List_First((List_Links *)segPtr->requestList);
    List_Remove((List_Links *)*requestPtrPtr);
    if (List_IsEmpty(segPtr->requestList)) {
	panic("DequeueBuffers: corrupted request list.\n");
    }
    *replyPtrPtr = (Sys_MsgBuffer *)
		List_First((List_Links *)segPtr->requestList);
    List_Remove((List_Links *)*replyPtrPtr);
    segPtr->queueSize--;

    return SUCCESS;
}
