/* 
 * fsRmtOps.c --
 *
 *	Routine for initializing the fsOpTable switch entries for remote
 *	domain naming and remote file/pipe/device access.
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

#include "sprite.h"
#include "fs.h"
#include "fsrmt.h"
#include "fsio.h"
#include "fsioFile.h"
#include "fsioDevice.h"
#include "fsioPipe.h"
#include "fsNameOps.h"
#include "fsrmtInt.h"

/*
 * fs_DomainLookup for FS_REMOTE_SPRITE_DOMAIN type.
 */

static ReturnStatus (*rmtDomainLookup[FS_NUM_NAME_OPS])() = {
     FsrmtImport, Fsio_NoProc, FsrmtOpen, FsrmtGetAttrPath,
     FsrmtSetAttrPath, FsrmtMakeDevice, FsrmtMakeDir, 
     FsrmtRemove, FsrmtRemoveDir, FsrmtRename, FsrmtHardLink
};

/*
 * Domain specific get/set attributes table.  These routines are used
 * to get/set attributes on the name server given a fileID (not a pathname).
 */
static Fs_AttrOps rmtAttrOpTable =   { FsrmtGetAttr, FsrmtSetAttr };


/*
 * File stream type ops for FSIO_RMT_FILE_STREAM, FSIO_RMT_DEVICE_STREAM,
 * and FSIO_RMT_PIPE_STREAM;
 */

static Fsio_StreamTypeOps rmtFileStreamOps[] = {
/*
 * Remote file stream.  The file is at a remote server but blocks might 
 * be cached in the block cache.
 */
    { FSIO_RMT_FILE_STREAM, FsrmtFileIoOpen, FsrmtFileRead, FsrmtFileWrite,
		FsrmtFileIOControl, Fsio_FileSelect,
		FsrmtFileGetIOAttr, FsrmtFileSetIOAttr,
		FsrmtFileVerify, FsrmtFileMigClose, FsrmtFileMigOpen,
		FsrmtFileMigrate, FsrmtFileReopen,
		FsrmtFileScavenge,
		Fsio_NullClientKill, FsrmtFileClose},
/*
 * Remote device stream.  Forward the operations to the remote I/O server.
 */
    { FSIO_RMT_DEVICE_STREAM, FsrmtDeviceIoOpen, Fsrmt_Read, Fsrmt_Write,
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FsrmtDeviceVerify, Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FsrmtDeviceMigrate, FsrmtDeviceReopen,
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill, Fsrmt_IOClose},
 /*
  * Remote anonymous pipe stream.  These arise because of migration.
  */
    { FSIO_RMT_PIPE_STREAM, Fsio_NoProc, Fsrmt_Read, Fsrmt_Write,
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FsrmtPipeVerify, Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FsrmtPipeMigrate, FsrmtPipeReopen,
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill, Fsrmt_IOClose},
};

static int numRmtFileStreamOps = sizeof(rmtFileStreamOps)/
				 sizeof(rmtFileStreamOps[0]);

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_InitializeOps --
 *
 *	Initialize the fsOpTable switch for the remote domain naming 
 *	and remote domain streams.
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
Fsrmt_InitializeOps()
{
    int	i;

    Fs_InstallDomainLookupOps(FS_REMOTE_SPRITE_DOMAIN, rmtDomainLookup, 
				&rmtAttrOpTable );
    for (i = 0; i < numRmtFileStreamOps; i++)  { 
	Fsio_InstallStreamOps(rmtFileStreamOps[i].type, &(rmtFileStreamOps[i]));
    }

}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_Bin() --
 *
 *	Setup objects to be binned.
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
Fsrmt_Bin()
{
    Mem_Bin(sizeof(Fsrmt_FileIOHandle));
}


