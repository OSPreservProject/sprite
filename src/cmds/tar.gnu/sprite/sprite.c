/*
 * sprite.c --
 *	Sprite dependent routines to make named pipes, pseudo devices, etc.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sprite.h"
#include "status.h"
#include "fs.h"
#include "tar.h"

/*
 * SpriteMakeNamedPipe --
 *	Create a named pipe under Sprite.  This uses the name from the
 *	tar record header, and the read-write-execute bits from the
 *	mode of the file.  This assumes that the tar tape was made
 *	under Sprite, or that the UNIX read-write-execute bits are
 *	the same as Sprite's, which is indeed the case.
 */
int
SpriteMakeNamedPipe(name, hstat)
    char *name;
    struct stat *hstat;
{
    ReturnStatus status;
    int streamID;
    int flags;

    if (f_keep) {
	flags = FS_CREATE|FS_NAMED_PIPE_OPEN|FS_EXCLUSIVE;
    } else {
	flags = FS_CREATE|FS_NAMED_PIPE_OPEN;
    }

    status = Fs_Open(name, flags, hstat->st_mode & 0777, &streamID);
    if (status == SUCCESS) {
	(void)Fs_Close(streamID);
	return(0);
    } else {
	/*
	 * Let our caller deal with errors.  It may need to make
	 * parent directories, etc.
	 */
	errno = Compat_MapCode(status);
	return(-1);
     }
}

/*
 * SpriteMakePseudoDev --
 *	Create a pseudo device under Sprite.  This uses the name from the
 *	tar record header, and the read-write-execute bits from the
 *	mode of the file.  This assumes that the tar tape was made
 *	under Sprite, or that the UNIX read-write-execute bits are
 *	the same as Sprite's, which is indeed the case.
 */
int
SpriteMakePseudoDev(name, hstat)
    char *name;
    struct stat *hstat;
{
    ReturnStatus status;
    int streamID;
    int flags;

    if (f_keep) {
	flags = FS_CREATE|FS_PDEV_MASTER|FS_EXCLUSIVE;
    } else {
	flags = FS_CREATE|FS_PDEV_MASTER;
    }

    status = Fs_Open(name, flags, hstat->st_mode & 0777, &streamID);
    if (status == SUCCESS) {
	(void)Fs_Close(streamID);
	return(0);
    } else {
	/*
	 * Let our caller deal with errors.  It may need to make
	 * parent directories, etc.
	 */
	errno = Compat_MapCode(status);
	return(-1);
     }
}

/*
 * SpriteMakeRemoteLink --
 *	Create a remote link under Sprite.  This uses the name from the
 *	tar record header, and the read-write-execute bits from the
 *	mode of the file.  This assumes that the tar tape was made
 *	under Sprite, or that the UNIX read-write-execute bits are
 *	the same as Sprite's, which is indeed the case.
 */
int
SpriteMakeRemoteLink(linkname, name)
    char *linkname;	/* Name referred to by link */
    char *name;		/* Name of file created */
{
    ReturnStatus status;

    status = Fs_SymLink(linkname, name, TRUE);
    if (status == SUCCESS) {
	return(0);
    } else {
	/*
	 * Let our caller deal with errors.  It may need to make
	 * parent directories, etc.
	 */
	errno = Compat_MapCode(status);
	return(-1);
     }
}

