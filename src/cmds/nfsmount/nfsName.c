/*
 * nfsName.c --
 * 
 *	Procedures that interface to a remote NFS filesystem.  The procedures
 *	here are called via the Pfs/Pdev library in response to naming requests
 *	from the Sprite kernel about files in an NFS filesystem.
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
static char rcsid[] = "$Header: /sprite/src/cmds/nfsmount/RCS/nfsName.c,v 1.18 91/09/24 11:47:50 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

#include "nfs.h"
#include "sys/stat.h"

int nfsToSpriteFileType[] = {
    FS_PSEUDO_DEV,		/* NFNON */
    FS_FILE,			/* NFREG */
    FS_DIRECTORY,		/* NFDIR */
    FS_DEVICE,			/* NFBLK */
    FS_DEVICE,			/* NFCHR */
    FS_SYMBOLIC_LINK,		/* NFLNK */
};
int spriteToNfsModeType[] = {
    S_IFREG, 	/* FS_FILE */
    S_IFDIR,	/* FS_DIRECTORY */
    S_IFLNK,	/* FS_SYMBOLIC_LINK */
    S_IFLNK,	/* FS_REMOTE_LINK */
    S_IFCHR,	/* FS_DEVICE */
    0,		/* FS_REMOTE_DEVICE - not used by Sprite */
    0,		/* FS_LOCAL_PIPE - never seen by open */
    S_IFIFO,	/* FS_NAMED_PIPE */
    S_IFSOCK,	/* FS_PSEUDO_DEVICE */
    S_IFLNK,	/* FS_PSEUDO_FS */
    0,		/* FS_XTRA_FILE - used to test new Sprite types */
};

/*
 * Open file table.  A simple array of pointers to nfs_fh that
 * is used to map between a Fs_FileID we give the Sprite kernel
 * each time an NFS file is opened.  The minor field of the Sprite
 * fileID is used as an index into this array of pointers to NFS handles.
 */
NfsOpenFile **nfsFileTable = (NfsOpenFile **)NULL;
NfsOpenFile **nextFreeSlot = (NfsOpenFile **)NULL;
int nfsFileTableSize = 0;

/*
 * The following macro is used to map from a Sprite file ID to an NFS handle.
 * This is used to interpret the prefixID passed to us on lookup operations.
 * The Sprite kernel passes us a completly zero'd fileID for
 * names that start at our root.  Otherwise it passes us the
 * fileID we gave it when the process opened its current directory.
 */
#define PrefixIDToHandle(prefixID) \
    ( (prefixID.type == TYPE_ROOT) ? (NfsOpenFile *)NULL : \
	((nfsFileTable == (NfsOpenFile **)NULL) ? (NfsOpenFile *)NULL : \
		nfsFileTable[prefixID.minor] ) )

/*
 * The set of callback procedures given to Pfs_Open.  These define the
 * procedures called in response to pseudo-filesystem naming operations.
 */
Pfs_CallBacks nfsNameService= {
    NfsOpen,			/* PFS_OPEN */
    NfsGetAttrPath,		/* PFS_GET_ATTR */
    NfsSetAttrPath,		/* PFS_SET_ATTR */
    NfsMakeDevice,		/* PFS_MAKE_DEVICE */
    NfsMakeDir,			/* PFS_MAKE_DIR */
    NfsRemove,			/* PFS_REMOVE */
    NfsRemoveDir,		/* PFS_REMOVE_DIR */
    NfsRename,			/* PFS_RENAME */
    NfsHardLink,		/* PFS_HARD_LINK */
    NfsSymLink,			/* PFS_SYM_LINK */
    NfsDomainInfo, 		/* PFS_DOMAIN_INFO */
};

void NfsSetupAuth();


/*
 *----------------------------------------------------------------------
 *
 * Nfs_InitClient --
 *
 *	Set up the CLIENT data structure needed to do SUN RPC to the
 *	NFS server running on a particular host.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls clnt_create which sets up sockets and other related state.
 *
 *----------------------------------------------------------------------
 */
