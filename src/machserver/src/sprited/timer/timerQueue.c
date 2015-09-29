/* 
 * timerQueue.c --
 *
 *	Routines to manage the timer queue.  This queue lists functions
 *	that are to be called at particular times (e.g., for network
 *	timeouts).
 *
 *	The queue is managed by a thread that waits for the next scheduled 
 *	time and then calls the requested function.  The thread waits by 
 *	doing a message receive on a private port and then timing out.  If 
 *	the queue is changed so that the thread needs to time out earlier, 
 *	a message is sent to the private port, which forces the thread to 
 *	look around and reschedule itself.
 *
 *
 * Copyright 1986, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/timer/RCS/timerQueue.c,v 1.6 92/04/29 22:08:13 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <bstring.h>
#include <mach.h>
#include <mach_error.h>
#include <mach/mach_host.h>
#include <status.h>
#include <stdlib.h>
#include <user/time.h>

#include <machCalls.h>
#include <proc.h>
#include <timer.h>
#include <timerInt.h>
#include <timerStat.h>
#include <sync.h>
#include <list.h>
#include <utilsMach.h>
#include <vm.h>

/* 
 * Someday TIMER_BACKLOG_STATS will go away (I hope).  However, we keep a 
 * separate flag to control statistics gathering, because that's the way 
 * native Sprite is set up.
 */
#if defined(TIMER_BACKLOG_STATS) && !defined(GATHER_STAT)
#define GATHER_STAT
#endif


/*
 *  The timer queue is a linked list of routines that need to be called at
 *  certain times. TimerQueueList points to the head structure for the queue.
 *
 * >>>>>>>>>>>>>>>>>>>
 * N.B. For debugging purposes, timerQueueList is global
 *      so it can be accessed by routines outside this module.
 * <<<<<<<<<<<<<<<<<<<
 */ 

/* static */ List_Links	*timerQueueList;


static mach_port_t timeoutPort;	/* timer thread sleeps on this port */
static mach_msg_header_t timeoutMsg; /* used to interrupt the sleeping 
				      * thread. */ 
static Boolean threadSleeping;	/* TRUE if the thread is sleeping, and no 
				 * "wakeup" messages have been sent.
				 * protected by timerMutex */

/*
 * The timer module mutex semaphore.  
 */

Sync_Semaphore timerMutex;

/* 
 * The process ID for the timer thread.
 */

static Proc_PID timerThreadPid;

/* 
 * Globals related to the timer resolution offered by Mach.
 */

int timer_Quantum;		/* minimum quantum offered by Mach, in msec */
static int timerMinTimeout;	/* minimum timeout offered by Mach, in msec */

Time timer_Resolution;		/* resolution for scheduling a callback 
				 * routine */ 

/*
 *  Array for debugging trace, protected by timerMutex.
 */

#ifdef DEBUG

#define TRACE_SIZE 500
#define INC(ctr) { (ctr) = ((ctr) == TRACE_SIZE-1) ? 0 : (ctr)+1; }
typedef struct {
    char *action;
    Timer_QueueElement *eltPtr;
    Proc_PID pid;
} TimerDebugElt;

static TimerDebugElt timerDebugArray[TRACE_SIZE];
static int timerDebugCounter;

#define TIMER_TRACE(timerPtr, string) \
{ \
    TimerDebugElt *ptr = &timerDebugArray[timerDebugCounter]; \
    INC(timerDebugCounter); \
    ptr->action = string; \
    ptr->eltPtr = timerPtr; \
    ptr->pid = Proc_GetCurrentProc()->processID; \
}

#else DEBUG

#define TIMER_TRACE(timerPtr, string)

#endif DEBUG

/* 
 * timer_QueueMaxBacklog is a debugging value.  If the timer queue gets this 
 * far behind, there may be a problem that we should notify the user about. 
 */
static Boolean timerQueueDebug = FALSE;
Time timer_QueueMaxBacklog;

/*
 * Instrumentation for counting how many times the routines get called.
 */

Timer_Statistics timer_Statistics;


/*
 * Procedures internal to this file
 */

static void GetSchedInfo _ARGS_((void));
static void InitTimerQueue _ARGS_((void));
static void TimerDumpElement _ARGS_((Timer_QueueElement *timerPtr));
static void TimerThread _ARGS_((void));
static mach_msg_timeout_t TimeToMs _ARGS_((Time someTime));


