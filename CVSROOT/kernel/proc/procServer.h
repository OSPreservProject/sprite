/* 
 *  procServer.h --
 *
 *	Declarations to manage pool of server processes.
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

#ifndef _PROCSERVER
#define _PROCSERVER
#define _PROCSERVER
#define _PROCSERVER

/*
 * Information kept for each function that is scheduled to be called in the
 * future.
 */
typedef struct {
    void		(*func)();	/* Function to call. */
    ClientData		data;		/* Data to pass to function. */
    Boolean		allocated;	/* TRUE => Struct was allocated by
					 *         malloc. */
    Timer_QueueElement	queueElement;	/* Element used to put onto timer
					 * queue. */
} FuncInfo;

/*
 * Element of queue of pending requests for functions to be called.
 */
typedef struct {
    void	(*func)();		/* Function to call. */
    ClientData	data;			/* Data to pass to function. */
    FuncInfo	*funcInfoPtr;		/* Pointer to function info struct
					 * that was allocated if were
					 * put onto timer queue. */
} QueueElement;

/*
 * NUM_QUEUE_ELEMENTS	Maximum number of entries in the queue of pending
 *			functions.
 */
#define	NUM_QUEUE_ELEMENTS	128

#define	QUEUE_EMPTY	(frontIndex == -1)
#define	QUEUE_FULL	(frontIndex == nextIndex)

/*
 * Information kept for each server process.
 */
typedef struct {
    int			index;
    int			flags;	/* Flags defined below. */
    QueueElement	info;	/* Information to indicate next function to
				 * execute. */
    Sync_Condition	condition;	/* Condition to sleep on when waiting
					 * for something to do. */
} ServerInfo;

/*
 * Flags for server info struct:
 *
 *	ENTRY_INUSE	There is a server process associated with this
 *			entry.
 *	SERVER_BUSY	The server is busy executing some command.
 *	FUNC_PENDING	There is a function to execute.
 */
#define	ENTRY_INUSE	0x1
#define	SERVER_BUSY	0x2
#define	FUNC_PENDING	0x4

/*
 * Number of server processes.  There have to be enough to allow for
 * pageouts and block cleaning at the same time. This occurs while
 * paging heavily on a file server (or with a local disk used for paging).
 */
#define PROC_NUM_SERVER_PROCS	(FSCACHE_MAX_CLEANER_PROCS + VM_MAX_PAGE_OUT_PROCS)

extern ServerInfo	*serverInfoTable;

#endif /* _PROCSERVER */
