/* 
 * fsIoOps.c --
 *
 *	Routine for initializing the operation switch table entries for 
 *	local files, devices, and pipes.
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

#include <sprite.h>
#include <fs.h>
#include <fspdev.h>
#include <fsconsist.h>
#include <fsio.h>
#include <fsNameOps.h>
#include <fsioDevice.h>
#include <fsioFile.h>
#include <fsioPipe.h>
#include <fsrmt.h>
#include <fsdm.h>

/*
 * The OpenOps are called to do preliminary open-time processing.  They
 * are called after pathname resolution has obtained a local file I/O handle
 * that represents a name of some object.  The NameOpen routine maps
 * the attributes kept with the local file into an objectID (Fs_FileID)
 * and any other information needed to create an I/O stream to the object.
 */
static Fsio_OpenOps ioOpenOps[] = { 
    /*
     * FILE through SYMBOLIC_LINK are all the same.
     */
    { FS_FILE, Fsio_FileNameOpen },
    { FS_DIRECTORY, Fsio_FileNameOpen },
    { FS_SYMBOLIC_LINK, Fsio_FileNameOpen },
    /*
     * Remote devices are opened like ordinary devices, so the old
     * remote-device type is unused.
     */
    { FS_DEVICE, Fsio_DeviceNameOpen },
    /*
     * Local pipes.
     */
    { FS_LOCAL_PIPE, Fsio_NoProc},
    /*
     * Named pipes.  Unimplemented, treat like a regular file.
     */
    { FS_NAMED_PIPE, Fsio_FileNameOpen },
    /*
     * Special file type for testing new kinds of files.
     */
    { FS_XTRA_FILE, Fsio_FileNameOpen},
};
static int numIoOpenOps = sizeof(ioOpenOps)/
				 sizeof(ioOpenOps[0]);
/*
 * Streams for Local files, device, and pipes
 */

static Fsio_StreamTypeOps ioStreamOps[] = {
    /*
     * Top level stream.  This is created by Fs_Open and returned to clients.
     * This in turn references a I/O handle of a different stream type.  The
     * main reason this exists as a handle is so that it can be found
     * during various cases of migration, although the reopen and scavenge
     * entry points in this table are used.
     */
    { FSIO_STREAM, Fsio_NoProc, Fsio_NoProc, Fsio_NoProc,/* open, read, write */
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc, /* pageRead, pageWrite */
		Fsio_NoProc, Fsio_NoProc,	/* iocontrol, select */
		Fsio_NoProc, Fsio_NoProc,	/* getIOAttr, setIOAttr */
		Fsio_NoHandle,			/* clientVerify */
		Fsio_NoProc, Fsio_NoProc,	/* release, migEnd */
		Fsio_NoProc,			/* migrate */
		Fsio_StreamReopen,
		(Boolean (*)())NIL,		/* scavenge */
		Fsio_NullClientKill,
		Fsio_NoProc},			/* close */
    /*
     * Local file stream.  The file is on a local disk and blocks are
     * cached in the block cache.  The GetIOAttr(SetIOAttr) routine
     * is Fsio_NullProc because all the work of getting(setting) cached attributes
     * is already done by FslclGetAttr(FslclSetAttr).
     */
    { FSIO_LCL_FILE_STREAM, Fsio_FileIoOpen, Fsio_FileRead, Fsio_FileWrite,
		Fsio_FileRead, Fsio_FileWrite, Fsio_FileBlockCopy,
				/* Paging routines */
		Fsio_FileIOControl, Fsio_FileSelect,
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		Fsio_NoHandle, Fsio_FileMigClose, Fsio_FileMigOpen, Fsio_FileMigrate,
		Fsio_FileReopen,
		Fsio_FileScavenge, Fsio_FileClientKill, Fsio_FileClose},
    /*
     * Local device stream.  These routines branch to the device driver
     * for the particular device type.
     */
    { FSIO_LCL_DEVICE_STREAM, Fsio_DeviceIoOpen, Fsio_DeviceRead, Fsio_DeviceWrite,
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc,	/* Paging routines */
		Fsio_DeviceIOControl, Fsio_DeviceSelect,
		Fsio_DeviceGetIOAttr, Fsio_DeviceSetIOAttr,
		Fsio_NoHandle,				/* clientVerify */
		Fsio_DeviceMigClose, Fsio_DeviceMigOpen, Fsio_DeviceMigrate,
		Fsrmt_DeviceReopen,
		Fsio_DeviceScavenge, Fsio_DeviceClientKill, Fsio_DeviceClose},
    /*
     * Local anonymous pipe stream.  
     */
    { FSIO_LCL_PIPE_STREAM, Fsio_NoProc, Fsio_PipeRead, Fsio_PipeWrite,
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc,	/* Paging routines */
		Fsio_PipeIOControl, Fsio_PipeSelect,
		Fsio_PipeGetIOAttr, Fsio_PipeSetIOAttr,
		Fsio_NoHandle,				/* clientVerify */
		Fsio_PipeMigClose, Fsio_PipeMigOpen, Fsio_PipeMigrate,
		Fsio_PipeReopen,
		Fsio_PipeScavenge, Fsio_PipeClientKill, Fsio_PipeClose},

};

static int numIoStreamOps = sizeof(ioStreamOps)/
				 sizeof(ioStreamOps[0]);

/*
 *----------------------------------------------------------------------
 *
 * Fsio_InitializeOps --
 *
 *	Initialize the operation switch tables for the NameOpen
 *	routines and the StreamOps for local objects.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds entries to the fsio_OpenOpTable and fsio_StreamOpTable.
 *
 *----------------------------------------------------------------------
 */

void
Fsio_InitializeOps()
{
    int	i;

    for (i = 0; i < numIoStreamOps; i++)  { 
	Fsio_InstallStreamOps(ioStreamOps[i].type, &(ioStreamOps[i]));
    }
    for (i = 0; i < numIoOpenOps; i++)  { 
	Fsio_InstallSrvOpenOp(ioOpenOps[i].type, &(ioOpenOps[i]));
    }

}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_Bin() --
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
Fsio_Bin()
{
    Mem_Bin(sizeof(Fsio_FileIOHandle));
    Mem_Bin(sizeof(Fsdm_FileDescriptor));
}

