/* 
 * dumpEvents.c --
 *
 * Routines to register events (characters) for debugging dump program.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev.h"
#include "dumpInt.h"

/*
 * Table of routines and their arguments to be called on dump events.
 */
static EventTableType sunEventTable[] = {
    {'k', Dev_ConsoleReset, (ClientData) TRUE,"Reset console to keyboard mode"},
    {'l', Dev_ConsoleReset, (ClientData) FALSE,
					"Reset console to raw mode (for X)"},
	/* This MUST be the last entry */
    {'\000', LAST_EVENT, NULL_ARG, (char *) 0 }
};


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
Dump_Register_Events(eventTable)
    EventTableType	*eventTable;
{
    EventTableType	*entry;

    for (entry = eventTable; entry->routine != LAST_EVENT; entry++) {
	if (entry->routine == RESERVED_EVENT) {
		continue;
	}
	Dev_RegisterConsoleCmd(entry->key, entry->routine, entry->argument);
    }

    for (entry = sunEventTable; entry->routine != LAST_EVENT; entry++) {
	if (entry->routine == RESERVED_EVENT) {
		continue;
	}
	Dev_RegisterConsoleCmd(entry->key, entry->routine, entry->argument);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Dump_Show_Local_Menu --
 *
 *	Dump out a list of the local to the Sun L1-key magic commands.
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

    for (entry = sunEventTable; entry->routine != LAST_EVENT; entry++) {
	printf("%c - %s\n",entry->key, entry->description);
    }
}
