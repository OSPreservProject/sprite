/* 
 * cvtStat.c --
 *
 *	Procedure to map from Unix *stat system calls to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "user/fs.h"
#include "compatInt.h"

#include "user/sys/types.h"
#include "stat.h"


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

ReturnStatus
CvtSpriteToUnixAtts(spriteAttsPtr, unixAttsPtr)
    register	Fs_Attributes	*spriteAttsPtr;
    register	struct stat	*unixAttsPtr;
{
    struct stat unixAtts;

    unixAtts.st_dev		= spriteAttsPtr->domain;
    unixAtts.st_ino		= spriteAttsPtr->fileNumber;
    unixAtts.st_mode	= spriteAttsPtr->permissions |
				    CvtSpriteToUnixType(spriteAttsPtr->type);
    unixAtts.st_nlink	= spriteAttsPtr->numLinks;
    unixAtts.st_uid		= spriteAttsPtr->uid;
    unixAtts.st_gid		= spriteAttsPtr->gid;
    unixAtts.st_rdev	= (spriteAttsPtr->devType << 8) |
				  (spriteAttsPtr->devUnit & 0xff);
    unixAtts.st_size	= spriteAttsPtr->size;
    unixAtts.st_blksize	= spriteAttsPtr->blockSize;
    unixAtts.st_blocks	= spriteAttsPtr->blocks * 2;
    unixAtts.st_atime	= spriteAttsPtr->accessTime.seconds;
    unixAtts.st_mtime	= spriteAttsPtr->dataModifyTime.seconds;
    unixAtts.st_ctime	= spriteAttsPtr->descModifyTime.seconds;
    unixAtts.st_serverID = spriteAttsPtr->serverID;
    unixAtts.st_version	= spriteAttsPtr->version;

    return(Vm_CopyOut(sizeof(unixAtts), &unixAtts, unixAttsPtr));
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
