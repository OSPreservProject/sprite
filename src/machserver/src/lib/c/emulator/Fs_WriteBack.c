/* 
 * Fs_WriteBack.c --
 *
 *	Source code for the Fs_WriteBack library procedure.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/Fs_WriteBack.c,v 1.3 92/03/12 19:26:05 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>
#include <spriteEmuInt.h>


/*
 *----------------------------------------------------------------------
 *
 * Fs_WriteBack --
 *
 *	Force the given file to disk.
 *
 * Results:
 *	SUCCESS if could open and force to disk, an error otherwise.
 *
 * Side effects:
 *	File forced to disk.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_WriteBack(fileName, firstByte, lastByte, shouldBlock)
    char	*fileName;		/* Name of file to write back. */
    int		firstByte;		/* First byte to write back, -1 if 
					 * should write-back the lowest first
					 * byte */
    int		lastByte;		/* Last byte to write back, -1 if 
					 * should write-back the highest last
					 * byte */
    Boolean	shouldBlock;		/* TRUE => should wait for the file
					 * to be put on disk. */
{
    int			fd;
    ReturnStatus	status;
    kern_return_t	kernStatus;
    Boolean sigPending;

    status = Fs_Open(fileName, FS_READ, 0, &fd);
    if (status != SUCCESS) {
	return(status);
    }
    kernStatus = Fs_FileWriteBackStub(SpriteEmu_ServerPort(), fd,
				      firstByte, lastByte, shouldBlock,
				      &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }

    Fs_Close(fd);
    return(status);
}
