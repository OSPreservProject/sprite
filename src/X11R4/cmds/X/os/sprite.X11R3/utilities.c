/*-
 * utilities.c --
 *	Various allocation and output utilities required by the
 *	server.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * TODO:
 *	- Snarf stuff for compressed fonts from 4.2bsd/utils.c
 *	- Add in support for !NOLOGOHACK
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: /mic/X11R3/src/cmds/Xsp/os/sprite/RCS/utilities.c,v 1.18 89/11/23 00:17:00 tve Exp $ SPRITE (Berkeley)";
#endif lint

/*
 * The following include file must be first, or this won't compile.
 */

#include    <stdlib.h>

#include    "spriteos.h"
#include    "opaque.h"
#include    "input.h"
#include    "site.h"

#include    <errno.h>
#include    <status.h>
#include    <stdio.h>
#include    <string.h>
#include    <sys/time.h>
#include    <varargs.h>

extern long defaultScreenSaverInterval;
extern long defaultScreenSaverTime;
extern int defaultScreenSaverBlanking;

Bool	clientsDoomed = 0;
int	debug = 0;

extern void KillServerResources();

/*-
 *-----------------------------------------------------------------------
 * AbortServer --
 *	Abort the server...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The server dies in a Big Way.
 *
 *-----------------------------------------------------------------------
 */
void
AbortServer()
{
/*
    fprintf(stdout,
	"Aborting... see /tmp/<hostname>:<display>.X11R3 for details\n");
    fflush(stdout);
*/
    panic ("Aborting...\n");
}
/*-
 *-----------------------------------------------------------------------
 * HangUp --
 *	The server has been interrupted. Close everything down...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The process exits.
 *
 *-----------------------------------------------------------------------
 */
void
HangUp ()
{
    KillServerResources();
    exit (0);
}

/*-
 *-----------------------------------------------------------------------
 * FatalError --
 *	An error has occurred that does not allow the server to continue.
 *	Print the given message and abort.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The server aborts/exits/dies/suicides/etc.
 *
 *-----------------------------------------------------------------------
 */
void
FatalError (va_alist)
    va_dcl
{
    va_list	ap;
    va_start(ap);
    ErrorF("\nFatal server bug!\n");
    vErrorF(&ap);
    ErrorF("\n");
    AbortServer();
    /*NOTREACHED*/
    va_end(ap);
}

/*-
 *-----------------------------------------------------------------------
 * Reset --
 *	Tell DIX to reset itself when SIG_TERM is received.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	clientsDoomed is set 1 and the dispatched is told to yield...
 *
 *-----------------------------------------------------------------------
 */
int
Reset()
{
    clientsDoomed = 1;
    SchedYield();
}

/*-
 *-----------------------------------------------------------------------
 * GetTimeInMillis --
 *	Return the current time in milliseconds. This really should be
 *	the time since boot, since that's what's passed in the events,
 *	but unfortunately, there's no way to get at that...
 *
 * Results:
 *	The time since Jan 1, 1970 in milliseconds.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
long
GetTimeInMillis()
{
    struct timeval tp;
    struct timezone tzp;

    if (gettimeofday(&tp, &tzp) != 0) {
	return 0;
    }
    return (tp.tv_sec*1000) + (tp.tv_usec/1000);
}

/*-
 *-----------------------------------------------------------------------
 * ErrorF --
 *	Print a formatted string on the error output.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Well...
 *
 *-----------------------------------------------------------------------
 */
void
ErrorF(va_alist)
	va_dcl
{
    va_list	ap;
    char	*fmt;
    va_start(ap);
    fmt = va_arg(ap, char*);
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
    va_end(ap);
}

/*-
 *-----------------------------------------------------------------------
 * vErrorF --
 *	Same as ErrorF, but with varargs parameter.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Well...
 *
 *-----------------------------------------------------------------------
 */
void
vErrorF(ap)
    va_list	*ap;
{
    char	*fmt;
    fmt = va_arg(*ap, char*);
    vfprintf(stderr, fmt, *ap);
    fflush(stderr);
}

/*-
 *-----------------------------------------------------------------------
 * Error --
 *	Report the status of the most recent operation.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A message is printed.
 *
 *-----------------------------------------------------------------------
 */
void
Error (str)
    char    *str;
{
    fprintf(stderr, "%s: %s\n", str, strerror(errno));
    fflush(stderr);
}

/*-
 *-----------------------------------------------------------------------
 * Notice --
 *	Print something which should be noticed...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	>...<
 *
 *-----------------------------------------------------------------------
 */
Notice()
{
    fprintf (stderr, "NOTE!\n");
    fflush (stderr);
}

