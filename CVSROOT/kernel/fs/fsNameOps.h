/*
 * fsNameOps.h --
 *
 *	Definitions for pathname related operations.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSNAMEOPS
#define _FSNAMEOPS

#include "fsFile.h"
#include "fsDevice.h"
#include "fsPdev.h"
#include "fsRpcInt.h"

/*
 * The arguments and results of the various lookup operations have to
 * be packaged into a struct so they can be passed through FsLookupOperation()
 * and into the domain specific lookup routine.  The following typedefs
 * are for those arguments and results.
 */

/*
 * FS_DOMAIN_OPEN arguments.  The results of a lookup is a stream-type
 * specific chunk of data used to set up the I/O handle.
 * FS_DOMAIN_PREFIX also gets FsOpenArgs, although it installs a complete
 * handle and returns a pointer to that.
 */

typedef struct FsOpenArgs {
    FsFileID	prefixID;	/* File ID from prefix handle, MUST BE FIRST */
    FsFileID	rootID;		/* File ID of root.  MUST FOLLOW prefix ID.
				 * Used to trap ".." past the root. */
    int		useFlags;	/* Flags defined in fs.h */
    int		permissions;	/* Permission bits for created files.  Already
				 * reflects per-process permission mask */
    int		type;		/* Used to contrain open to a specific type */
    int		clientID;	/* Host ID of client doing the open */
    FsUserIDs	id;		/* User and group IDs */
} FsOpenArgs;

typedef struct FsOpenResults {
    FsFileID	ioFileID;	/* FileID used to get to I/O server.  This is
				 * set by the name server, although the I/O
				 * server has the right to modify the major
				 * and minor numbers */
    FsFileID	streamID;	/* File ID of the stream being opened */
    FsFileID	nameID;		/* FileID used to get to the name of the file.*/
    int		dataSize;	/* Size of extra streamData */
    ClientData	streamData;	/* Pointer to stream specific extra data */
} FsOpenResults;

/*
 * Rpc storage reply parameter for both redirected and unredirected calls.
 */
typedef	struct	FsOpenResultsParam {
    int			prefixLength;
    FsOpenResults	openResults;
    FsUnionData		openData;
} FsOpenResultsParam;

/*
 * FS_DOMAIN_LOOKUP arguments and results.
 */
typedef struct FsLookupArgs {
    FsFileID prefixID;	/* FileID of the prefix, MUST BE FIRST */
    FsFileID rootID;	/* FileID of the root, MUST FOLLOW prefixID */
    int useFlags;	/* FS_EXECUTE or FS_RENAME */
    FsUserIDs id;	/* User and group IDs */
    int clientID;	/* Needed to expand $MACHINE */
} FsLookupArgs;

/*
 * FS_DOMAIN_GET_ATTR results.
 */
typedef struct FsGetAttrResults {
    FsFileID		*fileIDPtr;	/* File ID that indicates I/O server */
    Fs_Attributes	*attrPtr;	/* Returned results */
} FsGetAttrResults;

/*
 * Rpc storage reply parameter for both redirected and unredirected calls.
 */
typedef	union	FsGetAttrResultsParam {
    int	prefixLength;
    struct	AttrResults {
	FsFileID	fileID;
	Fs_Attributes	attrs;
    } attrResults;
} FsGetAttrResultsParam;

/*
 * FS_DOMAIN_SET_ATTR arguments.
 */
typedef struct FsSetAttrArgs {
    FsOpenArgs		openArgs;
    Fs_Attributes	attr;
    int			flags;	/* Set attr flags defined in user/fs.h */
} FsSetAttrArgs;

/*
 * FS_DOMAIN_MAKE_DEVICE arguments and results.
 */
typedef struct FsMakeDeviceArgs {
    FsFileID prefixID;	/* FileID of the prefix, MUST BE FIRST */
    FsFileID rootID;	/* FileID of the root, MUST BE SECOND */
    Fs_Device device;
    int permissions;	/* Permissions already reflect per-process mask */
    FsUserIDs id;
    int clientID;
} FsMakeDeviceArgs;

/*
 * All pathname operations may potentially return new prefix information
 * from the server, or redirected lookups.
 */
typedef struct FsRedirectInfo {
    int	prefixLength;		/* The length of the prefix embedded in
				 * fileName.  This is used when a server hits
				 * a remote link and has to return a new file
				 * name plus an indication of a new prefix. */
    char fileName[FS_MAX_PATH_NAME_LENGTH];	/* A new file name.  Returned
				 * from the server when its lookup is about
				 * to leave its domain. */
} FsRedirectInfo;


/*
 * Forward references.
 */
extern void FsSetIDs();

#endif _FSNAMEOPS
