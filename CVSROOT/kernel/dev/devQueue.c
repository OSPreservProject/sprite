/* 
 * devQueue.c --
 *
 *	Routines to implement the DevQueue data type. 
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "list.h"
#include "bit.h"
#include "devQueue.h"
#include "sync.h"

/*
 * Data structures used to implement device queues.
 *
 * CtrlQueues - Contains per controller list of device queues.  This structure
 * is returned by CtrlQueuesCreate and passed to QueueCreate to link
 * device queues of a controller. It contains the lock for the queues and 
 * information about combined queues.  Each combine queue has a ready
 * list that contains all the queues of a combined queue
 * that have entries in them.  When an entry is taken from a queue, the queue
 * is moved to the rear of the ready list.  This implements the round-
 * robin scheduling of combined queues. Another list, the empty list
 * contains all device queues of the controller that have no elements.
 * 
 * NUM_COMBINED_QUEUES - The number of combined queues per controller.
 *			 Since we use an integer to specify the tag
 *			 this is the number of bits in an int.
 * TAG_TO_INDEX	       - Convert a combined queue tag into an index into 
 *			 the ready list array of the CtrlQueues data structure.
 *			 Index zero is the non-combined queues and index
 *			 1 thru 32 is the bit number of first bit set in 
 *			 in the tag.
 * NONCOMBINDED_INDEX  - Index into readyList array of the noncombined queue's
 *			 ready list.
 */

#define	NUM_COMBINED_QUEUES	BIT_NUM_BITS_PER_INT 
#define	TAG_TO_INDEX(tag)	(Bit_FindFirstSet(NUM_COMBINED_QUEUES,&(tag))+1)
#define	NONCOMBINDED_INDEX	0

typedef struct CtrlQueues {
    Sync_Semaphore	mutex; 	/* Lock use to protect this data structures
				 * and all device queues attached to this
				 * structures. 
				 */
    void  (*entryAvailProc)();  /* Procedure to call when an entry becomes
				 * available in a combined queue.
				 */
    List_Links emptyListHdr;	/* A list of all the empty queues attached to 
				 * the controller. This is a list of Queue's.
				 */
    List_Links readyListHdr[NUM_COMBINED_QUEUES+1];
			        /* For each possible combined tag
				 * we keep a list of the device queues 
				 * that have elements.
				 * This array is index by 
				 * TAG_TO_INDEX(combineTag) and is a list
				 * of Queue's.
			         */
} CtrlQueues;

/*
 * Queue - The data structure for a device queue.  This structure contains
 * per queue information about a DevQueue.
 */

typedef struct Queue {
    List_Links	links;	/* Used to link the queue into ready and empty lists
			 * in the CtrlQueues structure.
			 */
    CtrlQueues	*ctrlPtr;   /* Controller for this device queue. */
    ClientData	clientData; /* The ClientData to use on entryAvail callbacks
			     * for this queue. 
			     */
    void   (*insertProc)(); /* Insert procedure to use from this queue. */
    int	   queueIndex;	    /* Index in the controller's readyList array that
			     * this Queue belongs.
			     */
    List_Links elementListHdr; /* List of elements on this queue. */

} Queue;

/*
 *----------------------------------------------------------------------
 *
 *  Dev_CtrlQueuesCreate --
 *
 *	Allocate the data structures necessary to maintain queues for a
 *	device controller with multiple devices.  Memory for the 
 *	DevCtrlQueue data structure is malloc-ed and initialized.
 *
 * Results:
 *	The DevCtrlQueue for this controller.
 *
 * Side effects:
 *	Memory is malloc-ed.
 *
 *----------------------------------------------------------------------
 */

