/*
 * fsNameOps.h --
 *
 *	Definitions for pathname related operations.  These data structures
 *	define the file system's naming interface.  This is used internally,
 *	for network RPCs, and for the interface to pseudo-file-systems.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSNAMEOPS
#define _FSNAMEOPS

/*
 * The arguments and results of the various lookup operations have to
 * be packaged into a struct so they can be passed through FsLookupOperation()
 * and into the domain specific lookup routine.  The following typedefs
 * are for those arguments and results.
 *
 * Pseudo-file-systems:  These structures are also used in the interface
 *	to pseudo-file-system servers.  See <dev/pfs.h>
 */

/*
 * 	FsOpenArgs are used for the DOMAIN_OPEN, DOMAIN_GET_ATTR,
 *	DOMAIN_SET_ATTR, DOMAIN_MAKE_DIR and DOMAIN_MAKE_DEVICE operations.
 *	(Actually, FsMakeDeviceArgs & FsSetAttrArgs embed FsOpenArgs)
 *
 *	The arguments for open specify the starting point of the lookup,
 *	then the root file of the lookup domain, then other parameters
 *	identifying the client and its intended use of the file.
 *
 *	The results of the open fileIDs for the I/O server, the name server,
 *	and the top-level stream to the file.  There is also some data that
 *	is specific to the file type
 *
 */

typedef struct FsOpenArgs {
    Fs_FileID	prefixID;	/* File ID from prefix handle, MUST BE FIRST */
    Fs_FileID	rootID;		/* File ID of root.  MUST FOLLOW prefix ID.
				 * Used to trap ".." past the root. */
    int		useFlags;	/* Flags defined in fs.h */
    int		permissions;	/* Permission bits for created files.  Already
				 * reflects per-process permission mask */
    int		type;		/* Used to contrain open to a specific type */
    int		clientID;	/* True Host ID of client doing the open */
    int		migClientID;	/* Logical host ID if migrated (the home node)*/
    Fs_UserIDs	id;		/* User and group IDs */
} FsOpenArgs;

typedef struct FsOpenResults {
    Fs_FileID	ioFileID;	/* FileID used to get to I/O server.  This is
				 * set by the name server, although the I/O
				 * server has the right to modify the major
				 * and minor numbers */
    Fs_FileID	streamID;	/* File ID of the stream being opened */
    Fs_FileID	nameID;		/* FileID used to get to the name of the file.*/
    int		dataSize;	/* Size of extra streamData */
    ClientData	streamData;	/* Pointer to stream specific extra data */
} FsOpenResults;

/*
 * FsLookupArgs are used for the DOMAIN_REMOVE and DOMAIN_REMOVE_DIR
 * operations.  Also, Fs2PathParams embeds FsLookupArgs.
 */
typedef struct FsLookupArgs {
    Fs_FileID prefixID;	/* FileID of the prefix, MUST BE FIRST */
    Fs_FileID rootID;	/* FileID of the root, MUST FOLLOW prefixID */
    int useFlags;	/* FS_EXECUTE or FS_RENAME */
    Fs_UserIDs id;	/* User and group IDs */
    int clientID;	/* True HostID, needed to expand $MACHINE */
    int migClientID;	/* Logical host ID if migrated (the home node) */
} FsLookupArgs;

/*
 * FS_DOMAIN_GET_ATTR results.
 */
typedef struct FsGetAttrResults {
    Fs_FileID		*fileIDPtr;	/* File ID that indicates I/O server */
    Fs_Attributes	*attrPtr;	/* Returned results */
} FsGetAttrResults;

/*
 * Rpc storage reply parameter for both redirected and unredirected calls.
 */
typedef	union	FsGetAttrResultsParam {
    int	prefixLength;
    struct	AttrResults {
	Fs_FileID	fileID;
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
    FsOpenArgs open;
    Fs_Device device;
} FsMakeDeviceArgs;

/*
 * FS_DOMAIN_RENAME and FS_DOMAIN_HARD_LINK
 */
typedef struct Fs2PathParams {
    FsLookupArgs	lookup;
    Fs_FileID		prefixID2;
} Fs2PathParams;

typedef struct Fs2PathData {
    char		path1[FS_MAX_PATH_NAME_LENGTH];
    char		path2[FS_MAX_PATH_NAME_LENGTH];
} Fs2PathData;

typedef struct Fs2PathReply {
    int		prefixLength;	/* Length of returned prefix on re-direct */
    Boolean	name1ErrorP;	/* TRUE if the error returned, which is either
				 * a re-direct or stale-handle, applies to
				 * the first of the two pathnames, FALSE if
				 * it applies to the second pathname */
} Fs2PathReply;

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

typedef struct Fs2PathRedirectInfo {
    int name1ErrorP;		/* TRUE if redirection or other error applies
				 * to the first pathname, FALSE if the error
				 * applies to second pathname, or no error */
    int	prefixLength;		/* The length of the prefix embedded in
				 * fileName.  This is used when a server hits
				 * a remote link and has to return a new file
				 * name plus an indication of a new prefix. */
    char fileName[FS_MAX_PATH_NAME_LENGTH];	/* A new file name.  Returned
				 * from the server when its lookup is about
				 * to leave its domain. */
} Fs2PathRedirectInfo;


/*
 * Forward references.
 */
extern void FsSetIDs();

#endif /* _FSNAMEOPS */
