/* 
 * devQueue.c --
 *
 *	Routines to implement the DevQueue data type. 
 *	Device queues define a queuing mechanism designed for implementing
 *	queuing in low level device drivers.  The module interface provided is 
 *	very simple.  Each controller in the system keeps one queue per
 *	device attached to it. Requests are sent to a device by inserting 
 *	the request into the queue for that device.  When a request becomes 
 *	available in the queue for a device, the queue module notifies the
 *	controller using a callback. The controller then has the option
 *	of processing the request or informing the queue module to
 *	enqueue it.  
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
#include "stdlib.h"

/*
 *	Design rationale
 *
 *	A queue per device is desirable for higher level software because
 *	it allows the software to view each device as an individual entity
 *	regardless of how the device is attached to the system.
 *	It also allows simple implementation of queue reordering (such as for 
 *	minimizing disk seek time).  Unfortunately, a queue per device 
 *	may be inappropriate for controllers that are single-threaded. 
 *	Each controller would have to implement collapsing these multiple 
 *	device queues into a single queue for the controller.
 *	The device queue module handles this complexity
 *	by allowing the controller to specify a set of queues to the
 *	dequeue operation and the queue module will handle the fair
 *	scheduling of these requests to the controller.
 *	When the controller looks for the next request
 *	to handle it can specify a bit mask indicating which queues it would
 *	be willing to take entries for.	The queue module will return a request
 *	from one of these queues using round-robin scheduling to decide which
 *	queue the request comes from.
 *
 *	How to use device queues.
 *
 *	1) Each controller that wishes to maintain device queues should 
 *	   call Dev_CtrlQueuesCreate() to get a DevCtrlQueues on to
 *	   which the device queues can be added. An argument to this
 *	   call is the procedure to call when an entry becomes available on a
 *	   queue attached to the controller.
 *	2) For each device attached to this controller, the driver should 
 *	   create a queue with the Dev_QueueCreate() call. The arguments
 *	   allow specification of the queue insert procedure (see below)
 *	   and the queue bit.  The queue bit must have at most one bit
 *	   set in it. Dev_QueueCreate() also lets the caller specify a
 *	   word of clientData passed to the controller's entryAvailProc when
 *	   an entry appears on the queue.
 *	3) Once the queue is created, entries can be inserted with the
 *	   Dev_QueueInsert() routine.
 *	4) To get the next entry off a queue the controller calls the
 *	   routine Dev_QueueGetNext(). The controller may also call
 *	   Dev_QueueGetNextFromSet() that takes the next entry from a 
 *	   set of queues represented in a bit mask.
 *
 * 	Other features available in Device Queues:
 *
 *	1) Data structures are locked with MASTER_LOCKS() using the 
 *	   Sync_Semaphore specified by the controller. This allows the
 *	   QueueGetNext routines can be called at interrupt time.
 *	2) The predefined insert "function" DEV_QUEUE_FIFO_INSERT can
 *	   be specified to the Dev_QueueCreate call to get first in
 *	   first out queuing.
 *
 * Data structures used to implement device queues.
 *
 * CtrlQueues - Contains per controller list of device queues.  This structure
 * is returned by CtrlQueuesCreate and passed to QueueCreate to link
 * device queues of a controller. It contains the lock for the queues and 
 * information about ready queues.  When an entry is taken from a queue, 
 * the queue is moved to the rear of the ready list.  This implements the 
 * round-robin scheduling of combined queues.  
 * 
 */

typedef struct CtrlQueues {
    Sync_Semaphore *mutexPtr; 	/* Lock use to protect this data structures
				 * and all device queues attached to this
				 * structure. This is specified by the 
				 * Controller when it creates the the
				 * DevCtrlQueues structure. */
    Boolean  (*entryAvailProc)();/* Procedure to call when an entry becomes
				 * available in a queue. Its calling sequence
				 * is defined in Dev_CtrlQueuesCreate. */
    List_Links readyListHdr;    /* A list of device queues with entries in
				 * them. This list is used to do the 
				 * round-robin scheduling. */
} CtrlQueues;

/*
 * Queue - The data structure for a device queue.  This structure contains
 * per queue information about a DevQueue.
 */