CLIENT *
Nfs_InitClient(host)
    char *host;
{
    register CLIENT *clnt;
    VoidPtr voidArg;
    VoidPtr voidRes;
    int retryCnt = -1; /* infinite retries */

    clnt = clnt_create(host, NFS_PROGRAM, NFS_VERSION, "udp");
    if (clnt == (CLIENT *)NULL) {
	clnt_pcreateerror(host);
    } else {
	clnt->cl_auth = authunix_create_default();
	if (!clnt_control(clnt, CLSET_RETRY_COUNT, &retryCnt)) {
	    clnt_perror(clnt, "clnt_control");
	} else {
	    voidRes = nfsproc_null_2(&voidArg, clnt);
	    if (voidRes == (VoidPtr)NULL) {
		clnt_perror(clnt, "nfsproc_null_2");
	    } else if (pdev_Trace) {
		printf("Null RPC to NFS service at %s succeeded\n", host);
	    }
	}
    }
    return(clnt);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsProbe --
 *
 *	Called to test NFS access.  This dos a stat of the root of the
 *	NFS system we have mounted.  The stat information is returned
 *	so the pseudo-file-system server can properly establish the
 *	user-visible fileID of the root.
 * 
 * Results:
 *	1 if probe succeeded, 0 otherwise.
 *
 * Side effects:
 *	Fills in the NFS attributes of the root directory.
 *
 *----------------------------------------------------------------------
 */

int
NfsProbe(nfsPtr, print, nfsAttrPtr)
    NfsState *nfsPtr;		/* Top level state for NFS connection */
    int print;			/* 1 to print out attributes */
    attrstat *nfsAttrPtr;	/* NFS attributes of root */
{

    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_GETATTR, xdr_nfs_fh,
		nfsPtr->mountHandle,
		xdr_attrstat, nfsAttrPtr, nfsTimeout) != RPC_SUCCESS) {
	clnt_perror(nfsPtr->nfsClnt, "NFSPROC_GETATTR");
	return(0);
    } else if (nfsAttrPtr->status != NFS_OK) {
	printf("NfsProbe: Get attributes status %d\n", nfsAttrPtr->status);
	return(0);
    } else if (print) {
	printf("Attributes of %s:%s\n", nfsPtr->host, nfsPtr->nfsName);
	printf("\tFileID %x FS_ID %x\n",
		nfsAttrPtr->attrstat_u.attributes.fileid,
		nfsAttrPtr->attrstat_u.attributes.fsid);
	printf("\tType %d mode 0%o links %d size %d\n",
		nfsAttrPtr->attrstat_u.attributes.type,
		nfsAttrPtr->attrstat_u.attributes.mode,
		nfsAttrPtr->attrstat_u.attributes.nlink,
		nfsAttrPtr->attrstat_u.attributes.size);
    }
    return(1);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsOpen --
 *
 *	Called to open a file in the NFS system.  This does the open
 *	with the NFS server and then opens a new pseudo-device connection
 *	that will be used for all further operatiosn on the NFS file.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls clnt_create which sets up sockets and other related state.
 *
 *----------------------------------------------------------------------
 */
