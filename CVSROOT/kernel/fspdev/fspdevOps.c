/* 
 * fsPdevOps.c --
 *
 *	Routine for initializing the fsOpTable switch entries for 
 *	pseudo-devices and pseudo-filesystems.
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
#include <fsconsist.h>
#include <fsio.h>
#include <fsNameOps.h>
#include <fspdev.h>
#include <fspdevInt.h>
#include <fsrmt.h>
#include <fsioFile.h>
/*
 * fs_DomainLookup for FS_REMOTE_SPRITE_DOMAIN type.
 */

static Fs_DomainLookupOps pdevDomainLookup =  {
     Fsio_NoProc, FspdevPfsExport, FspdevPfsOpen, FspdevPfsGetAttrPath, 
     FspdevPfsSetAttrPath,
     FspdevPfsMakeDevice, FspdevPfsMakeDir, FspdevPfsRemove, FspdevPfsRemoveDir,
     FspdevPfsRename, FspdevPfsHardLink
};

static Fs_AttrOps pdevAttrOpTable = { FspdevPseudoGetAttr, FspdevPseudoSetAttr };

static Fsio_OpenOps pdevOpenOps[] = { 
    /*
     * Pseudo devices.
     */
    {FS_PSEUDO_DEV, FspdevNameOpen},
    { FS_PSEUDO_FS, Fsio_NoProc},
    /*
     * A remote link can either be treated like a regular file,
     * or opened by a pseudo-filesystem server.
     */
    { FS_REMOTE_LINK, FspdevRmtLinkNameOpen },
};
static int numPdevOpenOps = sizeof(pdevOpenOps)/
				 sizeof(pdevOpenOps[0]);
/*
 * File stream type ops for FSIO_CONTROL_STREAM, FSIO_SERVER_STREAM,
 * and FSIO_LCL_PSEUDO_STREAM FSIO_RMT_PSEUDO_STREAM FSIO_PFS_CONTROL_STREAM
 * FSIO_PFS_NAMING_STREAM FSIO_LCL_PFS_STREAM FSIO_RMT_CONTROL_STREAM 
 * FSIO_PASSING_STREAM
 */

