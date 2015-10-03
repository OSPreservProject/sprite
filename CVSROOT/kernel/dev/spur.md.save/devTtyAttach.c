/* 
 * devTtyAttach.c --
 *
 *	This file provides glue routines between the device-independent
 *	tty driver and specific device interfaces.  Since SPUR has no
 *	terminals right now, this file just contains dummy procedures.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "ttyAttach.h"

DevTty *
DevTtyAttach(unit)
    int unit;
{
    return NULL;
}

/*
 * Stuff below used in sys:  should go away.
 */

typedef struct {
    int numInterrupts;          /* Total number of interrupts */
    int numMouseChars;          /* Number of characters due to mouse */
    int numKbdChars;            /* Number of characters due to keyboard */
    int numSerAChars;           /* Number of characters due to serial A port */
    int numSerBChars;           /* Number of characters due to serial B port */
    int mouseCharsLost;         /* Number of mouse characters lost */
    int kbdCharsLost;           /* Number of keyboard characters lost */
    int numBadSyncs;            /* Number of stray mouse chars (no sync) */
    Time mostRecentInterrupt;   /* Last time an interrupt was received */
} Dev_KbdInstrument;

Dev_KbdInstrument dev_KbdInstrument;
