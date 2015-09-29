/* 
 * _exit.c --
 *
 *	Procedure to map from Unix _exit system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/emulator/RCS/_exit.c,v 1.2 91/10/04 12:00:02 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <proc.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * _exit --
 *
 *	Procedure to map from Unix _exit system call to Sprite Proc_RawExit.
 *
 * Results:
 *	Never returns.
 *
 * Side effects:
 *	Any open streams are closed, then the process invoking _exit() is
 *	terminated.
 *
 *----------------------------------------------------------------------
 */

void
_exit(exitStatus)
    int exitStatus;		/* process's termination status */
{
    (void)Proc_RawExitStub(SpriteEmu_ServerPort(), exitStatus);
    /*
     * We should never reach this point, regardless of status value.
     */
    thread_terminate(mach_thread_self());
}
