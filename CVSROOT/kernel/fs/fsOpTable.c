/* 
 * fsOpTable.c --
 *
 *	The operation tables for the file system.  They are encountered
 *	by the system in roughly the order they are presented here.  First
 *	the Domain Lookup routines are used for name operations.  The
 *	are used by FsLookupOperation and FsTwoNameOperation which use
 *	the prefix table to choose a server.  If a stream is to be made
 *	the the Open operations are used.  Next comes the Stream operations,
 *	and lastly come the Device operations (WHICH SHOULD MOVE TO dev.o).
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "fsDisk.h"

/*
 * Domain specific include files.
 */
#include "fsSpriteDomain.h"
#include "fsLocalDomain.h"

/*
 * Stream specific include files.
 */
#include "fsStream.h"
#include "fsFile.h"
#include "fsDevice.h"
#include "fsPipe.h"
#include "fsNamedPipe.h"
#include "fsPdev.h"

/*
 * Device specific include files.
 */

#include "devConsole.h"
#include "devSCSIDisk.h"
#include "devSCSITape.h"
#include "devXylogicsDisk.h"
#include "net.h"

static ReturnStatus NullProc();
static void	    VoidProc();
static ReturnStatus NoProc();
static ReturnStatus NoDevice();
static FsHandleHeader *NoHandle();
static ReturnStatus NullSelectProc();

/*
 * Domain specific routine table for lookup operations:
 *	DomainPrefix
 *	DomainOpen
 *	DomainGetAttrPath
 *	DomainSetAttrPath
 *	DomainMakeDevice
 *	DomainMakeDir
 *	DomainRemove
 *	DomainRemoveDir
 *	DomainRename
 *	DomainHardLink
 */

/*
 * THIS ARRAY INDEXED BY DOMAIN TYPE.  Do not arbitrarily insert entries.
 */
ReturnStatus (*fsDomainLookup[FS_NUM_DOMAINS][FS_NUM_NAME_OPS])() = {
/* FS_LOCAL_DOMAIN */
    {FsLocalPrefix, FsLocalOpen, FsLocalGetAttrPath, FsLocalSetAttrPath, 
     FsLocalMakeDevice, FsLocalMakeDir,
     FsLocalRemove, FsLocalRemoveDir, FsLocalRename, FsLocalHardLink},
/* FS_REMOTE_SPRITE_DOMAIN */
    {FsSpritePrefix, FsSpriteOpen, FsSpriteGetAttrPath, FsSpriteSetAttrPath,
     FsSpriteMakeDevice, FsSpriteMakeDir, 
     FsSpriteRemove, FsSpriteRemoveDir, FsSpriteRename, FsSpriteHardLink},
/* FS_REMOTE_UNIX_DOMAIN */
#ifdef unix_domain
    {FsUnixPrefix, FsUnixOpen, FsUnixGetAttrPath, FsUnixSetAttrPath,
     FsUnixMakeDevice, FsUnixMakeDir, 
     FsUnixRemove, FsUnixRemoveDir, FsUnixRename, FsUnixHardLink},
#else
    {NoProc, NullProc, NullProc, NullProc, NullProc, NullProc, NullProc,
     NullProc, NullProc, NullProc},
#endif
/* FS_NFS_DOMAIN */
    {NoProc, NullProc, NullProc, NullProc, NullProc, NullProc, NullProc,
     NullProc, NullProc, NullProc},
};


/*
 * File type server open routine table:
 *
 * THIS ARRAY INDEXED BY FILE TYPE.  Do not arbitrarily insert entries.
 */
