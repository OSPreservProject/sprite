/*
 * dumpInt.h --
 *
 *	Declarations of the event table for the ``dump'' utility.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/utils/dumpInt.h,v 9.1 90/09/11 14:11:40 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _DUMPINT
#define _DUMPINT
/*
 * Type of table of routines and their arguments to be called on dump events.
 */

typedef struct EventTableType {
	char		key;		/* Key for dump event. */
	void		(*routine) _ARGS_((ClientData));
					/* Routine to call upon event. */
	ClientData	argument;	/* Argument to routine. */
	char		*description;	/* Help description of event. */
} EventTableType;

/*
 * Special EventTableType.routines ---
 * RESERVED_EVENT - A event char reserved by other parse of the system.
 * LAST_EVENT - The last event in the table.
 * Special EventTableType.arguments ---
 * NULL_ARG - A missing argument pointer.
 */

#define RESERVED_EVENT	((void (*) _ARGS_((ClientData)) ) 1)	
#define	LAST_EVENT	((void (*) _ARGS_((ClientData)) ) 0)
#define	NULL_ARG	((ClientData) 0)

/*
 * Machine dependent routines for registering dump events.
 */

extern void Dump_Register_Events _ARGS_((EventTableType *));
extern void Dump_Show_Local_Menu _ARGS_((void));

#endif /* _DUMPINT */
