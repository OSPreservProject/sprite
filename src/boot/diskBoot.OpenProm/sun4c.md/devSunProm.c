/* 
 * devSunProm.c --
 *
 *	Routines that access the Sun PROM device drivers.
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
static char rcsid[] = "$Header: /sprite/src/boot/sunprom/RCS/devSunProm.c,v 1.2 90/09/17 11:05:25 jhh Exp Locker: rab $ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "stdlib.h"
#include "user/fs.h"
#include "dev.h"
#include "devFsOpTable.h"
#include "machMon.h"
#include "boot.h"
#include "fs.h"

static void *fileId;


/*
 *----------------------------------------------------------------------
 *
 * SunPromDevOpen --
 *
 *	Open the device used for booting.  This depends on the initialization
 *	of the devicePtr->data field done in Dev_Config.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
SunPromDevOpen(devicePtr)
    Fs_Device	*devicePtr;	/* Sprite device description */
{
    char *bootDevName = (char *)devicePtr->data;

    if (romVectorPtr->v_romvec_version >= 2) {
	fileId = (void *)(*romVectorPtr->op_open)(bootDevName);
    } else {
	fileId = (void *)(*romVectorPtr->v_open)(bootDevName);
    }
    if (fileId != 0) {
	return(SUCCESS);
    } else {
	Mach_MonPrintf("v_open(\"%s\") failed\n", bootDevName);
	return(FAILURE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * SunPromDevClose --
 *
 *	Close the device used for booting.
 *
 * Results:
 *	void
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SunPromDevClose()
{
    if (romVectorPtr->v_romvec_version >= 2) {
	(void)(*romVectorPtr->op_close)(fileId);
    } else {
	(void)(*romVectorPtr->v_close)(fileId);
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * SunPromDevRead --
 *
 *	Read from the boot device used for booting.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	The read operation.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
SunPromDevRead(devicePtr, ioPtr, replyPtr)
    Fs_Device	*devicePtr;	/* Sprite device description */
    Fs_IOParam  *ioPtr;
    Fs_IOReply  *replyPtr;
{
    int numBytes, blockNumber, numBlocks, blocksRead;

    blockNumber = ioPtr->offset / DEV_BYTES_PER_SECTOR;
    numBlocks = ioPtr->length / DEV_BYTES_PER_SECTOR;

    if (romVectorPtr->v_romvec_version >= 2) {
	(*romVectorPtr->op_seek)(fileId,
	    0, blockNumber * DEV_BYTES_PER_SECTOR);
	replyPtr->length = (*romVectorPtr->op_read)(fileId,
	    ioPtr->buffer, numBlocks * DEV_BYTES_PER_SECTOR);
    } else {
	blocksRead = (*romVectorPtr->v_read_blocks)(fileId,
	    numBlocks, blockNumber, ioPtr->buffer);

	replyPtr->length = blocksRead * DEV_BYTES_PER_SECTOR;
    }

    if (numBlocks < 0) {
	return(FAILURE);
    } else {
	return(SUCCESS);
    }
}
