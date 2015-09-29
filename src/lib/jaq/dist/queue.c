/*
 * queue.c --
 *
 *     Queue package for Jaquith archive system
 *  
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *                                                           
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/queue.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

/*
 * Rather than allocate and free nodes all the time as items are   
 * enqueued and dequeued, we'll recycle them onto a freeList 
 * for later use.                                            
 */                                                           
static Q_Node *freeList = NULL;
static int freeCnt = 0;

#define FREEMAX 1000


/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Create -- create and initialize a new FIFO queue.          
 *                                                           
 * Results:
 *	returns an opaque handle to the user for later use.
 *
 * Side effects:
 *      Allocates a queue object.
 *                                                           
 *----------------------------------------------------------------------
 */

Q_Handle *
Q_Create(name, freeData)
    char *name;               /* name of queue for debugging */
    int freeData;             /* pass datum to Free when deallocating */
{
    Q_Handle *qPtr;                 

    qPtr = (Q_Handle *) MEM_ALLOC("Q_Create", sizeof(Q_Handle));
    qPtr->head = NULL;
    qPtr->tail = NULL;
    qPtr->count = 0;
    qPtr->freeData = freeData;
    qPtr->name = (char *) MEM_ALLOC("Q_Create", strlen(name)+1);
    strcpy(qPtr->name, name);
    return (qPtr);

}


/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Destroy -- Destroy a queue.                              
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Deallocates a queue object.
 *                                                           
 *----------------------------------------------------------------------
 */

void
Q_Destroy(qPtr)
    Q_Handle *qPtr;           /* queue handle */
{
    Q_Node *nodePtr;
    Q_Node *tmpPtr;

    if (!qPtr) {
        fprintf(stderr, "Q_Destroy: queue pointer argument is NULL!\n");
        exit(-1);
    }

    nodePtr = qPtr->head;

    while (nodePtr != NULL) {
	if (qPtr->freeData) {
	    MEM_FREE("Q_Destroy", nodePtr->datum);
	}
	if (freeCnt > FREEMAX) {
	    MEM_FREE("Q_Destroy", nodePtr);
	} else {
	    freeCnt++;
	    tmpPtr = nodePtr->link;
	    nodePtr->link = freeList;
	    freeList = nodePtr;
	    nodePtr = tmpPtr;
	}
    }

    MEM_FREE("Q_Destroy", qPtr->name);
    MEM_FREE("Q_Destroy", (char *)qPtr);

}


/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Add -- Add a new element to queue in priority order      
 *                                                           
 * Results:
 *	None.
 *
 * Side effects:
 *      None.
 *
 * priority tells where to add element:                      
 *   0            : at head                                  
 *   positive int : at appropriate place in queue            
 *                  (lower numbers nearer head of queue)     
 *   MAX_INT      : at tail                                  
 *                                                           
 *----------------------------------------------------------------------
 */

void
Q_Add(qPtr, datum, priority)
    Q_Handle *qPtr;           /* queue handle */
    Q_ClientData datum;       /* client datum to enqueue */
    int priority;             /* priority of item */
{
    Q_Node *nodePtr;
    Q_Node *frontPtr, *backPtr;

    if (!qPtr) {
        fprintf(stderr, "Q_Add: queue pointer argument is NULL!\n");
        exit(-1);
    }

    if (freeCnt > 0) {
	freeCnt--;
        nodePtr = freeList;
	freeList = nodePtr->link;
    } else {
        nodePtr = (Q_Node *) MEM_ALLOC("Q_Add", sizeof(Q_Node));
    }

    nodePtr->datum = datum;
    nodePtr->priority = priority;

    if (priority == Q_TAILQ) {
        nodePtr->link = NULL;
	if (qPtr->head) {
	    qPtr->tail->link = nodePtr;
	} else {
	    qPtr->head = nodePtr;
	}
	qPtr->tail = nodePtr;
    } else if (priority == Q_HEADQ) {
	if (!(nodePtr->link = qPtr->head)) {
	    qPtr->tail = nodePtr;
	}
        qPtr->head = nodePtr;
    } else if (priority > 0) {
        frontPtr = qPtr->head;
	backPtr = NULL;
	while (frontPtr != NULL) {
	    if (frontPtr->priority > priority) break;
	    backPtr = frontPtr;
	    frontPtr = frontPtr->link;
	}
	if (backPtr) {
	    backPtr->link = nodePtr;
	} else {
	    qPtr->head = nodePtr;
	}
	if (!(nodePtr->link = frontPtr)) {
	    qPtr->tail = nodePtr;
	}
    } else {
        fprintf(stderr,"Q_Add: bad queueing priority: %d",priority);
	exit(-1);
    }

    qPtr->count++;
}


/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Remove -- Remove an element from the head of the queue.  
 *                                                           
 * Results:
 *	None.
 *
 * Side effects:
 *      None.
 *
 * If the queue is empty the routine aborts.                 
 *                                                           
 *----------------------------------------------------------------------
 */

