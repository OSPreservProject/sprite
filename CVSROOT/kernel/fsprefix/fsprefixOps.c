/* 
 * fsPrefix.c --
 *
 *	Implementation of the prefix table.  The prefix table is used
 *	to determine the server for a file depending on the
 *	first part of the file's name.  Operations on pathnames get
 *	passed through Fsprefix_LookupOperation (and Fsprefix_TwoNameOperation) that
 *	handles the iteration over the prefix table that is due to redirections
 *	from servers as a pathname wanders from domain to domain.  There
 *	is also set of low-level procedures for direct operations on the prefix
 *	table itself; add, delete, initialize, etc.
 *
 *	TODO: Extract the recovery related junk.  The prefix table is used
 *	as a convenient place to record recovery state and synchronize
 *	opens with re-opens, but the recov module's user-state flags should be
 *	used instead.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsprefix.h"
#include "prefixInt.h"
#include "fsNameOps.h"
#include "fsutil.h"
#include "fsutilTrace.h"
#include "fsStat.h"
#include "fsio.h"
#include "vm.h"
#include "rpc.h"
#include "proc.h"
#include "dbg.h"
#include "string.h"

static List_Links prefixListHeader;
static List_Links *prefixList = &prefixListHeader;

static Sync_Lock prefixLock = Sync_LockInitStatic("Fs:prefixLock");
#define LOCKPTR (&prefixLock)

/*
 * Debuging variables.
 */
Boolean fsprefix_FileNameTrace = FALSE;

/*
 * Forward references.
 */
ReturnStatus FsprefixLookupRedirect();
static ReturnStatus LocatePrefix();
static ReturnStatus GetPrefix();
static void PrefixUpdate();
static Fsprefix *PrefixInsert();
static void GetNilPrefixes();
static char *NameOp();
extern void FsprefixHandleCloseInt();
static ReturnStatus DumpExportList();


