/*
 * nfsAttr.c --
 * 
 *	Attribute handling for NFS access.
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
static char rcsid[] = "$Header: /sprite/src/cmds/nfsmount/RCS/nfsAttr.c,v 1.10 91/09/10 19:24:56 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

#include "nfs.h"
#include "sys/stat.h"
#include "kernel/fslcl.h"

typedef struct SavedCookie {
    int offset;
    nfscookie cookie;
    struct SavedCookie *next;
} SavedCookie;

void NfsToSpriteAttr();
void SpriteToNFsAttr();

/*
 *----------------------------------------------------------------------
 *
 * NfsToSpriteDirectory --
 *
 *	Convert from an XDR list of directory entries to a Sprite directory.
 *
 * Results:
 *	This fills in the buffer with a Sprite format directory.
 *
 * Side effects:
 *	This saves the last nfscookie for use with the next READIR rpc.
 *
 *----------------------------------------------------------------------
 */
void
NfsToSpriteDirectory(dirListPtr, offset, countPtr, buffer, fileIDPtr)
    dirlist *dirListPtr;
    int offset;
    int *countPtr;
    char *buffer;
    Fs_FileID *fileIDPtr;
{
    register int count = 0;
    register entry *entryPtr;
    register Fslcl_DirEntry *spriteEntryPtr;
    register int nameLength;
    entry *tempEntryPtr;

    /* At one time we checked the dirListPtr->eof flag and bailed
       out early if it was set.  Unfortunately, Sys V sets this
       flag even when it returns some directory entries to indicate
       that a subsequent request will fail.  So now we rely on 
       the dirListPtr->entries ptr being NULL to indicate no entries.
    */

    entryPtr = dirListPtr->entries;
    spriteEntryPtr = (Fslcl_DirEntry *)buffer;
    while (entryPtr != (entry *)NULL) {
	nameLength = strlen(entryPtr->name);
	spriteEntryPtr->fileNumber = entryPtr->fileid;
	spriteEntryPtr->nameLength = nameLength;
	strcpy(spriteEntryPtr->fileName, entryPtr->name);
	/* The xdr routines do implicit mallocs which were never
	   getting freed so I added these two lines. Maybe it 
	   should be done with XDR_FREE ? */
	free(entryPtr->name);
	entryPtr->name = (filename)NULL;
	if (entryPtr->nextentry != (entry *)NULL) {
	    spriteEntryPtr->recordLength = Fslcl_DirRecLength(nameLength);
	    count += spriteEntryPtr->recordLength;
	    spriteEntryPtr = (Fslcl_DirEntry *)((int)spriteEntryPtr +
					    spriteEntryPtr->recordLength);
	} else {
	    /* 
	     * Make the last entry go up to the end of the directory block.
	     * We save the last cookie to support sequential reading of the
	     * directory.  To be completely genernal we'd have to save the
	     * complete list of cookie/offset pairs.
	     */
	    register int extraRoom;
	    SavedCookie *savedCookie;

	    extraRoom = FSLCL_DIR_BLOCK_SIZE - (count % FSLCL_DIR_BLOCK_SIZE);
	    spriteEntryPtr->recordLength = extraRoom;
	    count += extraRoom;
	    /*
	     * Push a saved cookie onto a list hanging from the major field.
	     */
	    savedCookie = (SavedCookie *)malloc(sizeof(SavedCookie));
	    savedCookie->offset = offset + count;
	    bcopy((char *)&entryPtr->cookie, (char *)&savedCookie->cookie,
		    sizeof(nfscookie));
	    savedCookie->next = (SavedCookie *)fileIDPtr->major;
	    fileIDPtr->major = (int)savedCookie;
	}
	/* Also, the implicitly acquired dirlist space wasn't going away,
	   so we'll zap it here. JMS */
	tempEntryPtr = entryPtr;
	entryPtr = entryPtr->nextentry;
	free(tempEntryPtr);
    }
    *countPtr = count;
}

