/* 
 *  procServer.c --
 *
 *	Routines to manage pool of server processes.
 *
 * Copyright 1987, 1988 Regents of the University of California
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

#include <sprite.h>
#include <mach.h>
#include <proc.h>
#include <procInt.h>
#include <sync.h>
#include <sched.h>
#include <timer.h>
#include <list.h>
#include <vm.h>
#include <fs.h>
#include <fscache.h>
#include <sys.h>
#include <string.h>
#include <status.h>
#include <stdlib.h>
#include <procServer.h>
#include <stdio.h>

/*
 * Circular queue of pending function calls.
 */
QueueElement	queue[NUM_QUEUE_ELEMENTS];

/*
 * Indices into circular queue.  frontIndex is index of next function to call.
 * nextIndex is index of where to put next call.  When frontIndex and nextIndex
 * are equal then queue is full.  When frontIndex = -1 then queue is empty.
 */
int	frontIndex = -1;
int	nextIndex = 0;

ServerInfo	*serverInfoTable;

int	proc_NumServers = PROC_NUM_SERVER_PROCS;

/* 
 * Mutex to synchronize accesses to the queue of pending requests and
 * to the process state.
 */
volatile Sync_Semaphore	serverMutex; 

static void 	ScheduleFunc _ARGS_((void (*func)(ClientData clientData,
						  Proc_CallInfo	*callInfoPtr),
			ClientData clientData, unsigned int interval, 
			FuncInfo *funcInfoPtr));
static void 	CallFuncFromTimer _ARGS_((Timer_Ticks time, 
			FuncInfo *funcInfoPtr));
static void	CallFunc _ARGS_((FuncInfo *funcInfoPtr));	

