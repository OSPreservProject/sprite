/* 
 * fsOpTable.c --
 *
 *	The operation tables for the file system.  They are encountered
 *	by the system in roughly the order they are presented here.  First
 *	the Domain Lookup routines are used for name operations.  They
 *	are used by FsLookupOperation and FsTwoNameOperation which use
 *	the prefix table to choose a server.  If a stream is to be made
 *	then the Open operations are used.  Next comes the Stream operations.
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

static ReturnStatus NullProc();
static void	    NullClientKill();
static ReturnStatus NoProc();
static FsHandleHeader *NoHandle();

/*
 * Domain specific routine table for lookup operations.
 * The following operate on a single pathname.  They are called via
 *	FsLookupOperation with arguments described in fsOpTable.h
 *	DomainImport
 *	DomainExport
 *	DomainOpen
 *	DomainGetAttrPath
 *	DomainSetAttrPath
 *	DomainMakeDevice
 *	DomainMakeDir
 *	DomainRemove
 *	DomainRemoveDir
 * The following operate on two pathnames.
 *	DomainRename
 *	DomainHardLink
 */

/*
 * THIS ARRAY INDEXED BY DOMAIN TYPE.  Do not arbitrarily insert entries.
 */
ReturnStatus (*fsDomainLookup[FS_NUM_DOMAINS][FS_NUM_NAME_OPS])() = {
/* FS_LOCAL_DOMAIN */
    {NoProc, FsLocalExport, FsLocalOpen, FsLocalGetAttrPath,
     FsLocalSetAttrPath, FsLocalMakeDevice, FsLocalMakeDir,
     FsLocalRemove, FsLocalRemoveDir, FsLocalRename, FsLocalHardLink},
/* FS_REMOTE_SPRITE_DOMAIN */
    {FsSpriteImport, NoProc, FsSpriteOpen, FsRemoteGetAttrPath,
     FsRemoteSetAttrPath, FsSpriteMakeDevice, FsSpriteMakeDir, 
     FsSpriteRemove, FsSpriteRemoveDir, FsSpriteRename, FsSpriteHardLink},
/* FS_PSEUDO_DOMAIN */
    {NoProc, FsPfsExport, FsPfsOpen, FsPfsGetAttrPath, FsPfsSetAttrPath,
     FsPfsMakeDevice, FsPfsMakeDir, FsPfsRemove, FsPfsRemoveDir,
     FsPfsRename, FsPfsHardLink},
/* FS_NFS_DOMAIN */
    {NoProc, NoProc, NullProc, NullProc, NullProc, NullProc, NullProc, NullProc,
     NullProc, NullProc, NullProc},
};


/*
 * File type server open routine table:
 *
 * THIS ARRAY INDEXED BY FILE TYPE.  Do not arbitrarily insert entries.
 */
FsOpenOps fsOpenOpTable[] = {
    /*
     * FILE through SYMBOLIC_LINK are all the same.
     */
    { FS_FILE, FsFileSrvOpen },
    { FS_DIRECTORY, FsFileSrvOpen },
    { FS_SYMBOLIC_LINK, FsFileSrvOpen },
    /*
     * A remote link can either be treated like a regular file,
     * or opened by a pseudo-filesystem server.
     */
    { FS_REMOTE_LINK, FsRmtLinkSrvOpen },
    /*
     * Remote devices are opened like ordinary devices, so the old
     * remote-device type is unused.
     */
    { FS_DEVICE, FsDeviceSrvOpen },
    { 0, NullProc },
    /*
     * Local pipes.
     */
    { FS_LOCAL_PIPE, NoProc},
    /*
     * Named pipes.  Unimplemented, treat like a regular file.
     */
    { FS_NAMED_PIPE, FsFileSrvOpen },
    /*
     * Pseudo devices.
     */
    { FS_PSEUDO_DEV, FsPseudoDevSrvOpen},
    { FS_PSEUDO_FS, NoProc},
    /*
     * Special file type for testing new kinds of files.
     */
    { FS_XTRA_FILE, FsFileSrvOpen},
};
/*
 * Domain specific get/set attributes table.  These routines are used
 * to get/set attributes on the name server given a fileID (not a pathname).
 */
