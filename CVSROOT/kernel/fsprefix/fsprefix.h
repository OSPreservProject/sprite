/*
 * fsPrefix.h --
 *
 *	Definitions for the prefix table.  The prefix table is used to
 *	map from a file name to a handle for the file's domain and the
 *	relative name after the prefix.  The search key of the prefix table is,
 *	of course, the prefix of the file name.  The relative name after the
 *	prefix is also returned after searching the prefix table,  and the
 *	server for a domain looks up this name relative to the root
 *	of the domain.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSPREFIX
#define _FSPREFIX

#include "list.h"
#include "fs.h"

/*
 * FsPrefix defines an entry in the prefix table.  Now it is just a linked
 * list of items with the prefix as the key and the client's token as the
 * data.  This structure requires an O(number of entries in the table)
 * search each time a complete path name is opened.  A trie structure
 * would be better because the time for the search would then be
 * 0(length of valid prefix).
 */
typedef struct FsPrefix {
    List_Links	links;		/* The prefix table is kept in a list */
    char 	*prefix;	/* The prefix is the key */
    int		prefixLength;	/* It must match the file name this much */
    FsHandleHeader *hdrPtr;	/* This data is the result of opening the
				 * prefix (a directory) on the server */
    int		domainType;	/* These are cached and used when trying to */
    int		serverID;	/* re-establish a prefix table entry. */
				/* serverID is also used to find a prefix.
				 * It contains the server id if it is
				 * known, otherwise it contains the 
				 * broadcast id. */
    int		flags;		/* One of the flags defined below. */
    /*
     * At recovery time things are simpler if there are no active opens
     * or closes.  The following counters, flags and condition variables
     * are used to arrange this.
     */
    int		activeOpens;	/* Count of active opens in the domain */
    Boolean	delayOpens;	/* TRUE if opens are delayed pending recovery */
    Sync_Condition okToOpen;	/* Notified when opens can proceed */
    Sync_Condition okToRecover;	/* Notified when there are no active opens */

    /*
     * An export list can be kept for a prefix to control access.
     * Support code is implemented but not tested, 9/87
     */
    List_Links	exportList;	/* List of remote hosts that this prefix
				 * can be exported to.  If empty, all other
				 * hosts have access. */
} FsPrefix;

/*
 * A list of hosts that can use a domain is kept for exported prefixes.
 */
typedef struct FsPrefixExport {
    List_Links	links;
    int		spriteID;
} FsPrefixExport;

/*
 * Type of prefix.
 *
 *	FS_EXPORTED_PREFIX		This prefix is exported.
 *	FS_IMPORTED_PREFIX		This prefix is imported.
 *	FS_LOCAL_PREFIX			This prefix is local and private.
 *	FS_EXACT_PREFIX			As an argument to FsPrefixLookup, this
 *					constrains the lookup to succeed only
 *					on exact matches.
 *	FS_OVERRIDE_PREFIX		This flag, when passed to PrefixLoad,
 *					causes any existing flags to be
 *					sub-sumed by the passed-in flags.
 *	FS_LINK_NOT_PREFIX		This indicates that the caller really
 *					wants a handle on the link file, not
 *					the file referenced by the link.  This
 *					allows lstat() to get the remote link,
 *					regardless if there is a corresponding
 *					prefix installed.
 *	FS_PREFIX_LOCKED		The prefix cannot be deleted while
 *					this flag is set.  The iteration
 *					procedure uses this flag.
 *	FS_REMOTE_PREFIX		This is set when a prefix is loaded
 *					under a specific serverID.  This
 *					action ties the prefix to this server
 *					forever and causes prefix requests
 *					to be issued directly to the server
 *					instead of using broadcast.
 *	FS_ANY_PREFIX			This is passed to FsPrefixHandleClose
 *					to indicate that any type of prefix
 *					is ok to nuke.  FsPrefixHandleClose
 *					also takes FS_LOCAL_PREFIX,
 *					FS_EXPORTED_PREFIX, FS_IMPORTED_PREFIX
 *					as types to remove.
 */

#define	FS_EXPORTED_PREFIX		0x1
#define	FS_IMPORTED_PREFIX		0x2
#define	FS_LOCAL_PREFIX			0x4
#define	FS_EXACT_PREFIX			0x8
#define	FS_OVERRIDE_PREFIX		0x10
#define FS_LINK_NOT_PREFIX		0x20
#define FS_PREFIX_LOCKED		0x40
#define FS_REMOTE_PREFIX		0x80
#define FS_ANY_PREFIX			0x100

/*
 * Major prefix table entry points.
 */
extern	void		FsPrefixInit();
extern	FsPrefix *	FsPrefixInstall();
extern	ReturnStatus	FsPrefixLookup();

/*
 * Recovery related procedures
 */
extern	void		FsPrefixReopen();
extern	void		FsPrefixAllowOpens();
extern	void		FsPrefixRecoveryCheck();
extern	ReturnStatus	FsPrefixOpenCheck();
extern	int		FsPrefixOpenInProgress();
extern	void		FsPrefixOpenDone();
extern	FsPrefix *	FsPrefixFromFileID();
extern	void		FsPrefixHandleClose();

#endif _FSPREFIX
