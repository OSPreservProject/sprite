/* 
 * fsPrefix.c --
 *
 *	Implementation of the prefix table.  The prefix table is used
 *	to determine the server for a file depending on the
 *	first part of the file's name.  Operations on pathnames get
 *	passed through FsLookupOperation (and FsTwoNameOperation) that
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
#include "fsInt.h"
#include "fsPrefix.h"
#include "fsNameOps.h"
#include "fsOpTable.h"
#include "fsRecovery.h"
#include "fsTrace.h"
#include "fsStat.h"
#include "vm.h"
#include "rpc.h"
#include "proc.h"
#include "dbg.h"
#include "string.h"

static List_Links prefixListHeader;
static List_Links *prefixList = &prefixListHeader;

static Sync_Lock prefixLock;
#define LOCKPTR (&prefixLock)

/*
 * Debuging variables.
 */
Boolean fsFileNameTrace = FALSE;

/*
 * Forward references.
 */
ReturnStatus FsLookupRedirect();
ReturnStatus LocatePrefix();
ReturnStatus GetPrefix();
void PrefixHandleClear();
void PrefixUpdate();
void PrefixInsert();
void GetNilPrefixes();
void DoReopen();
char *NameOp();


/*
 *----------------------------------------------------------------------
 *
 * FsLookupOperation --
 *
 *	This uses the prefix table to choose a server and domain-type for
 *	a pathname lookup operation.  This is called by the routines in
 *	fsNameOps.c to do opens, removes, mkdir, rmdir, etc. etc.  The
 *	domain-type name lookup routines may return pathnames instead of
 *	results	if the pathname left the domain of the server orignially
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
FsLookupOperation(fileName, operation, argsPtr, resultsPtr, nameInfoPtr)
    char 	*fileName;	/* File name to lookup */
    int 	operation;	/* Operation to perform on the file */
    Address 	argsPtr;	/* Operation specific arguments.  NOTE: it
				 * is assummed that the first thing in the
				 * arguments is a prefix file ID, except on
				 * the FS_DOMAIN_PREFIX operation.  We set the
				 * prefix fileID here as a convenience to
				 * the name lookup routines we branch to. */
    Address 	resultsPtr;	/* Operation specific results */
    FsNameInfo	*nameInfoPtr;	/* If non-NIL, set up to contain state needed
				 * to get back to the name server.  This is
				 * used with FS_DOMAIN_OPEN which passes
				 * in the nameInfoPtr from the stream. */
{
    ReturnStatus 	status;		/* General error code */
    int 		domainType;	/* Set from the prefix table lookup */
    FsHandleHeader 	*hdrPtr;	/* Set from the prefix table lookup */
    char 		*lookupName;	/* Returned from the prefix table 
					 * lookup */
    FsRedirectInfo 	*redirectInfoPtr;/* Returned from servers if their
					 * lookup leaves their domain */
    FsRedirectInfo 	*oldInfoPtr;	/* Needed to free up the new name
					 * buffer allocated by the domain
					 * lookup routine. */
    FsFileID		rootID;		/* ID of domain root */
    FsPrefix 		*prefixPtr;	/* Returned from prefix table lookup 
					 * and saved in the file handle */
    int 		numRedirects = 0;/* Number of iterations between 
					  * servers. This is used to catch the 
					  * looping occurs with absolute links
					  * that are circular. */

    redirectInfoPtr = (FsRedirectInfo *) NIL;
    oldInfoPtr = (FsRedirectInfo *) NIL;

    if (sys_ShuttingDown) {
	/*
	 * Lock processes out of the filesystem during a shutdown.
	 */
	return(FAILURE);
    }
    do {
	status = GetPrefix(fileName, &hdrPtr, &rootID, &lookupName,
			    &domainType, &prefixPtr);
	if (status == SUCCESS) {
	    if (operation == FS_DOMAIN_OPEN) {
		FS_TRACE_NAME(FS_TRACE_LOOKUP_START, lookupName);
	    }
	    /*
	     * It is assumed that the first part of the bundled arguments
	     * are the prefix fileID, which indicates the start of the lookup,
	     * and the prefix rootID, which indicates the top of the domain.
	     */
	    if (operation != FS_DOMAIN_PREFIX) {
		register FsLookupArgs *lookupArgsPtr = (FsLookupArgs *)argsPtr;
		lookupArgsPtr->prefixID = hdrPtr->fileID;
		lookupArgsPtr->rootID = rootID;
	    }
	    /*
	     * Fork out to the domain lookup operation.
	     */
	    status = (*fsDomainLookup[domainType][operation])
	       (hdrPtr, lookupName, argsPtr, resultsPtr, &redirectInfoPtr);
	    if (fsFileNameTrace) {
		Sys_Printf("\treturns <%x>\n", status);
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
		    fsStats.prefix.redirects++; numRedirects++;
		    if (numRedirects > FS_MAX_LINKS) {
			status = FS_NAME_LOOP;
			fsStats.prefix.loops++;
		    } else {
			status = FsLookupRedirect(redirectInfoPtr, prefixPtr,
								  &fileName);
			if (oldInfoPtr != (FsRedirectInfo *)NIL) {
			    Mem_Free((Address)oldInfoPtr);
			}
			oldInfoPtr = redirectInfoPtr;
			redirectInfoPtr = (FsRedirectInfo *)NIL;
		    }
		    break;
		}
	        case SUCCESS: {
		    if (nameInfoPtr != (FsNameInfo *)NIL) {
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
				((FsOpenResults *)resultsPtr)->nameID;
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
			fsStats.prefix.stale++;
		    } else {
			fsStats.prefix.timeouts++;
		    }
		    FsWantRecovery(hdrPtr);
		    Sys_Printf("%s of \"%s\" waiting for recovery\n",
				NameOp(operation), fileName);
		    status = FsWaitForRecovery(hdrPtr, status);
		    if (status == SUCCESS) {
			/*
			 * Successfully waited for the server to reboot.
			 * Set the status to redirect so we go around the
			 * loop again.
			 */
			status = FS_LOOKUP_REDIRECT;
		    } else if ((status == FS_STALE_HANDLE) &&
				(fileName[0] == '/')) {
			/*
			 * Recovery failed, so we clear handle of the prefix
			 * used to get to the server and try again.
			 */
			PrefixHandleClear(prefixPtr);
			status = FS_LOOKUP_REDIRECT;
		    }
		    break;
		}
		default:
		    break;
	    }
	}
    } while (status == FS_LOOKUP_REDIRECT);
    if (oldInfoPtr != (FsRedirectInfo *)NIL) {
	Mem_Free((Address) oldInfoPtr);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsTwoNameOperation --
 *
 *	This is a version of FsLookupOperation that deals with two
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
FsTwoNameOperation(operation, srcName, dstName, lookupArgsPtr)
    int			operation;   /* FS_DOMAIN_RENAME, FS_DOMAIN_HARD_LINK */
    char		*srcName;	/* Name of existing file */
    char		*dstName; 	/* New name, or link name */
    FsLookupArgs	*lookupArgsPtr; /* ID information */
{
    ReturnStatus	status;		/* General error code */
    int			srcDomain;	/* Domain type of srcName */
    int 		dstDomain;
    FsHandleHeader	*srcHdrPtr;	/* Prefix handle of srcName */
    FsHandleHeader 	*dstHdrPtr;
    char		*srcLookupName;	/* Relative version of srcName */
    char 		*dstLookupName;
    FsPrefix		*srcPrefixPtr;	/* Prefix entry for srcName */
    FsPrefix 		*dstPrefixPtr;
    FsFileID		srcRootID;	/* ID of srcName domain root */
    FsFileID		dstRootID;
    int			numRedirects;	/* To detect loops in directories */
    Boolean		srcNameError;	/* TRUE if redirect info or stale
					 * handle error applies to srcName,
					 * FALSE if it applies to dstName. */
    /*
     * The domain-lookup routine allocates a buffer for a redirected pathname.
     * FsLookupRedirect() subsequently generates a new absolute pathname
     * in this buffer.  We have to carefully hold onto this buffer during
     * the next iteration of the lookup loop, and still be able to free
     * it later to avoid core leaks.  The 'src' and 'dst' buffers are
     * used for this reason.
     */
    FsRedirectInfo	*redirectInfoPtr = (FsRedirectInfo *) NIL;
    FsRedirectInfo	*srcRedirectPtr = (FsRedirectInfo *) NIL;
    FsRedirectInfo	*dstRedirectPtr = (FsRedirectInfo *) NIL;

    if (sys_ShuttingDown) {
	/*
	 * Lock processes out of the filesystem during a shutdown.
	 */
	return(FAILURE);
    }
    numRedirects = 0;
getSrcPrefix:
    status = GetPrefix(srcName, &srcHdrPtr, &srcRootID, &srcLookupName,
				    &srcDomain, &srcPrefixPtr);
    if (status != SUCCESS) {
	goto exit;
    }
    lookupArgsPtr->prefixID = srcHdrPtr->fileID;
    lookupArgsPtr->rootID = srcRootID;
getDstPrefix:
    status = GetPrefix(dstName, &dstHdrPtr, &dstRootID, &dstLookupName,
				    &dstDomain, &dstPrefixPtr);
    if (status != SUCCESS) {
	goto exit;
    }
retry:
    status = (*fsDomainLookup[srcDomain][operation])
       (srcHdrPtr, srcLookupName, dstHdrPtr, dstLookupName, lookupArgsPtr,
		    &redirectInfoPtr, &srcNameError);
    if (fsFileNameTrace) {
	Sys_Printf("\treturns <%x>\n", status);
    }
    switch(status) {
	case RPC_SERVICE_DISABLED:
	case RPC_TIMEOUT:
	    srcNameError = TRUE;
	    /*
	     * FALL THROUGH to regular recovery.
	     */
	case FS_STALE_HANDLE: {
	    FsHandleHeader *staleHdrPtr;
	    staleHdrPtr = (srcNameError ? srcHdrPtr : dstHdrPtr);
	    FsWantRecovery(staleHdrPtr);
	    Sys_Printf("%s of \"%s\" and \"%s\" waiting for recovery\n",
			    NameOp(operation), srcName, dstName);
	    status = FsWaitForRecovery(staleHdrPtr, status);
	    if (status == SUCCESS) {
		goto retry;
	    } else if (status == FS_STALE_HANDLE) {
		/*
		 * Recovery failed.  On absolute paths clear handle of the
		 * prefix used to get to the server and try again.
		 */
		if ((srcNameError) && (srcName[0] == '/')) {
		    PrefixHandleClear(srcPrefixPtr);
		    status = FS_LOOKUP_REDIRECT;
		} else if ((!srcNameError) && (dstName[0] == '/')) {
		    PrefixHandleClear(dstPrefixPtr);
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
	    fsStats.prefix.redirects++;
	    numRedirects++;
	    if (numRedirects > FS_MAX_LINKS) {
		status = FS_NAME_LOOP;
		fsStats.prefix.loops++;
	    } else if (srcNameError) {
		status = FsLookupRedirect(redirectInfoPtr, srcPrefixPtr,
					  &srcName);
		if (srcRedirectPtr != (FsRedirectInfo *)NIL) {
		    Mem_Free((Address)srcRedirectPtr);
		}
		srcRedirectPtr = redirectInfoPtr;
		redirectInfoPtr = (FsRedirectInfo *)NIL;
	    } else {
		status = FsLookupRedirect(redirectInfoPtr, dstPrefixPtr,
					  &dstName);
		if (dstRedirectPtr != (FsRedirectInfo *)NIL) {
		    Mem_Free((Address)dstRedirectPtr);
		}
		dstRedirectPtr = redirectInfoPtr;
		redirectInfoPtr = (FsRedirectInfo *)NIL;
	    }
	    break;
	case FS_CROSS_DOMAIN_OPERATION: {
	    /*
	     * Should try to get the attributes of the parent in order
	     * to get the correct prefix handle and relative name for
	     * the destination pathname.
	     */
#ifdef notyet
	    Fs_Attributes	srcAttr;	/* Attrs of source file */
	    Fs_Attributes	dstParentAttr;	/* Attrs of parent of the
						 * destination */
	    char *lastSlash, *cPtr, *pPtr;	/* Used to make dstParentName */
	    char *dstParentName;		/* The parent of dstName */
#endif /* not yet */
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
    if (srcRedirectPtr != (FsRedirectInfo *)NIL) {
	Mem_Free((Address)srcRedirectPtr);
    }
    if (dstRedirectPtr != (FsRedirectInfo *)NIL) {
	Mem_Free((Address)dstRedirectPtr);
    }
    return(status);
}
#ifdef notdef
FooProc()
{
    status = Fs_GetAttributes(srcFile, FS_ATTR_FILE, &srcAttr);
    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Massage the dstName to get the name of its parent.
     * We copy the destination name and note the slash that
     * delimits the last component of the name.
     */
    dstParentName = (char *)Mem_Alloc(FS_MAX_PATH_NAME_LENGTH);
    pPtr = parentName;
    lastSlash = (char *)NIL;
    for (cPtr = dstName ; *cPtr ; cPtr++) {
	if (*cPtr == '/') {
	    lastSlash = cPtr;
	}
	*pPtr = *cPtr;
    }
    if (lastSlash == (char *)NIL) {
	Mem_Free((Address)dstParentName);
	dstParentName = ".";
    } else {
	*lastSlash = '\0';
    }
    status = Fs_GetAttributes(dstParentName, FS_ATTR_FILE, &dstParentAttr);
    if (lastSlash != (char *)NIL) {
	Mem_Free((Address)dstParentName);
    }
    if (status != SUCCESS) {
	return(status);
    }
    if (srcAttr.serverID != dstParentAttr.serverID ||
	srcAttr.domain != dstParentAttr.domain) {
	status = FS_CROSS_DOMAIN_OPERATION;
    }
}
#endif notdef

/*
 *----------------------------------------------------------------------
 *
 * FsLookupRedirect --
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
FsLookupRedirect(redirectInfoPtr, prefixPtr, fileNamePtr)
    FsRedirectInfo	*redirectInfoPtr;/* New name and prefix from server */
    FsPrefix		*prefixPtr;	/* Prefix table entry used to select
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

    if (fsFileNameTrace) {
	Sys_Printf("FsRedirect: \"%s\" (%d)\n", redirectInfoPtr->fileName,
				redirectInfoPtr->prefixLength);
    }
    if (redirectInfoPtr->prefixLength > 0) {
	/*
	 * We are being told about a new prefix after the server
	 * hit a remote link.  The prefix is embedded in the
	 * beginning of the returned complete pathname.
	 */
	prefix = (char *)Mem_Alloc(redirectInfoPtr->prefixLength + 1);
	(void)String_NCopy(redirectInfoPtr->prefixLength,
		     redirectInfoPtr->fileName, prefix);
	prefix[redirectInfoPtr->prefixLength] = '\0';
	Fs_PrefixLoad(prefix, FS_IMPORTED_PREFIX);
	Mem_Free((Address) prefix);
    }
    if (String_NCompare(2, "..", redirectInfoPtr->fileName) == 0) {
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
	preLen = String_Length(prefix);
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
	    for (i = String_Length(fileName); i >= 2; i--) {
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
	Sys_Panic(SYS_WARNING,
	  "FsLookupOperation: Bad format of returned file name \"%s\".\n",
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
	case FS_DOMAIN_PREFIX:
	    return("prefix");
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
 * FsPrefixInit --
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
FsPrefixInit()
{
    prefixLock.inUse = FALSE;
    prefixLock.waiting = FALSE;

    List_Init(prefixList);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixInstall --
 *
 *	Add an entry to the prefix table.
 *
 * Results:
 *	A return code.
 *
 * Side effects:
 *	Add an entry to the prefix table.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsPrefixInstall(prefix, hdrPtr, domainType, flags)
    char		*prefix;	/* String to install as a prefix */
    FsHandleHeader	*hdrPtr;	/* Handle from server of the prefix */
    int			domainType;	/* Default domain type for prefix. */
    int			flags;	/* FS_EXPORTED_PREFIX | FS_IMPORTED_PREFIX. */
{
    register FsPrefix *prefixPtr;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (String_Compare(prefixPtr->prefix, prefix) == 0) {
	    /*
	     * Update information in the table.
	     */
	    PrefixUpdate(prefixPtr, hdrPtr, domainType, flags);
	    UNLOCK_MONITOR;
	    return;
	}
    }
    PrefixInsert(prefix, hdrPtr, domainType, flags);
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PrefixLoad --
 *
 *	Force a prefix to occur in the prefix table.  This is needed because
 *	the Unix Domain server does not do REDIRECTS right so we have
 *	no other way to forcibly load a prefix.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Add an entry to the prefix table.  If the FS_OVERRIDE_PREFIX flag
 *	is present then the existing flags (like import/export) will be
 *	over-written by the passed-in flags.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fs_PrefixLoad(prefix, flags)
    char *prefix;		/* String to install as a prefix */
    int flags;			/* Prefix flags from fsPrefix.h */
{
    register FsPrefix *prefixPtr;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (String_Compare(prefixPtr->prefix, prefix) == 0) {
	    if (flags & FS_OVERRIDE_PREFIX) {
		prefixPtr->flags = flags & ~FS_OVERRIDE_PREFIX;
	    }
	    UNLOCK_MONITOR;
	    return;
	}
    }
    /*
     * Add new entry to the table.
     */
    PrefixInsert(prefix, (FsHandleHeader *)NIL, -1, flags&~FS_OVERRIDE_PREFIX);
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * PrefixInsert --
 *
 *	Insert an entry into the prefix table.
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
INTERNAL void
PrefixInsert(prefix, hdrPtr, domainType, flags)
    char		*prefix;	/* The prefix itself */
    FsHandleHeader	*hdrPtr;	/* Handle for the prefix from server */
    int			domainType;	/* Domain type of handle */
    int			flags;		/* import, export, etc. */
{
    register FsPrefix *prefixPtr;
    register char *prefixCopy;

    prefixPtr = (FsPrefix *)Mem_Alloc(sizeof(FsPrefix));
    if (hdrPtr != (FsHandleHeader *)NIL) {
	prefixPtr->serverID	= hdrPtr->fileID.serverID;
    } else {
	prefixPtr->serverID	= -1;
    }
    prefixPtr->prefixLength	= String_Length(prefix);
    prefixCopy			= (char *)Mem_Alloc(prefixPtr->prefixLength+1);
    (void)String_Copy(prefix, prefixCopy);
    prefixPtr->prefix		= prefixCopy;
    prefixPtr->hdrPtr		= hdrPtr;
    prefixPtr->domainType	= domainType;
    prefixPtr->flags		= flags;
    prefixPtr->activeOpens	= 0;
    prefixPtr->delayOpens	= FALSE;
    List_Init(&prefixPtr->exportList);

    List_Insert((List_Links *)prefixPtr, LIST_ATFRONT(prefixList));
}

/*
 *----------------------------------------------------------------------
 *
 * PrefixUpdate --
 *
 *	Reset the hdrPtr for an existing prefix.
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
INTERNAL void
PrefixUpdate(prefixPtr, hdrPtr, domainType, flags)
    FsPrefix		*prefixPtr;	/* Table entry to update */
    FsHandleHeader	*hdrPtr;	/* Handle for the prefix from server */
    int			domainType;	/* Domain type of handle */
    int			flags;		/* import, export, etc. */
{
    if (hdrPtr != (FsHandleHeader *)NIL) {
	prefixPtr->serverID	= hdrPtr->fileID.serverID;
    } else {
	prefixPtr->serverID	= -1;
    }
    prefixPtr->hdrPtr		= hdrPtr;
    prefixPtr->domainType	= domainType;
    prefixPtr->flags		= flags;
    prefixPtr->delayOpens	= FALSE;
    prefixPtr->activeOpens	= 0;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixLookup --
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
FsPrefixLookup(fileName, flags, clientID, hdrPtrPtr, rootIDPtr, lookupNamePtr, 
		domainTypePtr, prefixPtrPtr)
    register char *fileName;	/* File name to match against */
    int 	flags;		/* FS_IMPORTED_PREFIX | FS_EXACT_PREFIX and
				 * one of FS_EXPORTED_PREFIX|FS_LOCAL_PREFIX */
    int		clientID;	/* Use to check export list */
    FsHandleHeader **hdrPtrPtr;	/* Return, the handle for the prefix.  This is
				 * NOT LOCKED and has no extra references. */
    FsFileID	*rootIDPtr;	/* Return, ID of the root of the domain */
    char 	**lookupNamePtr;/* Return, If FS_NO_HANDLE this is the prefix
				 * itself.  If SUCCESS, this is the relative
				 * name after the prefix */
    int		*domainTypePtr;	/* Return, the domain of the prefix */
    FsPrefix	**prefixPtrPtr;	/* Return, prefix used to find the file */
{
    register Fs_ProcessState	*fsPtr;	   	    /* For this process, to
						     * return working dir.*/
    register FsPrefix 		*longestPrefixPtr;  /* Longest match */
    register FsPrefix		*prefixPtr;	    /* Pointer to table entry */
    register FsNameInfo		*nameInfoPtr;	    /* Name info for prefix */
    ReturnStatus		status = SUCCESS;   /* Return value */
    Boolean			exactMatch;	    /* TRUE the fileName has
						     * to match the prefix in 
						     * the table exactly */

    LOCK_MONITOR;

    longestPrefixPtr = (FsPrefix *) NIL;
    exactMatch = (flags & FS_EXACT_PREFIX);
    flags &= ~FS_EXACT_PREFIX;
    if (fileName[0] != '/') {
	/*
	 * For relative names just return the handle from the current
	 * working directory.  Also, don't accept relative names with
	 * exact matches - that happens occasionally in error conditions.
	 */
	fsStats.prefix.relative++;
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
	fsStats.prefix.absolute++;
	LIST_FORALL(prefixList, (List_Links *) prefixPtr) {
	    if (String_NCompare(prefixPtr->prefixLength, prefixPtr->prefix,
							fileName) == 0) {
		char	lastChar;

		if (!(flags & prefixPtr->flags)) {
		    continue;
		}

		/*
		 * The string matches the prefix.  See if the prefix is
		 * '/' (length == 1). If not, make sure that the prefix
		 * matches the name up through a complete pathname
		 * component by checking the next character in the fileName.
		 * This implies that /spur is not a prefix of /spurious.
		 */
		lastChar = fileName[prefixPtr->prefixLength];
		if (exactMatch && lastChar != '\0') {
		    continue;
		} else if ((prefixPtr->prefixLength == 1) ||
			   (lastChar == '\0') || (lastChar == '/')) {
		    if (longestPrefixPtr == (FsPrefix *)NIL) {
			longestPrefixPtr = prefixPtr;
		    } else if (longestPrefixPtr->prefixLength <
			       prefixPtr->prefixLength) {
			longestPrefixPtr = prefixPtr;
		    }
		}
	    }
	}
	if (longestPrefixPtr != (FsPrefix *)NIL) {
	    if ((flags & FS_EXPORTED_PREFIX) &&
		(! List_IsEmpty(&longestPrefixPtr->exportList))) {
		/*
		 * Check the export list to see if the remote client has
		 * access.  An empty export list implies everyone has access.
		 */
		register FsPrefixExport *exportPtr;
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
		if (*hdrPtrPtr == (FsHandleHeader *)NIL) {
		    /*
		     * Return our caller the prefix instead of a relative name
		     * so it can broadcast to get the prefix's handle.
		     */
		    *lookupNamePtr = longestPrefixPtr->prefix;
		    *prefixPtrPtr = (FsPrefix *)NIL;
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
 * Fs_PrefixExport --
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
Fs_PrefixExport(prefix, clientID, delete)
    char *prefix;		/* Update this prefix'es export list */
    int clientID;		/* Host ID of client to which to export */
    Boolean delete;		/* If TRUE, remove the client */
{
    register FsPrefix *prefixPtr;
    register FsPrefixExport *exportPtr;
    Boolean found = FALSE;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (String_Compare(prefixPtr->prefix, prefix) == 0) {
	    LIST_FORALL(&prefixPtr->exportList, (List_Links *)exportPtr) {
		if (exportPtr->spriteID == clientID) {
		    if (delete) {
			List_Remove((List_Links *)exportPtr);
			Mem_Free((Address)exportPtr);
		    }
		    found = TRUE;
		    break;
		}
	    }
	    if (!found && !delete) {
		exportPtr = Mem_New(FsPrefixExport);
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
 * Fs_PrefixClear --
 *
 *	Clear a prefix table entry.  If the deleteFlag argument is set
 *	then the entry is removed altogether, otherwise just the
 *	handle is closed and then cleared.  This is called from Fs_Command
 *	and used during testing.
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
Fs_PrefixClear(prefix, deleteFlag)
    char *prefix;		/* String to install as a prefix */
    int deleteFlag;		/* If TRUE then the prefix is removed from
				 * the table.  Otherwise just the handle
				 * information is cleared. */
{
    register FsPrefix *prefixPtr;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (String_Compare(prefixPtr->prefix, prefix) == 0) {
	    if (prefixPtr->hdrPtr != (FsHandleHeader *)NIL) {
		PrefixHandleClear(prefixPtr);
	    }
	    prefixPtr->flags &= ~FS_EXPORTED_PREFIX;
	    if (deleteFlag) {
		Mem_Free((Address) prefixPtr->prefix);
		while (! List_IsEmpty(&prefixPtr->exportList)) {
		    register FsPrefixExport *exportPtr;
		    exportPtr =
			(FsPrefixExport *)List_First(&prefixPtr->exportList);
		    List_Remove((List_Links *)exportPtr);
		    Mem_Free((Address)exportPtr);
		}

		List_Remove((List_Links *)prefixPtr);
		Mem_Free((Address) prefixPtr);
	    }
	    UNLOCK_MONITOR;
	    return(SUCCESS);
	}
    }
    /*
     * Not found, oh well.
     */
    UNLOCK_MONITOR;
    return(FAILURE);
}

/*
 *----------------------------------------------------------------------
 *
 * PrefixHandleClear --
 *
 *	Close the handle associated with a prefix.  We do remember the
 *	serverID from the handle as this is used in recovery later.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the prefix's hdrPtr to NIL
 *
 *----------------------------------------------------------------------
 */
void
PrefixHandleClear(prefixPtr)
    FsPrefix *prefixPtr;
{
    register FsHandleHeader *hdrPtr = prefixPtr->hdrPtr;
    Fs_Stream dummy;

    if (hdrPtr == (FsHandleHeader *)NIL) {
	return;
    }
    FsHandleLock(hdrPtr);
    dummy.ioHandlePtr = hdrPtr;
    dummy.hdr.fileID.type = -1;
    (void)(*fsStreamOpTable[hdrPtr->fileID.type].close)(&dummy, rpc_SpriteID,
		    0, 0, (ClientData)NIL);
    prefixPtr->hdrPtr = (FsHandleHeader *)NIL;
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
ReturnStatus
LocatePrefix(fileName, domainTypePtr, hdrPtrPtr)
    char	*fileName;	/* The prefix to find the server of */
    int		*domainTypePtr;	/* In/Out the type of the domain.  If -1 on
				 * entry then all domain prefix routines
				 * are polled in order to find the domain.
				 * Set upon return to domain type for prefix */
    FsHandleHeader **hdrPtrPtr;	/* The handle that the domain prefix routine
				 * returns for the prefix */
{
    ReturnStatus	status;
    FsUserIDs		ids;
    Proc_ControlBlock	*procPtr;	/* Used to get process IDs */
    int			domainType;

    procPtr = Proc_GetEffectiveProc();
    FsSetIDs(procPtr, &ids);
    domainType = *domainTypePtr;
    if (domainType < 0) {
	/*
	 * Iterate through the domain prefix location routines because
	 * we don't know what kind of domain it is.
	 */
	for (domainType = 0; domainType < FS_NUM_DOMAINS; domainType++) {
	    status = (*fsDomainLookup[domainType][FS_DOMAIN_PREFIX])
			((ClientData)NIL, fileName, (Address)&ids,
			 (Address)hdrPtrPtr, (FsRedirectInfo **)NIL);
	    if (status == SUCCESS) {
		break;
	    }
	}
    } else {
	/*
	 * Have seen this prefix before so we try the same type of domain.
	 */
	status = (*fsDomainLookup[domainType][FS_DOMAIN_PREFIX])
		    ((ClientData)NIL, fileName, (Address)&ids,
		     (Address)hdrPtrPtr, (FsRedirectInfo **)NIL);
    }
    if (status == SUCCESS) {
	*domainTypePtr = domainType;
	return(FS_NEW_PREFIX);
    } else {
	return(FS_FILE_NOT_FOUND);
    }
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
 *	May call LocatePrefix and FsPrefixInstall.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GetPrefix(fileName, hdrPtrPtr, rootIDPtr, lookupNamePtr, domainTypePtr, prefixPtrPtr)
    char 	*fileName;		/* File name that needs to be 
					 * operated on */
    FsHandleHeader **hdrPtrPtr;		/* Result, handle for the prefix */
    FsFileID	*rootIDPtr;		/* Result, ID of domain root */
    char 	**lookupNamePtr;	/* Result, remaining pathname to 
					 * lookup */
    int 	*domainTypePtr;		/* Result, domain type of the prefix */
    FsPrefix 	**prefixPtrPtr;		/* Result, reference to prefix table */
{
    ReturnStatus status;
    do {
	if (fsFileNameTrace) {
	    Sys_Printf("Lookup: %s,", fileName);
	}
	status = FsPrefixLookup(fileName, FS_IMPORTED_PREFIX,
				FS_LOCALHOST_ID, hdrPtrPtr, rootIDPtr,
				lookupNamePtr, domainTypePtr, prefixPtrPtr);
	if (status == FS_NO_HANDLE) {
	    /*
	     * The prefix exists but there is not a valid file handle for it.
	     * FsPrefixLookup has returned us the prefix in lookupName.
	     */
	    Sys_Printf("Broadcasting for server of \"%s\"\n", *lookupNamePtr);
	    status = LocatePrefix(*lookupNamePtr, domainTypePtr,
						    hdrPtrPtr);
	    if (status == FS_NEW_PREFIX) {
		fsStats.prefix.found++;
		FsPrefixInstall(*lookupNamePtr, *hdrPtrPtr, *domainTypePtr, 
				FS_IMPORTED_PREFIX);
	    }
	}
    } while (status == FS_NEW_PREFIX);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixReopen --
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
FsPrefixReopen(serverID)
    int serverID;		/* Server we are recovering with */
{
    FsPrefix *prefixPtr;
    ReturnStatus status;
    FsHandleHeader *hdrPtr;
    int domainType;
    List_Links nilPrefixList;

    GetNilPrefixes(&nilPrefixList);
    while (!List_IsEmpty(&nilPrefixList)) {
	prefixPtr = (FsPrefix *)List_First(&nilPrefixList);
	if (prefixPtr->serverID == serverID ||
	    prefixPtr->serverID == -1) {
	    /*
	     * Attempt to re-establish the prefix table entry before
	     * re-opening files under that prefix.  This is needed
	     * because the prefix table slot is a point of synchronization
	     * between opens and re-opens.
	     */
	    domainType = -1;
	    status = LocatePrefix(prefixPtr->prefix, &domainType, &hdrPtr);
	    if (status == FS_NEW_PREFIX) {
		FsPrefixInstall(prefixPtr->prefix, hdrPtr, domainType,
			    FS_IMPORTED_PREFIX);
	    }
	}
	List_Remove((List_Links *)prefixPtr);
	Mem_Free(prefixPtr->prefix);
	Mem_Free((Address)prefixPtr);
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
 *	Mem_Allocs, our caller should free each element in the list.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
GetNilPrefixes(listPtr)
    List_Links *listPtr;	/* Header for list of prefix table entries */
{
    FsPrefix *prefixPtr;

    LOCK_MONITOR;

    List_Init(listPtr);
    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (prefixPtr->hdrPtr == (FsHandleHeader *)NIL) {
	    register FsPrefix *newPrefixPtr;

	    newPrefixPtr = Mem_New(FsPrefix);
	    *newPrefixPtr = *prefixPtr;
	    newPrefixPtr->prefix = Mem_Alloc(newPrefixPtr->prefixLength + 1);
	    (void)String_Copy(prefixPtr->prefix, newPrefixPtr->prefix);
	    List_Insert((List_Links *)newPrefixPtr, LIST_ATREAR(listPtr));
	}
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixOpenCheck --
 *
 *	This is called to indicate that an open is occuring in
 *	this domain.  This will fail if recovery is in progress
 *	with the server.
 *
 * Results:
 *	FS_DOMAIN_UNAVAILABLE if the prefix is locked up because recovery
 *	actions are in progress.
 *
 * Side effects:
 *	Blocks recovery until
 *	FsPrefixOpenDone is called.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
FsPrefixOpenCheck(prefixHdrPtr)
    FsHandleHeader *prefixHdrPtr;	/* Handle from the prefix table */
{
    register ReturnStatus status;
    FsPrefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (FsHandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID ==
		prefixHdrPtr->fileID.serverID)) {
	    if (prefixPtr->delayOpens) {
		Sys_Panic(SYS_WARNING, 
		    "FsPrefixOpenCheck waiting for recovery\n");
		if (Sync_Wait(&prefixPtr->okToOpen, TRUE)) {
		    /*
		     * Wait was interrupted by a signal.
		     */
		    status = FS_DOMAIN_UNAVAILABLE;
		} else {
		    prefixPtr->activeOpens++;
		    status = SUCCESS;
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
    Sys_Panic(SYS_WARNING, "PrefixOpenCheck: didn't find prefix");
    UNLOCK_MONITOR;
    return(FS_DOMAIN_UNAVAILABLE);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixOpenDone --
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
FsPrefixOpenDone(prefixHdrPtr)
    FsHandleHeader *prefixHdrPtr;	/* Handle from the prefix table */
{
    FsPrefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (FsHandleHeader *)NIL) &&
	    (prefixPtr->hdrPtr->fileID.serverID ==
		prefixHdrPtr->fileID.serverID)) {
	    prefixPtr->activeOpens--;
	    if (prefixPtr->activeOpens < 0) {
		Sys_Panic(SYS_WARNING, "FsPrefixOpenDone, neg open cnt\n");
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
    Sys_Panic(SYS_WARNING, "PrefixOpenDone: no handle match\n");
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixRecoveryCheck --
 *
 *	This is called to indicate that we want to recover handles with
 *	this server.  This will block the calling process until any
 *	outstanding opens are completed so that opens and re-opens don't race.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Blocks recovery until FsPrefixOpenDone is called.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsPrefixRecoveryCheck(serverID)
    int serverID;
{
    FsPrefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (FsHandleHeader *)NIL) &&
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
 * FsPrefixAllowOpens --
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
FsPrefixAllowOpens(serverID)
    int serverID;		/* Server we are recovering with */
{
    FsPrefix *prefixPtr;

    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (FsHandleHeader *)NIL) &&
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
 * FsPrefixFromFileID --
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
ENTRY FsPrefix *
FsPrefixFromFileID(fileIDPtr)
    FsFileID	*fileIDPtr;		/* FileID from a client */
{
    FsPrefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (FsHandleHeader *)NIL) &&
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
    return((FsPrefix *)NIL);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPrefixOpenInProgress --
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
 *	FsPrefixOpenDone is called.
 *
 *----------------------------------------------------------------------
 */
ENTRY int
FsPrefixOpenInProgress(fileIDPtr)
    FsFileID *fileIDPtr;		/* ID for some file */
{
    int activeOpens;
    FsPrefix *prefixPtr;
    LOCK_MONITOR;

    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if ((prefixPtr->hdrPtr != (FsHandleHeader *)NIL) &&
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
 * Fs_PrefixDump --
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

ENTRY ReturnStatus
Fs_PrefixDump(index, argPtr)
    int index;		/* Prefix table index, -1 means dump to console */
    Address argPtr;	/* Buffer space for entry */
{
    register FsPrefix *prefixPtr;	/* Pointer to table entry */
    Boolean	foundPrefix = FALSE;
    int i;

    LOCK_MONITOR;

    i = 0;
    LIST_FORALL(prefixList, (List_Links *)prefixPtr) {
	if (index < 0) {
	    /*
	     * Dump the prefix entry to the console.
	     */
	    Sys_Printf("%-20s ", prefixPtr->prefix);
	    if (prefixPtr->hdrPtr != (FsHandleHeader *)NIL) {
		Sys_Printf("(%d,%d,%d,%x) ",
			      prefixPtr->hdrPtr->fileID.serverID,
			      prefixPtr->hdrPtr->fileID.type,
			      prefixPtr->hdrPtr->fileID.major,
			      prefixPtr->hdrPtr->fileID.minor);
	    } else {
		Sys_Printf(" (no handle) ");
	    }
	    if (prefixPtr->flags & FS_LOCAL_PREFIX) {
		Sys_Printf(" import ");
	    }
	    if (prefixPtr->flags & FS_IMPORTED_PREFIX) {
		Sys_Printf(" import ");
	    }
	    if (prefixPtr->flags & FS_EXPORTED_PREFIX) {
		Sys_Printf(" export ");
	    }
	    Sys_Printf("\n");
	} else if (i == index) {
	    Fs_Prefix userPrefix;
	    if (prefixPtr->hdrPtr != (FsHandleHeader *)NIL) {
		register FsFileID *fileIDPtr =
			&prefixPtr->hdrPtr->fileID;
		userPrefix.serverID	= fileIDPtr->serverID;
		userPrefix.domain	= fileIDPtr->major;
		userPrefix.fileNumber	= fileIDPtr->minor;
		userPrefix.version	= fileIDPtr->type;
		if (FsDomainInfo(prefixPtr->hdrPtr,
				 &userPrefix.domainInfo) != SUCCESS) {
		    userPrefix.domainInfo.maxKbytes = -1;
		    userPrefix.domainInfo.freeKbytes = -1;
		    userPrefix.domainInfo.maxFileDesc = -1;
		    userPrefix.domainInfo.freeFileDesc = -1;
		}
	    } else {
		userPrefix.serverID	= -1;
		userPrefix.domain	= -1;
		userPrefix.fileNumber	= -1;
		userPrefix.version	= -1;
		userPrefix.domainInfo.maxKbytes = -1;
		userPrefix.domainInfo.freeKbytes = -1;
		userPrefix.domainInfo.maxFileDesc = -1;
		userPrefix.domainInfo.freeFileDesc = -1;
	    }
	    userPrefix.flags = prefixPtr->flags;
	    if (prefixPtr->prefixLength >= FS_USER_PREFIX_LENGTH) {
		Byte_Copy(FS_USER_PREFIX_LENGTH, (Address)prefixPtr->prefix,
				(Address)userPrefix.prefix);
		userPrefix.prefix[FS_USER_PREFIX_LENGTH-1] = '\0';
	    } else {
		(void)String_Copy(prefixPtr->prefix, userPrefix.prefix);
	    }
	    Vm_CopyOut(sizeof(Fs_Prefix), (Address)&userPrefix, argPtr);
	    foundPrefix = TRUE;
	    break;
	}
	i++;
    }
    UNLOCK_MONITOR;
    if (index < 0 || foundPrefix) {
	return(SUCCESS);
    } else {
	return(FS_INVALID_ARG);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PrefixDumpExport --
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
Fs_PrefixDumpExport(size, buffer)
    int size;		/* Size of buffer in bytes */
    Address buffer;	/* Buffer space for prefix then export list */
{
    FsPrefix *prefixPtr;	/* Pointer to table entry */
    char     prefix[FS_MAX_NAME_LENGTH];
    FsHandleHeader *hdrPtr;
    FsFileID rootID;
    char *name;
    int domain;
    int length;
    ReturnStatus status;

    if (Fs_StringNCopy(FS_MAX_NAME_LENGTH, buffer, prefix, &length) !=
	    SUCCESS) {
	return(SYS_ARG_NOACCESS);
    } else if (length == FS_MAX_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }
    status = FsPrefixLookup(prefix, FS_EXACT_PREFIX, rpc_SpriteID,
	    &hdrPtr, &rootID, &name, &domain, &prefixPtr);
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

ENTRY ReturnStatus
DumpExportList(prefixPtr, size, buffer)
    FsPrefix *prefixPtr;
    int size;
    char *buffer;
{
    int *exportList;
    int *iPtr;
    ReturnStatus status;
    FsPrefixExport *exportPtr;

    LOCK_MONITOR;
    if (size > 1000 * sizeof(int)) {
	status = FS_INVALID_ARG;
    } else {
	exportList = (int *)Mem_Alloc(size);
	Byte_Zero(size, (Address)exportList);
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

