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
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSPREFIX
#define _FSPREFIX

#include <list.h>
#include <fs.h>
#include <fsNameOps.h>

/*
 * Fsprefix defines an entry in the prefix table.  Now it is just a linked
 * list of items with the prefix as the key and the client's token as the
 * data.  This structure requires an O(number of entries in the table)
 * search each time a complete path name is opened.  A trie structure
 * would be better because the time for the search would then be
 * 0(length of valid prefix).
 */
typedef struct Fsprefix {
    List_Links	links;		/* The prefix table is kept in a list */
    char 	*prefix;	/* The prefix is the key */
    int		prefixLength;	/* It must match the file name this much */
    Fs_HandleHeader *hdrPtr;	/* This data is the result of opening the
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
} Fsprefix;

/*
 * Type of prefix.
 *
 *	FSPREFIX_EXPORTED		This prefix is exported.
 *	FSPREFIX_IMPORTED		This prefix is imported.
 *	FSPREFIX_LOCAL			This prefix is local and private.
 *	FSPREFIX_EXACT			As an argument to Fsprefix_Lookup, this
 *					constrains the lookup to succeed only
 *					on exact matches.
 *	FSPREFIX_OVERRIDE		This flag, when passed to PrefixLoad,
 *					causes any existing flags to be
 *					sub-sumed by the passed-in flags.
 *	FSPREFIX_LINK_NOT_PREFIX		This indicates that the caller really
 *					wants a handle on the link file, not
 *					the file referenced by the link.  This
 *					allows lstat() to get the remote link,
 *					regardless if there is a corresponding
 *					prefix installed.
 *	FSPREFIX_LOCKED		The prefix cannot be deleted while
 *					this flag is set.  The iteration
 *					procedure uses this flag.
 *	FSPREFIX_REMOTE		This is set when a prefix is loaded
 *					under a specific serverID.  This
 *					action ties the prefix to this server
 *					forever and causes prefix requests
 *					to be issued directly to the server
 *					instead of using broadcast.
 *	FSPREFIX_ANY			This is passed to Fsprefix_HandleClose
 *					to indicate that any type of prefix
 *					is ok to nuke.  Fsprefix_HandleClose
 *					also takes FSPREFIX_LOCAL,
 *					FSPREFIX_EXPORTED, FSPREFIX_IMPORTED
 *					as types to remove.
 */

#define	FSPREFIX_EXPORTED		0x1
#define	FSPREFIX_IMPORTED		0x2
#define	FSPREFIX_LOCAL			0x4
#define	FSPREFIX_EXACT			0x8
#define	FSPREFIX_OVERRIDE		0x10
#define FSPREFIX_LINK_NOT_PREFIX		0x20
#define FSPREFIX_LOCKED		0x40
#define FSPREFIX_REMOTE		0x80
#define FSPREFIX_ANY			0x100

extern Boolean fsprefix_FileNameTrace;
/*
 * Major prefix table entry points.
 */
extern void Fsprefix_Init _ARGS_((void));
extern Fsprefix *Fsprefix_Install _ARGS_((char *prefix, 
		Fs_HandleHeader *hdrPtr, int domainType, int flags));
extern ReturnStatus Fsprefix_Lookup _ARGS_((char *fileName, int flags, 
		int clientID, Fs_HandleHeader **hdrPtrPtr,
		Fs_FileID *rootIDPtr, char **lookupNamePtr, int *serverIDPtr,
		int *domainTypePtr, Fsprefix **prefixPtrPtr));
extern void Fsprefix_Load _ARGS_((char *prefix, int serverID, int flags));
/*
 * Recovery related procedures
 */
extern void Fsprefix_Reopen _ARGS_((int serverID));
extern void Fsprefix_AllowOpens _ARGS_((int serverID));
extern void Fsprefix_RecoveryCheck _ARGS_((int serverID));
extern ReturnStatus Fsprefix_OpenCheck _ARGS_((Fs_HandleHeader *prefixHdrPtr));
extern int Fsprefix_OpenInProgress _ARGS_((Fs_FileID *fileIDPtr));
extern void Fsprefix_OpenDone _ARGS_((Fs_HandleHeader *prefixHdrPtr));
extern Fsprefix *Fsprefix_FromFileID _ARGS_((Fs_FileID *fileIDPtr));
extern void Fsprefix_HandleClose _ARGS_((Fsprefix *prefixPtr, int flags));

extern ReturnStatus Fsprefix_LookupOperation _ARGS_((char *fileName, 
		int operation, Boolean follow, Address argsPtr,
		Address resultsPtr, Fs_NameInfo *nameInfoPtr));
extern ReturnStatus FsprefixLookupRedirect _ARGS_((
		Fs_RedirectInfo *redirectInfoPtr, Fsprefix *prefixPtr,
		char **fileNamePtr));
extern ReturnStatus Fsprefix_TwoNameOperation _ARGS_((int operation, 
		char *srcName, char *dstName, Fs_LookupArgs *lookupArgsPtr));

extern Boolean Fsprefix_Clear _ARGS_((char *prefix, int deleteFlag));
extern ReturnStatus Fsprefix_DumpExport _ARGS_((int size, Address buffer));
extern void Fsprefix_Export _ARGS_((char *prefix, int clientID,Boolean delete));

#endif _FSPREFIX
