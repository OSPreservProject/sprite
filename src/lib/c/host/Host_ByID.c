/* 
 * Host_ByID.c --
 *
 *	Source code for the Host_ByID library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: Host_ByID.c,v 1.2 88/08/08 13:25:04 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <host.h>
#include <hostInt.h>


/*
 *-----------------------------------------------------------------------
 *
 * Host_ByID --
 *
 *	Return info about a host based on its Sprite ID.
 *
 * Results:
 *	A pointer to a Host_Entry structure, or NULL if no machine
 *	with that ID exists.  The Host_Entry is statically allocated,
 *	and may be modified on the next call to any Host_ procedure.
 *
 * Side Effects:
 *	The host database is opened if it wasn't already so.
 *
 *-----------------------------------------------------------------------
 */
Host_Entry *
Host_ByID(id)
    register int  	id;	  	/* ID of machine to find */
{
    register Host_Entry	*entry;	    	/* Current entry in database */

    if (Host_Start() != 0) {
	return (Host_Entry *) NULL;
    }
    
    while (1) {
	entry = Host_Next();
	if (entry == (Host_Entry *) NULL) {
	    return entry;
	} else if (entry->id == id) {
	    return (entry);
	}
    }
}