int
NfsOpen(clientData, name, openArgsPtr, redirectInfoPtr)
    ClientData clientData;		/* Ref. to NfsState */
    char *name;				/* Pathname to open */
    register Fs_OpenArgs *openArgsPtr;	/* Bundled arguments */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    NfsState *nfsPtr = (NfsState *)clientData;
    NfsOpenFile *cwdFilePtr;
    Pdev_Stream *streamPtr;
    diropres longDirResults;
    diropokres *dirResultsPtr = &longDirResults.diropres_u.diropres;
    createargs createArgs;
    diropargs *wherePtr = &createArgs.where;
    int status;
    char component[NFS_MAXNAMLEN];
    int created = 0;
    int writeBehind = nfs_PdevWriteBehind;

    wherePtr->name = component;
    cwdFilePtr = PrefixIDToHandle(openArgsPtr->prefixID);
    NfsSetupAuth(nfsPtr, cwdFilePtr, &openArgsPtr->id);
    status = NfsLookup(nfsPtr, cwdFilePtr, name, openArgsPtr->useFlags,
		       dirResultsPtr, &wherePtr, redirectInfoPtr);
    if (status == NFSERR_NOENT && wherePtr != (diropargs *)NULL &&
		(openArgsPtr->useFlags & FS_CREATE)) {
	/*
	 * We need to create the file.  NfsLookup has set up createArgs.where
	 * to have the handle on the parent and the component to create.
	 */
	register sattr *sattrPtr = &createArgs.attributes;

	sattrPtr->mode = openArgsPtr->permissions & 07777;
	if (openArgsPtr->type >= 0 && openArgsPtr->type <= FS_XTRA_FILE) {
	    sattrPtr->mode |= spriteToNfsModeType[openArgsPtr->type];
	} else {
	    printf("Open(\"%s\") bad type %d\n", name, openArgsPtr->type);
	    return(GEN_INVALID_ARG);
	}
	sattrPtr->uid = openArgsPtr->id.user;
	sattrPtr->gid = -1;
	sattrPtr->size = 0;
	sattrPtr->atime.seconds = -1;
	sattrPtr->atime.useconds = -1;
	sattrPtr->mtime.seconds = -1;
	sattrPtr->mtime.useconds = -1;
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_CREATE, xdr_createargs,
		&createArgs, xdr_diropres, &longDirResults, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_CREATE");
	    return(FAILURE);
	}
	status = longDirResults.status;
	created = 1;
    }
    if (status != NFS_OK) {
	status = NfsStatusMap(status);
    } else {
	register Fs_FileID *fileIDPtr;
	register int full = 0;
	/*
	 * We've got a good handle on the NFS file.  Time to do our own
	 * permission checking cause the NFS server is pretty lame.
	 */
	status = CheckPermissions(&dirResultsPtr->attributes, created,
	      openArgsPtr->useFlags, &openArgsPtr->id, openArgsPtr->type);
	if (status != SUCCESS) {
	    return(status);
	}
	/* 
	 * Save our file handle in an array indexed by part of the Fs_FileID
	 * we'll give to the Sprite kernel and to the pdev callback library.
	 * This will let us get back to the handle from prefixID's in open
	 * arguments, and from the client data passed to us from the callback
	 * library.
	 */
	if (nfsFileTable == (NfsOpenFile **)NULL) {
	    /*
	     * Allocate and initialize the table.  It might grow later.
	     */
	    nfsFileTable = (NfsOpenFile **)malloc(64 * sizeof(NfsOpenFile *));
	    bzero((char *)nfsFileTable, 64 * sizeof(NfsOpenFile *));
	    nextFreeSlot = &nfsFileTable[0];
	    nfsFileTableSize = 64;
	}
	while (*nextFreeSlot != (NfsOpenFile *)NULL) {
	    nextFreeSlot++;
	    if (nextFreeSlot >= &nfsFileTable[nfsFileTableSize]) {
		if (!full){
		    nextFreeSlot = &nfsFileTable[0];
		    full = 1;
		} else {
		    /*
		     * Grow the table.
		     */
		    register NfsOpenFile **newTable =
			    (NfsOpenFile **)malloc(nfsFileTableSize * 2 *
					      sizeof(NfsOpenFile *));
		    bcopy((char *)nfsFileTable, (char *)newTable,
			    nfsFileTableSize * sizeof(NfsOpenFile *));
		    bzero((char *)&newTable[nfsFileTableSize],
			    nfsFileTableSize * sizeof(NfsOpenFile *));
		    free((char *)nfsFileTable);
		    nfsFileTable = newTable;
		    nextFreeSlot = &nfsFileTable[nfsFileTableSize];
		    nfsFileTableSize *= 2;
		 }
	     }
	 }
	 /*
	  * Save the NFS handle and the UNIX authentication of this user.
	  */
	 *nextFreeSlot = (NfsOpenFile *)malloc(sizeof(NfsOpenFile));
	 (*nextFreeSlot)->handlePtr = (nfs_fh *)malloc(sizeof(nfs_fh));
	 bcopy((char *)&dirResultsPtr->file,
	       (char *)(*nextFreeSlot)->handlePtr, NFS_FHSIZE);
	 {
	     register int numGroups;
	     numGroups = (openArgsPtr->id.numGroupIDs <= NGRPS) ?
			     openArgsPtr->id.numGroupIDs : NGRPS ;
	     (*nextFreeSlot)->authPtr = authunix_create(myhostname,
		     openArgsPtr->id.user, openArgsPtr->id.group[0],
		     numGroups, openArgsPtr->id.group);
	}
	 (*nextFreeSlot)->openFlags = openArgsPtr->useFlags;
	 /*
	  * Tell the Sprite kernel to set up a pseudo-device connection
	  * over which we'll see all future operations on the NFS file.
	  * The fileID we give here will come back to us if this open is
	  * for a chdir(); the fileID will be passed as the prefixID in
	  * the openArgs.  We set the type in order to differentiate
	  * it from the type of zero used for the fileID of our root.
	  */
	 fileIDPtr = (Fs_FileID *)malloc(sizeof(Fs_FileID));
	 fileIDPtr->minor = nextFreeSlot - nfsFileTable;
	 fileIDPtr->major = 0;
	 fileIDPtr->serverID = (int)nfsPtr;
	 switch(dirResultsPtr->attributes.type) {
	     case NFDIR:
		 fileIDPtr->type = TYPE_DIRECTORY;
		 break;
	     case NFLNK:
		 fileIDPtr->type = TYPE_SYMLINK;
		 break;
	     default:
		 fileIDPtr->type = TYPE_FILE;
		 writeBehind = 1;
		 break;
	 }
	 streamPtr = Pfs_OpenConnection(nfsPtr->pfsToken, fileIDPtr,
			 (16 * 1024) + 128,	/* request buffer size */
			 0, NULL,		/* no read buffer */
			 FS_READABLE | FS_WRITABLE, &nfsFileService);
	 /*
	  * Enable write-behind.  We'd like to let a writer overlap its writes.
	  * The request buffer is large enough for 2 8K block writes.  Using
	  * write-behind increases the write bandwidth from 9k/sec to 40k/sec.
	  */
	 if (streamPtr != (Pdev_Stream *)NULL) {
	     if (Fs_IOControl(streamPtr->streamID, IOC_PDEV_WRITE_BEHIND,
			      sizeof(int), &writeBehind, 0, NULL) != 0) {
		 fprintf(stderr, "IOC_PDEV_WRITE_BEHIND failed\n");
	     }
	     streamPtr->clientData = (ClientData)fileIDPtr;
	 } else {
	     status = EINVAL;
	 }
     }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsSetupAuth --
 *
 *	This procedure initializes the authentication information for
 *	the NFS client structure.  It uses the AUTH * of the open
 *	NFS file if possible.  For absolute pathnames, however, this
 *	hasn't been setup yet so we do it here.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	For absolute paths we create an AUTH structure that won't get free'd.
 *   
 *      True; it wasn't being freed, and at 500 bytes a pop it was
 *      consuming major heap space. I added oldAuth to remember the pointer
 *      and free it next time we come through here. JMS.
 *      
 *----------------------------------------------------------------------
 */