FsOpenOps fsOpenOpTable[] = {
    /*
     * FILE through REMOTE_LINK are all the same.
     */
    { FS_FILE, FsFileSrvOpen },
    { FS_DIRECTORY, FsFileSrvOpen },
    { FS_SYMBOLIC_LINK, FsFileSrvOpen },
    { FS_REMOTE_LINK, FsFileSrvOpen },
    /*
     * Remote devices are opened like ordinary devices, so the old
     * remote-device type is unused.
     */
    { FS_DEVICE, FsDeviceSrvOpen },
    { 0, NullProc },
#ifdef not_done_yet
    /*
     * Local pipes.
     */
    { FS_LOCAL_PIPE, NoProc},
    /*
     * Named pipes.
     */
    { FS_NAMED_PIPE, FsNamedPipeSrvOpen },
    /*
     * Pseudo devices.
     */
    { FS_PSEUDO_DEV, FsPseudoDevSrvOpen},
    { FS_PSEUDO_FS, NullProc},
    /*
     * Special file type for testing new kinds of files.
     */
    { FS_XTRA_FILE, NullProc},
#endif
};
/*
 * Domain specific get/set attributes table.  These routines are used
 * to get/set attributes on the name server given a fileID (not a pathname).
 */
FsAttrOps fsAttrOpTable[FS_NUM_DOMAINS] = {
/* FS_LOCAL_DOMAIN */
    { FsLocalGetAttr, FsLocalSetAttr },
/* FS_REMOTE_SPRITE_DOMAIN */
    { FsSpriteGetAttr, FsSpriteSetAttr },
/* FS_REMOTE_UNIX_DOMAIN */
    { NoProc, NoProc },
/* FS_NFS_DOMAIN */
    { NoProc, NoProc },
};


/*
 * Stream type specific routine table.  See fsOpTable.h for an explaination
 *	of the calling sequence for each routine.
 *
 * THIS ARRAY INDEXED BY STREAM TYPE.  Do not arbitrarily insert entries.
 */
