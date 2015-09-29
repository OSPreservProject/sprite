/* 
 * mach.c --
 *
 *	This file contains machine-dependent information needed by
 *	the ps program.
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
static char rcsid[] = "$Header: /sprite/src/cmds/ps/sun4.md/RCS/mach.c,v 1.3 90/09/24 14:39:54 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sys.h"
#include "sprite.h"

/*
 *----------------------------------------------------------------------
 *
 * getTicksPerSecond --
 *
 *	Return one second's worth of timer ticks for the current machine.
 *      This is needed by ps to deal with weighted usages.
 *
 * Results:
 *	One second's worth of timer ticks.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getTicksPerSecond()
{
    Sys_MachineInfo machType;

    (void) Sys_GetMachineInfo (sizeof (machType), &machType);

    return ((machType.type & SYS_SUN_ARCH_MASK) == SYS_SUN_4_C) ? 1000000 : 1000;
}