void
NfsSetupAuth(nfsPtr, cwdFilePtr, idPtr)
    NfsState *nfsPtr;
    NfsOpenFile *cwdFilePtr;
    Fs_UserIDs *idPtr;
{
    static AUTH *oldAuth = (AUTH *)NULL;

    if (oldAuth != (AUTH *)NULL) {
	AUTH_DESTROY(oldAuth);
	oldAuth = (AUTH *)NULL;
    }
    if (cwdFilePtr != (NfsOpenFile *)NULL) {
	nfsPtr->nfsClnt->cl_auth = cwdFilePtr->authPtr;
    } else {
	if (idPtr->numGroupIDs > NGRPS) {
	    /*
	     * Patch so xdr_auth_unix doesn't fail.
	     */
	    idPtr->numGroupIDs = NGRPS;
	}
	oldAuth = nfsPtr->nfsClnt->cl_auth = authunix_create(myhostname,
	    idPtr->user, idPtr->group[0], idPtr->numGroupIDs, idPtr->group);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckPermissions --
 *
 *	Client side permission checking so we can make an open fail
 *	if the permissions are not sufficient.  The NFS server lets
 *	any lookup request succeed if the client has permission over
 *	the search path, and then does more permission checking at
 *	each I/O.  Accordingly, we have to do a little more work at open time.
 * 
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
    CheckPermissions(nfsAttrPtr, created, useFlags, idPtr, wantType)
    register fattr *nfsAttrPtr;
    int created;
    register int useFlags;
    register Fs_UserIDs *idPtr;
    register int wantType;
{
    register int thisType = nfsToSpriteFileType[(int)nfsAttrPtr->type];
    register int index;
    register int permBits;
    register int *groupPtr;
    register int status;

    /*
     * Make sure the file type matches.  FS_FILE means any type, otherwise
     * it should match exactly.  We have to patch 'thisType' of
     * SYMBOLIC_LINK to REMOTE_LINK because of a Sprite kernel bug
     * where it asks for REMOTE_LINKS instead of symbolic links
     * when implementing the readlink() system call.
     */
    if (wantType == FS_REMOTE_LINK) {
	wantType = FS_SYMBOLIC_LINK;
    }
    if ((wantType != FS_FILE) && (wantType != thisType)) {
	if (wantType == FS_DIRECTORY) {
	    return(FS_NOT_DIRECTORY);
	} else {
	    return(FS_WRONG_TYPE);
	}
    }
    /*
     * Dis-allow execution of directories...
     */
    if ((wantType == FS_FILE) && (useFlags & FS_EXECUTE) &&
	(thisType != FS_FILE)) {
	return(FS_WRONG_TYPE);
    }

    /* Removed check which disallowed execution of file across NFS */
    /* since the kernel can now do it (without setuid or setgid) JMS */

    if (idPtr->user == 0) {
	/*
	 * For normal files, only check for execute permission.  This
	 * prevents root from being able to execute ordinary files by
	 * accident.  However, root has complete access to directories.
	 */
	if (thisType == FS_DIRECTORY) {
	    return(SUCCESS);
	}
	useFlags &= FS_EXECUTE;
    }
    /*
     * Check read/write/exec permissions against one of the owner bits,
     * the group bits, or the world bits.  'permBits' is set to
     * be the corresponding bits from the attributes and then
     * shifted over so the comparisions are against the WORLD bits.
     */
    if (idPtr->user == nfsAttrPtr->uid) {
	/*
	 * Because NFS is stateless it can't do permission checking right.
	 * A direct quote:
	 * "the server's permission checking algorithm should allow the owner
	 * of a file to access it regardless of the permission setting"
	 *
	 * Thus we don't enforce any permission checking on the owner here
	 * if the file has just been created.
	 * This lets programs like "update" and "cp -p" that preserve
	 * permissions copy a read-only file.
	 */
	if (!created) {
	    permBits = (nfsAttrPtr->mode >> 6) & 07;
	} else {
	    permBits = 07;
	}
    } else {
	for (index = idPtr->numGroupIDs, groupPtr = idPtr->group;
	     index > 0;
	     index--, groupPtr++) {
	    if (*groupPtr == nfsAttrPtr->gid) {
		permBits = (nfsAttrPtr->mode >> 3) & 07;
		goto havePermBits;
	    }
	}
	permBits = nfsAttrPtr->mode & 07;
    }
havePermBits:
    if (((useFlags & FS_READ) && ((permBits & FS_WORLD_READ) == 0)) ||
	((useFlags & FS_WRITE) && ((permBits & FS_WORLD_WRITE) == 0)) ||
	((useFlags & FS_EXECUTE) && ((permBits & FS_WORLD_EXEC) == 0))) {
	/*
	 * The file's permission don't include what is needed.
	 */
	status = FS_NO_ACCESS;
    } else {
	status = SUCCESS;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsGetAttrPath --
 *
 *	Called to stat a file in the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsGetAttrPath(clientData, name, openArgsPtr, spriteAttrPtr, redirectInfoPtr)
    ClientData clientData;			/* Ref. to NfsState */
    char *name;				/* Pathname to open */
    Fs_OpenArgs *openArgsPtr;		/* Bundled arguments */
    Fs_Attributes *spriteAttrPtr;	/* Return - attributes of the file */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    NfsState *nfsPtr = (NfsState *)clientData;
    diropokres dirResults;
    fattr *nfsAttrPtr = &dirResults.attributes;
    NfsOpenFile *cwdFilePtr;
    int status;

    cwdFilePtr = PrefixIDToHandle(openArgsPtr->prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, name, openArgsPtr->useFlags,
		    &dirResults, (diropargs *)NULL, redirectInfoPtr);

    if (status != NFS_OK) {
	return(NfsStatusMap(status));
    }
    NfsToSpriteAttr(nfsAttrPtr, spriteAttrPtr);
    return(0);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsSetAttrPath --
 *
 *	Called to change attributes of a file in the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsSetAttrPath(clientData, name, openArgsPtr, flags, attrPtr, redirectInfoPtr)
    ClientData clientData;		/* Ref. to NfsState */
    char *name;			/* Pathname to open */
    Fs_OpenArgs *openArgsPtr;	/* Bundled arguments */
    int flags;			/* Specify which attributes to set */
    Fs_Attributes *attrPtr;	/* New attributes of the file */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    NfsState *nfsPtr = (NfsState *)clientData;
    diropokres dirResults;
    sattrargs sattrArgs;
    attrstat attrStat;
    NfsOpenFile *cwdFilePtr;
    int status;

    cwdFilePtr = PrefixIDToHandle(openArgsPtr->prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, name, openArgsPtr->useFlags,
		    &dirResults, (diropargs **)NULL, redirectInfoPtr);

    if (status != NFS_OK) {
	return(NfsStatusMap(status));
    }
    bcopy((char *)&dirResults.file, (char *)&sattrArgs.file, sizeof(nfs_fh));
    SpriteToNfsAttr(flags, attrPtr, &sattrArgs.attributes);
    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_SETATTR, xdr_sattrargs, &sattrArgs,
	    xdr_attrstat, &attrStat, nfsTimeout) != RPC_SUCCESS) {
	clnt_perror(nfsPtr->nfsClnt, "NFSPROC_SETATTR");
	return(FAILURE);
    } else {
	return(NfsStatusMap((int)attrStat.status));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NfsMakeDevice --
 *
 *	Called to create a special file (device) in the NFS system.
 *	This isn't supported by the NFS protocol.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsMakeDevice(clientData, name, makeDevArgsPtr, redirectInfoPtr)
    ClientData clientData;			/* Ref. to NfsState */
    char *name;				/* Pathname to open */
    Fs_MakeDeviceArgs *makeDevArgsPtr;	/* Bundled arguments */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    return(FS_NO_ACCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsMakeDir --
 *
 *	Called to make a directory in the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsMakeDir(clientData, name, openArgsPtr, redirectInfoPtr)
    ClientData clientData;		/* Ref. to NfsState */
    char *name;			/* Pathname to open */
    Fs_OpenArgs *openArgsPtr;	/* Bundled arguments */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    NfsOpenFile *cwdFilePtr;
    NfsState *nfsPtr = (NfsState *)clientData;
    diropres longDirResults;
    diropokres *dirResultsPtr = &longDirResults.diropres_u.diropres;
    createargs createArgs;
    diropargs *wherePtr = &createArgs.where;
    register sattr *sattrPtr = &createArgs.attributes;
    int status;
    char component[NFS_MAXNAMLEN];

    wherePtr->name = component;
    cwdFilePtr = PrefixIDToHandle(openArgsPtr->prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, name, openArgsPtr->useFlags,
		       dirResultsPtr, &wherePtr, redirectInfoPtr);
    if (status == NFS_OK) {
	return(EEXIST);
    } else if (status == NFSERR_NOENT) {
	sattrPtr->mode = openArgsPtr->permissions & 07777 | S_IFDIR;
	sattrPtr->uid = openArgsPtr->id.user;
	sattrPtr->gid = -1;
	sattrPtr->size = 0;
	sattrPtr->atime.seconds = -1;
	sattrPtr->atime.useconds = -1;
	sattrPtr->mtime.seconds = -1;
	sattrPtr->mtime.useconds = -1;
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_MKDIR, xdr_createargs,
		&createArgs, xdr_diropres, &longDirResults, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_MKDIR");
	    return(FAILURE);
	}
	status = longDirResults.status;
    }
    return(NfsStatusMap(status));
}

/*
 *----------------------------------------------------------------------
 *
 * NfsRemove --
 *
 *	Called to remove a file from the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsRemove(clientData, name, lookupArgsPtr, redirectInfoPtr)
    ClientData clientData;			/* Ref. to NfsState */
    char *name;				/* Pathname to open */
    Fs_LookupArgs *lookupArgsPtr;	/* Bundled arguments */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    NfsOpenFile *cwdFilePtr;
    NfsState *nfsPtr = (NfsState *)clientData;
    diropokres dirResults;
    diropargs dirOpArgs;
    diropargs *wherePtr = &dirOpArgs;
    nfsstat nfsStatus;
    int status;
    char component[NFS_MAXNAMLEN];

    wherePtr->name = component;
    cwdFilePtr = PrefixIDToHandle(lookupArgsPtr->prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, name, lookupArgsPtr->useFlags,
		       &dirResults, &wherePtr, redirectInfoPtr);
    if (status == NFS_OK) {
	if (dirResults.attributes.type == NFDIR) {
	    return(FS_WRONG_TYPE);
	}
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_REMOVE, xdr_diropargs,
		wherePtr, xdr_nfsstat, &nfsStatus, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_REMOVE");
	    return(FAILURE);
	}
	status = (int)nfsStatus;
    }
    return(NfsStatusMap(status));
}