static Fsio_StreamTypeOps pdevFileStreamOps[] = {
    /*
     * A control stream is what a pdev server process gets when it opens a
     * pseudo device file.  This stream is used to notify the server
     * of new clients; the ID of the server stream set up for each
     * new client is passed over this control stream.
     */
    { FSIO_CONTROL_STREAM, FspdevControlIoOpen,
		FspdevControlRead, Fsio_NoProc,
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc, 	/* Paging routines */
		FspdevControlIOControl, FspdevControlSelect,
		FspdevControlGetIOAttr, FspdevControlSetIOAttr,
		FspdevControlVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, FspdevControlReopen,	/* migrate, reopen */
		FspdevControlScavenge, FspdevControlClientKill,
		FspdevControlClose },
    /*
     * A server stream gets set up for the server whenever a client opens
     * a pseudo device.  The server reads the stream the learn about new
     * requests from the client.  IOControls on the stream are used
     * to control the connection to the client.
     */
    { FSIO_SERVER_STREAM, Fsio_NoProc,
		FspdevServerStreamRead, Fsio_NoProc,
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc, 	/* Paging routines */
		FspdevServerStreamIOControl, FspdevServerStreamSelect,
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		Fsio_NoHandle,				/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		Fsio_NullClientKill, FspdevServerStreamClose },
    /*
     * A pseudo stream with the server process running locally.  
     */
    { FSIO_LCL_PSEUDO_STREAM, FspdevPseudoStreamIoOpen,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite, /* Paging */
		Fsio_NoProc, 	/* Paging routines */
		FspdevPseudoStreamIOControl,
		FspdevPseudoStreamSelect,
		FspdevPseudoStreamGetIOAttr, FspdevPseudoStreamSetIOAttr,
		Fsio_NoHandle,				/* verify */
		FspdevPseudoStreamMigClose, FspdevPseudoStreamMigOpen,
		FspdevPseudoStreamMigrate,
		Fsio_NoProc,				/* reopen */
		(Boolean (*)())NIL,			/* scavenge */
		Fsio_NullClientKill, FspdevPseudoStreamClose },
    /*
     * A pseudo stream with a remote server.  
     */
    { FSIO_RMT_PSEUDO_STREAM, FspdevRmtPseudoStreamIoOpen,
		Fsrmt_Read, Fsrmt_Write,
		Fsrmt_Read, Fsrmt_Write,		/* Paging I/O */
		Fsio_NoProc, 	/* Paging routines */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FspdevRmtPseudoStreamVerify,
		Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FspdevRmtPseudoStreamMigrate,
		Fsio_NoProc,				/* reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		FspdevRmtPseudoStreamClose },
    /*
     * A control stream used to mark the existence of a pseudo-filesystem.
     * The server doesn't do I/O to this stream; it is only used at
     * open and close time.
     */
    { FSIO_PFS_CONTROL_STREAM, FspdevPfsIoOpen,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc, 	/* Paging I/O */
		Fsio_NoProc, Fsio_NoProc,		/* IOControl, select */
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		FspdevControlVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, FspdevControlReopen,	/* migrate, reopen */
		FspdevControlScavenge, FspdevControlClientKill,
		FspdevControlClose },
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
    { FSIO_PFS_NAMING_STREAM, FspdevPfsNamingIoOpen,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc, 	/* Paging I/O */
		Fsio_NoProc, Fsio_NoProc,		/* IOControl, select */
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		FspdevRmtPseudoStreamVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		Fsrmt_IOClose },
    /*
     * A pseudo stream to a pseudo-filesystem server.  This is just like
     * a pseudo stream to a pseudo-device server, except for the CltOpen
     * routine because setup is different.  
     */
    { FSIO_LCL_PFS_STREAM, FspdevPfsStreamIoOpen,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite, /* Paging */
		Fsio_NoProc, 	/* Paging I/O */
		FspdevPseudoStreamIOControl,
		FspdevPseudoStreamSelect,
		FspdevPseudoStreamGetIOAttr, FspdevPseudoStreamSetIOAttr,
		Fsio_NoHandle,					/* verify */
		FspdevPseudoStreamMigClose, FspdevPseudoStreamMigOpen,
		FspdevPseudoStreamMigrate,
		Fsio_NoProc,					/* reopen */
		(Boolean (*)())NIL,				/* scavenge */
		Fsio_NullClientKill, FspdevPseudoStreamClose },
    /*
     * A pseudo stream to a remote pseudo-filesystem server.  This is
     * like the remote pseudo-device stream, except for setup because the
     * pseudo-device connection is already set up by the time the
     * CltOpen routine is called.
     */
    { FSIO_RMT_PFS_STREAM, FspdevRmtPfsStreamIoOpen,
		Fsrmt_Read, Fsrmt_Write,
		Fsrmt_Read, Fsrmt_Write,			/* Paging */
		Fsio_NoProc, 	
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FspdevRmtPseudoStreamVerify,
		Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FspdevRmtPseudoStreamMigrate,
		Fsio_NoProc,					/* reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		FspdevRmtPseudoStreamClose },
    /*
     * This stream type is only used during get/set I/O attributes when
     * the pseudo-device server is remote.  No handles of this type are
     * actually created, only fileIDs that map to FSIO_CONTROL_STREAM.  
     */
    { FSIO_RMT_CONTROL_STREAM, Fsio_NoProc,		/* ioOpen */
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc, 	/* Paging */
		Fsio_NoProc, Fsio_NoProc,		/* ioctl, select */
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		(Fs_HandleHeader *(*)())Fsio_NoProc,	/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* release, migend */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		(void (*)())Fsio_NoProc, Fsio_NoProc },	/* kill, close */
    /*
     * Stream used to pass streams from a pseudo-device server to
     * a client in response to an open request.
     */
    { FSIO_PASSING_STREAM, FspdevPassStream,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc, Fsio_NoProc, 	/* Paging */
		Fsio_NoProc, Fsio_NoProc,		/* ioctl, select */
		Fsio_NoProc, Fsio_NoProc,		/* get/set attr */
		(Fs_HandleHeader *(*)())Fsio_NoProc,	/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* release, migend */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		(void (*)())Fsio_NoProc, Fsio_NoProc },	/* kill, close */


};

static int numPdevFileStreamOps = sizeof(pdevFileStreamOps)/
				 sizeof(pdevFileStreamOps[0]);

/*
 *----------------------------------------------------------------------
 *
 * FsdPdevRmtInitializeOps --
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
Fspdev_InitializeOps()
{
    int	i;

    Fs_InstallDomainLookupOps(FS_PSEUDO_DOMAIN, &pdevDomainLookup, 
			&pdevAttrOpTable);
    for (i = 0; i < numPdevFileStreamOps; i++)  { 
	Fsio_InstallStreamOps(pdevFileStreamOps[i].type, &(pdevFileStreamOps[i]));
    }
    for (i = 0; i < numPdevOpenOps; i++)  { 
	Fsio_InstallSrvOpenOp(pdevOpenOps[i].type, &(pdevOpenOps[i]));
    }

}

/*
 *----------------------------------------------------------------------
 *
 * Fspdev_Bin() --
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
Fspdev_Bin()
{
    Mem_Bin(sizeof(Fspdev_ServerIOHandle));
    Mem_Bin(sizeof(Fspdev_ControlIOHandle));
}


/*
 * ----------------------------------------------------------------------------
 *
 * FspdevPassStream --
 *
 *	This is called from Fs_Open as a ioOpen routine.  It's job is
 *	to take an encapsulated stream from a pseudo-device server and
 *	unencapsulate it so the Fs_Open returns the stream that the
 *	pseudo-device server had.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Deencapsulates a stream.
 *
 * ----------------------------------------------------------------------------
 *
 */

/* ARGSUSED */
ReturnStatus
FspdevPassStream(ioFileIDPtr, flagsPtr, clientID, streamData, name, ioHandlePtrPtr)
    Fs_FileID		*ioFileIDPtr;	/* I/O fileID from the name server */
    int			*flagsPtr;	/* Return only.  The server returns
					 * a modified useFlags in Fsio_FileState */
    int			clientID;	/* IGNORED */
    ClientData		streamData;	/* Pointer to encapsulated stream. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a handle set up for
					 * I/O to a file, NIL if failure. */
{
    return(FAILURE);
}