/*
 *----------------------------------------------------------------------
 *
 * NfsFindCookie --
 *
 *	Look through the list of savedCookie that are hung off the
 *	fileID.major field of a directory to find one that corresponds
 *	to the given offset.
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
NfsFindCookie(fileIDPtr, offset, cookiePtr)
    Fs_FileID *fileIDPtr;
    int offset;
    nfscookie *cookiePtr;
{
    register SavedCookie *savedCookiePtr;
    register SavedCookie *savedCookiePtrTrail = (SavedCookie *)NULL;
    int badCookie = -1;

    savedCookiePtr = (SavedCookie *)fileIDPtr->major;
    while (savedCookiePtr != (SavedCookie *)NULL) {
	if (savedCookiePtr->offset == offset) {
	    bcopy((char *)&savedCookiePtr->cookie, (char *)cookiePtr,
		sizeof(nfscookie));
	    /* Add code to release cookie so we don't lose heap space. JMS */
	    if (savedCookiePtrTrail == (SavedCookie *)NULL) {
		fileIDPtr->major = (int)savedCookiePtr->next;
	    } else {
		savedCookiePtrTrail->next = savedCookiePtr->next;
	    }
	    free((char *)savedCookiePtr);
	    return;
	}
	savedCookiePtrTrail = savedCookiePtr;
	savedCookiePtr = savedCookiePtr->next;
    }
    printf("NfsFindCookie: no directory cookie for offset %d\n");
    bcopy((char *)&badCookie, (char *)cookiePtr, sizeof(nfscookie));
}

/*
 *----------------------------------------------------------------------
 *
 * NfsGetAttrStream --
 *
 *	The default GetAttributes handling procedure called when an
 *	PDEV_GET_ATTR request is received over a request stream.
 *
 * Results:
 *	None.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
NfsGetAttrStream(streamPtr, spriteAttrPtr, selectBitsPtr)
    Pdev_Stream *streamPtr;
    Fs_Attributes *spriteAttrPtr;
    int *selectBitsPtr;
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    register int status;
    NfsState *nfsPtr;
    attrstat attrStat;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	handlePtr = nfsFileTable[fileIDPtr->minor]->handlePtr;
	nfsPtr = (NfsState *)fileIDPtr->serverID;
	nfsPtr->nfsClnt->cl_auth = nfsFileTable[fileIDPtr->minor]->authPtr;

	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_GETATTR, xdr_nfs_fh, handlePtr,
		    xdr_attrstat, &attrStat, nfsTimeout) != RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_GETATTR");
	    status = FAILURE;
	} else {
	    status = attrStat.status;
	    if (status == NFS_OK) {
		NfsToSpriteAttr(&attrStat.attrstat_u.attributes, spriteAttrPtr);
	    } else {
		status = NfsStatusMap(status);
	    }
	}
    } else {
	printf("NfsGetAttrStream: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	status = FAILURE;
    }
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(status);
}
/*
 *----------------------------------------------------------------------
 *
 * NfsToSpriteAttr --
 *
 *	Map from NFS to Sprite attributes.
 *
 * Results:
 *	Fills in the Sprite attributes from the NFS ones.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
NfsToSpriteAttr(nfsAttrPtr, spriteAttrPtr)
    register fattr *nfsAttrPtr;
    register Fs_Attributes *spriteAttrPtr;
{
    spriteAttrPtr->serverID		= -1;
    spriteAttrPtr->domain		= nfsAttrPtr->fsid;
    spriteAttrPtr->fileNumber		= nfsAttrPtr->fileid;
    spriteAttrPtr->type			= nfsToSpriteFileType[(int) nfsAttrPtr->type];
    spriteAttrPtr->size			= nfsAttrPtr->size;
    spriteAttrPtr->numLinks		= nfsAttrPtr->nlink;
    spriteAttrPtr->permissions		= nfsAttrPtr->mode & 07777;
    spriteAttrPtr->uid			= nfsAttrPtr->uid;
    spriteAttrPtr->gid			= nfsAttrPtr->gid;
    spriteAttrPtr->devServerID		= -1;
    if (nfsAttrPtr->type == NFBLK || nfsAttrPtr->type == NFCHR) {
	spriteAttrPtr->devType		= unix_major(nfsAttrPtr->rdev);
	spriteAttrPtr->devUnit		= unix_minor(nfsAttrPtr->rdev);
    } else {
	spriteAttrPtr->devType		= -1;
	spriteAttrPtr->devUnit		= -1;
    }
    spriteAttrPtr->createTime.seconds		= nfsAttrPtr->mtime.seconds;
    spriteAttrPtr->createTime.microseconds	= nfsAttrPtr->mtime.useconds;
    spriteAttrPtr->accessTime.seconds		= nfsAttrPtr->atime.seconds;
    spriteAttrPtr->accessTime.microseconds	= nfsAttrPtr->atime.useconds;
    spriteAttrPtr->descModifyTime.seconds	= nfsAttrPtr->ctime.seconds;
    spriteAttrPtr->descModifyTime.microseconds	= nfsAttrPtr->ctime.useconds;
    spriteAttrPtr->dataModifyTime.seconds	= nfsAttrPtr->mtime.seconds;
    spriteAttrPtr->dataModifyTime.microseconds	= nfsAttrPtr->mtime.useconds;
    /*
     * Sprite "blocks" means 1K.  Unix "blocks" mean 512 bytes.
     * In both cases, blocksize is the file system block size, which
     * has nothing to do with the units of the blocksize field!
     */
    spriteAttrPtr->blocks		= nfsAttrPtr->blocks / 2;
    spriteAttrPtr->blockSize		= nfsAttrPtr->blocksize;
    spriteAttrPtr->version		= nfsAttrPtr->mtime.seconds;
    spriteAttrPtr->userType		= 0;
}

