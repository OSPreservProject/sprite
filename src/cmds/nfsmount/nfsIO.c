/*
 * nfsIO.c --
 * 
 *	I/O procedures for NFS files.
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
static char rcsid[] = "$Header: /sprite/src/cmds/nfsmount/RCS/nfsIO.c,v 1.13 91/09/10 19:24:27 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

#include "nfs.h"
#include "sys/stat.h"
#include "kernel/fsdm.h"
#include "sig.h"


/*
 * Service switch that the pdev library will use for the pseudo-device
 * connections to each NFS file.
 */
Pdev_CallBacks nfsFileService = {
    NULL, 			/* PDEV_OPEN - only for pseudo-devices */
    NfsRead,			/* PDEV_READ */
    NfsWrite,			/* PDEV_WRITE */
    NfsIoctl,			/* PDEV_IOCTL */
    NfsClose, 			/* PDEV_CLOSE */
    NfsGetAttrStream,		/* PDEV_GET_ATTR - called on open pfs streams */
    NfsSetAttrStream,		/* PDEV_SET_ATTR - called on open pfs streams */
};



/*
 *----------------------------------------------------------------------
 *
 * NfsClose --
 *
 *	Default procedure is called when an PDEV_CLOSE request is
 *	received over an service stream.
 *
 * Results:
 *	Returns SUCCESS and the select state of the pseudo-device.
 *
 * Side effects:
 *	Free's up the handle slot used to remember the open file.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
NfsClose(streamPtr)
    Pdev_Stream *streamPtr;
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    register int status = SUCCESS;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	if (nfsFileTable[fileIDPtr->minor] != (NfsOpenFile *)NULL) {
	    free((char *)nfsFileTable[fileIDPtr->minor]->handlePtr);
	    AUTH_DESTROY(nfsFileTable[fileIDPtr->minor]->authPtr);
	    free((char *)nfsFileTable[fileIDPtr->minor]);
	    nfsFileTable[fileIDPtr->minor] = (NfsOpenFile *)NULL;
	} else {
	    printf("NfsClose: no open file for file ID <%d,%d,%d,%d>\n",
		fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	    status = EINVAL;
	}
	free((char *)fileIDPtr);
    } else {
	printf("NfsClose: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	status = EINVAL;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsRead --
 *
 *	Read from an NFS file.
 *
 * Results:
 *	The number of bytes read.
 *
 * Side effects:
 *	Do the read.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
NfsRead(streamPtr, readPtr, freeItPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Private data */
    Pdev_RWParam *readPtr;	/* Return - how much data was generated */
    Boolean *freeItPtr;		/* In/Out indicates if *bufferPtr is malloc'd */
    int *selectBitsPtr;		/* Return - the select state of the pdev */
    Pdev_Signal *sigPtr;	/* Return - signal to return, if any */
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    register int status = NFS_OK;
    register int bytesToRead = readPtr->length;
    register int toRead;
    register Address buffer = readPtr->buffer;
    NfsState *nfsPtr;
    readargs readArgs;
    readres readResult;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	handlePtr = nfsFileTable[fileIDPtr->minor]->handlePtr;
	nfsPtr = (NfsState *)fileIDPtr->serverID;
	nfsPtr->nfsClnt->cl_auth = nfsFileTable[fileIDPtr->minor]->authPtr;
	if (fileIDPtr->type == TYPE_FILE) {
	    /*
	     * Reading from a regular file.  Use standard read RPC.
	     * Tell the XDR routines about our pre-allocated buffer.
	     */
	    readPtr->length = 0;
	    readArgs.totalcount = 0;	/* unused by protocol */
	    bcopy((char *)handlePtr, (char *)&readArgs.file, sizeof(nfs_fh));
	    while (bytesToRead > 0 && status == NFS_OK) {
		toRead = (bytesToRead > NFS_MAXDATA) ? NFS_MAXDATA :
							bytesToRead;
		readResult.readres_u.reply.data.data_len = toRead;
		readResult.readres_u.reply.data.data_val = buffer;
	
		readArgs.offset = readPtr->offset;
		readArgs.count = toRead;
	
		if (clnt_call(nfsPtr->nfsClnt, NFSPROC_READ, xdr_readargs,
			&readArgs, xdr_readres, &readResult, nfsTimeout)
			    != RPC_SUCCESS) {
		    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_READ");
		    status = EINVAL;
		} else {
		    status = readResult.status;
		    if (status == NFS_OK) {
			if (toRead > readResult.readres_u.reply.data.data_len) {
			    /*
			     * Short read.
			     */
			    readPtr->length +=
				readResult.readres_u.reply.data.data_len;
			    break;
			} else {
			    toRead = readResult.readres_u.reply.data.data_len;
			    readPtr->length += toRead;
			    readPtr->offset += toRead;
			    buffer += toRead;
			    bytesToRead -= toRead;
			}
		    } else {
			status = NfsStatusMap(status);
		    }
		}
	    }
	} else if (fileIDPtr->type == TYPE_SYMLINK) {
	    /*
	     * Use the NFS READLINK procedure to read the link value.
	     */
	    readlinkres		readLinkResult;

	    readLinkResult.readlinkres_u.data = buffer;
	    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_READLINK, xdr_nfs_fh,
			handlePtr, xdr_readlinkres,
			&readLinkResult, nfsTimeout) != RPC_SUCCESS) {
		clnt_perror(nfsPtr->nfsClnt, "NFSPROC_READLINK");
		status = EINVAL;
	    } else {
		status = readLinkResult.status;
		if (status == NFS_OK) {
		    readPtr->length = strlen(readLinkResult.readlinkres_u.data);
		} else {
		    status = NfsStatusMap(status);
		}
	    }
	} else {
	    /*
	     * We have to use the NFS READDIR procedure to read a directory.
	     * There are two tricks required.  First we have to
	     * save the nfscookie that is returned by the READDIR so we can
	     * use it again on the next call to read from the directory.  This
	     * trick only works with sequential reading of directories.  The
	     * second trick is no biggie, we just have to convert from the
	     * linked list returned by the XDR routines to a Sprite format
	     * directory.
	     */
	    readdirargs readDirArgs;
	    readdirres readDirResult;

	    bcopy((char *)handlePtr, (char *)&readDirArgs.dir, sizeof(nfs_fh));
	    if (readPtr->offset == 0) {
		bzero((char *)&readDirArgs.cookie, sizeof(nfscookie));
	    } else {
		NfsFindCookie(fileIDPtr, readPtr->offset, &readDirArgs.cookie);
	    }
	    readDirArgs.count = readPtr->length;
	    bzero((char *)&readDirResult, sizeof(readdirres));
	    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_READDIR, xdr_readdirargs,
		    &readDirArgs, xdr_readdirres, &readDirResult, nfsTimeout)
				!= RPC_SUCCESS) {
		clnt_perror(nfsPtr->nfsClnt, "NFSPROC_READDIR");
		status = EINVAL;
		readPtr->length = 0;
	    } else {
		status = readDirResult.status;
		if (status != NFS_OK) {
		    status = NfsStatusMap(status);
		    readPtr->length = 0;
		} else {
		    NfsToSpriteDirectory(&readDirResult.readdirres_u.reply,
			readPtr->offset, &readPtr->length, buffer, fileIDPtr);
		}
	    }
	}
    } else {
	printf("NfsRead: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	readPtr->length = 0;
	status = EINVAL;
    }
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsWrite --
 *
 *	Write to an NFS file.  This is a thin layer on top of the
 *	basic RPC.  This could be enhanced to pass the data block
 *	off to a subordinate writing process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
int
NfsWrite(streamPtr, async, writePtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Private data */
    int async;			/* ought not to be TRUE in NFS! */
    Pdev_RWParam *writePtr;	/* Information about the write and writer */
    int *selectBitsPtr;		/* Result - select state of the pseudo-device */
    Pdev_Signal *sigPtr;	/* Result - signal to return, if any */
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    NfsState *nfsPtr;
    register int status = NFS_OK;
    register int bytesToWrite;
    register int toWrite;
    int openFlags;
    writeargs writeArgs;
    attrstat attrStat;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	handlePtr = nfsFileTable[fileIDPtr->minor]->handlePtr;
	openFlags = nfsFileTable[fileIDPtr->minor]->openFlags;
	nfsPtr = (NfsState *)fileIDPtr->serverID;
	nfsPtr->nfsClnt->cl_auth = nfsFileTable[fileIDPtr->minor]->authPtr;

	if (openFlags & FS_APPEND) {
	    /*
	     * Find out how big the file in order to approximate
	     * append-mode writing.
	     */
	    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_GETATTR, xdr_nfs_fh,
			handlePtr, xdr_attrstat, &attrStat, nfsTimeout)
			    != RPC_SUCCESS) {
		clnt_perror(nfsPtr->nfsClnt, "NFSPROC_GETATTR");
		status = FAILURE;
		goto exit;
	    } else {
		status = attrStat.status;
		if (status == NFS_OK) {
		    writePtr->offset = attrStat.attrstat_u.attributes.size;
		} else {
		    status = NfsStatusMap(status);
		    goto exit;
		}
	    }
	}
	bcopy((char *)handlePtr, (char *)&writeArgs.file, sizeof(nfs_fh));
	bytesToWrite = writePtr->length;
	writePtr->length = 0;
	writeArgs.beginoffset = 0;	/* unused by NFS protocol */
	writeArgs.totalcount = 0;	/* unused by NFS protocol */
	while (bytesToWrite > 0 && status == NFS_OK) {
	    writeArgs.offset = writePtr->offset;
	    toWrite = (bytesToWrite > NFS_MAXDATA) ? NFS_MAXDATA : bytesToWrite;
	    writeArgs.data.data_len = toWrite;
	    writeArgs.data.data_val = writePtr->buffer;
    
	    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_WRITE, xdr_writeargs,
		    &writeArgs, xdr_attrstat, &attrStat, nfsTimeout)
			!= RPC_SUCCESS) {
		clnt_perror(nfsPtr->nfsClnt, "NFSPROC_WRITE");
		status = EINVAL;
	    } else {
		status = attrStat.status;
		if (status != NFS_OK) {
		    status = NfsStatusMap(status);
		} else {
		    NfsCacheAttributes(fileIDPtr,
					&attrStat.attrstat_u.attributes);
		    bytesToWrite -= toWrite;
		    writePtr->buffer += toWrite;
		    writePtr->length += toWrite;
		    writePtr->offset += toWrite;
		}
	    }
	}
	if (async && (bytesToWrite > 0)) {
	    fprintf(stderr, "Warning: short async NFS write\n");
	}
    } else {
	printf("NfsWrite: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	writePtr->length = 0;
	status = EINVAL;
    }
exit:
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsIoctl --
 *
 *	Take special actions on an NFS file.  This handles truncation,
 *	and could be modified to handle locking, some day, some how.
 *
 * Results:
 *	IOC_TRUNCATE maps to a SetAttributes with a short size.
 *
 * Side effects
 *	None to internal data structure.  The I/O controls have various
 *	effects on NFS files.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
NfsIoctl(streamPtr, ioctlPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;
    Pdev_IOCParam *ioctlPtr;
    int *selectBitsPtr;
    Pdev_Signal *sigPtr;
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    NfsState *nfsPtr;
    attrstat attrStat;
    int status;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	handlePtr = nfsFileTable[fileIDPtr->minor]->handlePtr;
	nfsPtr = (NfsState *)fileIDPtr->serverID;
	nfsPtr->nfsClnt->cl_auth = nfsFileTable[fileIDPtr->minor]->authPtr;

	switch (ioctlPtr->command) {
	    case IOC_PDEV_SIGNAL_OWNER: {
		/*
		 * Special test of IOC_PDEV_SIGNAL_REPLY.
		 * We have the user program ask for it!
		 */
		sigPtr->signal = SIG_SUSPEND;
		sigPtr->code = SIG_NO_CODE;
		status = SUCCESS;
		break;
	    }
	    case IOC_TRUNCATE: {
		sattrargs sattrArgs;

		if (ioctlPtr->inBufSize < sizeof(int) ||
		    ioctlPtr->inBuffer == NULL) {
		    status = GEN_INVALID_ARG;
		    break;
		}
		bcopy((char *)handlePtr, (char *)&sattrArgs.file,
			sizeof(nfs_fh));
		sattrArgs.attributes.mode = -1;
		sattrArgs.attributes.uid = -1;
		sattrArgs.attributes.gid = -1;
		sattrArgs.attributes.size = *(int *)ioctlPtr->inBuffer;
		sattrArgs.attributes.atime.seconds = -1;
		sattrArgs.attributes.atime.useconds = -1;
		sattrArgs.attributes.mtime.seconds = -1;
		sattrArgs.attributes.mtime.useconds = -1;
		if (clnt_call(nfsPtr->nfsClnt, NFSPROC_SETATTR, xdr_sattrargs,
			    &sattrArgs, xdr_attrstat, &attrStat, nfsTimeout)
				!= RPC_SUCCESS) {
		    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_SETATTR");
		    status = EINVAL;
		} else {
		    status = NfsStatusMap((int) attrStat.status);
		}
		break;
	    }
	    case IOC_GET_FLAGS:
	    case IOC_SET_FLAGS:
	    case IOC_SET_BITS:
	    case IOC_CLEAR_BITS:
	    case IOC_REPOSITION:
	    case IOC_GET_OWNER:
	    case IOC_SET_OWNER:
	    case IOC_WRITE_BACK:
		status = SUCCESS;
		break;
	    case IOC_LOCK:
	    case IOC_UNLOCK:
	    case IOC_MAP:
	    case IOC_NUM_READABLE:
	    default:
		status = GEN_NOT_IMPLEMENTED;
		break;
	}
    } else {
	printf("NfsIoctl: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
    }
    ioctlPtr->outBufSize = 0;
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(status);
}
