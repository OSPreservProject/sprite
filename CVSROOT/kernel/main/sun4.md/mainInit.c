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
    extern	void	Timer_TimerInit();
    extern	void	Timer_TimerStart();
   /*
    * Initialize machine dependent info.  MUST BE CALLED HERE!!!.
    */
    Mach_Init();
    diddly(20);
    Timer_TimerInit();
    Timer_TimerStart();
#ifdef NOTDEF
    Mach_EnableIntr();		/* Should be ENABLE_INTR when I'm ready. */
#endif NOTDEF
    for ( ; ; ) {
	;
    }
}

int
diddly(x)
int	x;
{
    if (x == 0) {
	return 0;
    } else {
	Mach_MonPrintf("Hello World!\n");
	diddly(x - 1);
    }
    return 1;
}

printf(args)
char	*args;
{
	return;
}