/*
 *----------------------------------------------------------------------
 *
 * Timer_Init --
 *
 *	Timer module initialization.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The timer queue is initialized and a thread created to monitor it.  
 *     Initialization is done so that we can get the time of day.
 *
 *----------------------------------------------------------------------
 */

void
Timer_Init()
{
    static	Boolean	initialized	= FALSE;

    Sync_SemInitDynamic(&timerMutex,"Timer:timerMutex");

    if (initialized) {
	printf("Timer_Init: Timer module initialized more that once!\n");
    }
    initialized = TRUE;

    bzero((Address) &timer_Statistics, sizeof(timer_Statistics));

    GetSchedInfo();

    /*
     * Initialize the time of day clock.
     */
    TimerClock_Init();

    InitTimerQueue();
}


/*
 *----------------------------------------------------------------------
 *
 * InitTimerQueue --
 *
 *	Initialization for the timer queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer queue is created and initialized.  A thread is created to 
 *	monitor it.
 *
 *----------------------------------------------------------------------
 */

static void
InitTimerQueue()
{
    kern_return_t kernStatus;

    /* 
     * Complain if the timer gets behind by more than twice its resolution.
     */
    Time_Multiply(timer_Resolution, 2, &timer_QueueMaxBacklog);

    timerQueueList = (List_Links *) Vm_BootAlloc(sizeof(List_Links));
    List_Init(timerQueueList);

    /* 
     * Create the private port that the timer thread will sleep on.  Make 
     * sure we can also send "reschedule" messages to it.
     */
    kernStatus = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				    &timeoutPort);
    if (kernStatus != KERN_SUCCESS) {
	panic("InitTimerQueue: couldn't allocate timeout port: %s\n",
	      mach_error_string(kernStatus));
    }
    kernStatus = mach_port_insert_right(mach_task_self(), timeoutPort,
					timeoutPort,
					MACH_MSG_TYPE_MAKE_SEND);
    if (kernStatus != KERN_SUCCESS) {
	panic("InitTimerQueue: couldn't make timeout send right: %s\n",
	      mach_error_string(kernStatus));
    }

    /* 
     * Create a prototype message that we can use to reschedule the timer 
     * thread. 
     */
    timeoutMsg.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    timeoutMsg.msgh_size = 0;
    timeoutMsg.msgh_remote_port = timeoutPort;
    timeoutMsg.msgh_local_port = MACH_PORT_NULL;
    timeoutMsg.msgh_kind = MACH_MSGH_KIND_NORMAL;
    timeoutMsg.msgh_id = 0;

    /* 
     * Fork off a thread to manage the queue.
     */
    if (Proc_NewProc((Address) UTILSMACH_MAGIC_CAST TimerThread,
		     (Address)0, PROC_KERNEL, FALSE,
		     &timerThreadPid, "timer thread") != SUCCESS) {
	panic("InitTimerQueue: couldn't start thread.");
    }
}


/*
 *----------------------------------------------------------------------
 *
 *  TimerThread --
 *
 *      This routine calls functions from the timer queue when their time 
 *      is reached.
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Routines on the timer queue may cause side effects.
 *
 *----------------------------------------------------------------------
 */