DevCtrlQueuesPtr
Dev_CtrlQueuesCreate(entryAvailProc)
    void (*entryAvailProc)();	/* Procedure to call when a queue for a device
				 * on this controller moves from empty to
				 * having an entry present. See devQueue.h
				 * for the calling sequence.
				 */
{
    CtrlQueues	 *ctrlPtr;
    int		 queueIndex;

    ctrlPtr = (CtrlQueues *) malloc(sizeof(CtrlQueues));
    bzero((char *) ctrlPtr, sizeof(CtrlQueues));
    /*
     * Initialize the list headers for the empty and ready list for
     * all the device queue list possibly attached to this controller.
     */
    List_Init(&(ctrlPtr->emptyListHdr));
    for (queueIndex = 0; queueIndex < NUM_COMBINED_QUEUES+1; queueIndex++) {
	List_Init(&(ctrlPtr->readyListHdr[queueIndex]));
    }
    ctrlPtr->entryAvailProc = entryAvailProc;
    /*
     * Initialize the lock used to protect queues under the control
     * of this controller.
     */
    Sync_SemInitDynamic(&(ctrlPtr->mutex), "DevCtrlQueuesMutex");
    return (DevCtrlQueuesPtr) ctrlPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueCreate --
 *
 *	Allocate and initialize the data structures for a device queue.
 *	The queue will come up empty. The combineTag is used to group the 
 *	newly created queue with the other queues with the same combineTag bit.
 *
 * Results:
 *	The allocated DevQueuePtr structure.
 *
 * Side effects:
 *	Memory allocated and the newly created queue is added to the 
 *	empty queue list of the controller.
 *
 *----------------------------------------------------------------------
 */

DevQueuePtr
Dev_QueueCreate(ctrlQueuePtr, combineTag, insertProc, clientData)
    DevCtrlQueuesPtr ctrlQueuePtr;/* Pointer to the controller to which this
				 * queue belongs. Must be a pointer returned
				 * from Dev_CtrlQueuesCreate.
			         */
    void (*insertProc)();	/* Queue insert routine. See devQueue.h for
				 * its calling sequence. 
				 */
    unsigned int combineTag;	/* Bit value used to group queue together. 
				 * Only zero or one bit should be set in this
				 * integer. 
				 */
    ClientData	clientData;	/* Field passed to the entryAvailProc for the
				 * controller when an entry because available
				 * in this DevQueue.
				 */
{
    CtrlQueues	 *ctrlPtr = (CtrlQueues *) ctrlQueuePtr;
    Queue	 *queuePtr;
    /*
     * Allocated and initialize the DevQueue structure for this queue. The
     * queue starts out empty so it is put on the controller's empty queue
     * list. 
     */
    queuePtr = (Queue *) malloc(sizeof(Queue));
    bzero((char *)queuePtr, sizeof(Queue));
    queuePtr->clientData = clientData;
    queuePtr->insertProc = insertProc;
    queuePtr->ctrlPtr = ctrlPtr;
    List_Init(&(queuePtr->elementListHdr));
    queuePtr->queueIndex = TAG_TO_INDEX(combineTag);
    /*
     * Link this queue into the empty queue list for the
     * controller.  
     */
    List_InitElement((List_Links *) queuePtr)
    MASTER_LOCK(&(ctrlPtr->mutex));
    List_Insert((List_Links *) queuePtr,LIST_ATREAR(&(ctrlPtr->emptyListHdr)));
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    return (DevQueuePtr) queuePtr;

}

/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueDestory --
 *
 *	Release the resources held by a queue.
 *
 * Results:
 *	TRUE if the queue is was destroyed. FALSE if the queue could not
 * 	be freed because it was not empty.
 *
 * Side effects:
 *	Memory may be free'ed.
 *
 *----------------------------------------------------------------------
 */

Boolean
Dev_QueueDestory(devQueuePtr)
    DevQueuePtr	devQueuePtr;	/* Queue to destory. */
{
    Queue	 *queuePtr = (Queue *) devQueuePtr;

    MASTER_LOCK(&(queuePtr->ctrlPtr->mutex));
    /*
     * Don't try to delete empty queue.
     */
    if (!List_IsEmpty(&(queuePtr->elementListHdr))) {
	MASTER_UNLOCK(&(queuePtr->ctrlPtr->mutex));
	return FALSE;
    }
    /*
     * Release the lock on the queue entry.  There is an inherent race 
     * condition with the freeing of a queue if someone else is still
     * trying to add entries to the queue. The user of the queue must 
     * make sure this doesn't happen.
     *
     * Remove the queue from the controller's list and free the element.
     */
    List_Remove((List_Links *) queuePtr);
    MASTER_UNLOCK(&(queuePtr->ctrlPtr->mutex));
    free((char *) queuePtr);
    return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueInsert --
 *	
 *	Insert an entry into the specified device queue.  If the device
 *	queue is on the empty list it is moved to the end of the ready
 *	list for its combinded tag.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The queue controller's entryAvailProc maybe called. The queue may
 *	be moved from the controller's emptyList to the readyList of its
 *	combine tag.
 *
 *----------------------------------------------------------------------
 */

void
Dev_QueueInsert(devQueuePtr, elementPtr)
    DevQueuePtr	devQueuePtr;	/* Queue to insert into. */
    List_Links  *elementPtr;	/* Entry to insert. */
{
    register Queue	 *queuePtr = (Queue *) devQueuePtr;
    register CtrlQueues  *ctrlPtr = queuePtr->ctrlPtr;
    register Boolean	 wasEmpty;

    MASTER_LOCK(&(ctrlPtr->mutex));
    wasEmpty = List_IsEmpty(&(queuePtr->elementListHdr));
    /*
     * Insert the elment in the queue's list of elements using if
     * user specified insertProc if defined.
     */
    if (queuePtr->insertProc != DEV_QUEUE_FIFO_INSERT) { 
	(queuePtr->insertProc)(elementPtr,&(queuePtr->elementListHdr));
    } else {
	List_Insert(elementPtr, LIST_ATREAR(&(queuePtr->elementListHdr)));
    }
    /*
     * If the queue moved from empty to available, move this queue from
     * the empty list to ready list.  wasEmpty is modified to mean
     * "was the combined queue empty?".
     */
    if (wasEmpty) {
	register List_Links *readyList;
	readyList = &(ctrlPtr->readyListHdr[queuePtr->queueIndex]);
	wasEmpty = List_IsEmpty(readyList);
	List_Move((List_Links *) queuePtr, LIST_ATREAR(readyList));
    }
    /*
     * Release the lock and call the entryAvailProc if this element caused
     * a combined queue to move from empty to ready.
     */
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    if (wasEmpty) {
	(ctrlPtr->entryAvailProc)(queuePtr->clientData);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueGetNext --
 *	
 *	Remove the element from the combined queue of a device. 
 *
 * Results:
 *	The element removed.  NIL if queue was empty.
 *
 * Side effects:
 *	Entry removed from a queue and the queue may be moved from the
 *	ready list to empty list.
 *----------------------------------------------------------------------
 */

List_Links  *
Dev_QueueGetNext(devQueuePtr)
    DevQueuePtr	devQueuePtr;		/* Queue remove element from. */
{
    register Queue	 *queuePtr = (Queue *) devQueuePtr;
    register CtrlQueues  *ctrlPtr = queuePtr->ctrlPtr;
    register List_Links *readyList;
    Boolean  nonCombinedQueue;
    List_Links	*returnValue;

    MASTER_LOCK(&(ctrlPtr->mutex));
    /*
     * If the ready list is empty for the device's combine queue we
     * know there can't be any elements on the device's queue.
     */
    readyList = &(ctrlPtr->readyListHdr[queuePtr->queueIndex]);
    if (List_IsEmpty(readyList)) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return (List_Links *) NIL;
    }
    /*
     * We need to treat combined queue and noncombined queues a
     * little differently.  With combine queues we use the
     * first queue on the ready list of queues with a common
     * tag bit. Non combine queue always use the device queue specified.
     * Since there is no ready list of non-combined queue we must 
     * check to see if the element list is empty.
     */
    nonCombinedQueue = (queuePtr->queueIndex == NONCOMBINDED_INDEX);
    if (nonCombinedQueue && List_IsEmpty(&(queuePtr->elementListHdr))) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	return (List_Links *) NIL;
    } else {
	queuePtr = (Queue *) List_First(readyList);
    }
    /*
     * Take the first entry of the queue. If the queue is now empty move
     * it to the empty list. Otherwise move the queue 
     * to the rear of the readyList so the other device queues with this
     * tag can get their fair share. This reordering of the ready list
     * is not needed for noncombined queues.
     */
    returnValue = List_First(&(queuePtr->elementListHdr));
    List_Remove(returnValue);

    if (List_IsEmpty(&(queuePtr->elementListHdr))) {
	List_Move((List_Links *) queuePtr,
				LIST_ATREAR(&(ctrlPtr->emptyListHdr)));
    } else if (!nonCombinedQueue) {
	List_Move((List_Links *) queuePtr, LIST_ATREAR(readyList));
    }
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    return returnValue;

}