/*
 *----------------------------------------------------------------------
 *
 * NfsRemoveDir --
 *
 *	Called to remove a directory from the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsRemoveDir(clientData, name, lookupArgsPtr, redirectInfoPtr)
    ClientData clientData;			/* Ref. to NfsState */
    char *name;				/* Pathname to open */
    Fs_LookupArgs *lookupArgsPtr;	/* Bundled arguments */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    NfsOpenFile *cwdFilePtr;
    NfsState *nfsPtr = (NfsState *)clientData;
    diropokres dirResults;
    diropargs dirOpArgs;
    diropargs *wherePtr = &dirOpArgs;
    nfsstat nfsStatus;
    int status;
    char component[NFS_MAXNAMLEN];

    wherePtr->name = component;
    cwdFilePtr = PrefixIDToHandle(lookupArgsPtr->prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, name, lookupArgsPtr->useFlags,
		       &dirResults, &wherePtr, redirectInfoPtr);
    if (status == NFS_OK) {
	if (dirResults.attributes.type != NFDIR) {
	    return(FS_NOT_DIRECTORY);
	}
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_RMDIR, xdr_diropargs,
		wherePtr, xdr_nfsstat, &nfsStatus, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_RMDIR");
	    return(FAILURE);
	}
	status = (int)nfsStatus;
    }
    return(NfsStatusMap(status));
}

