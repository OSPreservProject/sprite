/*
 * devQueue.h --
 *
 *	Declarations for the Device Queue interface.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVQUEUE
#define _DEVQUEUE

/*
 * Definitions in this file use the List_Links data structure.
 */
#include "list.h"

/*
 *	Overview of device queues.
 *
 *	This file defines the interface to the Sprite device queue routines.
 *	Device queues define a queuing mechanism designed for implementing
 *	queuing in low level device drivers.  The module interface provided is 
 *	very simple.  Each controller in the system keeps one queue per
 *	device attached to it. Requests are sent to a device by inserting 
 *	the request into the queue for that device.  When a request becomes 
 *	available in the queue for a device, the queue module notifies the
 *	controller using a callback. The controller then dequeues the
 *	request and issue it to the device.  
 *	
 *	Queue combining
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
 *	by allowing the controller to specify tags for each queue attached
 *	to the controller. Queues with the same tag appear to the controller
 *	to be a single queue.  The queue module handles the scheduling of
 *	requests into the combined queue using round-robin scheduling.
 *
 *	How to use device queues.
 *
 *	1) Each controller that wishes to maintain device queues should 
 *	   call Dev_CtrlQueuesCreate() to get a DevCtrlQueuesPtr on to
 *	   which the device queues can be added. An argument to this
 *	   call is the procedure to call when an entry becomes available on a
 *	   combined queue (see below for calling sequence of the
 *	   entryAvailProc).
 *	2) For each device attached to this controller, the driver should 
 *	   create a queue with the Dev_QueueCreate() call. The arguments
 *	   allow specification of the queue insert procedure (see below)
 *	   and the combineTag.  The combineTag must have at most one bit
 *	   set in it.  Queues with the same non-zero tag are treated 
 *	   as a combined queue.
 *	   A tag of zero means that this queue is not combined with
 *	   any others. Dev_QueueCreate() also lets the caller specify a
 *	   word of clientData passed to the controller's entryAvailProc when
 *	   this queue moves from the empty to non-empty state.
 *	3) Once the queue is created, entries can be inserted with the
 *	   Dev_QueueInsert() routine.
 *	4) To get the next entry off the queue the controller calls the
 *	   routine Dev_QueueGetNext().  Note that if queue is a combinded
 *	   queue the entry returned may be to any device in that combined
 *	   queue.
 *
 *	The queue entry available procedure specified to Dev_CtrlQueuesCreate
 *	should be defined as follows:
 *
 *	void entryAvailProc(clientData)
 *		ClientData clientData;
 *
 *	It is called whenever an entry becomes available on a device queue
 *	associated with this controller.
 *	The argument is the clientData passed to Dev_QueueCreate() for the
 *	device queue that has a new entry.
 *
 *	The queue insert routine specified to Dev_QueueCreate is responsible
 *	for inserting the new entry in the linked list for the device. The
 *	entries in the list are given to the device in the list order.
 * 	It should be declared:
 *
 *	void insertProc(, elementPtr,listHeaderPtr)
 *		List_Links  *elementPtr;    -- Element to add.
 *		List_Links  *listHeaderPtr; -- Header of list to add to.
 *
 *	See the List man page for a description on how list work.
 *
 * 	Other features available in Device Queues:
 *	1) Data structures are locked with MASTER_LOCKS() so QueueInsert and
 *	   QueueGetNext routines can be called at interrupt time.
 *	2) The predefined insert "function" DEV_QUEUE_FIFO_INSERT can
 *	   be specified to the Dev_QueueCreate call to get first in
 *	   first out queuing.
 *
 */


/*
 * InsertProc argument to Dev_QueueCreate specifing FIFO ordering.
 */

#define	DEV_QUEUE_FIFO_INSERT	((void (*)())NIL)

/* data structures 
 * 
 * DevCtrlQueuesPtr - An anonymous pointer to structure containing the 
 *		      device queues of a controller.
 * DevQueue	    - An anonymous pointer to a structure containing a
 *		      device queue.
 */

typedef struct DevCtrlQueues *DevCtrlQueuesPtr;
typedef struct DevQueue	     *DevQueuePtr;

/* procedures */

extern DevCtrlQueuesPtr	Dev_CtrlQueuesCreate();
extern DevQueuePtr	Dev_QueueCreate();
extern Boolean		Dev_QueueDestory();
extern void		Dev_QueueInsert();
extern List_Links	*Dev_QueueGetNext();


#endif /* _DEVQUEUE */

