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
 *
 *	There are twp operation switch tables defined here.  (See fsOpTable.c
 *	for their initialization.)
 *	1. The DOMAIN table used for naming operations like OPEN or REMOVE_DIR.
 *		These operations take file names as arguments and have to
 *		be pre-processed by the prefix table module in order to
 *		chose the correct domain type and server.
 *	2. The ATTR table is used when getting/setting attributes when
 *		starting with an open stream (not with a file name).  This
 *		is keyed on the type of the nameFileID in the stream.
 *
 * Name Domain Types:
 *
 *	FS_LOCAL_DOMAIN		The file is stored locally.
 *	FS_REMOTE_SPRITE_DOMAIN	The file is stored on a Sprite server.
 *	FS_PSEUDO_DOMAIN	The file system is implemented by
 *				a user-level server process
 *	FS_NFS_DOMAIN		The file is stored on an NFS server.
 *
 */

#define FS_LOCAL_DOMAIN			0
#define FS_REMOTE_SPRITE_DOMAIN		1
#define FS_PSEUDO_DOMAIN		2
#define FS_NFS_DOMAIN			3

#define FS_NUM_DOMAINS			4

/*
 * DOMAIN SWITCH
 * Domain specific operations that operate on file names for lookup.
 * Naming operations are done through Fsprefix_LookupOperation, which uses
 * the prefix table to choose the domain type and the server for the name.
 * It then branches through the fs_DomainLookup table to complete the operation.
 * The arguments to these operations are documented in fsNameOps.h
 * because they are collected into structs (declared in fsNameOps.h)
 * and passed through Fsprefix_LookupOperation() to domain-specific routines.
 */

#define	FS_DOMAIN_IMPORT		0
#define	FS_DOMAIN_EXPORT		1
#define	FS_DOMAIN_OPEN			2
#define	FS_DOMAIN_GET_ATTR		3
#define	FS_DOMAIN_SET_ATTR		4
#define	FS_DOMAIN_MAKE_DEVICE		5
#define	FS_DOMAIN_MAKE_DIR		6
#define	FS_DOMAIN_REMOVE		7
#define	FS_DOMAIN_REMOVE_DIR		8
#define	FS_DOMAIN_RENAME		9
#define	FS_DOMAIN_HARD_LINK		10

#define	FS_NUM_NAME_OPS			11

extern	ReturnStatus (*fs_DomainLookup[FS_NUM_DOMAINS][FS_NUM_NAME_OPS])();

/*
 * ATTRIBUTE SWITCH
 * A switch is used to get to the name server in set/get attributesID,
 * which take an open stream.  The stream refereneces a nameFileID, and
 * this switch is keyed on the nameFileID.type (i.e. local or remote file).
 */

#ifdef SOSP91
typedef struct Fs_AttrOps {
    ReturnStatus	(*getAttr) _ARGS_((Fs_FileID *fileIDPtr, int clientID,
						Fs_Attributes *attrPtr,
						int hostID, int userID));

    ReturnStatus	(*setAttr) _ARGS_((Fs_FileID *fileIDPtr, 
					  Fs_Attributes *attrPtr, 
					  Fs_UserIDs *idPtr, int flags,
					  int hostID, int userID));
} Fs_AttrOps;
#else
typedef struct Fs_AttrOps {
    ReturnStatus	(*getAttr) _ARGS_((Fs_FileID *fileIDPtr, int clientID,
						Fs_Attributes *attrPtr));

    ReturnStatus	(*setAttr) _ARGS_((Fs_FileID *fileIDPtr, 
					  Fs_Attributes *attrPtr, 
					  Fs_UserIDs *idPtr, int flags));
} Fs_AttrOps;
#endif

extern Fs_AttrOps fs_AttrOpTable[];


/*
 * The arguments and results of the various lookup operations have to
 * be packaged into a struct so they can be passed through Fsprefix_LookupOperation()
 * and into the domain specific lookup routine.  The following typedefs
 * are for those arguments and results.
 *
 * Pseudo-file-systems:  These structures are also used in the interface
 *	to pseudo-file-system servers.  See <dev/pfs.h>
 */

