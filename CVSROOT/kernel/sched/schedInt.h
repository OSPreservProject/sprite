/*
 * schedInt.h --
 *
 *	Declarations of constants and variables local to the scheduling 
 *	module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SCHEDINT
#define _SCHEDINT

#include <list.h>

#define SCHED_MAX_DUMP_SIZE 100

extern List_Links *schedReadyQueueHdrPtr;

/*
 * To the scheduler module, a processor may be in one of following states:
 *	PROCESSOR_NOT_STARTED - The processor has not be started running
 * 				Sprite.
 *	PROCESSOR_ACTIVE      - The processor is currently running Sprite.
 *	PROCESSOR_IDLE	      - The processor is current running Sprite but
 *				should not be given processes to execute.
 *	PROCESSOR_STARTING    - The processor is in the act of starting.
 *	PROCESSOR_COUNTING_TICKS - The processor is idling for 5 seconds
 *				   to count idle ticks per second.  This
 *			 	   state is different from idling because
 *				   when the process is made ready after 5
 *				   seconds we want this processor to run it.
 */
typedef enum { 
	SCHED_PROCESSOR_NOT_STARTED, 
	SCHED_PROCESSOR_STARTING,
	SCHED_PROCESSOR_ACTIVE, 
	SCHED_PROCESSOR_IDLE,
	SCHED_PROCESSOR_COUNTING_TICKS
} Sched_ProcessorStatus;

extern	Sched_ProcessorStatus sched_ProcessorStatus[MACH_MAX_NUM_PROCESSORS];

/*
 * The desired quantum length, in microseconds.  The real quantum length
 * might be different since it has to be a multiple of the timer interrupt
 * interval.
 */

#define SCHED_DESIRED_QUANTUM 100000

#endif /* _SCHEDINT */
