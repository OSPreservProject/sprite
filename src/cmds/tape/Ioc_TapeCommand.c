/* 
 * Ioc_TapeCommand.c --
 *
 *	Source code for the Ioc_TapeCommand library procedure.
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
static char rcsid[] = "$Header: Ioc_Reposition.c,v 1.2 88/07/29 17:08:46 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>
#include <dev/tape.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_TapeCommand --
 *
 *	Issue a command to a tape drive.  There are several commands defined
 *	that do tape specific operations like rewind, file skip, write file
 *	mark, etc.  These are defined in <dev/tape.h>.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Manipulate a tape drive.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_TapeCommand(streamID, command, count)
    int streamID;		/* StreamID returned from Fs_Open */
    int command;		/* Commands defined in <dev/tape.h>. */
    int count;			/* Repetition count for the command. */
{
    register ReturnStatus status;
    Dev_TapeCommand args;

    args.command = command;
    args.count = count;
    status = Fs_IOControl(streamID, IOC_TAPE_COMMAND, sizeof(Dev_TapeCommand),
			(Address)&args, 0, (Address) 0);
    return(status);
}