FsAttrOps fsAttrOpTable[FS_NUM_DOMAINS] = {
/* FS_LOCAL_DOMAIN */
    { FsLocalGetAttr, FsLocalSetAttr },
/* FS_REMOTE_SPRITE_DOMAIN */
    { FsRemoteGetAttr, FsRemoteSetAttr },
/* FS_PSEUDO_DOMAIN */
    { FsPseudoGetAttr, FsPseudoSetAttr },
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
		NoProc, NoProc,			/* migStart, migEnd */
		FsStreamMigrate,		/* Used to tell source of mig.
						 * to release stream */
		FsStreamReopen,	NoProc,		/* reopen, blockAlloc */
		NoProc, NoProc,	NoProc,		/* blkRead, blkWrite, blkCopy */
		FsStreamScavenge, NullClientKill,/* scavenge, kill */
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
		NoHandle, FsFileRelease, FsFileMigEnd, FsFileMigrate,
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
		FsRmtFileVerify, FsRmtFileRelease, FsRmtFileMigEnd,
		FsRmtFileMigrate, FsRmtFileReopen,
		FsRmtFileAllocate, FsRmtFileBlockRead, FsRmtFileBlockWrite,
		FsRemoteBlockCopy, FsRmtFileScavenge,
		NullClientKill, FsRmtFileClose},
    /*
     * Local device stream.  These routines branch to the device driver
     * for the particular device type.
     */
    { FS_LCL_DEVICE_STREAM, FsDeviceCltOpen, FsDeviceRead, FsDeviceWrite,
		FsDeviceIOControl, FsDeviceSelect,
		FsDeviceGetIOAttr, FsDeviceSetIOAttr,
		NoHandle,				/* clientVerify */
		FsDeviceRelease, FsDeviceMigEnd, FsDeviceMigrate,
		NullProc,				/* reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsDeviceScavenge, FsDeviceClientKill, FsDeviceClose},
    /*
     * Remote device stream.  Forward the operations to the remote I/O server.
     */
    { FS_RMT_DEVICE_STREAM, FsRmtDeviceCltOpen, FsRemoteRead, FsRemoteWrite,
		FsRemoteIOControl, FsRemoteSelect,
		FsRemoteGetIOAttr, FsRemoteSetIOAttr,
		FsRmtDeviceVerify, FsRemoteIORelease, FsRemoteIOMigEnd,
		FsRmtDeviceMigrate, FsRmtDeviceReopen,
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsRemoteHandleScavenge, NullClientKill, FsRemoteIOClose},
    /*
     * Local anonymous pipe stream.  
     */
    { FS_LCL_PIPE_STREAM, NoProc, FsPipeRead, FsPipeWrite,
		FsPipeIOControl, FsPipeSelect,
		FsPipeGetIOAttr, FsPipeSetIOAttr,
		NoHandle,				/* clientVerify */
		FsPipeRelease, FsPipeMigEnd, FsPipeMigrate,
		FsPipeReopen,
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsPipeScavenge, FsPipeClientKill, FsPipeClose},
    /*
     * Remote anonymous pipe stream.  These arise because of migration.
     */
    { FS_RMT_PIPE_STREAM, NoProc, FsRemoteRead, FsRemoteWrite,
		FsRemoteIOControl, FsRemoteSelect,
		FsRemoteGetIOAttr, FsRemoteSetIOAttr,
		FsRmtPipeVerify, FsRemoteIORelease, FsRemoteIOMigEnd,
		FsRmtPipeMigrate, FsRmtPipeReopen,
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsRemoteHandleScavenge, NullClientKill, FsRemoteIOClose},
    /*
     * A control stream is what a server program gets when it opens a
     * pseudo device file.  This stream is used to notify the server
     * of new clients; the ID of the server stream set up for each
     * new client is passed over this control stream.
     */
    { FS_CONTROL_STREAM, FsControlCltOpen, FsControlRead, NoProc,
		FsControlIOControl, FsControlSelect,
		FsControlGetIOAttr, FsControlSetIOAttr,
		FsControlVerify,
		NoProc, NoProc,				/* migStart, migEnd */
		NoProc, FsControlReopen,		/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsControlScavenge, FsControlClientKill, FsControlClose },
    /*
     * A server stream gets set up for the server whenever a client opens
     * a pseudo device.  The server reads the stream the learn about new
     * requests from the client.  IOControls on the stream are used
     * to control the connection to the client.
     */
    { FS_SERVER_STREAM, NoProc, FsServerStreamRead, NoProc,
		FsServerStreamIOControl, FsServerStreamSelect,
		NullProc, NullProc,			/* Get/Set IO Attr */
		NoHandle,				/* verify */
		NoProc, NoProc,				/* migStart, migEnd */
		NoProc, NoProc,				/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsHandleUnlockHdr, NullClientKill, FsServerStreamClose },
    /*
     * A pseudo stream with the server process running locally.  
     */
    { FS_LCL_PSEUDO_STREAM, FsPseudoStreamCltOpen, FsPseudoStreamRead,
		FsPseudoStreamWrite, FsPseudoStreamIOControl,
		FsPseudoStreamSelect,
		FsPseudoStreamGetIOAttr, FsPseudoStreamSetIOAttr,
		NoHandle, FsPseudoStreamRelease, FsPseudoStreamMigEnd,
		FsPseudoStreamMigrate, NoProc,		/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsHandleUnlockHdr, NullClientKill, FsPseudoStreamClose },
    /*
     * A pseudo stream with a remote server.  
     */
    { FS_RMT_PSEUDO_STREAM, FsRmtPseudoStreamCltOpen, FsRemoteRead,
		FsRemoteWrite,
		FsRemoteIOControl, FsRemoteSelect,
		FsRemoteGetIOAttr, FsRemoteSetIOAttr,
		FsRmtPseudoStreamVerify,
		FsRemoteIORelease, FsRemoteIOMigEnd,
		FsRmtPseudoStreamMigrate, NoProc,	/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsRemoteHandleScavenge, NullClientKill, FsRemoteIOClose },
    /*
     * A control stream used to mark the existence of a pseudo-filesystem.
     * The server doesn't do I/O to this stream; it is only used at
     * open and close time.
     */
    { FS_PFS_CONTROL_STREAM, FsPfsCltOpen,
		NoProc, NoProc,				/* read, write */
		NoProc, NoProc,				/* IOControl, select */
		NullProc, NullProc,			/* Get/Set IO Attr */
		FsControlVerify,
		NoProc, NoProc,				/* migStart, migEnd */
		NoProc, FsControlReopen,		/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsControlScavenge, FsControlClientKill,
		FsControlClose },
    /*
     * The 'naming stream' for a pseudo-filesystem is used to forward lookup
     * operations from the kernel up to the user-level server.  It is like
     * a regular pdev request-response stream, except that extra connections
     * to it are established via the prefix table when remote Sprite hosts
     * import a pseudo-filesystem.  The routines here are used to set this up.
     * This stream is only accessed locally.  To remote hosts the domain is
     * a regular remote sprite domain, and the RPC stubs on the server then
     * switch out to either local-domain or pseudo-domain routines.
     */
    { FS_PFS_NAMING_STREAM, FsPfsNamingCltOpen,
		NoProc, NoProc,				/* read, write */
		NoProc, NoProc,				/* IOControl, select */
		NullProc, NullProc,			/* Get/Set IO Attr */
		FsRmtPseudoStreamVerify,
		NoProc, NoProc,				/* migStart, migEnd */
		NoProc, NoProc,				/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsRemoteHandleScavenge, NullClientKill,
		FsRemoteIOClose },
    /*
     * A pseudo stream to a pseudo-filesystem server.  This is just like
     * a pseudo stream to a pseudo-device server, except for the CltOpen
     * routine because setup is different.  
     */
    { FS_LCL_PFS_STREAM, FsPfsStreamCltOpen, FsPseudoStreamRead,
		FsPseudoStreamWrite, FsPseudoStreamIOControl,
		FsPseudoStreamSelect,
		FsPseudoStreamGetIOAttr, FsPseudoStreamSetIOAttr,
		NoHandle, FsPseudoStreamRelease, FsPseudoStreamMigEnd,
		FsPseudoStreamMigrate, NoProc,		/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsHandleUnlockHdr, NullClientKill, FsPseudoStreamClose },
    /*
     * A pseudo stream to a remote pseudo-filesystem server.  This is
     * like the remote pseudo-device stream, except for setup because the
     * pseudo-device connection is already set up by the time the
     * CltOpen routine is called.
     */
    { FS_RMT_PFS_STREAM, FsRmtPfsStreamCltOpen, FsRemoteRead,
		FsRemoteWrite,
		FsRemoteIOControl, FsRemoteSelect,
		FsRemoteGetIOAttr, FsRemoteSetIOAttr,
		FsRmtPseudoStreamVerify,
		FsRemoteIORelease, FsRemoteIOMigEnd,
		FsRmtPseudoStreamMigrate, NoProc,	/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,		/* cache ops */
		FsRemoteHandleScavenge, NullClientKill, FsRemoteIOClose },
    /*
     * This stream type is only used during get/set I/O attributes when
     * the pseudo-device server is remote.  No handles of this type are
     * actually created, only fileIDs that map to FS_CONTROL_STREAM.  
     */
    { FS_RMT_CONTROL_STREAM, NoProc, NoProc,	/* cltOpen, read */
		NoProc,				/* write */
		NoProc, NoProc,			/* ioctl, select */
		FsRemoteGetIOAttr, FsRemoteSetIOAttr,
		(FsHandleHeader *(*)())NoProc,	/* verify */
		NoProc, NoProc,			/* release, migend */
		NoProc, NoProc,			/* migrate, reopen */
		NoProc, NoProc, NoProc, NoProc,	/* cache ops */
		(Boolean (*)())NoProc,		/* scavenge */
		(void (*)())NoProc, NoProc },	/* kill, close */
#ifdef notdef
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
    { FS_RMT_NAMED_PIPE_STREAM, FsRmtNamedPipeCltOpen, FsRemoteRead,
		FsRemoteWrite, FsRemoteIOControl,
		FsRemoteSelect, FsRemoteGetIOAttr, FsRemoteSetIOAttr,
		FsRmtNamedPipeVerify, FsRmtDeviceEncap, FsRmtDeviceDeencap,
		NoProc, NoProc, NoProc, NoProc,
		FsRmtDeviceClose, FsRmtDeviceDelete},
    /*
     * A stream to the hybrid Sprite/Unix server.  DECOMMISIONED.
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
#endif /* notdef */
};