/*
 * 	Fs_OpenArgs are used for the DOMAIN_OPEN, DOMAIN_GET_ATTR,
 *	DOMAIN_SET_ATTR, DOMAIN_MAKE_DIR and DOMAIN_MAKE_DEVICE operations.
 *	(Actually, Fs_MakeDeviceArgs & Fs_SetAttrArgs embed Fs_OpenArgs)
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

typedef struct Fs_OpenArgs {
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
} Fs_OpenArgs;

#ifdef SOSP91
typedef struct Fs_OpenArgsSOSP {
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
    int		realID;
} Fs_OpenArgsSOSP;
#endif

typedef struct Fs_OpenResults {
    Fs_FileID	ioFileID;	/* FileID used to get to I/O server.  This is
				 * set by the name server, although the I/O
				 * server has the right to modify the major
				 * and minor numbers */
    Fs_FileID	streamID;	/* File ID of the stream being opened */
    Fs_FileID	nameID;		/* FileID used to get to the name of the file.*/
    int		dataSize;	/* Size of extra streamData */
    ClientData	streamData;	/* Pointer to stream specific extra data */
} Fs_OpenResults;

/*
 * Fs_LookupArgs are used for the DOMAIN_REMOVE and DOMAIN_REMOVE_DIR
 * operations.  Also, Fs_2PathParams embeds Fs_LookupArgs.
 */
typedef struct Fs_LookupArgs {
    Fs_FileID prefixID;	/* FileID of the prefix, MUST BE FIRST */
    Fs_FileID rootID;	/* FileID of the root, MUST FOLLOW prefixID */
    int useFlags;	/* FS_EXECUTE or FS_RENAME */
    Fs_UserIDs id;	/* User and group IDs */
    int clientID;	/* True HostID, needed to expand $MACHINE */
    int migClientID;	/* Logical host ID if migrated (the home node) */
} Fs_LookupArgs;

/*
 * FS_DOMAIN_GET_ATTR results.
 */
typedef struct Fs_GetAttrResults {
    Fs_FileID		*fileIDPtr;	/* File ID that indicates I/O server */
    Fs_Attributes	*attrPtr;	/* Returned results */
} Fs_GetAttrResults;

/*
 * Rpc storage reply parameter for both redirected and unredirected calls.
 */
typedef	union	Fs_GetAttrResultsParam {
    int	prefixLength;
    struct	AttrResults {
	Fs_FileID	fileID;
	Fs_Attributes	attrs;
    } attrResults;
} Fs_GetAttrResultsParam;

/*
 * FS_DOMAIN_SET_ATTR arguments.
 */
typedef struct Fs_SetAttrArgs {
    Fs_OpenArgs		openArgs;
    Fs_Attributes	attr;
    int			flags;	/* Set attr flags defined in user/fs.h */
} Fs_SetAttrArgs;

/*
 * FS_DOMAIN_MAKE_DEVICE arguments and results.
 */
typedef struct Fs_MakeDeviceArgs {
    Fs_OpenArgs open;
    Fs_Device device;
} Fs_MakeDeviceArgs;

/*
 * FS_DOMAIN_RENAME and FS_DOMAIN_HARD_LINK
 */
typedef struct Fs_2PathParams {
    Fs_LookupArgs	lookup;
    Fs_FileID		prefixID2;
} Fs_2PathParams;

typedef struct Fs_2PathData {
    char		path1[FS_MAX_PATH_NAME_LENGTH];
    char		path2[FS_MAX_PATH_NAME_LENGTH];
} Fs_2PathData;

typedef struct Fs_2PathReply {
    int		prefixLength;	/* Length of returned prefix on re-direct */
    Boolean	name1ErrorP;	/* TRUE if the error returned, which is either
				 * a re-direct or stale-handle, applies to
				 * the first of the two pathnames, FALSE if
				 * it applies to the second pathname */
} Fs_2PathReply;

/*
 * All pathname operations may potentially return new prefix information
 * from the server, or redirected lookups.
 */