typedef struct Queue {
    List_Links	links;	/* Used to link the queue into ready and empty lists
			 * in the CtrlQueues structure. */
    CtrlQueues	*ctrlPtr;   /* Controller for this device queue. */
    ClientData	clientData; /* The ClientData to use on entryAvail callbacks
			     * for this queue.   */
    void   (*insertProc)(); /* Insert procedure to use from this queue. */
    unsigned int  queueBit; /* Bit used to specify this queue to the
			     * GetNextFromSet() routine. */
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
 *	Memory for the DevCtrlQueue data structure is malloc-ed and
 *	initialized. The entryAvalProc may be called at a latter time
 *	with the following arguments:
 *
 *	Boolean entryAvailProc(clientData, newRequestPtr)
 *		ClientData clientData;	-- clientData specified for queue 
 *					   this entry was for.
 *		List_Links *newRequestPtr; -- New queue entry 
 *	The entryAvailProc should return TRUE if the newRequest was
 *	processed. If the FALSE is return the queue module will enqeue
 *	the newRequest.
 *
 *----------------------------------------------------------------------
 */

DevCtrlQueues
Dev_CtrlQueuesCreate(mutexPtr, entryAvailProc)
    Sync_Semaphore *mutexPtr;	/* Semaphore used to protect this data 
				 * structure. */
    Boolean (*entryAvailProc)();/* Procedure to call when a queue for a device
				 * on this controller moves from empty to
				 * having an entry present. */
{
    CtrlQueues	 *ctrlPtr;

    ctrlPtr = (CtrlQueues *) malloc(sizeof(CtrlQueues));
    bzero((char *) ctrlPtr, sizeof(CtrlQueues));
    /*
     * Initialize the lock used to protect queues under the control
     * of this controller.
     */
    ctrlPtr->mutexPtr = mutexPtr;
    /*
     * Initialize the envtyAvailProc and the ready mask.
     */
    ctrlPtr->entryAvailProc = entryAvailProc;
    /*
     * Initialize the list headers for the ready list for
     * the device queues attached to this controller.
     */
    List_Init(&(ctrlPtr->readyListHdr));
    return (DevCtrlQueues) ctrlPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueCreate --
 *
 *	Allocate and initialize the data structures for a device queue.
 *	The queue will come up empty. The queueBit is used to group the 
 *	newly created queue with the other queues using the same queueBit bit.
 *
 * Results:
 *	The allocated DevQueue structure.
 *
 * Side effects:
 *	Memory allocated and the newly created queue is added to the 
 *	empty queue list of the controller. The insertProc procedure
 *	may be called when elements become available.
 *	The queue insert routine is responsible
 *	for inserting the new entry in the linked list for the device. The
 *	entries in the list are given to the device in the list order.
 * 	It should be declared:
 *
 *	void insertProc(elementPtr,listHeaderPtr)
 *		List_Links  *elementPtr;    -- Element to add.
 *		List_Links  *listHeaderPtr; -- Header of list to add to.
 *
 *	See the List man page for a description on how list work.
 *
 *----------------------------------------------------------------------
 */

DevQueue
Dev_QueueCreate(ctrlQueue, queueBit, insertProc, clientData)
    DevCtrlQueues ctrlQueue;    /* Pointer to the controller to which this
				 * queue belongs. Must be a pointer returned
				 * from Dev_CtrlQueuesCreate.*/
    void (*insertProc)();	/* Queue insert routine.  */
    unsigned int queueBit;	/* Bit value used to identify queue in 
				 * select mask for the GetNextFromSet() call. 
				 * Only zero or one bit should be set in this
				 * integer.  A zero value means the queue
				 * is not in a set. */
    ClientData	clientData;	/* Field passed to the entryAvailProc for the
				 * controller when an entry because available
				 * in this DevQueue. */
{
    CtrlQueues	 *ctrlPtr = (CtrlQueues *) ctrlQueue;
    Queue	 *queuePtr;
    /*
     * Allocated and initialize the DevQueue structure for this queue. 
     */
    queuePtr = (Queue *) malloc(sizeof(Queue));
    bzero((char *)queuePtr, sizeof(Queue));
    queuePtr->ctrlPtr = ctrlPtr;
    queuePtr->clientData = clientData;
    queuePtr->insertProc = insertProc;
    queuePtr->queueBit = queueBit;
    List_Init(&(queuePtr->elementListHdr));
    return (DevQueue) queuePtr;

}

/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueDestroy --
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
Dev_QueueDestroy(devQueue)
    DevQueue	devQueue;	/* Queue to destory. */
{
    Queue	 *queuePtr = (Queue *) devQueue;

    /*
     * Don't try to delete empty queue.
     */
    if (!List_IsEmpty(&(queuePtr->elementListHdr))) {
	return FALSE;
    }
    /*
     * Release the lock on the queue entry.  There is an inherent race 
     * condition with the freeing of a queue if someone else is still
     * trying to add entries to the queue. The user of the queue must 
     * make sure this doesn't happen.
     *
     */
    free((char *) queuePtr);
    return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueInsert --
 *	
 *	Insert an entry into the specified device queue.  If the device
 *	queue is not on the ready list it is added to the ready list.
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
Dev_QueueInsert(devQueue, elementPtr)
    DevQueue	devQueue;	/* Queue to insert into. */
    List_Links  *elementPtr;	/* Entry to insert. */
{
    register Queue	 *queuePtr = (Queue *) devQueue;
    register CtrlQueues  *ctrlPtr = queuePtr->ctrlPtr;
    register Boolean	 wasEmpty;

    MASTER_LOCK(ctrlPtr->mutexPtr);
    wasEmpty = List_IsEmpty(&(queuePtr->elementListHdr));
    /*
     * Insert the elment in the queue's list of elements if the entry
     * avail procedure doesn't take it.
     */
    if (wasEmpty) {
	Boolean	taken;
	taken = (ctrlPtr->entryAvailProc)(queuePtr->clientData,elementPtr);
	if (taken) {
	    MASTER_UNLOCK(ctrlPtr->mutexPtr);
	    return;
	}
    }
    if (queuePtr->insertProc != DEV_QUEUE_FIFO_INSERT) { 
	(queuePtr->insertProc)(elementPtr,&(queuePtr->elementListHdr));
    } else {
	List_Insert(elementPtr, LIST_ATREAR(&(queuePtr->elementListHdr)));
    }
    /*
     * If the queue moved from empty to available, move this queue 
     * to the ready list.  This need only be done if the queue is in
     * a set.
     */
    if (wasEmpty && (queuePtr->queueBit != 0)) {
	register List_Links *readyList;
	readyList = &(ctrlPtr->readyListHdr);
	wasEmpty = List_IsEmpty(readyList);
	List_Insert((List_Links *) queuePtr, LIST_ATREAR(readyList));
    }
    MASTER_UNLOCK(ctrlPtr->mutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueGetNext --
 *	
 *	Remove the element from a device queue.
 *
 *	NOTE: This routine assumes that the caller as the ctrlPtr->mutexPtr
 *	held.
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
Dev_QueueGetNext(devQueue)
    DevQueue	devQueue;		/* Queue remove element from. */
{
    register Queue	 *queuePtr = (Queue *) devQueue;
    register CtrlQueues  *ctrlPtr = queuePtr->ctrlPtr;
    List_Links	*returnValue;

    /*
     * If the queue is empty just return.
     */
    if (List_IsEmpty(&(queuePtr->elementListHdr))) {
	return (List_Links *) NIL;
    } 
    /*
     * Take the first entry of the queue. If the queue is now empty 
     * remove it from the readyList. Otherwise move it to the end 
     * of the ready list to implement round-robining in the
     * Dev_QueueGetNextFromSet routine. 
     */
    returnValue = List_First(&(queuePtr->elementListHdr));
    List_Remove(returnValue);
    if (queuePtr->queueBit != 0) {
	if (List_IsEmpty(&(queuePtr->elementListHdr))) {
	    List_Remove((List_Links *) queuePtr);
	 } else {
	    List_Move((List_Links *) queuePtr, 
			LIST_ATREAR(&(ctrlPtr->readyListHdr)));
	}
    }
    return returnValue;

}

/*
 *----------------------------------------------------------------------
 *
 * Dev_QueueGetNextFromSet --
 *	
 *	Remove next the element from a set of queues using LRU scheduling.
 *
 *	NOTE: This routine assumes that the caller as the ctrlPtr->mutexPtr
 *	held.
 *
 * Results:
 *	The element removed.  NIL if all queues in the mask were empty.
 *
 * Side effects:
 *	Entry removed from a queue and the queue may be moved from the
 *	ready list to empty list.
 *----------------------------------------------------------------------
 */

List_Links  *
Dev_QueueGetNextFromSet(ctrl, queueMask, clientDataPtr)
    DevCtrlQueues ctrl;		/* Controller to examine queues of. */
    unsigned int queueMask;		/* Mask of queues to examine. */
    ClientData	*clientDataPtr;		/* Filled in the with clientdata
					 * for the queue of the entry 
					 * returned. */
{
    register Queue	 *queuePtr;
    register List_Links *itemPtr;
    CtrlQueues  *ctrlPtr = (CtrlQueues *) ctrl;

    /*
     * Scan down the ready list for the first queue that has the queue bit 
     * set in the mask.
     */
    LIST_FORALL(&(ctrlPtr->readyListHdr), itemPtr) {
	queuePtr = (Queue *) itemPtr;
	if (queueMask & (queuePtr->queueBit)) {
		*clientDataPtr = queuePtr->clientData;
		return Dev_QueueGetNext((DevQueue) queuePtr);
	}
    }
    return (List_Links *) NIL;
}