/*
 * Simple arrays are used to map between local and remote types.
 */
int fsRmtToLclType[FS_NUM_STREAM_TYPES] = {
    FS_STREAM, 			/* FS_STREAM */
    FS_LCL_FILE_STREAM,		/* FS_LCL_FILE_STREAM */
    FS_LCL_FILE_STREAM,		/* FS_RMT_FILE_STREAM */
    FS_LCL_DEVICE_STREAM,	/* FS_LCL_DEVICE_STREAM */
    FS_LCL_DEVICE_STREAM,	/* FS_RMT_DEVICE_STREAM */
    FS_LCL_PIPE_STREAM,		/* FS_LCL_PIPE_STREAM */
    FS_LCL_PIPE_STREAM,		/* FS_RMT_PIPE_STREAM */
    FS_CONTROL_STREAM,		/* FS_CONTROL_STREAM */
    FS_SERVER_STREAM,		/* FS_SERVER_STREAM */
    FS_LCL_PSEUDO_STREAM,	/* FS_LCL_PSEUDO_STREAM */
    FS_LCL_PSEUDO_STREAM,	/* FS_RMT_PSEUDO_STREAM */
    FS_PFS_CONTROL_STREAM,	/* FS_PFS_CONTROL_STREAM */
    FS_LCL_PSEUDO_STREAM,	/* FS_PFS_NAMING_STREAM */
    FS_LCL_PFS_STREAM,		/* FS_LCL_PFS_STREAM */
    FS_LCL_PFS_STREAM,		/* FS_RMT_PFS_STREAM */
    FS_CONTROL_STREAM,		/* FS_RMT_CONTROL_STREAM */

    -1,				/* FS_RMT_NFS_STREAM */
    FS_LCL_NAMED_PIPE_STREAM,	/* FS_LCL_NAMED_PIPE_STREAM */
    FS_LCL_NAMED_PIPE_STREAM,	/* FS_RMT_NAMED_PIPE_STREAM */
    -1,				/* FS_RMT_UNIX_STREAM */
};