typedef struct Fs_RedirectInfo {
    int	prefixLength;		/* The length of the prefix embedded in
				 * fileName.  This is used when a server hits
				 * a remote link and has to return a new file
				 * name plus an indication of a new prefix. */
    char fileName[FS_MAX_PATH_NAME_LENGTH];	/* A new file name.  Returned
				 * from the server when its lookup is about
				 * to leave its domain. */
} Fs_RedirectInfo;

typedef struct Fs_2PathRedirectInfo {
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
} Fs_2PathRedirectInfo;


/*
 * Table of name lookup routine maintained by each domain type.
 */
typedef struct Fs_DomainLookupOps {
     ReturnStatus (*import) _ARGS_((char *prefix, int serverID, 
		    Fs_UserIDs *idPtr, int *domainTypePtr,
		    Fs_HandleHeader **hdrPtrPtr));
     ReturnStatus (*export) _ARGS_((Fs_HandleHeader *hdrPtr, int clientID,
		    Fs_FileID *ioFileIDPtr, int *dataSizePtr, 
		    ClientData *clientDataPtr));
     ReturnStatus (*open) _ARGS_((Fs_HandleHeader *prefixHandlePtr,
		    char *relativeName, Address argsPtr, Address resultsPtr, 
		    Fs_RedirectInfo **newNameInfoPtrPtr));
     ReturnStatus (*getAttrPath) _ARGS_((Fs_HandleHeader *prefixHandlePtr,
		    char *relativeName, Address argsPtr, Address resultsPtr,
		    Fs_RedirectInfo **newNameInfoPtrPtr));
     ReturnStatus (*setAttrPath) _ARGS_((Fs_HandleHeader *prefixHandlePtr,
		    char *relativeName, Address argsPtr, Address resultsPtr,
		    Fs_RedirectInfo **newNameInfoPtrPtr));
     ReturnStatus (*makeDevice) _ARGS_((Fs_HandleHeader *prefixHandle, 
		    char *relativeName, Address argsPtr, Address resultsPtr,
		    Fs_RedirectInfo **newNameInfoPtrPtr));
     ReturnStatus (*makeDir) _ARGS_((Fs_HandleHeader *prefixHandle, 
		    char *relativeName, Address argsPtr, Address resultsPtr, 
		    Fs_RedirectInfo **newNameInfoPtrPtr));
     ReturnStatus (*remove) _ARGS_((Fs_HandleHeader *prefixHandle, 
		    char *relativeName, Address argsPtr, Address resultsPtr, 
		    Fs_RedirectInfo **newNameInfoPtrPtr));
     ReturnStatus (*removeDir) _ARGS_((Fs_HandleHeader *prefixHandle, 
		    char *relativeName, Address argsPtr, Address resultsPtr, 
		    Fs_RedirectInfo **newNameInfoPtrPtr));
     ReturnStatus (*rename) _ARGS_((Fs_HandleHeader *prefixHandle1, 
		    char *relativeName1, Fs_HandleHeader *prefixHandle2, 
		    char *relativeName2, Fs_LookupArgs *lookupArgsPtr, 
		    Fs_RedirectInfo **newNameInfoPtrPtr, 
		    Boolean *name1ErrorPtr));
     ReturnStatus (*hardLink) _ARGS_((Fs_HandleHeader *prefixHandle1, 
		    char *relativeName1, Fs_HandleHeader *prefixHandle2,
		    char *relativeName2, Fs_LookupArgs *lookupArgsPtr, 
		    Fs_RedirectInfo **newNameInfoPtrPtr, 
		    Boolean *name1ErrorPtr));
} Fs_DomainLookupOps;


/*
 * Forward references.
 */
extern void Fs_SetIDs _ARGS_((Proc_ControlBlock *procPtr, Fs_UserIDs *idPtr));
extern void Fs_InstallDomainLookupOps _ARGS_((int domainType, 
		Fs_DomainLookupOps *nameLookupOpsPtr, 
		Fs_AttrOps *attrOpTablePtr));

#endif _FSNAMEOPS