static void
TimerThread()
{
    register Timer_QueueElement	*readyPtr; /* Ptr to queue element that's ready
					    * to be called. */
    Time	timeOfDay;	/* current time of day */
    Time	nextActivity;	/* time of expected next activity */
    Time	sleepTime;	/* time to wait for next activity */
    mach_msg_header_t dummyMsg;	/* buffer to read reschedule messages into */
    Time	backlog;	/* how late is the current call */
    int callsThisInterval;	/* count of callbacks since last pass thru 
				 * the queue*/
    ReturnStatus status;
    
    /* 
     * Raise our priority, to help ensure that functions are called when
     * they're supposed to be.  First make sure that we are wired to a
     * kernel thread, so that changing the kernel thread's priority makes
     * sense.  
     * XXX I'm not sure how much good this really does.
     */
    cthread_wire();
    status = Proc_SetPriority(PROC_MY_PID, PROC_NO_INTR_PRIORITY, FALSE);
    if (status != SUCCESS) {
	printf("TimerThread: warning: couldn't set priority: %s\n",
	       Stat_GetMsg(status));
    }

    for (;;) {

	/*
	 *  Go through the queue and call all routines that are scheduled
	 *  to be called. Since the queue is ordered by time, we can quit
	 *  looking when we find the first routine that does not need to be
	 *  called.
	 */
	
	Timer_GetTimeOfDay(&timeOfDay, (int *)NULL, (Boolean *)NULL);
	callsThisInterval = 0;
    
#ifdef SPRITED_LOCALDISK	
	Dev_GatherDiskStats();
#endif
    
	MASTER_LOCK(&timerMutex);
	while (!List_IsEmpty(timerQueueList)) {
	    readyPtr = (Timer_QueueElement *)List_First(timerQueueList); 
	    if (Time_GT(readyPtr->time, timeOfDay)) {
		break;
	    }
	    ++callsThisInterval;
#ifdef GATHER_STAT
	    Time_Subtract(timeOfDay, readyPtr->time, &backlog);
	    if (Time_GT(backlog, timer_QueueMaxBacklog)) {
		timer_Statistics.totalLateCalls++;
		if (timerQueueDebug) {
		    printf("%s: %d.%03d call #%d: called 0x%x %d msec late.\n",
			   "TimerThread", timeOfDay.seconds % 60,
			   timeOfDay.microseconds / 1000,
			   callsThisInterval,
			   readyPtr->routine, TimeToMs(backlog));
		}
	    }
#endif
		
	    /*
	     *  First remove the item before calling it so the routine 
	     *  can call Timer_ScheduleRoutine to reschedule itself on 
	     *  the timer queue and not mess up the pointers on the 
	     *  queue.
	     */
		
	    TIMER_TRACE(readyPtr, "about to call");
	    List_Remove((List_Links *)readyPtr);
		
	    /*
	     *  Now call the routine.  The routine is passed the time
	     *  it was scheduled to be called at and a client-specified
	     *  argument.
	     * 
	     *  We release the timerMutex during the call backs to
	     *  prevent deadlocks.
	     */
	    
	    if (readyPtr->routine == 0) {
		panic("TimerThread: t.q.e. routine == 0\n");
	    } else {
		void        (*routine) _ARGS_((Timer_Ticks timeTicks,
					       ClientData  clientData));
		Timer_Ticks timeTk;
		ClientData  clientData;
		
		readyPtr->processed = TRUE;
		routine = readyPtr->routine;
		timeTk = readyPtr->time;
		clientData = readyPtr->clientData;
		MASTER_UNLOCK(&timerMutex);
		(routine) (timeTk, clientData);
		MASTER_LOCK(&timerMutex);
	    }
	}

	/* 
	 * No more elements to process for now.  Get a new time of day in 
	 * case it took a long time to process the list.  Then figure out
	 * how long to sleep and wait for that long.  If there's nothing in
	 * the timer queue, sleep for 10 seconds, just to pick a random
	 * number.
	 */
	
	Timer_GetTimeOfDay(&timeOfDay, (int *)NULL, (Boolean *)NULL);
	if (List_IsEmpty(timerQueueList)) {
	    nextActivity = timeOfDay;
	    nextActivity.seconds += 10;
	} else {
	    readyPtr = (Timer_QueueElement *)List_First(timerQueueList);
	    nextActivity = readyPtr->time;
	}
	threadSleeping = TRUE;
	MASTER_UNLOCK(&timerMutex);

	Time_Subtract(nextActivity, timeOfDay, &sleepTime);
	if (Time_LE(sleepTime, time_ZeroSeconds)) {
#ifdef GATHER_STAT
	    timer_Statistics.skipSleep++;
#endif
	} else {
	    (void)mach_msg(&dummyMsg, MACH_RCV_MSG|MACH_RCV_TIMEOUT,
			   0, sizeof(dummyMsg), timeoutPort,
			   TimeToMs(sleepTime), MACH_PORT_NULL);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * TimeToMs --
 *
 *	Convert a timeout Time into milliseconds.
 *
 * Results:
 *	Returns the number of milliseconds corresponding to aTime.  May
 *	round down to zero.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
static mach_msg_timeout_t
TimeToMs(aTime)
    Time aTime;			/* time to convert */
{
    return (aTime.seconds * 1000 + aTime.microseconds / 1000);
}



/*
 *----------------------------------------------------------------------
 *
 * Timer_ScheduleRoutine --
 *
 *	Schedules a routine to be called at a certain time by adding 
 *	it to the timer queue. A routine is specified using a
 *	Timer_QueueElement structure, which is described in timer.h.
 *
 *	When the client routine is called at its scheduled time, it is 
 *	passed two parameters:
 *	 a) the time it is scheduled to be called at, and
 *	 b) uninterpreted data.
 *	Hence the routine should be declared as:
 *
 *	    void
 *	    ExampleRoutine(time, data)
 *		Timer_Ticks time;
 *		ClientData data;
 *	    {}
 *
 *
 *	The time a routine should be called at can be specified in two
 *	ways: an absolute time or an interval. For example, to have 
 *	ExampleRoutine called in 1 hour, the timer queue element would 
 *	be setup as:
 *	    Timer_QueueElement	element;
 *
 *	    element.routine	= ExampleRoutine;
 *	    element.clientData	= (ClientData) 0;
 *	    element.interval	= timer_IntOneHour;
 *	    Timer_ScheduleRoutine(&element, TRUE);
 *
 *	The 2nd argument (TRUE) to Timer_ScheduleRoutine means the routine
 *	will be called at the interval + the current time.
 *
 *      Once ExampleRoutine is called, it can schedule itself to be
 *      called again using Timer_ScheduleRoutine().   
 *
 *	    Timer_ScheduleRoutine(&element, TRUE);
 *
 *	The 2nd argument again means schedule the routine relative to the
 *	current time. Since we still want ExampleRoutine to be called in
 *	an hour, we don't have to change the interval field in the timer
 *	queue element.
 *      Obviously, the timer queue element has to be accessible 
 *	to ExampleRoutine.
 *
 *	If we want ExampleRoutine to be called at a specific time, say
 *	March 1, 1986, the time field in the t.q. element must be set:
 *
 *	    element.routine	= ExampleRoutine;
 *	    element.clientData	= (ClientData) 0;
 *	    element.time	= march1;
 *	    Timer_ScheduleRoutine(&element, FALSE);
 *
 *	(Assume march1 has the appropriate value for the date 3/1/86.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer queue element is added to the timer queue.  A message is 
 *	sent to the timer thread if the new element must be processed 
 *	earlier than the previous head of the timer queue.
 *
 *----------------------------------------------------------------------
 */

void
Timer_ScheduleRoutine(newElementPtr, interval)
    register	Timer_QueueElement *newElementPtr; /* routine to be added */
    Boolean	interval;	/* TRUE if schedule relative to current time. */
{
    register List_Links	 *itemPtr;
    Boolean inserted;		/* TRUE if added to Q in FORALL loop. */
    Boolean emptyList;		/* TRUE if queue was empty at proc entry */
    Time queuedActivity;	/* time to run the 1st element in the queue */
    Time thisActivity;		/* time to run this element */
    kern_return_t kernStatus;

    MASTER_LOCK(&timerMutex); 

    if (List_IsEmpty(timerQueueList)) {
	emptyList = TRUE;
    } else {
	emptyList = FALSE;
	itemPtr = List_First(timerQueueList);
	queuedActivity = ((Timer_QueueElement *)itemPtr)->time;
    }

    /*
     *  Go through the timer queue and insert the new routine.  The queue
     *  is ordered by the time field in the element.  The sooner the
     *  routine needs to be called, the closer it is to the front of the
     *  queue.  The new routine will not be added to the queue inside the
     *  FOR loop if its scheduled time is after all elements in the queue
     *  or the queue is empty.  It will be added after the last element in
     *  the queue.
     */

    inserted = FALSE;  /* assume new element not inserted inside FOR loop.*/

#ifdef GATHER_STAT
    timer_Statistics.schedule++;
#endif

    /*
     * Safety check.
     */
    if (newElementPtr->routine == 0) {
	panic("Timer_ScheduleRoutine: bad address for t.q.e. routine.\n");
    }

    /* 
     *  Reset the processed flag. It is used by the client to see if 
     *  the routine is being called from the timer queue. This flag is
     *  necessary because the client passes in the timer queue element
     *  and it may need to examine the element to determine its status.
     */
    newElementPtr->processed = FALSE;

    /*
     * Convert the interval into an absolute time by adding the 
     * interval to the current time.
     */
    if (interval) {
	Timer_Ticks currentTime;
	Timer_GetCurrentTicks(&currentTime);
	Timer_AddIntervalToTicks(currentTime, newElementPtr->interval,
            	       &(newElementPtr->time));
    }
    thisActivity = newElementPtr->time;

    /* 
     * Sanity check.  The element should not already be in the list (the 
     * List package will break if it is).
     */
    if (timerQueueDebug) {
	LIST_FORALL(timerQueueList, itemPtr) {
	    if (itemPtr == (List_Links *)newElementPtr) {
		panic("%s: element is already in the queue.\n",
		      "Timer_ScheduleRoutine");
	    }
	}
    }

    List_InitElement((List_Links *) newElementPtr);

    LIST_FORALL(timerQueueList, itemPtr) {

       if (Timer_TickLT(newElementPtr->time, 
	   ((Timer_QueueElement *)itemPtr)->time)) {
	    List_Insert((List_Links *) newElementPtr, LIST_BEFORE(itemPtr));
	    inserted = TRUE;
	    break;
	}
    }

    if (!inserted) {
	List_Insert((List_Links *) newElementPtr, LIST_ATREAR(timerQueueList));
    }
    TIMER_TRACE(newElementPtr, "inserted");

    /* 
     * Make the timer thread reschedule if either the queue was empty or
     * the new element has to get run before anything else on the queue.
     * On the other hand, if the current thread *is* the timer thread,
     * don't bother sending a message; the thread will recompute the new
     * sleep time anyway. 
     * Unless we think that the message is needed (because the thread is 
     * sleeping or about to sleep), don't send it.  Bad things seem to
     * happen if the queue fills up (XXX).
     */
    
    if ((emptyList || Time_LT(thisActivity, queuedActivity)) &&
	    Proc_GetCurrentProc()->processID != timerThreadPid &&
	    threadSleeping) {
#ifdef GATHER_STAT
	timer_Statistics.resched++;
#endif
	kernStatus = mach_msg(&timeoutMsg, MACH_SEND_MSG | MACH_SEND_TIMEOUT,
			      sizeof(timeoutMsg), 0, MACH_PORT_NULL,
			      0, MACH_PORT_NULL);
	if (kernStatus == KERN_SUCCESS) {
	    threadSleeping = FALSE;
	} else {
	    printf("Timer_ScheduleRoutine: couldn't prod timer: %s\n",
		   mach_error_string(kernStatus));
	}
    }

    MASTER_UNLOCK(&timerMutex); 
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_DescheduleRoutine --
 *
 *	Deschedules a routine to be called at a certain time by removing 
 *	it from the timer queue.
 *
 *	Note that Timer_DescheduleRoutine does NOT guarantee that the 
 *	routine to be descheduled will not be called, only that the 
 *	routine will not be on the timer queue when Timer_DescheduleRoutine 
 *	returns.
 *
 *	If Timer_DescheduleRoutine is able to obtain the timer mutex before
 *	the timer thread wakes up, the routine will be removed from the
 *	timer queue before the interrupt handler has a chance to call it.
 *	If the timer thread is able to obtain the timer mutex before
 *	Timer_DescheduleRoutine, then the thread will remove the element
 *	from the queue and call the routine before Timer_DescheduleRoutine
 *	has a chance to remove it.
 *
 * Results:
 *	TRUE if the element was removed, FALSE if it was already gone.
 *
 * Side effects:
 *	The timer queue structure is updated. 
 *
 *----------------------------------------------------------------------
 */

Boolean
Timer_DescheduleRoutine(elementPtr)
    register Timer_QueueElement *elementPtr;	/* routine to be removed */
{
    register List_Links	 *itemPtr;
    Boolean foundIt = FALSE;

    /*
     *  Go through the timer queue and remove the routine.  
     */

    MASTER_LOCK(&timerMutex); 
#ifdef GATHER_STAT
    timer_Statistics.desched++;
#endif
    TIMER_TRACE(elementPtr, "about to deschedule");

    LIST_FORALL(timerQueueList, itemPtr) {
	if ((List_Links *) elementPtr == itemPtr) {
	    List_Remove(itemPtr);
	    TIMER_TRACE(elementPtr, "descheduled");
	    foundIt = TRUE;
	    break;
	}
    }
    if (!foundIt) {
	timer_Statistics.failedDesched++;
    }

    /* 
     * Don't make the timer thread reschedule; it'll find out soon enough 
     * that the queue has been changed.
     */

    MASTER_UNLOCK(&timerMutex);
    return(foundIt);
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_DumpQueue --
 *
 *	Output the timer queue on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Timer_DumpQueue(data)
    ClientData	data;	/* Not used. */
{
    Timer_Ticks	ticks;
    Time	time;
    List_Links *itemPtr;

    Timer_GetCurrentTicks(&ticks);
    Timer_TicksToTime(ticks, &time);
    printf("Now: %d.%06u sec\n", time.seconds, time.microseconds);

    if (List_IsEmpty(timerQueueList)) {
	printf("\nList is empty.\n");
    } else {
	printf("\n");

	MASTER_LOCK(&timerMutex); 

	LIST_FORALL(timerQueueList, itemPtr) {
	    TimerDumpElement((Timer_QueueElement *) itemPtr);
	}

	MASTER_UNLOCK(&timerMutex); 

    }
}


/*
 *----------------------------------------------------------------------
 *
 * TimerDumpElement --
 *
 *	Output the more important parts of a Timer_QueueElement on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */

static void
TimerDumpElement(timerPtr)
    Timer_QueueElement *timerPtr;
{
    Time  	time;

    Timer_TicksToTime(timerPtr->time, &time);

    printf("(*0x%x)(0x%x) @ %d.%06u\n",
	    (Address) timerPtr->routine, 
	    (Address) timerPtr->clientData,
	    time.seconds, time.microseconds);
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_DumpStats --
 *
 *	Initializes and prints the timer module statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output is written to the display.
 *
 *----------------------------------------------------------------------
 */
void
Timer_DumpStats(arg)
    ClientData	arg;
{
    static   Timer_Ticks	start;
    static   Timer_Ticks	end;
    Timer_Ticks	diff;
    Time  	time;

    if (arg ==  (ClientData) 's') {
	Timer_GetCurrentTicks(&start);
	bzero((Address) &timer_Statistics,sizeof(timer_Statistics));
    } else {
	Timer_GetCurrentTicks(&end);
	Timer_SubtractTicks(end, start, &diff);
	Timer_TicksToTime(diff, &time);

	printf("\n%d.%06d Sched %d Res %d Des %d\n",
	    time.seconds, time.microseconds,
	    timer_Statistics.schedule,
	    timer_Statistics.resched,
	    timer_Statistics.desched
	);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_IsScheduled --
 *
 *	Is the given entry in the timer queue?  (This routine is intended 
 *	for use in debugging.)
 *
 * Results:
 *	Returns TRUE if the given entry is found in the queue, FALSE if 
 *	not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Timer_IsScheduled(elementPtr)
    Timer_QueueElement *elementPtr; /* routine to look for */
{
    Boolean foundIt = FALSE;
    List_Links *itemPtr;

    MASTER_LOCK(&timerMutex); 
    LIST_FORALL(timerQueueList, itemPtr) {
	if ((List_Links *)elementPtr == itemPtr) {
	    foundIt = TRUE;
	    break;
	}
    }
    MASTER_UNLOCK(&timerMutex);
    return foundIt;
}


/*
 *----------------------------------------------------------------------
 *
 * GetSchedInfo --
 *
 *	Get scheduling information from Mach.  Currently (13-Apr-92) this 
 *	information is mostly used by the timer module, so we obtain it 
 *	here, too.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes timer_Quantum and timerMinTimeout.
 *
 *----------------------------------------------------------------------
 */

static void
GetSchedInfo()
{
    kern_return_t kernStatus;
    mach_msg_type_number_t infoCount;
    struct host_sched_info schedInfo;

    infoCount = HOST_SCHED_INFO_COUNT;
    kernStatus = host_info(mach_host_self(), HOST_SCHED_INFO,
			   (host_info_t)&schedInfo, &infoCount);
    if (kernStatus != KERN_SUCCESS) {
	panic("GetSchedInfo failed: %s\n", mach_error_string(kernStatus));
    }
    if (infoCount != HOST_SCHED_INFO_COUNT) {
	panic("GetSchedInfo: expected %d words, got %d.\n",
	      HOST_SCHED_INFO_COUNT, infoCount);
    }

    /* 
     * XXX the sched info times are in usec, up thru MK73, even though they 
     * are documented as msec.
     */
    timer_Quantum = schedInfo.min_quantum / 1000;
    timerMinTimeout = schedInfo.min_timeout / 1000; 
    timer_Resolution.seconds = schedInfo.min_timeout / 1000000;
    timer_Resolution.microseconds = schedInfo.min_timeout % 1000000;
}
