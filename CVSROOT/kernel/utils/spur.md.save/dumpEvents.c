/* 
 * dumpEvents.c --
 *
 * Routines to register events (characters) for debugging dump program.
 *
 * Since SPUR currently has no keyboard to bind command to the only way these
 * commands can be called is from the debugger. 
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dumpInt.h"

/*
 * Table of routines and their arguments to be called on dump events.
 */
static EventTableType spurEventTable[] = {
	/* This MUST be the last entry */
    {'\000', LAST_EVENT, NULL_ARG, (char *) 0 }
};

/*
 * Set to point at the machine independent events.
 */ 
static EventTableType	*eventTable;


/*
 *----------------------------------------------------------------------
 *
 * Dump_Register_Events --
 *
 *	Establish default procedural attachments for keyboard invocation
 *	of Dump routines.
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
Dump_Register_Events(eventTbl)
    EventTableType	*eventTbl;
{
    /*
     * Since there are no keys to bind to, just save a pointer to the
     * table.
     */

    eventTable = eventTbl;
}


/*
 *----------------------------------------------------------------------
 *
 * Dump_Show_Local_Menu --
 *
 *	Dump out a list of the local to the SPUR L1-key magic commands.
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
Dump_Show_Local_Menu()
{

    EventTableType	*entry;
    for (entry = spurEventTable; entry->routine != LAST_EVENT; entry++) {
	Sys_Printf("%c - %s\n",entry->key, entry->description);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DumpL1() --
 *
 *	Simulate the Sun L1 key on the SPUR. 
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
DumpL1(secondKey)
	char	secondKey;	/* Character typed with the L1. */
{
    EventTableType	*entry;
    /*
     * Check the machine dependent table first.
     */

    for (entry = spurEventTable; entry->routine != LAST_EVENT; entry++) {
	if (entry->key == secondKey) { 
		/*
		 * We have a match. Call if it is not a reserved_event.
		 */
		if (entry->routine != RESERVED_EVENT) {
			(entry->routine) (entry->argument);
		}
		return;
	}
    }
    /*
     * Try checking the machine independent table.
     */
    for (entry = eventTable; entry->routine != LAST_EVENT; entry++) {
	if (entry->key == secondKey) { 
		/*
		 * We have a match. Call if it is not a reserved_event.
		 */
		if (entry->routine != RESERVED_EVENT) {
			(entry->routine) (entry->argument);
		}
		return;
	}
    }

}

