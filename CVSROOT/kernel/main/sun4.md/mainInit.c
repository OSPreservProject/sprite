/* 
 *  main.c --
 *
 *	The main program for Sprite: initializes modules and creates
 *	system processes. Also creates a process to run the Init program.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* !lint */

#include "machMon.h"


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	First main routine for sun4.  All it does is print Hello World.
 *	It should loop, doing this forever.
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
main()
{
    for ( ; ; ) {
	Mach_MonPrintf("Hello World!\n");
    }
}