/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_LookupOperation --
 *
 *	This uses the prefix table to choose a server and domain-type for
 *	a pathname lookup operation.  This is called by the routines in
 *	fsNameOps.c to do opens, removes, mkdir, rmdir, etc. etc.  The
 *	domain-type name lookup routines may return pathnames instead of
 *	results	if the pathname left the domain of the server originally
 *	chosen by the prefix table.  This routine handles these "re-directed"
 *	pathnames and hides the iteration between the prefix table and
 *	the various servers.
 *
 * Results:
 *	The results of the lookup operation.
 *
 * Side effects:
 *	This may fault new entries into the prefix table.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsprefix_LookupOperation(fileName, operation, follow, argsPtr, resultsPtr, nameInfoPtr)
    char 	*fileName;	/* File name to lookup */
    int 	operation;	/* Operation to perform on the file */
    Boolean	follow;		/* TRUE if lookup will follow links.  FALSE
				 * means we won't indirect via a prefix which
				 * matches the name exactly. */
    Address 	argsPtr;	/* Operation specific arguments.  NOTE: it
				 * is assumed that the first thing in the
				 * arguments is a prefix file ID, except on
				 * the IMPORT/EXPORT operations.  We set the
				 * prefix fileID here as a convenience to
				 * the name lookup routines we branch to. */
    Address 	resultsPtr;	/* Operation specific results */
    Fs_NameInfo	*nameInfoPtr;	/* If non-NIL, set up to contain state needed
				 * to get back to the name server.  This is
				 * used with FS_DOMAIN_OPEN which passes
				 * in the nameInfoPtr from the stream. */
{
    ReturnStatus 	status;		/* General error code */
    int 		domainType;	/* Set from the prefix table lookup */
    Fs_HandleHeader 	*hdrPtr;	/* Set from the prefix table lookup */
    char 		*lookupName;	/* Returned from the prefix table 
					 * lookup */
    Fs_RedirectInfo 	*redirectInfoPtr;/* Returned from servers if their
					 * lookup leaves their domain */
    Fs_RedirectInfo 	*oldInfoPtr;	/* Needed to free up the new name
					 * buffer allocated by the domain
					 * lookup routine. */
    Fs_FileID		rootID;		/* ID of domain root */
    Fsprefix 		*prefixPtr;	/* Returned from prefix table lookup 
					 * and saved in the file handle */
    int 		numRedirects = 0;/* Number of iterations between 
					  * servers. This is used to catch the 
					  * looping occurs with absolute links
					  * that are circular. */

    redirectInfoPtr = (Fs_RedirectInfo *) NIL;
    oldInfoPtr = (Fs_RedirectInfo *) NIL;

    if (sys_ShuttingDown) {
	/*
	 * Lock processes out of the filesystem during a shutdown.
	 */
	return(FAILURE);
    }
    do {
	status = GetPrefix(fileName, follow, &hdrPtr, &rootID, &lookupName,
			    &domainType, &prefixPtr);
	if (status == SUCCESS) {
	    switch(operation) {
		case FS_DOMAIN_IMPORT:
		case FS_DOMAIN_EXPORT:
		    break;
		case FS_DOMAIN_OPEN:
		    FSUTIL_TRACE_NAME(FSUTIL_TRACE_LOOKUP_START, lookupName);
		    /* Fall Through */
		default: {
		    /*
		     * It is assumed that the first part of the bundled
		     * arguments are the prefix fileID, which indicates the
		     * start of the lookup, and the prefix rootID, which
		     * indicates the top of the domain.
		     */
		    register Fs_LookupArgs *lookupArgsPtr =
			    (Fs_LookupArgs *)argsPtr;
		    lookupArgsPtr->prefixID = hdrPtr->fileID;
		    lookupArgsPtr->rootID = rootID;
		    break;
		}
	    }
	    /*
	     * Fork out to the domain lookup operation.
	     */
	    status = (*fs_DomainLookup[domainType][operation])
	       (hdrPtr, lookupName, argsPtr, resultsPtr, &redirectInfoPtr);
	    if (fsprefix_FileNameTrace) {
		printf("\treturns <%x>\n", status);
	    }
	    switch (status) {
	        case FS_LOOKUP_REDIRECT: {
		    /*
		     * Lookup left the domain of the server chosen on the
		     * basis of the prefix table.  Generate an absolute name
		     * from the one returned by the server and loop back to
		     * the prefix table lookup.  We are careful to save a
		     * pointer to the redirect info because it contains the
		     * current pathname.  We also free any redirect info
		     * from previous iterations to prevent a core leak.
		     */
		    fs_Stats.prefix.redirects++; numRedirects++;
		    if (numRedirects > FS_MAX_LINKS) {
			status = FS_NAME_LOOP;
			fs_Stats.prefix.loops++;
		    } else {
			status = FsprefixLookupRedirect(redirectInfoPtr, prefixPtr,
								  &fileName);
			if (oldInfoPtr != (Fs_RedirectInfo *)NIL) {
			    free((Address)oldInfoPtr);
			}
			oldInfoPtr = redirectInfoPtr;
			redirectInfoPtr = (Fs_RedirectInfo *)NIL;
		    }
		    break;
		}
	        case SUCCESS: {
		    if (nameInfoPtr != (Fs_NameInfo *)NIL) {
			/*
			 * Set up the name info for the file.  The back pointer
			 * to the prefix table is used by us later to handle
			 * re-directs.  The rootID is noted here and passed
			 * to the server during relative lookups to trap
			 * ascending off the root of a domain via "..".
			 * The fileID is used by the attributes routines to get
			 * to the name server for open streams.
			 */
			nameInfoPtr->fileID =
				((Fs_OpenResults *)resultsPtr)->nameID;
			nameInfoPtr->rootID = rootID;
			nameInfoPtr->domainType = domainType;
			nameInfoPtr->prefixPtr = prefixPtr;
		    }
		    break;
		}
		case RPC_TIMEOUT:
		case RPC_SERVICE_DISABLED:
		case FS_STALE_HANDLE: {
		    /*
		     * Block waiting for regular recovery of the prefix handle.
		     */
		    if (status == FS_STALE_HANDLE) {
			fs_Stats.prefix.stale++;
		    } else {
			fs_Stats.prefix.timeouts++;
		    }
		    Fsutil_WantRecovery(hdrPtr);
		    printf("%s of \"%s\" waiting for recovery\n",
				NameOp(operation), fileName);
		    status = Fsutil_WaitForRecovery(hdrPtr, status);
		    if (status == SUCCESS) {
			/*
			 * Successfully waited for the server to reboot.
			 * Set the status to redirect so we go around the
			 * loop again.
			 */
			status = FS_LOOKUP_REDIRECT;
		    } else if (fileName[0] == '/') {
			/*
			 * Recovery failed, so we clear handle of the prefix
			 * used to get to the server and try again in case
			 * the prefix is served elsewhere.
			 */
			Fsprefix_HandleClose(prefixPtr, FSPREFIX_IMPORTED);
			status = FS_LOOKUP_REDIRECT;
		    }
		    break;
		}
		default:
		    break;
	    }
	}
    } while (status == FS_LOOKUP_REDIRECT);
    if (oldInfoPtr != (Fs_RedirectInfo *)NIL) {
	free((Address) oldInfoPtr);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_TwoNameOperation --
 *
 *	This is a version of Fsprefix_LookupOperation that deals with two
 *	pathnames.  The operation, either Rename or HardLink, will only
 *	be attempted if the two pathnames are part of the same domain.
 *	Two pathnames are determined to be in the same domain in part
 *	by the domain specific lookup routine, and in part by us in
 *	the following way.  The standard iteration over prefix table
 *	for the first pathname (the already existing file) is done,
 *	and the prefix info and relative name for it are passed to
 *	the domain specific procedure along with our first guess as
 *	to the domain and relative name of the second pathname.  The
 *	domain specific procedure will abort with a CROSS_DOMAIN error
 *	if it thinks the second pathname is served elsewhere.  In this case
 *	this procedure has to verify that by getting the attributes of
 *	the parent directory of the second name.  This may cause more
 *	iteration over the prefix table to get the final prefix info
 *	and relative name for the second name.  At that point we can
 *	finally determine if the pathnames are in the same domain.
 *
 * Results:
 *	The results of the two pathname operation.
 *
 * Side effects:
 *	This may fault new entries into the prefix table.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsprefix_TwoNameOperation(operation, srcName, dstName, lookupArgsPtr)
    int			operation;   /* FS_DOMAIN_RENAME, FS_DOMAIN_HARD_LINK */
    char		*srcName;	/* Name of existing file */
    char		*dstName; 	/* New name, or link name */
    Fs_LookupArgs	*lookupArgsPtr; /* ID information */
{
    ReturnStatus	status;		/* General error code */
    int			srcDomain;	/* Domain type of srcName */
    int 		dstDomain;
    Fs_HandleHeader	*srcHdrPtr;	/* Prefix handle of srcName */
    Fs_HandleHeader 	*dstHdrPtr;
    char		*srcLookupName;	/* Relative version of srcName */
    char 		*dstLookupName;
    Fsprefix		*srcPrefixPtr;	/* Prefix entry for srcName */
    Fsprefix 		*dstPrefixPtr;
    Fs_FileID		srcRootID;	/* ID of srcName domain root */
    Fs_FileID		dstRootID;
    int			numRedirects;	/* To detect loops in directories */
    Boolean		srcNameError;	/* TRUE if redirect info or stale
					 * handle error applies to srcName,
					 * FALSE if it applies to dstName. */
    /*
     * The domain-lookup routine allocates a buffer for a redirected pathname.
     * FsprefixLookupRedirect() subsequently generates a new absolute pathname
     * in this buffer.  We have to carefully hold onto this buffer during
     * the next iteration of the lookup loop, and still be able to free
     * it later to avoid core leaks.  The 'src' and 'dst' buffers are
     * used for this reason.
     */
    Fs_RedirectInfo	*redirectInfoPtr = (Fs_RedirectInfo *) NIL;
    Fs_RedirectInfo	*srcRedirectPtr = (Fs_RedirectInfo *) NIL;
    Fs_RedirectInfo	*dstRedirectPtr = (Fs_RedirectInfo *) NIL;

    if (sys_ShuttingDown) {
	/*
	 * Lock processes out of the filesystem during a shutdown.
	 */
	return(FAILURE);
    }
    numRedirects = 0;
getSrcPrefix:
    status = GetPrefix(srcName, 0, &srcHdrPtr, &srcRootID, &srcLookupName,
				    &srcDomain, &srcPrefixPtr);
    if (status != SUCCESS) {
	goto exit;
    }
    lookupArgsPtr->prefixID = srcHdrPtr->fileID;
    lookupArgsPtr->rootID = srcRootID;
getDstPrefix:
    status = GetPrefix(dstName, 0, &dstHdrPtr, &dstRootID, &dstLookupName,
				    &dstDomain, &dstPrefixPtr);
    if (status != SUCCESS) {
	goto exit;
    }
retry:
    status = (*fs_DomainLookup[srcDomain][operation])
       (srcHdrPtr, srcLookupName, dstHdrPtr, dstLookupName, lookupArgsPtr,
		    &redirectInfoPtr, &srcNameError);
    if (fsprefix_FileNameTrace) {
	printf("\treturns <%x>\n", status);
    }
    switch(status) {
	case RPC_SERVICE_DISABLED:
	case RPC_TIMEOUT:
	    srcNameError = TRUE;
	    /*
	     * FALL THROUGH to regular recovery.
	     */
	case FS_STALE_HANDLE: {
	    Fs_HandleHeader *staleHdrPtr;
	    staleHdrPtr = (srcNameError ? srcHdrPtr : dstHdrPtr);
	    Fsutil_WantRecovery(staleHdrPtr);
	    printf("%s of \"%s\" and \"%s\" waiting for recovery\n",
			    NameOp(operation), srcName, dstName);
	    status = Fsutil_WaitForRecovery(staleHdrPtr, status);
	    if (status == SUCCESS) {
		goto retry;
	    } else {
		/*
		 * Recovery failed.  On absolute paths clear handle of the
		 * prefix used to get to the server and try again.
		 */
		if ((srcNameError) && (srcName[0] == '/')) {
		    Fsprefix_HandleClose(srcPrefixPtr, FSPREFIX_IMPORTED);
		    status = FS_LOOKUP_REDIRECT;
		} else if ((!srcNameError) && (dstName[0] == '/')) {
		    Fsprefix_HandleClose(dstPrefixPtr, FSPREFIX_IMPORTED);
		    status = FS_LOOKUP_REDIRECT;
		}
	    }
	    break;
	}
	case FS_LOOKUP_REDIRECT:
	    /*
	     * The pathname left the server's domain, and it has returned
	     * us a new name.  We generate a new absolute pathname and
	     * save the pointer to the buffer.  It is now safe to free
	     * the buffer used during the last iteration as well.
	     */
	    fs_Stats.prefix.redirects++;
	    numRedirects++;
	    if (numRedirects > FS_MAX_LINKS) {
		status = FS_NAME_LOOP;
		fs_Stats.prefix.loops++;
	    } else if (srcNameError) {
		status = FsprefixLookupRedirect(redirectInfoPtr, srcPrefixPtr,
					  &srcName);
		if (srcRedirectPtr != (Fs_RedirectInfo *)NIL) {
		    free((Address)srcRedirectPtr);
		}
		srcRedirectPtr = redirectInfoPtr;
		redirectInfoPtr = (Fs_RedirectInfo *)NIL;
	    } else {
		status = FsprefixLookupRedirect(redirectInfoPtr, dstPrefixPtr,
					  &dstName);
		if (dstRedirectPtr != (Fs_RedirectInfo *)NIL) {
		    free((Address)dstRedirectPtr);
		}
		dstRedirectPtr = redirectInfoPtr;
		redirectInfoPtr = (Fs_RedirectInfo *)NIL;
	    }
	    break;
	case FS_CROSS_DOMAIN_OPERATION: {
	    /*
	     * The server thinks the second name is not served by it.
	     * Here we attempt to get the attributes of the second name
	     * in order to bounce through links and end up with a good prefix.
	     */
	    ReturnStatus	status2;
	    Fs_OpenArgs		openArgs;
	    Fs_GetAttrResults	getAttrResults;
	    Fs_Attributes	dstAttr;	/* Attrs of destination */
	    Fs_FileID		dstFileID;

	    openArgs.useFlags = FS_FOLLOW;
	    openArgs.permissions = 0;
	    openArgs.type = FS_FILE;
	    openArgs.clientID = rpc_SpriteID;
	    Fs_SetIDs((Proc_ControlBlock *)NIL, &openArgs.id);

	    openArgs.prefixID = dstHdrPtr->fileID;
	    openArgs.rootID = dstRootID;

	    getAttrResults.attrPtr = &dstAttr;
	    getAttrResults.fileIDPtr = &dstFileID;
getAttr:
	    status2 = (*fs_DomainLookup[dstDomain][FS_DOMAIN_GET_ATTR])
			(dstHdrPtr, dstLookupName, (Address)&openArgs,
			(Address)&getAttrResults, &redirectInfoPtr);
	    switch(status2) {
		default:
		    if (dstRootID.serverID != srcRootID.serverID ||
			dstRootID.major != srcRootID.major) {
			/*
			 * Really is a cross-domain operation.
			 */
			status = FS_CROSS_DOMAIN_OPERATION;
			break;
		    } else {
			goto retry;
		    }
		case FS_LOOKUP_REDIRECT: {
		    fs_Stats.prefix.redirects++;
		    numRedirects++;
		    if (numRedirects > FS_MAX_LINKS) {
			status = FS_NAME_LOOP;
			fs_Stats.prefix.loops++;
		    } else {
			status = FsprefixLookupRedirect(redirectInfoPtr, dstPrefixPtr,
						  &dstName);
			if (dstRedirectPtr != (Fs_RedirectInfo *)NIL) {
			    free((Address)dstRedirectPtr);
			}
			dstRedirectPtr = redirectInfoPtr;
			redirectInfoPtr = (Fs_RedirectInfo *)NIL;
			srcNameError = FALSE;
			/*
			 * Will fall out and then zip up to getDstPrefix.
			 */
		    }
		    break;
		}
		case RPC_SERVICE_DISABLED:
		case RPC_TIMEOUT:
		case FS_STALE_HANDLE: {
		    Fsutil_WantRecovery(dstHdrPtr);
		    printf("Get Attr of \"%s\" waiting for recovery\n",
				     dstName);
		    status2 = Fsutil_WaitForRecovery(dstHdrPtr, status2);
		    if (status2 == SUCCESS) {
			goto getAttr;
		    } else if (dstName[0] == '/') {
			Fsprefix_HandleClose(dstPrefixPtr, FSPREFIX_IMPORTED);
			status = FS_LOOKUP_REDIRECT;
			srcNameError = FALSE;
		    }
		    break;
		}
	    }
	    break;
	}
	default:
	    /*
	     * SUCCESS or simple lookup failure.
	     */
	    break;
    }
    if (status == FS_LOOKUP_REDIRECT) {
	if (srcNameError) {
	    goto getSrcPrefix;
	} else {
	    goto getDstPrefix;
	}
    }
exit:
    if (srcRedirectPtr != (Fs_RedirectInfo *)NIL) {
	free((Address)srcRedirectPtr);
    }
    if (dstRedirectPtr != (Fs_RedirectInfo *)NIL) {
	free((Address)dstRedirectPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsprefixLookupRedirect --
 *
 *	Process a filename returned from a server after the lookup left
 *	the server's domain.  This takes any prefix information and adds
 *	to new entries to the prefix table, then it recomputes a new filename
 *	and returns that so the caller can re-iterate the lookup.
 *
 * Results:
 *	A return status, and a new file name.
 *
 * Side effects:
 *	If the server tells us about a prefix it gets added to the
 *	prefix table, but with no token or domain type.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsprefixLookupRedirect(redirectInfoPtr, prefixPtr, fileNamePtr)
    Fs_RedirectInfo	*redirectInfoPtr;/* New name and prefix from server */
    Fsprefix		*prefixPtr;	/* Prefix table entry used to select
					 * the server. */
    char **fileNamePtr;			/* Return, new name to lookup. This is a
					 * pointer into the fileName buffer of
					 * *redirectInfoPtr.  This means that the
					 * caller has to be careful and not
					 * reference *fileNamePtr after it calls
					 * the domain lookup routine which will
					 * overwrite *redirectInfoPtr */
{
    register char *prefix;

    if (fsprefix_FileNameTrace) {
	printf("FsRedirect: \"%s\" (%d)\n", redirectInfoPtr->fileName,
				redirectInfoPtr->prefixLength);
    }
    if (redirectInfoPtr->prefixLength > 0) {
	/*
	 * We are being told about a new prefix after the server
	 * hit a remote link.  The prefix is embedded in the
	 * beginning of the returned complete pathname.
	 */
	prefix = (char *)malloc(redirectInfoPtr->prefixLength + 1);
	(void)strncpy(prefix, redirectInfoPtr->fileName,
			redirectInfoPtr->prefixLength);
	prefix[redirectInfoPtr->prefixLength] = '\0';
	Fsprefix_Load(prefix, RPC_BROADCAST_SERVER_ID, FSPREFIX_IMPORTED);
	free((Address) prefix);
    }
    if (redirectInfoPtr->fileName[0] == '.' &&
	redirectInfoPtr->fileName[1] == '.') {
	register int i;
	register int preLen;
	register char *fileName;
	/*
	 * The server ran off the top of its domain.  Compute a new name
	 * from the prefix for the domain and the relative name returned.
	 * Again, we use the redirectInfoPtr buffer to construct the new
	 * name so we have to be careful not to use fileName after the
	 * domain lookup routine returns.  At this point
	 * prefix = "/pre/fix"
	 * fileName = "../rest/of/path"
	 * and we need
	 * fileName = "/pre/rest/of/path"
	 */
	prefix = prefixPtr->prefix;
	fileName = redirectInfoPtr->fileName;
	preLen = strlen(prefix);
	/*
	 * Scan the prefix from the right end for the first '/'
	 */
	for (i = preLen-1; i >= 0 ; i--) {
	    if (prefix[i] == '/') {
		break;
	    }
	}
	preLen = i+1;
	if (preLen == 1) {
	    /*
	     * Have to shift the name to the left, up against the beginning /
	     */
	    for (i=3; ; i++) {
		fileName[i-2] = fileName[i];
		if (fileName[i] == '\0') {
		    break;
		}
	    }
	} else {
	    /*
	     * Shift the fileName over to the right so the beginning of the
	     * prefix can be inserted before it.  The magic 2 refers to the
	     * length of ".."
	     */
	    for (i = strlen(fileName); i >= 2; i--) {
		fileName[i + preLen - 2] = fileName[i];
	    }
	}
	/*
	 * Insert the prefix.
	 */
	for (i = 0 ; i < preLen ; i++) {
	    fileName[i] = prefix[i];
	}
    }
    if (redirectInfoPtr->fileName[0] == '/') {
	/*
	 * Either just computed a new pathname or the server returned
	 * an absolute name to us.
	 */
	*fileNamePtr = redirectInfoPtr->fileName;
	return(FS_LOOKUP_REDIRECT);
    } else {
	printf(
	  "Fsprefix_LookupOperation: Bad format of returned file name \"%s\".\n",
	  redirectInfoPtr->fileName);
	return(FAILURE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NameOp --
 *
 *	Return a string for a name operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set up the list links.
 *
 *----------------------------------------------------------------------
 */
static char *
NameOp(lookupOperation)
    int lookupOperation;
{
    switch(lookupOperation) {
	case FS_DOMAIN_IMPORT:
	    return("import");
	case FS_DOMAIN_EXPORT:
	    return("export");
	case FS_DOMAIN_OPEN:
	    return("open");
	case FS_DOMAIN_GET_ATTR:
	    return("get attr");
	case FS_DOMAIN_SET_ATTR:
	    return("set attr");
	case FS_DOMAIN_MAKE_DEVICE:
	    return("make device");
	case FS_DOMAIN_MAKE_DIR:
	    return("make directory");
	case FS_DOMAIN_REMOVE:
	    return("remove");
	case FS_DOMAIN_REMOVE_DIR:
	    return("remove directory");
	case FS_DOMAIN_RENAME:
	    return("rename");
	case FS_DOMAIN_HARD_LINK:
	    return("link");
	default:
	    return("(unknown lookup operation)");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Init --
 *
 *	Initialize the prefix table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set up the list links.
 *
 *----------------------------------------------------------------------
 */
void
Fsprefix_Init()
{
    List_Init(prefixList);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Install --
 *
 *	Add an entry to the prefix table.
 *
 * Results:
 *	A pointer to the prefix table entry.
 *
 * Side effects:
 *	Add an entry to the prefix table.
 *
 *----------------------------------------------------------------------
 */
ENTRY Fsprefix *
Fsprefix_Install(prefix, hdrPtr, domainType, flags)
    char		*prefix;	/* String to install as a prefix */
    Fs_HandleHeader	*hdrPtr;	/* Handle from server of the prefix */
    int			domainType;	/* Default domain type for prefix. */
    int			flags;	/* FSPREFIX_EXPORTED | FSPREFIX_IMPORTED. */
{
    register Fsprefix *prefixPtr;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (strcmp(prefixPtr->prefix, prefix) == 0) {
	    /*
	     * Update information in the table.
	     */
	    PrefixUpdate(prefixPtr, FS_NO_SERVER, hdrPtr, domainType, flags);
	    UNLOCK_MONITOR;
	    return(prefixPtr);
	}
    }
    prefixPtr = PrefixInsert(prefix, FS_NO_SERVER, hdrPtr, domainType, flags);
    UNLOCK_MONITOR;
    return(prefixPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Load --
 *
 *	Force a prefix to occur in the prefix table.  This is needed because
 *	the Unix Domain server does not do REDIRECTS right so we have
 *	no other way to forcibly load a prefix.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Add an entry to the prefix table.  
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsprefix_Load(prefix, serverID, flags)
    char *prefix;		/* String to install as a prefix */
    int  serverID;		/* Id of server for prefix */
    int flags;			/* Prefix flags from fsPrefix.h */
{
    register Fsprefix *prefixPtr;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (strcmp(prefixPtr->prefix, prefix) == 0) {
	    /*
	     * Update information in the table.
	     */
	    PrefixUpdate(prefixPtr, serverID, (Fs_HandleHeader *)NIL, -1, 
		flags);	
	    UNLOCK_MONITOR;
	    return;
	}
    }
    /*
     * Add new entry to the table.
     */
    (void)PrefixInsert(prefix, serverID, (Fs_HandleHeader *)NIL, -1,
			flags);
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * PrefixInsert --
 *
 *	Insert an entry into the prefix table. If the hdtPtr is not
 *	NIL then the serverID it contains is used, otherwise the
 *	serverID parameter is used.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the hdrPtr, etc. of the prefix.  Also resets number of
 *	active opens/delay opens state for the prefix.
 *
 *----------------------------------------------------------------------
 */
static INTERNAL Fsprefix *
PrefixInsert(prefix, serverID, hdrPtr, domainType, flags)
    char		*prefix;	/* The prefix itself */
    int			serverID;	/* Id of server for prefix */
    Fs_HandleHeader	*hdrPtr;	/* Handle for the prefix from server */
    int			domainType;	/* Domain type of handle */
    int			flags;		/* import, export, etc. */
{
    register Fsprefix *prefixPtr;
    register char *prefixCopy;

    prefixPtr = (Fsprefix *)malloc(sizeof(Fsprefix));
    if (hdrPtr != (Fs_HandleHeader *)NIL) {
	prefixPtr->serverID	= hdrPtr->fileID.serverID;
    } else {
	prefixPtr->serverID	= serverID;
    }
    prefixPtr->prefixLength	= strlen(prefix);
    prefixCopy			= (char *)malloc(prefixPtr->prefixLength+1);
    (void)strcpy(prefixCopy, prefix);
    prefixPtr->prefix		= prefixCopy;
    prefixPtr->hdrPtr		= hdrPtr;
    prefixPtr->domainType	= domainType;
    prefixPtr->flags		= flags;
    prefixPtr->activeOpens	= 0;
    prefixPtr->delayOpens	= FALSE;
    List_Init(&prefixPtr->exportList);

    List_Insert((List_Links *)prefixPtr, LIST_ATFRONT(prefixList));
    return(prefixPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * PrefixUpdate --
 *
 *	Update a prefix table entry.  This is called in several cases:
 *	during lookup redirection (via Fs_PrefixLoad), when exporting
 *	a local domain as part of bootstrap, and when setting up the
 *	serverID for domains not reachable by broadcast.  Basically,
 *	if there is no handle associated with the prefix then the
 *	handled that was passed in is installed.  Otherwise, we let
 *	the flags change in order to export an existing entry, and
 *	we let the serverID field be set in order to by-pass broadcast.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reinitializes the hdrPtr, etc. of the prefix.  Also resets number of
 *	active opens/delay opens state for the prefix.
 *
 *----------------------------------------------------------------------
 */
static INTERNAL void
PrefixUpdate(prefixPtr, serverID, hdrPtr, domainType, flags)
    Fsprefix		*prefixPtr;	/* Table entry to update */
    int			serverID;	/* Id of server for prefix */
    Fs_HandleHeader	*hdrPtr;	/* Handle for the prefix from server */
    int			domainType;	/* Domain type of handle */
    int			flags;		/* import, export, etc. */
{
    if (hdrPtr != (Fs_HandleHeader *)NIL) {
	prefixPtr->serverID	= hdrPtr->fileID.serverID;
    } else {
	/*
	 * The serverID field is used when locating the server for
	 * a prefix.  Here we set this field whether the prefix
	 * table entry has been established or not.  This lets us
	 * associate a prefix with a particular remote server,
	 * i.e. a server that is not contacted via broadcast.
	 */
	prefixPtr->serverID	= serverID;
    }
    if (prefixPtr->hdrPtr == (Fs_HandleHeader *)NIL) {
	prefixPtr->hdrPtr	= hdrPtr;
	prefixPtr->domainType	= domainType;
	prefixPtr->flags	= flags & ~FSPREFIX_OVERRIDE;
	prefixPtr->delayOpens	= FALSE;
	prefixPtr->activeOpens	= 0;
    } else {
	/*
	 * Verify the new flags for the prefix table entry.
	 */
	if (flags & FS_EXPORTED_PREFIX) {
	    /*
	     * A local prefix is being exported.  This happens during bootstrap.
	     */
	    prefixPtr->flags	|= FS_EXPORTED_PREFIX;
	}
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Lookup --
 *
 *	Find an entry in the prefix table.  It is the caller's responsibility
 *	to broadcast to get the handle for the prefix, if necessary.
 *
 * Results:
 *	SUCCESS means there was a prefix match.  Still, *hdrPtr and
 *	*domainTypePtr may be NIL to indicate that they are not
 *	instantiated.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Fsprefix_Lookup(fileName, flags, clientID, hdrPtrPtr, rootIDPtr, lookupNamePtr, 
		serverIDPtr, domainTypePtr, prefixPtrPtr)
    register char *fileName;	/* File name to match against */
    int 	flags;		/* FSPREFIX_IMPORTED | FSPREFIX_EXACT and
				 * one of FSPREFIX_EXPORTED|FSPREFIX_LOCAL */
    int		clientID;	/* Use to check export list */
    Fs_HandleHeader **hdrPtrPtr;	/* Return, the handle for the prefix.  This is
				 * NOT LOCKED and has no extra references. */
    Fs_FileID	*rootIDPtr;	/* Return, ID of the root of the domain */
    char 	**lookupNamePtr;/* Return, If FS_NO_HANDLE this is the prefix
				 * itself.  If SUCCESS, this is the relative
				 * name after the prefix */
    int		*serverIDPtr;	/* Return, If FS_NO_HANDLE this is the id
				 * of the server for the prefix */
    int		*domainTypePtr;	/* Return, the domain of the prefix */
    Fsprefix	**prefixPtrPtr;	/* Return, prefix used to find the file */
{
    register Fs_ProcessState	*fsPtr;	   	    /* For this process, to
						     * return working dir.*/
    register Fsprefix 		*longestPrefixPtr;  /* Longest match */
    register Fsprefix		*prefixPtr;	    /* Pointer to table entry */
    register Fs_NameInfo		*nameInfoPtr;	    /* Name info for prefix */
    ReturnStatus		status = SUCCESS;   /* Return value */
    Boolean			exactMatch;	    /* TRUE the fileName has
						     * to match the prefix in 
						     * the table exactly */
    Boolean			wantLink;	    /* TRUE if caller wants to
						     * inhibit indirection via
						     * a prefix so it can lstat
						     * the link file itself. */

    LOCK_MONITOR;

    longestPrefixPtr = (Fsprefix *) NIL;
    exactMatch = (flags & FSPREFIX_EXACT);
    wantLink = (flags & FSPREFIX_LINK_NOT_PREFIX);
    flags &= ~(FSPREFIX_EXACT|FSPREFIX_LINK_NOT_PREFIX);
    if (fileName[0] != '/') {
	/*
	 * For relative names just return the handle from the current
	 * working directory.  Also, don't accept relative names with
	 * exact matches - that happens occasionally in error conditions.
	 */
	fs_Stats.prefix.relative++;
	fsPtr = (Proc_GetEffectiveProc())->fsPtr;
	if (!exactMatch && fsPtr->cwdPtr != (Fs_Stream *)NIL) {
	    *hdrPtrPtr = fsPtr->cwdPtr->ioHandlePtr;
	    nameInfoPtr = fsPtr->cwdPtr->nameInfoPtr;
	    *rootIDPtr = nameInfoPtr->rootID;
	    *lookupNamePtr = fileName;
	    *domainTypePtr = nameInfoPtr->domainType;
	    *prefixPtrPtr = nameInfoPtr->prefixPtr;
	} else {
	    status = FS_FILE_NOT_FOUND;
	}
    } else {
	fs_Stats.prefix.absolute++;
	LIST_FORALL(prefixList, (List_Links *) prefixPtr) {
	    if (strncmp(prefixPtr->prefix, fileName, prefixPtr->prefixLength)
					== 0) {
		char	lastChar;

		if (!(flags & prefixPtr->flags)) {
		    /*
		     * Only hit on imported or exported prefixes, as requested.
		     */
		    continue;
		}
		lastChar = fileName[prefixPtr->prefixLength];
		if (exactMatch && lastChar != '\0') {
		    /*
		     * Need an exact match, but there is more filename left.
		     */
		    continue;
		} else if (wantLink && lastChar == '\0' &&
			    prefixPtr->prefixLength != 1) {
		    /*
		     * The opposite of exact match.  We skip an exact match
		     * if we are trying to open a remote link.  This makes
		     * lstat() behave the same on all remote links, whether
		     * or not there is an installed prefix for the link.
		     */
		    continue;
		} else if ((prefixPtr->prefixLength == 1) ||
			   (lastChar == '\0') || (lastChar == '/')) {
		    /*
		     * The prefix is "/", or the prefix matches up through
		     * a complete pathname component.  This implies that
		     * /spur is not a valid prefix of /spurios.
		     */
		    if (longestPrefixPtr == (Fsprefix *)NIL) {
			longestPrefixPtr = prefixPtr;
		    } else if (longestPrefixPtr->prefixLength <
			       prefixPtr->prefixLength) {
			longestPrefixPtr = prefixPtr;
		    }
		}
	    }
	}
	if (longestPrefixPtr != (Fsprefix *)NIL) {
	    if ((flags & FSPREFIX_EXPORTED) && (clientID >= 0) &&
		(! List_IsEmpty(&longestPrefixPtr->exportList))) {
		/*
		 * Check the export list to see if the remote client has
		 * access.  An empty export list implies everyone has access.
		 */
		register FsprefixExport *exportPtr;
		status = FS_NO_ACCESS;
		LIST_FORALL(&longestPrefixPtr->exportList,
			    (List_Links *)exportPtr) {
		    if (exportPtr->spriteID == clientID) {
			status = SUCCESS;
			break;
		    }
		}
	    }
	    if (status == SUCCESS) {
		*hdrPtrPtr = longestPrefixPtr->hdrPtr;
		*domainTypePtr = longestPrefixPtr->domainType;
		if (*hdrPtrPtr == (Fs_HandleHeader *)NIL) {
		    /*
		     * Return our caller the prefix instead of a relative name
		     * so it can broadcast to get the prefix's handle.  If
		     * the prefix has been installed under a specific serverID
		     * then we return that so internet RPC (presumably) can
		     * be used to contact the server.  Otherwise we'll
		     * broadcast to locate the server.
		     */
		    *lookupNamePtr = longestPrefixPtr->prefix;
		    if (longestPrefixPtr->flags & FSPREFIX_REMOTE) {
			*serverIDPtr = longestPrefixPtr->serverID;
		    } else {
			*serverIDPtr = RPC_BROADCAST_SERVER_ID;
		    }
		    *prefixPtrPtr = (Fsprefix *)NIL;
		    status = FS_NO_HANDLE;
		} else {
		    /*
		     * All set, return our caller the name after the prefix.
		     * A name not starting with a slash is returned as the
		     * relative name.  This is because of domains that
		     * think that a name starting with a slash is absolute.
		     */
		    *rootIDPtr = (*hdrPtrPtr)->fileID;
		    *lookupNamePtr = &fileName[longestPrefixPtr->prefixLength];
		    while (**lookupNamePtr == '/') {
			(*lookupNamePtr)++;
		    }
		    *prefixPtrPtr = longestPrefixPtr;
		}
	    }
	} else {
	    status = FS_FILE_NOT_FOUND;
	}
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Export --
 *
 *	Add (or subtract) a client from the export list associated with
 *	a prefix.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Update the export list of the prefix.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsprefix_Export(prefix, clientID, delete)
    char *prefix;		/* Update this prefix'es export list */
    int clientID;		/* Host ID of client to which to export */
    Boolean delete;		/* If TRUE, remove the client */
{
    register Fsprefix *prefixPtr;
    register FsprefixExport *exportPtr;
    Boolean found = FALSE;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (strcmp(prefixPtr->prefix, prefix) == 0) {
	    LIST_FORALL(&prefixPtr->exportList, (List_Links *)exportPtr) {
		if (exportPtr->spriteID == clientID) {
		    if (delete) {
			List_Remove((List_Links *)exportPtr);
			free((Address)exportPtr);
		    }
		    found = TRUE;
		    break;
		}
	    }
	    if (!found && !delete) {
		exportPtr = mnew(FsprefixExport);
		List_InitElement((List_Links *)exportPtr);
		exportPtr->spriteID = clientID;
		List_Insert((List_Links *)exportPtr,
			    LIST_ATREAR(&prefixPtr->exportList));
	    }
	    break;
	}
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Clear --
 *
 *	Clear a prefix table entry.  If the deleteFlag argument is set
 *	then the entry is removed altogether, otherwise just the
 *	handle is closed and then cleared.  This is called from Fs_Command
 *	and used during testing.  This won't delete the root prefix,
 *	although it will clear its handle so subsequent lookups will
 *	rebroadcast.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Delete a prefix or just clear its handle.
 *
 *----------------------------------------------------------------------
 */
ENTRY Boolean
Fsprefix_Clear(prefix, deleteFlag)
    char *prefix;		/* String to install as a prefix */
    int deleteFlag;		/* If TRUE then the prefix is removed from
				 * the table.  Otherwise just the handle
				 * information is cleared. */
{
    register Fsprefix *prefixPtr;
    Fsprefix *targetPrefixPtr = (Fsprefix *)NIL;
    Fs_FileID prefixID;
    Boolean okToNuke = TRUE;

    LOCK_MONITOR;

    /*
     * First pass to scan the table for the prefix and get the
     * flags and fileID associated with the prefix table entry.
     */
    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (strcmp(prefixPtr->prefix, prefix) == 0) {
	    if (prefixPtr->flags & (FSPREFIX_EXPORTED|FSPREFIX_LOCAL)) {
		/*
		 * We export the prefix and have to be careful about
		 * deleting the prefix table entry for it.  Only if
		 * there is another prefix corresponding to the same
		 * domain can we delete this one.  This situation occurs
		 * during bootstrap where "/bootTmp" is aliased to "/"
		 * but eventually we want to nuke the local "/" so
		 * we can hook up to the network "/".
		 */
		if (prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) {
		    prefixID = prefixPtr->hdrPtr->fileID;
		    okToNuke = FALSE;
		}
	    }
	    targetPrefixPtr = prefixPtr;
	    break;
	}
    }
    /*
     * Second pass to look for an alias prefix.
     */
    if (!okToNuke) {
	LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	    if (strcmp(prefixPtr->prefix, prefix) != 0) {
		if (prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) {
		    register Fs_HandleHeader *hdrPtr = prefixPtr->hdrPtr;
		    if (prefixID.type != hdrPtr->fileID.type ||
			prefixID.serverID != hdrPtr->fileID.serverID ||
			prefixID.major != hdrPtr->fileID.major ||
			prefixID.minor != hdrPtr->fileID.minor) {
			continue;
		    }
		    /*
		     * Found an alias.
		     */
		    goto nukeIt;
		}
	    }
	}
	/*
	 * No other alias
	 */
	UNLOCK_MONITOR;
	return(FAILURE);
    }
nukeIt:
    if (targetPrefixPtr == (Fsprefix *)NIL) {
	/*
	 * No prefix match.
	 */
	UNLOCK_MONITOR;
	return(FAILURE);
    } else {
	if (targetPrefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) {
	    FsprefixHandleCloseInt(targetPrefixPtr, FSPREFIX_ANY);
	}
	targetPrefixPtr->serverID = RPC_BROADCAST_SERVER_ID;
	targetPrefixPtr->flags &= ~(FSPREFIX_EXPORTED|FSPREFIX_LOCAL);
	if (deleteFlag && targetPrefixPtr->prefixLength != 1) {
	    free((Address) targetPrefixPtr->prefix);
	    while (! List_IsEmpty(&targetPrefixPtr->exportList)) {
		register FsprefixExport *exportPtr;
		exportPtr =
		    (FsprefixExport *)List_First(&targetPrefixPtr->exportList);
		List_Remove((List_Links *)exportPtr);
		free((Address)exportPtr);
	    }
    
	    List_Remove((List_Links *)targetPrefixPtr);
	    free((Address) targetPrefixPtr);
	}
	UNLOCK_MONITOR;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_HandleClose --
 *
 *	Close the handle associated with a prefix.  This is called when
 *	cleaning up a prefix table entry.  The flags argument indicates
 *	what sort of prefix we export to nuke, and this is used for
 *	consistency checking.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None here, see the internal routine called inside the monitor lock.
 *
 *----------------------------------------------------------------------
 */
void
Fsprefix_HandleClose(prefixPtr, flags)
    Fsprefix *prefixPtr;
    int flags;			/* FSPREFIX_ANY, FSPREFIX_EXPORTED */
{
    register Fs_HandleHeader *hdrPtr;
    Fs_Stream dummy;

    LOCK_MONITOR;
    FsprefixHandleCloseInt(prefixPtr, flags);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * FsprefixHandleCloseInt --
 *
 *	Close the handle associated with a prefix.  The serverID from
 *	the handle is saved for use in recovery later.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the prefix's hdrPtr to NIL.  This notifies the okToRecover
 *	condition if there were active opens on the prefix so that
 *	recovery can proceed on this very handle.
 *
 *----------------------------------------------------------------------
 */
void
FsprefixHandleCloseInt(prefixPtr, flags)
    Fsprefix *prefixPtr;
    int flags;
{
    register Fs_HandleHeader *hdrPtr;
    Fs_Stream dummy;

    if (prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) {
	if ((flags & FSPREFIX_IMPORTED) &&
		(prefixPtr->flags & (FSPREFIX_EXPORTED|FSPREFIX_LOCAL))) {
	    printf("Fsprefix_HandleClose: \"%s\" is exported (mounted)\n",
		    prefixPtr->prefix);
	    return;
	} else if ((flags & FSPREFIX_EXPORTED) &&
		(prefixPtr->flags & (FSPREFIX_EXPORTED|FSPREFIX_LOCAL) == 0)){
	    printf("Fsprefix_HandleClose: \"%s\" is not exported (mounted)\n",
		    prefixPtr->prefix);
	    return;
	}
	printf("Fsprefix_HandleClose nuking \"%s\"\n", prefixPtr->prefix);
	hdrPtr = prefixPtr->hdrPtr;
	prefixPtr->hdrPtr = (Fs_HandleHeader *)NIL;
	Fsutil_HandleLock(hdrPtr);
	dummy.ioHandlePtr = hdrPtr;
	dummy.hdr.fileID.type = -1;
	(void)(*fsio_StreamOpTable[hdrPtr->fileID.type].close)(&dummy,
		    rpc_SpriteID, 0, 0, (ClientData)NIL);
	if (prefixPtr->activeOpens > 0) {
	    prefixPtr->activeOpens = 0;
	    Sync_Broadcast(&prefixPtr->okToRecover);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * LocatePrefix --
 *
 *	Call a domain specific routine to get the token for a prefix.
 *
 * Results:
 *	The token for the prefix table and a return code.
 *
 * Side effects:
 *	Those of the domain specific Prefix routine.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
LocatePrefix(fileName, serverID, domainTypePtr, hdrPtrPtr)
    char	*fileName;	/* The prefix to find the server of */
    int		serverID;	/* Id of server for prefix. */
    int		*domainTypePtr;	/* In/Out the type of the domain.  If -1 on
				 * entry then all domain prefix routines
				 * are polled in order to find the domain.
				 * Set upon return to domain type for prefix */
    Fs_HandleHeader **hdrPtrPtr;	/* The handle that the domain prefix routine
				 * returns for the prefix */
{
    register ReturnStatus	status;
    register int		domainType;
    Fs_UserIDs			ids;

    Fs_SetIDs(Proc_GetEffectiveProc(), &ids);
    for (domainType = 0; domainType < FS_NUM_DOMAINS; domainType++) {
	status = (*fs_DomainLookup[domainType][FS_DOMAIN_IMPORT])
		    (fileName, serverID, &ids, domainTypePtr, hdrPtrPtr);
#ifdef lint
	status = FsrmtImport(fileName, serverID, &ids, domainTypePtr,
				hdrPtrPtr);
#endif /* lint */
	if (status == SUCCESS) {
	    return(FS_NEW_PREFIX);
	}
    }
    return(FS_FILE_NOT_FOUND);
}


/*
 *----------------------------------------------------------------------
 *
 * GetPrefix --
 *
 *	A common loop to deal with prefixes that have no handle yet.
 *	This takes care of finding the handle for a prefix if needed.
 *
 * Results:
 *	The handle for the prefix of the file.
 *
 * Side effects:
 *	May call LocatePrefix and Fsprefix_Install.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GetPrefix(fileName, follow, hdrPtrPtr, rootIDPtr, lookupNamePtr, domainTypePtr,
	    prefixPtrPtr)
    char 	*fileName;		/* File name that needs to be 
					 * operated on */
    Boolean	follow;			/* TRUE means lookup will follow links.
					 * FALSE allows opening of links */
    Fs_HandleHeader **hdrPtrPtr;		/* Result, handle for the prefix */
    Fs_FileID	*rootIDPtr;		/* Result, ID of domain root */
    char 	**lookupNamePtr;	/* Result, remaining pathname to 
					 * lookup */
    int 	*domainTypePtr;		/* Result, domain type of the prefix */
    Fsprefix 	**prefixPtrPtr;		/* Result, reference to prefix table */
{
    ReturnStatus status;
    register int flags = FSPREFIX_IMPORTED;
    int		 serverID;

    if (!follow) {
	flags |= FSPREFIX_LINK_NOT_PREFIX;
    }
    do {
	if (fsprefix_FileNameTrace) {
	    printf("Lookup: %s,", fileName);
	}
	status = Fsprefix_Lookup(fileName, flags, FS_LOCALHOST_ID, hdrPtrPtr,
			rootIDPtr, lookupNamePtr, &serverID, domainTypePtr, 
			prefixPtrPtr);
	if (status == FS_NO_HANDLE) {
	    /*
	     * The prefix exists but there is not a valid file handle for it.
	     * Fsprefix_Lookup has returned us the prefix in lookupName.
	     */
	    /*
	     * If the server is ourself then return.  The prefix has 
	     * probably been installed because we are going to export it
	     * later and we shouldn't bother broadcasting for
	     * it now.
	     */
	    if (serverID == rpc_SpriteID) {
		return FAILURE;
	    }
	    if (serverID == RPC_BROADCAST_SERVER_ID) {
		printf("Broadcasting for server of \"%s\"\n", *lookupNamePtr);
	    } else {
		printf("Contacting server %d for \"%s\" prefix\n", serverID,
		    *lookupNamePtr);
	    }
	    status = LocatePrefix(*lookupNamePtr, serverID, domainTypePtr,
						    hdrPtrPtr);
	    if (status == FS_NEW_PREFIX) {
		fs_Stats.prefix.found++;
		(void)Fsprefix_Install(*lookupNamePtr, *hdrPtrPtr,*domainTypePtr,
				FSPREFIX_IMPORTED);
	    }
	}
    } while (status == FS_NEW_PREFIX);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Reopen --
 *
 *	This is called to enter the re-open phase of recovery.
 *	This finds prefix table entries that have been
 *	cleared out (because the server went away) and tries
 *	to re-establish these - the prefix token is needed when
 *	re-opening other handles..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Retries nil'ed prefixes.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsprefix_Reopen(serverID)
    int serverID;		/* Server we are recovering with */
{
    Fsprefix *prefixPtr;
    ReturnStatus status;
    Fs_HandleHeader *hdrPtr;
    int domainType;
    List_Links nilPrefixList;

    GetNilPrefixes(&nilPrefixList);
    while (!List_IsEmpty(&nilPrefixList)) {
	prefixPtr = (Fsprefix *)List_First(&nilPrefixList);
	if (prefixPtr->serverID == serverID ||
	    prefixPtr->serverID == RPC_BROADCAST_SERVER_ID) {
	    /*
	     * Attempt to re-establish the prefix table entry before
	     * re-opening files under that prefix.  This is needed
	     * because the prefix table slot is a point of synchronization
	     * between opens and re-opens.
	     */
	    domainType = -1;
	    status = LocatePrefix(prefixPtr->prefix, prefixPtr->serverID,
			    &domainType, &hdrPtr);
	    if (status == FS_NEW_PREFIX) {
		(void)Fsprefix_Install(prefixPtr->prefix, hdrPtr, domainType,
			    FSPREFIX_IMPORTED);
	    }
	}
	List_Remove((List_Links *)prefixPtr);
	free(prefixPtr->prefix);
	free((Address)prefixPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetNilPrefixes --
 *
 *	Return a list of prefixes that have lost their handles.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	mallocs, our caller should free each element in the list.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
GetNilPrefixes(listPtr)
    List_Links *listPtr;	/* Header for list of prefix table entries */
{
    Fsprefix *prefixPtr;

    LOCK_MONITOR;

    List_Init(listPtr);
    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (prefixPtr->hdrPtr == (Fs_HandleHeader *)NIL) {
	    register Fsprefix *newPrefixPtr;

	    newPrefixPtr = mnew(Fsprefix);
	    *newPrefixPtr = *prefixPtr;
	    newPrefixPtr->prefix = (Address)malloc(newPrefixPtr->prefixLength + 1);
	    (void)strcpy(newPrefixPtr->prefix, prefixPtr->prefix);
	    List_Insert((List_Links *)newPrefixPtr, LIST_ATREAR(listPtr));
	}
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_OpenCheck --
 *
 *	This is called to indicate that an open is occurring in
 *	this domain.  This will fail if recovery is in progress
 *	with the server.
 *
 * Results:
 *	FS_DOMAIN_UNAVAILABLE if the prefix is locked up because recovery
 *	actions are in progress.
 *
 * Side effects:
 *	Blocks recovery until
 *	Fsprefix_OpenDone is called.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Fsprefix_OpenCheck(prefixHdrPtr)
    Fs_HandleHeader *prefixHdrPtr;	/* Handle from the prefix table */
{
    register ReturnStatus status;
    Fsprefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID ==
		prefixHdrPtr->fileID.serverID)) {
	    if (prefixPtr->delayOpens) {
		printf( 
		    "Fsprefix_OpenCheck waiting for recovery\n");
		if (Sync_Wait(&prefixPtr->okToOpen, TRUE)) {
		    /*
		     * Wait was interrupted by a signal.
		     */
		    status = FS_DOMAIN_UNAVAILABLE;
		    printf("Fsprefix_OpenCheck aborted\n");
		} else {
		    prefixPtr->activeOpens++;
		    status = SUCCESS;
		    printf("Fsprefix_OpenCheck ok\n");
		}
	    } else {
		prefixPtr->activeOpens++;
		status = SUCCESS;
	    }
	    UNLOCK_MONITOR;
	    return(status);
	}
    }
    /*
     * No match with the prefix handle.
     */
    printf( "PrefixOpenCheck: didn't find prefix");
    UNLOCK_MONITOR;
    return(FS_DOMAIN_UNAVAILABLE);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_OpenDone --
 *
 *	The complement of FsPrefixOpenStart, this takes away the
 *	open reference count on the prefix and notifies any
 *	waiting recovery processes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Notifies the prefix okToRecover condition if activeOpens is zero.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsprefix_OpenDone(prefixHdrPtr)
    Fs_HandleHeader *prefixHdrPtr;	/* Handle from the prefix table */
{
    Fsprefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID ==
		prefixHdrPtr->fileID.serverID)) {
	    prefixPtr->activeOpens--;
	    if (prefixPtr->activeOpens < 0) {
		printf( "Fsprefix_OpenDone, neg open cnt\n");
		prefixPtr->activeOpens = 0;
	    }
	    if (prefixPtr->activeOpens == 0) {
		Sync_Broadcast(&prefixPtr->okToRecover);
	    }
	    UNLOCK_MONITOR;
	    return;
	}
    }
    /*
     * No match with the prefix handle.
     */
    printf( "PrefixOpenDone: no handle match\n");
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_RecoveryCheck --
 *
 *	This is called to indicate that we want to recover handles with
 *	this server.  This will block the calling process until any
 *	outstanding opens are completed so that opens and re-opens don't race.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Blocks recovery until Fsprefix_OpenDone is called.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsprefix_RecoveryCheck(serverID)
    int serverID;
{
    Fsprefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID == serverID)) {
	    while (prefixPtr->activeOpens > 0) {
		(void)Sync_Wait(&prefixPtr->okToRecover, FALSE);
	    }
	    prefixPtr->delayOpens = TRUE;
	    UNLOCK_MONITOR;
	    return;
	}
    }
    /*
     * No match with the prefix handle means the other host isn't a server.
     * There is no possibility of opens to race with re-opens.  We will
     * still need to re-open handles, however, because of remote devices.
     */
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_AllowOpens --
 *
 *	As part of recovery, regular opens to a server are blocked
 *	until all the re-opens have been done.  This procedure indicates
 *	that the re-open phase is done and regular opens can proceed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Notifies the prefix okToOpen condition and clears the
 *	delayOpens boolean.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsprefix_AllowOpens(serverID)
    int serverID;		/* Server we are recovering with */
{
    Fsprefix *prefixPtr;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID == serverID)) {
	    prefixPtr->delayOpens = FALSE;
	    Sync_Broadcast(&prefixPtr->okToOpen);
	}
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_FromFileID --
 *
 *	Return the prefix table entry given the fileID for the prefix.
 *	This reverse mapping is needed during recovery in order to
 *	re-establish the back pointer from a handle to the prefix
 *	table entry.  This in turn is used in ".." processing.
 *
 * Results:
 *	A pointer to the prefix table for the prefix rooted at
 *	the input fileID, or NIL if not found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY Fsprefix *
Fsprefix_FromFileID(fileIDPtr)
    Fs_FileID	*fileIDPtr;		/* FileID from a client */
{
    Fsprefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID ==
		fileIDPtr->serverID) &&
	    (prefixPtr->hdrPtr->fileID.major ==	fileIDPtr->major) &&
	    (prefixPtr->hdrPtr->fileID.minor ==	fileIDPtr->minor)) {
	    UNLOCK_MONITOR;
	    return(prefixPtr);
	}
    }
    /*
     * No match with the fileID.
     */
    UNLOCK_MONITOR;
    return((Fsprefix *)NIL);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_OpenInProgress --
 *
 *	This is called to find out if opens are in progress in
 *	a particular domain.  This is used by the cache consistency
 *	routines to decide if a consistency message might apply
 *	to an open hasn't quite completed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Blocks recovery until
 *	Fsprefix_OpenDone is called.
 *
 *----------------------------------------------------------------------
 */
ENTRY int
Fsprefix_OpenInProgress(fileIDPtr)
    Fs_FileID *fileIDPtr;		/* ID for some file */
{
    int activeOpens;
    Fsprefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID == fileIDPtr->serverID) &&
	    (prefixPtr->hdrPtr->fileID.major ==	fileIDPtr->major)) {
	    activeOpens = prefixPtr->activeOpens;
	    UNLOCK_MONITOR;
	    return(activeOpens);
	}
    }
    /*
     * No match with any prefix, must not be any active opens.
     */
    UNLOCK_MONITOR;
    return(0);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixInterate --
 *
 *	This is called to loop through the prefix table entries.
 *	Our caller is given 'read-only' access to the prefix table
 *	entry.  This is used for the 'df' and 'prefix' programs
 *	which call Fsprefix_Dump.
 *	
 *	Upon entry, the *prefixPtr should be NIL to indicate the
 *	beginning of the iteration.
 *
 * Results:
 *	A pointer to the next prefix table entry.
 *
 * Side effects:
 *	Marks the next prefix table entry as non-deletable, and
 *	returns a pointer to it.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsprefixIterate(prefixPtrPtr)
    Fsprefix **prefixPtrPtr;		/* In/Out pointer to prefix entry */
{
    register Fsprefix *prefixPtr;
    LOCK_MONITOR;

    prefixPtr = *prefixPtrPtr;
    if (prefixPtr == (Fsprefix *)NIL) {
	prefixPtr = (Fsprefix *)List_First(prefixList);
    } else {
	prefixPtr->flags &= ~FSPREFIX_LOCKED;
	prefixPtr = (Fsprefix *)List_Next(((List_Links *)prefixPtr));
	if (List_IsAtEnd(prefixList, (List_Links *)prefixPtr)) {
	    prefixPtr = (Fsprefix *)NIL;
	}
    }
    if (prefixPtr != (Fsprefix *)NIL) {
	prefixPtr->flags |= FSPREFIX_LOCKED;
    }
    *prefixPtrPtr = prefixPtr;
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * FsprefixDone --
 *
 *	This is called to terminate a prefix table iteration.  The
 *	current entry is unlocked so it could be deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the FSPREFIX_LOCKED bit in the prefix table entry.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsprefixDone(prefixPtr)
    Fsprefix *prefixPtr;
{
    LOCK_MONITOR;

    if (prefixPtr != (Fsprefix *)NIL) {
	prefixPtr->flags &= ~FSPREFIX_LOCKED;
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_Dump --
 *
 *	Dump out the prefix table to the console, or copy individual
 *	elements out to user space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Console prints.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsprefix_Dump(index, argPtr)
    int index;		/* Prefix table index, -1 means dump to console */
    Address argPtr;	/* Buffer space for entry */
{
    Fsprefix *prefixPtr;		/* Pointer to table entry */
    Boolean	foundPrefix = FALSE;
    int i;

    i = 0;
    prefixPtr = (Fsprefix *)NIL;
    FsprefixIterate(&prefixPtr);
    while (prefixPtr != (Fsprefix *)NIL) {
	if (index < 0) {
	    /*
	     * Dump the prefix entry to the console.
	     */
	    printf("%-20s ", prefixPtr->prefix);
	    if (prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) {
		printf("(%d,%d,%d,%x) ",
			      prefixPtr->hdrPtr->fileID.serverID,
			      prefixPtr->hdrPtr->fileID.type,
			      prefixPtr->hdrPtr->fileID.major,
			      prefixPtr->hdrPtr->fileID.minor);
	    } else {
		printf(" (no handle) ");
	    }
	    if (prefixPtr->flags & FSPREFIX_LOCAL) {
		printf(" import ");
	    }
	    if (prefixPtr->flags & FSPREFIX_IMPORTED) {
		printf(" import ");
	    }
	    if (prefixPtr->flags & FSPREFIX_EXPORTED) {
		printf(" export ");
	    }
	    printf("\n");
	} else if (i == index) {
	    Fs_Prefix userPrefix;
	    if (prefixPtr->hdrPtr != (Fs_HandleHeader *)NIL) {
		/*
		 * Call down to a domain-specific routine to get the
		 * information about the domain.  We pass in a copy
		 * of the file ID here because pseudo-file-system's
		 * will change the ID to match the user-visible one,
		 * not the internal one we use in the kernel.
		 */
		Fs_FileID fileID;
		register Fs_FileID *fileIDPtr = &fileID;

		*fileIDPtr = prefixPtr->hdrPtr->fileID;
		(void) Fsutil_DomainInfo(fileIDPtr, &userPrefix.domainInfo);
		userPrefix.serverID	= fileIDPtr->serverID;
		userPrefix.domain	= fileIDPtr->major;
		userPrefix.fileNumber	= fileIDPtr->minor;
		userPrefix.version	= fileIDPtr->type;
	    } else {
		userPrefix.serverID	= RPC_BROADCAST_SERVER_ID;
		userPrefix.domain	= -1;
		userPrefix.fileNumber	= -1;
		userPrefix.version	= -1;
		userPrefix.domainInfo.maxKbytes = -1;
		userPrefix.domainInfo.freeKbytes = -1;
		userPrefix.domainInfo.maxFileDesc = -1;
		userPrefix.domainInfo.freeFileDesc = -1;
		userPrefix.domainInfo.blockSize = -1;
		userPrefix.domainInfo.optSize = -1;
	    }
	    userPrefix.flags = prefixPtr->flags & ~FSPREFIX_LOCKED;
	    if (prefixPtr->prefixLength >= FS_USER_PREFIX_LENGTH) {
		bcopy((Address)prefixPtr->prefix, (Address)userPrefix.prefix, FS_USER_PREFIX_LENGTH);
		userPrefix.prefix[FS_USER_PREFIX_LENGTH-1] = '\0';
	    } else {
		(void)strcpy(userPrefix.prefix, prefixPtr->prefix);
	    }
	    Vm_CopyOut(sizeof(Fs_Prefix), (Address)&userPrefix, argPtr);
	    foundPrefix = TRUE;
	}
	i++;
	if (!foundPrefix) {
	    FsprefixIterate(&prefixPtr);
	} else {
	    FsprefixDone(prefixPtr);
	    break;
	}
    }
    if (index < 0 || foundPrefix) {
	return(SUCCESS);
    } else {
	return(FS_INVALID_ARG);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsprefix_DumpExport --
 *
 *	Return the export list of a prefix to user space.  The input
 *	buffer contains a prefix upon entry, and we then overwrite
 *	that with an array of SpriteIDs that corresponds to the export list.
 *	The end of the list is indicated by a spriteID of zero.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Copies stuff out to user space.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsprefix_DumpExport(size, buffer)
    int size;		/* Size of buffer in bytes */
    Address buffer;	/* Buffer space for prefix then export list */
{
    Fsprefix *prefixPtr;	/* Pointer to table entry */
    char     prefix[FS_MAX_NAME_LENGTH];
    Fs_HandleHeader *hdrPtr;
    Fs_FileID rootID;
    char *name;
    int domain;
    int length;
    int serverID;
    ReturnStatus status;

    if (Fsutil_StringNCopy(FS_MAX_NAME_LENGTH, buffer, prefix, &length) !=
	    SUCCESS) {
	return(SYS_ARG_NOACCESS);
    } else if (length == FS_MAX_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }
    status = Fsprefix_Lookup(prefix, FSPREFIX_EXACT|FSPREFIX_EXPORTED, -1,
	    &hdrPtr, &rootID, &name, &serverID, &domain, &prefixPtr);
    if (status == SUCCESS) {
	status = DumpExportList(prefixPtr, size, buffer);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DumpExportList --
 *
 *	A monitored routine to copy out the export list to user space.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Copies stuff out to user space.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DumpExportList(prefixPtr, size, buffer)
    Fsprefix *prefixPtr;
    int size;
    char *buffer;
{
    int *exportList;
    int *iPtr;
    ReturnStatus status;
    FsprefixExport *exportPtr;

    LOCK_MONITOR;
    if (size > 1000 * sizeof(int)) {
	status = FS_INVALID_ARG;
    } else {
	exportList = (int *)malloc(size);
	bzero((Address)exportList, size);
	iPtr = exportList;
	LIST_FORALL(&prefixPtr->exportList, (List_Links *)exportPtr) {
	    *iPtr = exportPtr->spriteID;
	    iPtr++;
	}
	status = Vm_CopyOut(size, (Address)exportList, (Address)buffer);
    }
    UNLOCK_MONITOR;
    return(status);
}

