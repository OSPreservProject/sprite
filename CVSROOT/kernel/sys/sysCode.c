/* 
 * sysCode.c --
 *
 *	Miscellaneous routines for the system.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "dbg.h"
#include "sys.h"
#include "rpc.h"
#include "sync.h"
#include "sched.h"
#include "proc.h"
#include "vm.h"



/*
 * ----------------------------------------------------------------------------
 *
 * Sys_Init --
 *
 *	Initializes system-dependent data structures.
 *
 *	The number of calls to disable interrupts is set to 1 for 
 *	each processor, since Sys_Init is assumed to be called with 
 *	interrupts off and to be followed with an explicit call to 
 *	enable interrupts.
 *
 *	Until ENABLE_INTR() is called without a prior DISABLE_INTR() (i.e.,
 *	when it is called outside the context of a MASTER_UNLOCK), interrupts
 *	will remain disabled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	For each processor, the number of disable interrupt calls outstanding
 *	is initialized.  
 *
 * ----------------------------------------------------------------------------
 */

void 
Sys_Init()
{
    SysInitSysCall();
}

/*
 *----------------------------------------------------------------------
 *
 * Sys_GetHostId --
 *
 *	This returns the Sprite Host Id for the system.  This Id is
 *	guaranteed to be unique accross all Sprite Hosts participating
 *	in the system.  This is plucked from the RPC system now,
 *	but perhaps should be determined from the filesystem.
 *
 * Results:
 *	The Sprite Host Id.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Sys_GetHostId()
{
    return(rpc_SpriteID);
}