FsStreamTypeOps fsStreamOpTable[] = {
    /*
     * Top level stream.  This is created by Fs_Open and returned to clients.
     * This in turn references a I/O handle of a different stream type.  The
     * main reason this exists as a handle is so that it can be found
     * during various cases of migration, although the reopen and scavenge
     * entry points in this table are used.
     */
    { FS_STREAM, NoProc, NoProc, NoProc,	/* cltOpen, read, write */
		NoProc, NoProc,			/* iocontrol, select */
		NoProc, NoProc,			/* getIOAttr, setIOAttr */
		NoHandle,			/* clientVerify */
		NoProc, NoProc, NoProc,		/* migStart, migEnd, migrate */
		FsStreamReopen,	NoProc,		/* reopen, blockAlloc */
		NoProc, NoProc,	NoProc,		/* blkRead, blkWrite, blkCopy */
		FsStreamScavenge, VoidProc,	/* scavenge, kill */
		NoProc},			/* close */
    /*
     * Local file stream.  The file is on a local disk and blocks are
     * cached in the block cache.  The GetIOAttr(SetIOAttr) routine
     * is NullProc because all the work of getting(setting) cached attributes
     * is already done by FsLocalGetAttr(FsLocalSetAttr).
     */
    { FS_LCL_FILE_STREAM, FsFileCltOpen, FsFileRead, FsFileWrite,
		FsFileIOControl, FsFileSelect,
		NullProc, NullProc,		/* Get/Set IO Attr */
		NoHandle, FsFileMigStart, FsFileMigEnd, FsFileMigrate,
		FsFileReopen,
		FsFileBlockAllocate, FsFileBlockRead, FsFileBlockWrite,
		FsFileBlockCopy, FsFileScavenge, FsFileClientKill, FsFileClose},
    /*
     * Remote file stream.  The file is at a remote server but blocks might
     * be cached in the block cache.
     */
    { FS_RMT_FILE_STREAM, FsRmtFileCltOpen, FsRmtFileRead, FsRmtFileWrite,
		FsRmtFileIOControl, FsFileSelect,
		FsRmtFileGetIOAttr, FsRmtFileSetIOAttr,
		FsRmtFileVerify, FsRmtFileMigStart, FsRmtFileMigEnd,
		FsRmtFileMigrate, FsRmtFileReopen,
		FsRmtFileAllocate, FsRmtFileBlockRead, FsRmtFileBlockWrite,
		FsSpriteBlockCopy, FsRmtFileScavenge, VoidProc, FsRmtFileClose},
    /*
     * Local device stream.  These routines branch to the device driver
     * for the particular device type.
     */
    { FS_LCL_DEVICE_STREAM, FsDeviceCltOpen, FsDeviceRead, FsDeviceWrite,
		FsDeviceIOControl, FsDeviceSelect,
		FsDeviceGetIOAttr, FsDeviceSetIOAttr,
		NoHandle,				/* clientVerify */
		FsDeviceMigStart, FsDeviceMigEnd, FsDeviceMigrate,
		NullProc,				/* reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsDeviceScavenge, FsDeviceClientKill, FsDeviceClose},
    /*
     * Remote device stream.  Forward the operations to the remote I/O server.
     */
    { FS_RMT_DEVICE_STREAM, FsRmtDeviceCltOpen, FsSpriteRead, FsSpriteWrite,
		FsRemoteIOControl, FsSpriteSelect,
		FsSpriteGetIOAttr, FsSpriteSetIOAttr,
		FsRmtDeviceVerify, FsRemoteIOMigStart, FsRemoteIOMigEnd,
		FsRmtDeviceMigrate, FsRmtDeviceReopen,
		NoProc, NoProc, NoProc, NoProc,
		FsRemoteHandleScavenge, VoidProc, FsRemoteIOClose},
    /*
     * Local anonymous pipe stream.  
     */
    { FS_LCL_PIPE_STREAM, NoProc, FsPipeRead, FsPipeWrite,
		FsPipeIOControl, FsPipeSelect,
		FsPipeGetIOAttr, FsPipeSetIOAttr,
		NoHandle,				/* clientVerify */
		FsPipeMigStart, FsPipeMigEnd, FsPipeMigrate,
		FsPipeReopen,
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsPipeScavenge, FsPipeClientKill, FsPipeClose},
    /*
     * Remote anonymous pipe stream.  These arise because of migration.
     */
    { FS_RMT_PIPE_STREAM, NoProc, FsSpriteRead, FsSpriteWrite,
		FsRemoteIOControl, FsSpriteSelect,
		FsSpriteGetIOAttr, FsSpriteSetIOAttr,
		FsRmtPipeVerify, FsRemoteIOMigStart, FsRemoteIOMigEnd,
		FsRmtPipeMigrate, FsRmtPipeReopen,
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsRemoteHandleScavenge, VoidProc, FsRemoteIOClose},
#ifdef not_done_yet
    /*
     * Locally cached named pipe stream.  
     */
    { FS_LCL_NAMED_PIPE_STREAM, FsNamedPipeCltOpen, FsNamedPipeRead,
		FsNamedPipeWrite, FsNamedPipeIOControl,
		FsNamedPipeSelect, FsNamedPipeGetIOAttr, FsNamedPipeSetIOAttr,
		NoHandle, FsNamedPipeEncap, FsNamedPipeDeencap,
		FsNamedPipeAllocate, FsNamedPipeBlockRead,
		FsNamedPipeBlockWrite, FsNamedPipeBlockCopy,
		FsNamedPipeClose, FsNamedPipeDelete },
    /*
     * Remotely cached name pipe stream.  There is little to do on this
     * machine so we use the remote device routines to forward operations
     * to the I/O server.
     */
    { FS_RMT_NAMED_PIPE_STREAM, FsRmtNamedPipeCltOpen, FsSpriteRead,
		FsSpriteWrite, FsSpriteIOControl,
		FsSpriteSelect, FsSpriteGetIOAttr, FsSpriteSetIOAttr,
		FsRmtNamedPipeVerify, FsRmtDeviceEncap, FsRmtDeviceDeencap,
		NoProc, NoProc, NoProc, NoProc,
		FsRmtDeviceClose, FsRmtDeviceDelete},
    /*
     * A control stream is what a server program gets when it opens a
     * pseudo device file.  This stream is used to notify the server
     * of new clients; the ID of the server stream set up for each
     * new client is passed over this control stream.
     */
    { FS_CONTROL_STREAM, FsControlCltOpen, FsControlRead, NoProc,
		FsControlIOControl, FsControlSelect,
		FsControlGetIOAttr, FsControlSetIOAttr,
		NoHandle, FsControlEncap, FsControlDeencap,
		NoProc, NoProc, NoProc, NoProc,
		FsControlClose, FsControlDelete },
    /*
     * A server stream gets set up for the server whenever a client opens
     * a pseudo device.  The server reads the stream the learn about new
     * requests from the client.  IOControls on the stream are used
     * to control the connection to the client.
     */
    { FS_SERVER_STREAM, FsServerStreamCltOpen, FsServerStreamRead, NoProc,
		FsServerStreamIOControl, FsServerStreamSelect,
		FsServerStreamGetIOAttr, FsServerStreamSetIOAttr,
		NoHandle, FsServerStreamEncap, FsServerStreamDeencap,
		NoProc, NoProc, NoProc, NoProc,
		FsServerStreamClose, FsServerStreamDelete },
    /*
     * A pseudo stream with the server process running locally.  
     */
    { FS_LCL_PSEUDO_STREAM, FsPseudoStreamCltOpen, FsPseudoStreamRead,
		FsPseudoStreamWrite, FsPseudoStreamIOControl,
		FsPseudoStreamSelect,
		FsPseudoStreamGetIOAttr, FsPseudoStreamSetIOAttr,
		NoHandle, FsPseudoStreamEncap, FsPseudoStreamDeencap,
		NoProc, NoProc, NoProc, NoProc,
		FsPseudoStreamClose, FsPseudoStreamDelete},
    /*
     * A pseudo stream with a remote server.  
     */
    { FS_RMT_PSEUDO_STREAM, FsRmtPseudoStreamCltOpen, FsRmtPseudoStreamRead,
		FsRmtPseudoStreamWrite,
		FsRmtPseudoStreamIOControl, FsRmtPseudoStreamSelect,
		FsRmtPseudoStreamGetIOAttr, FsRmtPseudoStreamSetIOAttr,
		FsRmtPseudoStreamVerify,
		FsRmtPseudoStreamEncap, FsRmtPseudoStreamDeencap,
		NoProc, NoProc, NoProc, NoProc,
		FsRmtPseudoStreamClose, FsRmtPseudoStreamDelete},
    /*
     * A stream to the hybrid Sprite/Unix server.  
     */
    { FS_RMT_UNIX_STREAM, FsRmtUnixCltOpen, FsRmtUnixRead, FsRmtUnixWrite,
		FsRmtUnixIOControl, FsRmtUnixSelect,
		NullProc, NullProc,
		NoHandle, NoProc, NoProc,
		FsRmtUnixAllocate, FsRmtUnixBlockRead,
		FsRmtUnixBlockWrite, FsRmtUnixBlockCopy,
		FsRmtUnixClose, FsRmtUnixDelete},
    /*
     * A stream to a remote NFS file.  
     */
    { FS_RMT_NFS_STREAM, NoProc, NoProc, NoProc,
		NoProc, NoProc,
		NoProc, NoProc,
		NoHandle, NoProc, NoProc,
		NoProc, NoProc,
		NoProc, NoProc,
		NoProc, NoProc},
#endif not_done_yet
};

/*
 * Device type specific routine table:
 *	This is for the file-like operations as they apply to devices.
 *	DeviceOpen
 *	DeviceRead
 *	DeviceWrite
 *	DeviceIOControl
 *	DeviceClose
 *	DeviceSelect
 */

extern ReturnStatus Dev_NullRead();
extern ReturnStatus Dev_NullWrite();

FsDeviceTypeOps fsDeviceOpTable[] = {
    /*
     * The console.  The workstation's display and keyboard.
     */
    {FS_DEV_CONSOLE, Dev_ConsoleOpen, Dev_ConsoleRead, Dev_ConsoleWrite,
		     Dev_ConsoleIOControl, Dev_ConsoleClose, Dev_ConsoleSelect},
    /*
     * The system error log.  If this is not open then error messages go
     * to the console.
     */
    {FS_DEV_SYSLOG, Dev_SyslogOpen, Dev_SyslogRead, Dev_SyslogWrite,
		    Dev_SyslogIOControl, Dev_SyslogClose, Dev_SyslogSelect},
    /*
     * The following device numbers are unused.
     */
    {FS_DEV_KEYBOARD, NoDevice,NullProc,NullProc, NullProc, NullProc, NullProc},
    {FS_DEV_PLACEHOLDER_2, NoDevice, NullProc, NullProc,
		    NullProc, NullProc, NullProc},
    /*
     * SCSI Disk interface.
     */
    {FS_DEV_SCSI_DISK, Dev_SCSIDiskOpen, Dev_SCSIDiskRead, Dev_SCSIDiskWrite,
		    Dev_SCSIDiskIOControl, Dev_SCSIDiskClose, NullProc},
    /*
     * SCSI Tape interface.
     */
    {FS_DEV_SCSI_TAPE, Dev_SCSITapeOpen, Dev_SCSITapeRead, Dev_SCSITapeWrite,
		    Dev_SCSITapeIOControl, Dev_SCSITapeClose, NullProc},
    /*
     * /dev/null
     */
    {FS_DEV_MEMORY, NullProc, Dev_NullRead, Dev_NullWrite,
		    NullProc, NullProc, NullSelectProc},
    /*
     * Xylogics 450 disk controller.
     */
    {FS_DEV_XYLOGICS, Dev_XylogicsDiskOpen, Dev_XylogicsDiskRead,
		    Dev_XylogicsDiskWrite, Dev_XylogicsDiskIOControl,
		    Dev_XylogicsDiskClose, NullProc},
    /*
     * Network devices.  The unit number specifies the ethernet protocol number.
     */
    {FS_DEV_NET, Net_FsOpen, Net_FsRead, Net_FsWrite, Net_FsIOControl,
		    Net_FsClose, Net_FsSelect},
};

/*
 * Device Block I/O operation table.  This table is sparse because not
 * all devices support block I/O.
 *	FsBlockIOInit
 *	FsBlockIO
 */
FsBlockOps fsBlockOpTable[] = {
    { FS_DEV_CONSOLE, 0 },
    { FS_DEV_SYSLOG, 0 },
    { FS_DEV_KEYBOARD, 0 },
    { FS_DEV_PLACEHOLDER_2, 0 },
    { FS_DEV_SCSI_DISK, Dev_SCSIDiskBlockIO },
    { FS_DEV_SCSI_TAPE, 0 },
    { FS_DEV_MEMORY, 0 },
    { FS_DEV_XYLOGICS, Dev_XylogicsDiskBlockIO },
    { FS_DEV_NET, 0 },
};

static ReturnStatus
NullProc()
{
    return(SUCCESS);
}

static void
VoidProc()
{
    return;
}

static ReturnStatus
NoProc()
{
    return(FAILURE);
}

static ReturnStatus
NoDevice()
{
    return(FS_INVALID_ARG);
}

static FsHandleHeader *
NoHandle()
{
    return((FsHandleHeader *)NIL);
}

/*ARGSUSED*/
static ReturnStatus
NullSelectProc(devicePtr, inFlags, outFlagsPtr)
    Fs_Device	*devicePtr;	/* Ignored. */
    int		inFlags;	/* FS_READBLE, FS_WRITABLE, FS_EXCEPTION. */
    int		*outFlagsPtr;	/* Copy of inFlags. */
{
    *outFlagsPtr = inFlags;
    return(SUCCESS);
}