UseMsg()
{
    ErrorF("use: X <display> [option] <tty>\n");
    ErrorF("-a #                   mouse acceleration (pixels)\n");
    ErrorF("-bp<:screen> color     BlackPixel for screen\n");
    ErrorF("-c                     turns off key-click\n");
    ErrorF("c #                    key-click volume (0-8)\n");
    ErrorF("-co string             color database file\n");
    ErrorF("-fc string             cursor font\n");
    ErrorF("-fn string             default text font name\n");
    ErrorF("-fp string             default text font path\n");
    ErrorF("-p #                   screen-saver pattern duration (seconds)\n");
    ErrorF("-r                     turns off auto-repeat\n");
    ErrorF("r                      turns on auto-repeat \n");
    ErrorF("-f #                   bell base (0-100)\n");
    ErrorF("-x string              loads named extension at init time \n");
    ErrorF("-help                  prints message with these options\n");
    ErrorF("-s #                   screen-saver timeout (seconds)\n");
    ErrorF("-t #                   mouse threshold (pixels)\n");
    ErrorF("-to #                  connection time out\n");
    ErrorF("v                      video blanking for screen-saver\n");
    ErrorF("-v                     screen-saver without video blanking\n");
    ErrorF("-wp<:screen> color     WhitePixel for screen\n");
    ErrorF("There may be other device-dependent options as well\n");
}

/*-
 *-----------------------------------------------------------------------
 * ProcessCommandLine --
 *	Process the arguments the server was given and do something with
 *	them.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Things may be set...
 *
 *-----------------------------------------------------------------------
 */
void
ProcessCommandLine ( argc, argv )
    int	    argc;
    char    *argv[];
    
{
    int	    i;
    
    for ( i = 1; i < argc; i++ )
    {
	/* initialize display */
	if(argv[i][0] ==  ':')  
	{
	    display = argv[i];
	    display++;
	}
	else if ( strcmp( argv[i], "-a") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.num = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "c") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.click = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-c") == 0)
	{
	    defaultKeyboardControl.click = 0;
	}
	else if ( strcmp( argv[i], "-co") == 0)
	{
	    if(++i < argc)
	        rgbPath = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-f") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.bell = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fc") == 0)
	{
	    if(++i < argc)
	        defaultCursorFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fn") == 0)
	{
	    if(++i < argc)
	        defaultTextFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fp") == 0)
	{
	    if(++i < argc)
	        defaultFontPath = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-help") == 0)
	{
	    UseMsg();
	    exit(0);
	}
	else if ( strcmp( argv[i], "-p") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverInterval = atoi(argv[i]) * MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "r") == 0)
	    defaultKeyboardControl.autoRepeat = 1;
	else if ( strcmp( argv[i], "-r") == 0)
	    defaultKeyboardControl.autoRepeat = 0;
	else if ( strcmp( argv[i], "-s") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverTime = atoi(argv[i]) * MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-t") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.threshold = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-to") == 0)
	{
	    if(++i < argc)
	    {
	        if((TimeOutValue = atoi(argv[i])) < 0)
		    TimeOutValue = DEFAULT_TIMEOUT;
	    }
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "v") == 0)
	    defaultScreenSaverBlanking = PreferBlanking;
	else if ( strcmp( argv[i], "-v") == 0)
	    defaultScreenSaverBlanking = DontPreferBlanking;
	else if ( strcmp( argv[i], "-x") == 0)
	{
	    if(++i >= argc)
		UseMsg();
	    /* For U**x, which doesn't support dynamic loading, there's nothing
	     * to do when we see a -x.  Either the extension is linked in or
	     * it isn't */
	} else if ( strcmp( argv[i], "-d") == 0)
	{
	    i += 1;
	    if (i >= argc) {
		UseMsg();
	    } else {
		char *opt;

		for (opt = argv[i]; *opt != '\0'; opt++) {
		    switch (*opt) {
			case 's': debug |= DEBUG_SCHED; break;
			case 'c': debug |= DEBUG_CONN; break;
			case 'p': debug |= DEBUG_PDEV; break;
			case 't': debug |= DEBUG_TCP; break;
			case 'a': debug = ~0; break;
		    }
		}
	    }
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * Xalloc --
 *	Allocate a chunk of memory.
 *
 * Results:
 *	Pointer to the properly-aligned memory.
 *
 * Side Effects:
 *	The program will exit if the allocator gets excited.
 *
 *-----------------------------------------------------------------------
 */
unsigned long *
Xalloc (amount)
    int	    amount;	    /* Amount of space required */
{
    return ((unsigned long *) malloc ((unsigned) amount));
}

/*-
 *-----------------------------------------------------------------------
 * Xrealloc --
 *	Enlarge a chunk of memory. The data will be copied if the
 *	allocated block is too small to contain the new size.
 *
 * Results:
 *	A pointer to the enlarged (and possibly moved) block.
 *
 * Side Effects:
 *	The old block is freed.
 *
 *-----------------------------------------------------------------------
 */
unsigned long *
Xrealloc (oldPtr, amount)
    pointer oldPtr;
    int	    amount;
{
    return (unsigned long *) realloc((char *) oldPtr, (unsigned) amount);
}

/*-
 *-----------------------------------------------------------------------
 * Xfree --
 *	Free a block of storage.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The program will die if the block is already freed or is not
 *	on the heap.
 *
 *-----------------------------------------------------------------------
 */
void
Xfree (ptr)
    pointer ptr;
{
    if (ptr != (pointer)NULL) {
	free((char *) ptr);
    } else {
	ErrorF ("Freeing NULL pointer\n");
    }
}