/*
 *----------------------------------------------------------------------
 *
 * Proc_CallFunc --
 *
 *	Start a process that calls the given function.  The process will
 *	be started after waiting for interval amount of time where interval is
 *	of the form expected by the timer module (e.g. timer_IntOneSecond). 
 *	Proc_CallFunc can be called with interrupts disabled as long as
 *	interval is 0 (when interval is non-zero, the memory allocator is
 *	called).  When func is called it will be called as 
 *
 *		void
 *		func(clientData, callInfoPtr)
 *			ClientData	clientData;
 *			Proc_CallInfo	*callInfoPtr;
 *
 *	The callInfoPtr struct contains two fields: a client data field and
 *	an interval field.  callInfoPtr->interval is initialized to 0 and
 *	callInfoPtr->clientData is initialized to clientData.  If when func
 *	returns the callInfoPtr->interval is non-zero then the function will
 *	be scheduled to be called again after waiting the given interval.  It
 *	will be passed the client data in callInfoPtr->clientData.
 *
 *	NOTE: There are a fixed number of processes to execute functions 
 *	      specified by Proc_CallFunc.  Therefore the functions given
 *	      to Proc_CallFunc should always return after a short period
 *	      of time.  Otherwise all processes will be tied up.
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
Proc_CallFunc(func, clientData, interval)
    void		(*func) _ARGS_((ClientData clientData, 
			Proc_CallInfo	*callInfoPtr));	/* Function to call. */
    ClientData		clientData;	/* Data to pass function. */
    unsigned	int	interval;	/* Time to wait before calling func. */
{
    FuncInfo	funcInfo;

    if (interval != 0) {
	ScheduleFunc(func, clientData, interval, (FuncInfo *) NIL);
    } else {
	funcInfo.func = func;
	funcInfo.data = clientData;
	funcInfo.allocated = FALSE;
	CallFunc(&funcInfo);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_CallFuncAbsTime --
 *
 *	This routine is a variant of Proc_CallFunc. It starts a process
 *	to call the given function at a specific time.  Proc_CallFuncAbsTime 
 *	can not be called with interrupts disabled. When func is called it 
 *	will be called as:
 *
 *		void
 *		func(clientData, callInfoPtr)
 *			ClientData	clientData;
 *			Proc_CallInfo	*callInfoPtr;
 *
 *	The callInfoPtr struct must not be modified!! (it is used by
 *	routines scheduled with Proc_CallFunc).	The only field of interest
 *	is "token" -- it is the same value that was returned by 
 *	Proc_CallFuncAbsTime when func was scheduled. Func will be called 
 *	exactly once: if func needs to be resecheduled, it must call 
 *	Proc_CallFuncAbsTime with a new time value.
 *
 *	NOTE:	There are a fixed number of processes to execute functions 
 *		specified by the Proc_CallFunc* routines.  Therefore the 
 *		functions given to Proc_CallFuncAbsTime should always 
 *		return after a short period of time.  Otherwise all 
 *		processes will be tied up.
 *	      
 * Results:
 *	A token to identify this instance of the Proc_CallFuncAbsTime call.
 *	The token is passed to the func in the Proc_CallInfo struct.
 *
 * Side effects:
 *	Memory for the FuncInfo struct is allocated.
 *
 *----------------------------------------------------------------------
 */
ClientData
Proc_CallFuncAbsTime(func, clientData, time)
    void		(*func) _ARGS_((ClientData clientData, 
			Proc_CallInfo	*callInfoPtr));	/* Function to call. */
    ClientData		clientData;	/* Data to pass to func. */
    Timer_Ticks		time;		/* Time when to call func. */
{
    register FuncInfo	*funcInfoPtr;

    funcInfoPtr = (FuncInfo *) malloc(sizeof (FuncInfo));
    funcInfoPtr->func = func;
    funcInfoPtr->data = clientData;
    funcInfoPtr->allocated = TRUE;
    funcInfoPtr->queueElement.routine = CallFuncFromTimer;
    funcInfoPtr->queueElement.clientData = (ClientData) funcInfoPtr;
    funcInfoPtr->queueElement.time = time;
    funcInfoPtr->queueElement.interval = 0;
    Timer_ScheduleRoutine(&funcInfoPtr->queueElement, FALSE);
    return((ClientData) funcInfoPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_CancelCallFunc --
 *
 *	This routine is used to deschedule a timer entry created by
 *	Proc_CallFuncAbsTime.   
 *	      
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer entry is removed from the timer queue.
 *
 *----------------------------------------------------------------------
 */
void
Proc_CancelCallFunc(token)
    ClientData		token;	/* Opaque identifier for function info */
{
    register FuncInfo	*funcInfoPtr = (FuncInfo *) token;
    Boolean removed;

    removed = Timer_DescheduleRoutine(&funcInfoPtr->queueElement);
    if (removed && funcInfoPtr->allocated) {
	free((Address) funcInfoPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_ServerInit --
 *
 *	Initialize the state and the set of processes needed to execute
 *	functions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Server info table initialized.
 *
 *----------------------------------------------------------------------
 */
void
Proc_ServerInit()
{
    int		i;

    serverInfoTable = 
	    (ServerInfo *) Vm_RawAlloc(proc_NumServers * sizeof(ServerInfo));
    for (i = 0; i < proc_NumServers; i++) {
	serverInfoTable[i].index = i;
	serverInfoTable[i].flags = 0;
	serverInfoTable[i].condition.waiting = 0;
    }
    Sync_SemInitDynamic(&serverMutex, "Proc:serverMutex");
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ServerProc --
 *
 *	Function for a server process.
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
Proc_ServerProc()
{
    register	ServerInfo	*serverInfoPtr;
    Proc_CallInfo		callInfo;
    int				i;

    MASTER_LOCK(&serverMutex);
    Sync_SemRegister(&serverMutex);
    /*
     * Find which server table entry that we are to use.
     */
    for (i = 0, serverInfoPtr = serverInfoTable;
	 i < proc_NumServers;
	 i++, serverInfoPtr++) {
	if (serverInfoPtr->flags == 0) {
	    serverInfoPtr->flags = ENTRY_INUSE;
	    break;
	}
    }
    if (i == proc_NumServers) {
	MASTER_UNLOCK(&serverMutex);
	printf("Warning: Proc_ServerProc: No server entries free.\n");
	Proc_Exit(0);
    }

    Sched_SetClearUsageFlag();

    while (!sys_ShuttingDown) {
	if (!(serverInfoPtr->flags & FUNC_PENDING)) {
	    /*
	     * There is nothing scheduled for us to do.  If there is something
	     * on the queue then dequeue it.  Otherwise sleep.
	     */
	    if (!QUEUE_EMPTY) {
		serverInfoPtr->info = queue[frontIndex];
		if (frontIndex == NUM_QUEUE_ELEMENTS - 1) {
		    frontIndex = 0;
		} else {
		    frontIndex++;
		}
		if (frontIndex == nextIndex) {
		    frontIndex = -1;
		    nextIndex = 0;
		}
	    } else {
		Sync_MasterWait(&serverInfoPtr->condition,
				&serverMutex, TRUE);
		continue;
	    }
	}

	serverInfoPtr->flags |= SERVER_BUSY;
	serverInfoPtr->flags &= ~FUNC_PENDING;

	MASTER_UNLOCK(&serverMutex);

	/*
	 * Call the function.
	 */
	callInfo.interval = 0;
	callInfo.clientData = serverInfoPtr->info.data;
	callInfo.token = (ClientData) serverInfoPtr->info.funcInfoPtr;
	serverInfoPtr->info.func(serverInfoPtr->info.data, &callInfo);

	if (callInfo.interval != 0) {
	    /* 
	     * It wants us to call it again.
	     */
	    ScheduleFunc(serverInfoPtr->info.func, callInfo.clientData,
			 callInfo.interval, serverInfoPtr->info.funcInfoPtr);
	} else {
	    /*
	     * Aren't supposed to call it again.  Free up function info
	     * if was allocated for this function.
	     */
	    if (serverInfoPtr->info.funcInfoPtr != (FuncInfo *) NIL) {
		free((Address) serverInfoPtr->info.funcInfoPtr);
	    }
	}

	/*
	 * Go back around looking for something else to do.
	 */
	MASTER_LOCK(&serverMutex);
	serverInfoPtr->flags &= ~SERVER_BUSY;
    }
    MASTER_UNLOCK(&serverMutex);
    printf("Proc_ServerProc exiting.\n");
}


/*
 *----------------------------------------------------------------------
 *
 * ScheduleFunc --
 *
 *	Schedule the given function to be called at the given time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ScheduleFunc(func, clientData, interval, funcInfoPtr)
    void		(*func) _ARGS_((ClientData clientData, 
			Proc_CallInfo	*callInfoPtr));	/* Function to call. */
    ClientData		clientData;	/* Data to pass function. */
    unsigned	int	interval;	/* Time to wait before calling func. */
    FuncInfo		*funcInfoPtr;	/* Pointer to function information
					 * structure that may already exist. */
{
    if (funcInfoPtr == (FuncInfo *) NIL) {
	/*
	 * We have not allocated a structure yet for waiting.  Do it now.
	 */
	funcInfoPtr = (FuncInfo *) malloc(sizeof (FuncInfo));
	funcInfoPtr->func = func;
	funcInfoPtr->data = clientData;
	funcInfoPtr->allocated = TRUE;
	funcInfoPtr->queueElement.routine = CallFuncFromTimer;
	funcInfoPtr->queueElement.clientData = (ClientData) funcInfoPtr;
    } else {
	funcInfoPtr->data = clientData;
    }

    /*
     * Schedule the call back.
     */
    funcInfoPtr->queueElement.interval = interval;
    Timer_ScheduleRoutine(&funcInfoPtr->queueElement, TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * CallFuncFromTimer --
 *
 *	Actually schedule the calling of the function by one of the
 *	server processes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Item will be enqueued if no free processes are available.  Otherwise
 *	The state of one of the processes is mucked with so that it knows
 *	that it has a function to executed.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
CallFuncFromTimer(time, funcInfoPtr)
    Timer_Ticks		time;
    FuncInfo		*funcInfoPtr;
{
    CallFunc(funcInfoPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * CallFunc --
 *
 *	Actually schedule the calling of the function by one of the
 *	server processes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Item will be enqueued if no free processes are available.  Otherwise
 *	The state of one of the processes is mucked with so that it knows
 *	that it has a function to executed.
 *
 *----------------------------------------------------------------------
 */

static void
CallFunc(funcInfoPtr)
    FuncInfo		*funcInfoPtr;
{
    register	ServerInfo	*serverInfoPtr;
    register	QueueElement	*queueElementPtr;
    Boolean			queueIt = TRUE;
    int				i;

    MASTER_LOCK(&serverMutex);
    if (QUEUE_EMPTY) {
	/*
	 * If the the queue is empty then there may in fact be a
	 * server ready to call out function.
	 */
	for (i = 0, serverInfoPtr = serverInfoTable;
	     i < proc_NumServers;
	     i++, serverInfoPtr++) {
	    if (!(serverInfoPtr->flags & ENTRY_INUSE) ||
	        (serverInfoPtr->flags & (SERVER_BUSY | FUNC_PENDING))) {
		continue;
	    }
	    serverInfoPtr->flags |= FUNC_PENDING;
	    serverInfoPtr->info.func = funcInfoPtr->func;
	    serverInfoPtr->info.data = funcInfoPtr->data;
	    if (funcInfoPtr->allocated) {
		serverInfoPtr->info.funcInfoPtr = funcInfoPtr;
	    } else {
		serverInfoPtr->info.funcInfoPtr = (FuncInfo *) NIL;
	    }
	    Sync_MasterBroadcast(&serverInfoPtr->condition);
	    queueIt = FALSE;
	    break;
	}
    }

    if (queueIt) {
	/*
	 * There are no free servers available so we have to queue up the
	 * message.
	 */
	if (QUEUE_FULL) {
	    extern Boolean sys_ShouldSyncDisks;
	    Mach_EnableIntr();
	    sys_ShouldSyncDisks = FALSE;
	    panic("CallFunc: Process queue full.\n");
	}
	queueElementPtr = &queue[nextIndex];
	queueElementPtr->func = funcInfoPtr->func;
	queueElementPtr->data = funcInfoPtr->data;
	if (funcInfoPtr->allocated) {
	    queueElementPtr->funcInfoPtr = funcInfoPtr;
	} else {
	    queueElementPtr->funcInfoPtr = (FuncInfo *) NIL;
	}
	if (nextIndex == NUM_QUEUE_ELEMENTS - 1) {
	    nextIndex = 0;
	} else {
	    nextIndex++;
	}
	if (frontIndex == -1) {
	    frontIndex = 0;
	}
    }

    MASTER_UNLOCK(&serverMutex);
}
