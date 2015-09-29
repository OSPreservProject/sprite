/* 
 * stat.c --
 *
 *	Procedure to map from Unix *stat system calls to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/stat.c,v 1.6 89/11/20 12:57:52 mendel Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"

#include <sys/types.h>
#include <sys/stat.h>

static int CvtSpriteToUnixType();


/*
 *----------------------------------------------------------------------
 *
 * CvtSpriteToUnixAtts --
 *
 *	Procedure to convert the Sprite file system attributes 
 *	structure to the Unix format.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	 none.
 *
 *----------------------------------------------------------------------
 */

static void
CvtSpriteToUnixAtts(spriteAttsPtr, unixAttsPtr)
    register	struct stat	*unixAttsPtr;
    register	Fs_Attributes	*spriteAttsPtr;
{
    unixAttsPtr->st_dev		= spriteAttsPtr->domain;
    unixAttsPtr->st_ino		= spriteAttsPtr->fileNumber;
    unixAttsPtr->st_mode	= (spriteAttsPtr->permissions & 0xfff) |
				    CvtSpriteToUnixType(spriteAttsPtr->type);
    unixAttsPtr->st_nlink	= spriteAttsPtr->numLinks;
    unixAttsPtr->st_uid		= spriteAttsPtr->uid;
    unixAttsPtr->st_gid		= spriteAttsPtr->gid;
    unixAttsPtr->st_rdev	= (spriteAttsPtr->devType << 8) |
				  (spriteAttsPtr->devUnit & 0xff);
    unixAttsPtr->st_size	= spriteAttsPtr->size;
    unixAttsPtr->st_blksize	= spriteAttsPtr->blockSize;
    unixAttsPtr->st_blocks	= spriteAttsPtr->blocks * 2;
    unixAttsPtr->st_atime	= spriteAttsPtr->accessTime.seconds;
    unixAttsPtr->st_spare1	= 0;
    unixAttsPtr->st_mtime	= spriteAttsPtr->dataModifyTime.seconds;
    unixAttsPtr->st_spare2	= 0;
    unixAttsPtr->st_ctime	= spriteAttsPtr->descModifyTime.seconds;
    unixAttsPtr->st_spare3	= 0;
    unixAttsPtr->st_serverID	= spriteAttsPtr->serverID;
    unixAttsPtr->st_version	= spriteAttsPtr->version;
    unixAttsPtr->st_userType	= spriteAttsPtr->userType;
    unixAttsPtr->st_devServerID = spriteAttsPtr->devServerID;
}

/*
 *----------------------------------------------------------------------
 *
 * CvtSpriteToUnixType --
 *
 *	Convert from Sprite file types to the IFMT mode bits of a unix file
 *
 * Results:
 *	Unix file type bits.
 *
 * Side effects:
 *	 none.
 *
 *----------------------------------------------------------------------
 */

static int
CvtSpriteToUnixType(spriteFileType)
    register	int spriteFileType;
{
    register unixModeBits;

    switch (spriteFileType) {
	default:
	case FS_FILE:
	    unixModeBits = S_IFREG;
	    break;
	case FS_DIRECTORY:
	    unixModeBits = S_IFDIR;
	    break;
	case FS_SYMBOLIC_LINK:
	    unixModeBits = S_IFLNK;
	    break;
	case FS_DEVICE:
	case FS_REMOTE_DEVICE:		/* not used */
	    unixModeBits = S_IFCHR;
	    break;
	case FS_LOCAL_PIPE:		/* not used */
	case FS_NAMED_PIPE:
	    unixModeBits = S_IFIFO;
	    break;
	case FS_REMOTE_LINK:
	    unixModeBits = S_IFRLNK;
	    break;
	case FS_PSEUDO_DEV:
	    unixModeBits = S_IFPDEV;
	    break;
    }
    return(unixModeBits);
}


/*
 *----------------------------------------------------------------------
 *
 * stat --
 *
 *	Procedure to map from Unix stat system call to Sprite
 *	Fs_GetAttributes.  
 *
 * Results:
 *	UNIX_SUCCESS 	- the call was successful.
 *	UNIX_ERROR 	- the call was not successful. 
 *			  The actual error code stored in errno.  
 *
 * Side effects:
 *	The attributes of the specified file are stored in *attsBufPtr.
 *	Errno may be modified.
 *
 *----------------------------------------------------------------------
 */

int
stat(pathName, attsBufPtr)
    char *pathName;		/* The name of the file to stat */
    struct stat *attsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAtts;	/* buffer for attributes using
					   Sprite format. */

    status = Fs_GetAttributes(pathName, FS_ATTRIB_FILE, &spriteAtts);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	CvtSpriteToUnixAtts(&spriteAtts, attsBufPtr);
	return(UNIX_SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * lstat --
 *
 *	Procedure to map from Unix lstat system call to Sprite
 *	Fs_GetAttributes.  If the file specified is a symbolic link,
 *	follow the link and perform Fs_GetAttributes again.
 *
 * Results:
 *	UNIX_SUCCESS 	- the call was successful.
 *	UNIX_ERROR 	- the call was not successful. 
 *			  The actual error code stored in errno.  
 *
 * Side effects:
 *	The attributes of the specified file are stored in *attsBufPtr.
 *	Errno may be modified.
 *
 *----------------------------------------------------------------------
 */

int
lstat(pathName, attsBufPtr)
    char *pathName;		/* The name of the file to stat */
    struct stat *attsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAtts;	/* buffer for attributes using
					   Sprite format. */

    status = Fs_GetAttributes(pathName, FS_ATTRIB_LINK, &spriteAtts);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	CvtSpriteToUnixAtts(&spriteAtts, attsBufPtr);
	return(UNIX_SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * fstat --
 *
 *	Procedure to map from Unix fstat system call to Sprite
 *	Fs_GetAttributesID.  
 *
 * Results:
 *	UNIX_SUCCESS 	- the call was successful.
 *	UNIX_ERROR 	- the call was not successful. 
 *			  The actual error code stored in errno.  
 *
 * Side effects:
 *	The attributes of the specified file are stored in *attsBufPtr.
 *	Errno may be modified.
 *
 *----------------------------------------------------------------------
 */

int
fstat(fd, attsBufPtr)
    int fd;			/* Descriptor for file to stat */
    struct stat *attsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributesID */
    Fs_Attributes	spriteAtts;	/* buffer for attributes using
					   Sprite format. */

    status = Fs_GetAttributesID(fd,  &spriteAtts);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	CvtSpriteToUnixAtts(&spriteAtts, attsBufPtr);
	return(UNIX_SUCCESS);
    }
}