Q_ClientData
Q_Remove(qPtr)
    Q_Handle *qPtr;           /* queue handle */
{
    Q_Node *nodePtr;
    Q_ClientData datum;

    if (!qPtr) {
        fprintf(stderr,"Q_Remove: queue pointer argument is NULL!\n");
        exit(-1);
    }

    if (!qPtr->head) {
        fprintf(stderr,"Q_Remove: Queue '%s' is empty!\n", qPtr->name);
        exit(-1);
    }

    nodePtr = qPtr->head;
    if (!(qPtr->head = nodePtr->link)) {
	qPtr->tail = NULL;
    }
    qPtr->count--;
    datum = nodePtr->datum;
    nodePtr->link = freeList;
    freeList = nodePtr;

    return (datum);

}



/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Count -- return the number of elements in the queue.     
 *
 * Results:
 *	Number of elements on the queue.
 *
 * Side effects:
 *      None.
 *                                                           
 *----------------------------------------------------------------------
 */

int
Q_Count(qPtr)
    Q_Handle *qPtr;           /* queue handle */
{
    if (!qPtr) {
        fprintf(stderr,"Q_Count: queue pointer argument is NULL!\n");
        exit(-1);
    }

    return (qPtr->count);

}



/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Peek -- return 1st item in queue w/o removing it         
 * 
 * Results:
 *	Ptr to item at head of queue.
 *
 * Side effects:
 *      None.
 *                                                           
 *----------------------------------------------------------------------
 */

Q_ClientData
Q_Peek(qPtr)
    Q_Handle *qPtr;           /* queue handle */
{

    if (!qPtr) {
        fprintf(stderr,"Q_Peek: queue pointer argument is NULL!\n");
        exit(-1);
    }

    if (!qPtr->head) {
        fprintf(stderr,"Q_Peek: Queue '%s' is empty!\n", qPtr->name);
        exit(-1);
    }

    return(qPtr->head->datum);

}


/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Print -- Print all the items in the queue (for debugging)
 *                                                           
 * Results:
 *	None.
 *
 * Side effects:
 *      Items are printed on the specified channel, or stderr.
 *                                                           
 *----------------------------------------------------------------------
 */

void
Q_Print(qPtr, fd)
    Q_Handle *qPtr;           /* queue handle */
    FILE *fd;                 /* file descriptor for output */
{
    Q_Node *nodePtr;
    int i = 0;

    if (!qPtr) {
        fprintf(stderr,"Q_Print: queue pointer argument is NULL!\n");
        exit(-1);
    }

    if (!fd) fd = stderr;
    fprintf(fd,"Q_Print: Queue '%s' has %d items",
	    qPtr->name, qPtr->count);

    for (nodePtr=qPtr->head; nodePtr != NULL; nodePtr=nodePtr->link) {
       if (!(i++ % 4)) fprintf(fd,"\n\t");
       fprintf(fd, "  node %08x (datum %08x)", nodePtr, nodePtr->datum);
    }
    fprintf(fd,"\n");

}


/*
 *----------------------------------------------------------------------
 *                                                           
 * Q_Iterate -- Call user-supplied callback function on each  
 *             element in the queue.                         
 *                                                           
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * 
 * The callback function is provided with:                   
 *    qPtr  : so it knows what queue it's operating on        
 *   datum  : for processing                                  
 *   count  : # of items remaining in queue after this one    
 *   callVal: Arbitray ptr provided by Q_Iterate caller.
 *                                                           
 * Callback routine must not call Q_Add or Q_Remove for this Q.
 *                                                           
 * We respond to the return code as follows:                 
 *    = Q_ITER_REMOVE_STOP : remove item from queue and stop
 *    = Q_ITER_REMOVE_CONT : remove item from queue and continue 
 *    = Q_ITER_STOP : stop
 *    = Q_ITER_CONTINUE  : do nothing to the item and continue
 *    other  : update priority field with given value and continue
 *                                                           
 *----------------------------------------------------------------------
*/

int
Q_Iterate(qPtr, func, callVal)
    Q_Handle *qPtr;           /* queue handle */
    int (*func)();            /* callback function */
    int *callVal;             /* arbitrary user-supplied datum */
{
    Q_Node *nodePtr, *backPtr;
    int rc;
    int retCode = 0;

    if (!qPtr) {
        fprintf(stderr,"Q_Iterate: queue pointer argument is NULL!\n");
        exit(-1);
    }

    for (nodePtr=qPtr->head, backPtr = NULL;
	 nodePtr != NULL;
	 backPtr = nodePtr, nodePtr=nodePtr->link) {
        rc = (*func)(qPtr, nodePtr->datum, &retCode, callVal);
	if (rc == Q_ITER_STOP) {
	    break;
	} else if (rc == Q_ITER_CONTINUE) {
	    /* do nothing */;
	} else if ((rc == Q_ITER_REMOVE_CONT) ||
		   (rc == Q_ITER_REMOVE_STOP)) {
	    qPtr->count--;
	    if (nodePtr == qPtr->head)
		qPtr->head = nodePtr->link;
	    else 
		backPtr->link = nodePtr->link;
	    if (nodePtr == qPtr->tail) {
		qPtr->tail = backPtr;
	    }
	    if (rc == Q_ITER_REMOVE_STOP) {
		break;
	    }
	} else {
	    nodePtr->priority = rc;
	}
    }

    return retCode;

}