int fsLclToRmtType[FS_NUM_STREAM_TYPES] = {
    FS_STREAM, 			/* FS_STREAM */
    FS_RMT_FILE_STREAM,		/* FS_LCL_FILE_STREAM */
    FS_RMT_FILE_STREAM,		/* FS_RMT_FILE_STREAM */
    FS_RMT_DEVICE_STREAM,	/* FS_LCL_DEVICE_STREAM */
    FS_RMT_DEVICE_STREAM,	/* FS_RMT_DEVICE_STREAM */
    FS_RMT_PIPE_STREAM,		/* FS_LCL_PIPE_STREAM */
    FS_RMT_PIPE_STREAM,		/* FS_RMT_PIPE_STREAM */
    FS_CONTROL_STREAM,		/* FS_CONTROL_STREAM */
    -1,				/* FS_SERVER_STREAM */
    FS_RMT_PSEUDO_STREAM,	/* FS_LCL_PSEUDO_STREAM */
    FS_RMT_PSEUDO_STREAM,	/* FS_RMT_PSEUDO_STREAM */
    FS_PFS_CONTROL_STREAM,	/* FS_PFS_CONTROL_STREAM */
    FS_PFS_NAMING_STREAM,	/* FS_PFS_NAMING_STREAM */
    FS_RMT_PFS_STREAM,		/* FS_LCL_PFS_STREAM */
    FS_RMT_PFS_STREAM,		/* FS_RMT_PFS_STREAM */
    FS_RMT_CONTROL_STREAM,	/* FS_RMT_CONTROL_STREAM */

    FS_RMT_NFS_STREAM,		/* FS_RMT_NFS_STREAM */
    FS_RMT_NAMED_PIPE_STREAM,	/* FS_LCL_NAMED_PIPE_STREAM */
    FS_RMT_NAMED_PIPE_STREAM,	/* FS_RMT_NAMED_PIPE_STREAM */
    FS_RMT_UNIX_STREAM,		/* FS_RMT_UNIX_STREAM */
};


static ReturnStatus
NullProc()
{
    return(SUCCESS);
}
static ReturnStatus
NoProc()
{
    return(FAILURE);
}

/*ARGSUSED*/
static void
NullClientKill(hdrPtr, clientID)
    FsHandleHeader *hdrPtr;
    int clientID;
{
    FsHandleUnlock(hdrPtr);
}

static FsHandleHeader *
NoHandle()
{
    return((FsHandleHeader *)NIL);
}