/*
 *----------------------------------------------------------------------
 *
 * NfsRename --
 *
 *	Called to rename a file in the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsRename(clientData, srcName, dstName, twoNameArgsPtr, redirect2InfoPtr)
    ClientData clientData;			/* Ref. to NfsState */
    char *srcName;			/* Original name */
    char *dstName;			/* New name */
    Fs_2PathParams *twoNameArgsPtr;	/* Lookup args plus prefixID2 */
    Fs_2PathRedirectInfo *redirect2InfoPtr;/* Used when name leaves our domain */
{
    NfsOpenFile *cwdFilePtr;
    NfsState *nfsPtr = (NfsState *)clientData;
    diropokres dirResults;
    renameargs rename;
    diropargs *fromPtr = &rename.from;
    diropargs *toPtr = &rename.to;
    nfsstat nfsStatus;
    int status;
    char component[NFS_MAXNAMLEN];
    char component2[NFS_MAXNAMLEN];
    Fs_RedirectInfo redirectInfo;

    fromPtr->name = component;
    cwdFilePtr = PrefixIDToHandle(twoNameArgsPtr->lookup.prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, srcName,
		twoNameArgsPtr->lookup.useFlags,
	        &dirResults, &fromPtr, &redirectInfo);
    if (status == EREMOTE) {
	redirect2InfoPtr->name1ErrorP = 1;
	redirect2InfoPtr->prefixLength = redirectInfo.prefixLength;
	strcpy(redirect2InfoPtr->fileName, redirectInfo.fileName);
	return(EREMOTE);
    } else if (status == NFS_OK) {
	if (twoNameArgsPtr->prefixID2.type == -1) {
	    redirect2InfoPtr->name1ErrorP = 0;
	    redirect2InfoPtr->prefixLength = 0;
	    redirect2InfoPtr->fileName[0] = '\0';
	    return(FS_CROSS_DOMAIN_OPERATION);
	}
	toPtr->name = component2;
	cwdFilePtr = PrefixIDToHandle(twoNameArgsPtr->prefixID2);
	status = NfsLookup(nfsPtr, cwdFilePtr, dstName,
		    twoNameArgsPtr->lookup.useFlags,
		    &dirResults, &toPtr, &redirectInfo);
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_RENAME, xdr_renameargs,
		&rename, xdr_nfsstat, &nfsStatus, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_RENAME");
	    return(FAILURE);
	}
	status = (int)nfsStatus;
    }
    return(NfsStatusMap(status));
}

