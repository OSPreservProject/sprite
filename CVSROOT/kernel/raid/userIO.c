/* 
 * userIO.c --
 *
 *	Emulate C library IO routines for use in kernel.
 *
 * Copyright 1990 Regents of the University of California
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
#include "/sprite/src/lib/c/stdio/fileInt.h"
#include "/sprite/src/lib/c/unixSyscall/compatInt.h"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#ifndef TESTING

Fs_Stream *
open(pathName, unixFlags, permissions)
    char *pathName;		/* The name of the file to open */
    register int unixFlags;	/* O_RDONLY O_WRONLY O_RDWR O_NDELAY
				 * O_APPEND O_CREAT O_TRUNC O_EXCL */
    int permissions;		/* Permission mask to use on creation */
{
    Fs_Stream *streamId;		/* place to hold stream id allocated by
				 * Fs_Open */
    ReturnStatus status;	/* result returned by Fs_Open */
    register int useFlags = 0;	/* Sprite version of flags */

    /*
     * Convert unixFlags to FS_READ, etc.
     */
     
    if (unixFlags & FASYNC) {
	fprintf(stderr, "open - FASYNC not supported\n");
	errno = EINVAL;
	return((Fs_Stream *) UNIX_ERROR);
    }
    if (unixFlags & O_RDWR) {
	useFlags |= FS_READ|FS_WRITE;
    } else if (unixFlags & O_WRONLY) {
	useFlags |= FS_WRITE;
    } else {
	useFlags |= FS_READ;
    }
    if (unixFlags & FNDELAY) {
	useFlags |= FS_NON_BLOCKING;
    }
    if (unixFlags & FAPPEND) {
	useFlags |= FS_APPEND;
    }
    if (unixFlags & FTRUNC) {
	useFlags |= FS_TRUNC;
    }
    if (unixFlags & FEXCL) {
	useFlags |= FS_EXCLUSIVE;
    }
    if (unixFlags & O_MASTER) {
	useFlags |= FS_PDEV_MASTER;
    }
    if (unixFlags & O_PFS_MASTER) {
	useFlags |= FS_PFS_MASTER;
    }
    if (unixFlags & FCREAT) {
	useFlags |= FS_CREATE;
    }

    status = Fs_Open(pathName, useFlags, FS_FILE, permissions, &streamId);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return((Fs_Stream *) UNIX_ERROR);
    } else {
	return(streamId);
    }
}

int
read(streamPtr, buffer, numBytes)
    Fs_Stream *streamPtr;             /* descriptor for stream to read */
    char *buffer;               /* pointer to buffer area */
    int numBytes;               /* number of bytes to read */
{
    ReturnStatus status;        /* result returned by Fs_Read */
    int amountRead = numBytes;  /* place to hold number of bytes read */

    status=Fs_Read(streamPtr, (Address) buffer, streamPtr->offset, &amountRead);
    if (status != SUCCESS) {
        errno = Compat_MapCode(status);
        return(UNIX_ERROR);
    } else { 
        return(amountRead);
    }
}

int
write(streamPtr, buffer, numBytes)
    Fs_Stream *streamPtr;             /* descriptor for stream to read */
    char *buffer;               /* pointer to buffer area */
    int numBytes;               /* number of bytes to read */
{
    ReturnStatus status;        /* result returned by Fs_Read */
    int amountwritten = numBytes;/* place to hold number of bytes written */

    status=Fs_Write(streamPtr,
	    (Address) buffer, streamPtr->offset, &amountwritten);
    if (status != SUCCESS) {
        errno = Compat_MapCode(status);
        return(UNIX_ERROR);
    } else { 
        return(amountwritten);
    }
}

/*
int
close(streamPtr)
    Fs_Stream *streamPtr;
{
    ReturnStatus status;

    status = Fs_Close(streamPtr);
    if (status != SUCCESS) {
        errno = Compat_MapCode(status);
        return(UNIX_ERROR);
    } else {
        return(UNIX_SUCCESS);
    }
} 

int
rename(from, to)
    char *from;
    char *to;
{
    ReturnStatus status;

    status = Fs_Rename(from, to);
    if (status != SUCCESS) {
        errno = Compat_MapCode(status);
        return(UNIX_ERROR);
    } else {
        return(UNIX_SUCCESS);
    } 
}

int
unlink(pathName)
    char *pathName;
{
    ReturnStatus status;

    status = Fs_Remove(pathName);
    if (status != SUCCESS) {
        errno = Compat_MapCode(status);
        return(UNIX_ERROR);
    } else {
        return(UNIX_SUCCESS);
    }
}
*/

FILE *
fdopen(streamID, access)
    Fs_Stream *streamID;	/* Index of a stream identifier, such as
				 * returned by open.  */
    char *access;		/* Indicates the type of access to be
				 * made on streamID, just as in fopen.
				 * Must match the permissions on streamID.  */
{
    register FILE * 	stream;
    int 		read, write;
    struct stat fsStat;

    if (streamID == (Fs_Stream *) NIL) {
	return NULL;
    }

    stream = (FILE *) malloc(sizeof(FILE));

    read = write = 0;
    if ((access[1] == '+') || ((access[1] == 'b') && (access[2] == '+'))) {
	read = write = 1;
    } else if (access[0]  == 'r') {
	read = 1;
    } else {
	write = 1;
    }

    /*
     * Seek to the end of the file if we're in append mode (I'm not sure
     * this should be necessary, but UNIX programs seem to depend on it).
     */
    if (access[0] == 'a') {
	if (fstat(streamID, &fsStat) < 0) {
	    return (FILE *) NULL;
	}
	streamID->offset = fsStat.st_size;
    }

    Stdio_Setup(stream, read, write, stdioTempBuffer, 0,
            StdioFileReadProc, StdioFileWriteProc, StdioFileCloseProc,
            (ClientData) streamID);
    return(stream);
}

FILE *
fopen(fileName, access)
    char *fileName;		/* Name of file to be opened. */
    char *access;		/* Indicates type of access:  "r" for reading,
				 * "w" for writing, "a" for appending, "r+"
				 * for reading and writing, "w+" for reading
				 * and writing with initial truncation, "a+"
				 * for reading and writing with initial
				 * position at the end of the file.  The
				 * letter "b" may also appear in the string,
				 * for ANSI compatibility, but only after
				 * the first letter.  It is ignored. */
{
    Fs_Stream 	*streamID;
    int 	flags;

    flags = StdioFileOpenMode(access);
    if (flags == -1) {
	return (FILE *) NULL;
    }

    streamID = open(fileName, flags, 0666);
    if (streamID == (Fs_Stream *) NIL) {
	return (FILE *) NULL;
    }

    /*
     * Initialize the stream structure.
     */
    return fdopen(streamID, access);
}

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

int
fstat(fd, attsBufPtr)
    int fd;			/* Descriptor for file to stat */
    struct stat *attsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributesID */
    Fs_Attributes	spriteAtts;	/* buffer for attributes using
					   Sprite format. */

    status = Fs_GetAttributes(fd, FS_ATTRIB_FILE, &spriteAtts);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	CvtSpriteToUnixAtts(&spriteAtts, attsBufPtr);
	return(UNIX_SUCCESS);
    }
}

#endif TESTING
