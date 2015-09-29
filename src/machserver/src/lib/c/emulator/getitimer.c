/* 
 * getitimer.c --
 *
 *	UNIX getitimer() and setitimer() for the Sprite server.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/getitimer.c,v 1.1 92/03/13 20:39:39 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <sys/time.h>


/*
 *----------------------------------------------------------------------
 *
 * getitimer --
 *
 *	Map the UNIX getitimer call to the Sprite Proc_GetIntervalTimer 
 *	request.
 *
 * Results:
 *	UNIX_SUCCESS if the Sprite return returns SUCCESS.
 *	Otherwise, UNIX_ERROR and errno is set to the Unix equivalent
 *	status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getitimer(which, value)
    int which;
    struct itimerval *value;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;

    /*
     * The Sprite and Unix timer values have the same layout.
     */
    kernStatus = Proc_GetIntervalTimerStub(SpriteEmu_ServerPort(), which,
					   &status,
					   (Proc_TimerInterval *) value,
					   &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * setitimer --
 *
 *	map from UNIX setitimer to Sprite Proc_SetIntervalTimer..
 *
 * Results:
 *	UNIX_SUCCESS if the Sprite return returns SUCCESS.
 *	Otherwise, UNIX_ERROR and errno is set to the Unix equivalent
 *	status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
setitimer(which, value, ovalue)
    int which;
    struct itimerval *value;
    struct itimerval *ovalue;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;
    struct itimerval dummyVal;	/* if the user doesn't want the old value */

    if (ovalue == (struct itimerval *)NULL) {
	ovalue = &dummyVal;
    }

    /*
     * The Sprite and Unix timer values have the same layout.
     */
    kernStatus = Proc_SetIntervalTimerStub(SpriteEmu_ServerPort(), which,
					   *(Proc_TimerInterval *) value,
					   &status, 
					   (Proc_TimerInterval *) ovalue,
					   &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}