/*
 *----------------------------------------------------------------------
 *
 * NfsHardLink --
 *
 *	Make a hard link between two files in the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsHardLink(clientData, srcName, dstName, twoNameArgsPtr, redirect2InfoPtr)
    ClientData clientData;			/* Ref. to NfsState */
    char *srcName;			/* Original name */
    char *dstName;			/* New name */
    Fs_2PathParams *twoNameArgsPtr;	/* Lookup args plus prefixID2 */
    Fs_2PathRedirectInfo *redirect2InfoPtr;/* Used when name leaves our domain */
{
    NfsOpenFile *cwdFilePtr;
    NfsState *nfsPtr = (NfsState *)clientData;
    diropokres dirResults;
    linkargs link;
    diropargs *toPtr = &link.to;
    nfsstat nfsStatus;
    int status;
    char component[NFS_MAXNAMLEN];
    Fs_RedirectInfo redirectInfo;

    cwdFilePtr = PrefixIDToHandle(twoNameArgsPtr->lookup.prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, srcName,
		twoNameArgsPtr->lookup.useFlags,
	        &dirResults, (diropargs **)NULL, &redirectInfo);
    if (status == EREMOTE) {
	redirect2InfoPtr->name1ErrorP = 1;
	redirect2InfoPtr->prefixLength = redirectInfo.prefixLength;
	strcpy(redirect2InfoPtr->fileName, redirectInfo.fileName);
	return(EREMOTE);
    } else if (status == NFS_OK) {
	if (twoNameArgsPtr->prefixID2.type == -1) {
	    redirect2InfoPtr->name1ErrorP = 0;
	    redirect2InfoPtr->prefixLength = 0;
	    redirect2InfoPtr->fileName[0] = '\0';
	    return(FS_CROSS_DOMAIN_OPERATION);
	}
	bcopy((char *)&dirResults.file, (char *)&link.from, sizeof(nfs_fh));
	toPtr->name = component;
	cwdFilePtr = PrefixIDToHandle(twoNameArgsPtr->prefixID2);
	status = NfsLookup(nfsPtr, cwdFilePtr, dstName,
		    twoNameArgsPtr->lookup.useFlags,
		    &dirResults, &toPtr, &redirectInfo);
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_LINK, xdr_linkargs,
		&link, xdr_nfsstat, &nfsStatus, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_LINK");
	    return(FAILURE);
	}
	status = (int)nfsStatus;
    }
    return(NfsStatusMap(status));
}


