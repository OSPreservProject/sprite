/*
 * devQueue.h --
 *
 *	Declarations for the Device Queue interface.  This file defines the 
 *	interface to the Sprite device queue routines used to order I/O
 *	request for disk and other devices. 
 *
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
 * DEV_QUEUE_FIFO_INSERT - InsertProc argument to Dev_QueueCreate specifing 
 *			   FIFO ordering.
 * DEV_QUEUE_ANY_QUEUE_MASK - Mask to Dev_QueueGetNextFromSet specifing all
 *			      queue sets.
 */

#define	DEV_QUEUE_FIFO_INSERT	((void (*)())NIL)
#define	DEV_QUEUE_ANY_QUEUE_MASK ((unsigned int) 0xffffffff)

/* data structures 
 * 
 * DevCtrlQueues    - An anonymous pointer to structure containing the 
 *		      device queues of a controller.
 * DevQueue	    - An anonymous pointer to a structure containing a
 *		      device queue.
 */

typedef struct DevCtrlQueues *DevCtrlQueues;
typedef struct DevQueue	     *DevQueue;

/* procedures */

extern DevCtrlQueues	Dev_CtrlQueuesCreate();
extern DevQueue		Dev_QueueCreate();
extern Boolean		Dev_QueueDestory();
extern void		Dev_QueueInsert();
extern List_Links	*Dev_QueueGetNext();
extern List_Links	*Dev_QueueGetNextFromSet();


#endif /* _DEVQUEUE */