/*
 *----------------------------------------------------------------------
 *
 * NfsSetAttrStream --
 *
 *	PDEV_SET_ATTR callback to set attributes of an open NFS file.
 *
 * Results:
 *	None.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
NfsSetAttrStream(streamPtr, flags, uid, gid, spriteAttrPtr, selectBitsPtr)
    Pdev_Stream *streamPtr;
    int flags;			/* What attributes to set */
    int uid;			/* UserID of process doing the set attr */
    int gid;			/* GroupID of process doing the set attr */
    Fs_Attributes *spriteAttrPtr;
    int *selectBitsPtr;
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    register int status;
    NfsState *nfsPtr;
    sattrargs sattrArgs;
    attrstat attrStat;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	handlePtr = nfsFileTable[fileIDPtr->minor]->handlePtr;
	nfsPtr = (NfsState *)fileIDPtr->serverID;
	nfsPtr->nfsClnt->cl_auth = nfsFileTable[fileIDPtr->minor]->authPtr;

	bcopy((char *)handlePtr, (char *)&sattrArgs.file, sizeof(nfs_fh));
	SpriteToNfsAttr(flags, spriteAttrPtr, &sattrArgs.attributes);
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_SETATTR, xdr_sattrargs,
		    &sattrArgs, xdr_attrstat, &attrStat, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_SETATTR");
	    status = FAILURE;
	} else {
	    status = NfsStatusMap(((int)attrStat.status));
	}
    } else {
	printf("NfsSetAttrStream: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	status = FAILURE;
    }
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * SpriteToNfsAttr --
 *
 *	Map from Sprite to NFS attributes.
 *
 * Results:
 *	Fills in the NFS attributes from the Sprite ones.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SpriteToNfsAttr(flags, spriteAttrPtr, nfsAttrPtr)
    register int flags;			/* Indicate which attrs to set */
    register Fs_Attributes *spriteAttrPtr;
    register sattr *nfsAttrPtr;
{
    if (flags & FS_SET_MODE) {
	nfsAttrPtr->mode = spriteAttrPtr->permissions & 07777;
    } else {
	nfsAttrPtr->mode = -1;
    }
    if (flags & FS_SET_OWNER) {
	nfsAttrPtr->uid		= spriteAttrPtr->uid;
	nfsAttrPtr->gid		= spriteAttrPtr->gid;
    } else {
	nfsAttrPtr->uid		= -1;
	nfsAttrPtr->gid		= -1;
    }
    nfsAttrPtr->size		= -1;		/* Not used for truncate */
    if (flags & FS_SET_TIMES) {
	nfsAttrPtr->atime.seconds  = spriteAttrPtr->accessTime.seconds;
	nfsAttrPtr->atime.useconds = spriteAttrPtr->accessTime.microseconds;
	nfsAttrPtr->mtime.seconds  = spriteAttrPtr->dataModifyTime.seconds;
	nfsAttrPtr->mtime.useconds = spriteAttrPtr->dataModifyTime.microseconds;
    } else {
	nfsAttrPtr->atime.seconds  = -1;
	nfsAttrPtr->atime.useconds = -1;
	nfsAttrPtr->mtime.seconds  = -1;
	nfsAttrPtr->mtime.useconds = -1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NfsCacheAttributes --
 *
 *	Cache NFS attributes.
 *
 * Results:
 *	None.
 *
 * Side effects
 *	Tucks away the attributes with a coarse date stamp.
 *
 *----------------------------------------------------------------------
 */
void
NfsCacheAttributes(fileIDPtr, nfsAttrPtr)
    register Fs_FileID *fileIDPtr;
    register fattr *nfsAttrPtr;
{
    return;
}