/*
 *----------------------------------------------------------------------
 *
 * NfsSymLink --
 *
 *	Called to make a symbolic link in the NFS system.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsSymLink(clientData, linkName, value, openArgsPtr, redirectInfoPtr)
    ClientData clientData;		/* Ref. to NfsState */
    char *linkName;			/* Original name */
    char *value;			/* New name */
    Fs_OpenArgs *openArgsPtr;		/* Open arguments */
    Fs_RedirectInfo *redirectInfoPtr;	/* Used when name leaves our domain */
{
    NfsOpenFile *cwdFilePtr;
    NfsState *nfsPtr = (NfsState *)clientData;
    diropokres dirResults;
    symlinkargs symLinkArgs;
    diropargs *wherePtr = &symLinkArgs.from;
    register sattr *sattrPtr = &symLinkArgs.attributes;
    nfsstat nfsStatus;
    int status;
    char component[NFS_MAXNAMLEN];

    symLinkArgs.to = value;
    wherePtr->name = component;
    cwdFilePtr = PrefixIDToHandle(openArgsPtr->prefixID);
    status = NfsLookup(nfsPtr, cwdFilePtr, linkName,
		openArgsPtr->useFlags,
	        &dirResults, &wherePtr, redirectInfoPtr);
    if (status == NFS_OK) {
	return(EEXIST);
    } else if (status == NFSERR_NOENT) {
	sattrPtr->mode = openArgsPtr->permissions & 07777 | S_IFLNK;
	sattrPtr->uid = openArgsPtr->id.user;
	sattrPtr->gid = -1;
	sattrPtr->size = strlen(value);
	sattrPtr->atime.seconds = -1;
	sattrPtr->atime.useconds = -1;
	sattrPtr->mtime.seconds = -1;
	sattrPtr->mtime.useconds = -1;
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_SYMLINK, xdr_symlinkargs,
		&symLinkArgs, xdr_nfsstat, &nfsStatus, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_MKDIR");
	    return(FAILURE);
	}
	status = (int)nfsStatus;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * NfsDomainInfo --
 *
 *	Called to find out about an NFS file system.
 * 
 * Results:
 *	The number of blocks and the number of free blocks in the file system.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
NfsDomainInfo(clientData, fileIDPtr, domainInfoPtr)
    ClientData clientData;		/* Ref. to NfsState */
    Fs_FileID *fileIDPtr;		/* Handle on top-level directory */
    Fs_DomainInfo *domainInfoPtr;	/* Return information */
{
    NfsState *nfsPtr = (NfsState *)clientData;
    statfsres fsStat;

    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_STATFS, xdr_nfs_fh,
	    nfsPtr->mountHandle, xdr_statfsres, &fsStat, nfsTimeout)
		    != RPC_SUCCESS) {
	clnt_perror(nfsPtr->nfsClnt, "NFSPROC_STATFS");
	return(FAILURE);
    } else if (fsStat.status != NFS_OK) {
	return(fsStat.status);
    } else {
	domainInfoPtr->maxKbytes = fsStat.statfsres_u.reply.blocks *
				   fsStat.statfsres_u.reply.bsize / 1024;
	domainInfoPtr->freeKbytes = fsStat.statfsres_u.reply.bfree *
				   fsStat.statfsres_u.reply.bsize / 1024;
	domainInfoPtr->maxFileDesc = -1;
	domainInfoPtr->freeFileDesc = -1;
	domainInfoPtr->blockSize = fsStat.statfsres_u.reply.bsize;
	domainInfoPtr->optSize = fsStat.statfsres_u.reply.tsize;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NfsHandlePrint --
 *
 *	Print out an NFS handle.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NfsHandlePrint(handlePtr)
    nfs_fh *handlePtr;
{
    register int i;
    register int *intPtr = (int *)handlePtr;

    printf("<");
    for (i=0 ; i<FHSIZE ; i += sizeof(int)) {
	printf("%x,", *intPtr);
	intPtr++;
    }
    printf(">");
}

