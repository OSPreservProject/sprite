/* 
 * sync.c --
 *
 *	Write the file system's cache to disk.
 *
 * Copyright (C) 1988 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /a/newcmds/sync/RCS/sync.c,v 1.1 88/08/16 11:04:28 nelson Exp Locker: mgbaker $ SPRITE (Berkeley)";
#endif not lint

#include "sys.h"


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	The main program for sync.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
main()
{
    Sys_Shutdown(SYS_WRITE_BACK, NULL);
}
