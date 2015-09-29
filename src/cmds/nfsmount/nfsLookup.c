/*
 * nfsLookup.c --
 * 
 *	The main NFS pathname lookup procedure.  NFS does lookup component
 *	at a time, while the Sprite interface is based on complete relative
 *	names.  The NfsLookup procedure takes care of chopping the relative
 *	name into components and doing NFS calls to get the handle for
 *	each successive component.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/nfsmount/RCS/nfsLookup.c,v 1.5 91/10/20 12:38:28 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

#include "nfs.h"
#include "sys/time.h"
#include "sys/stat.h"

#include "kernel/fs.h"

static int ExpandLink();
static void LookupCacheInsert();
static int CheckMountPoint();

NfsReadTest() { return; } ;

/*
 * This array holds the names of mount points within our namespace
 * so that we can force redirect lookups. The array is initialized
 * from the command line options using the callback function
 * NfsRecordMountPointProc.
 */
#define MAXMOUNTPOINTS 10

typedef struct RedirectNode {
    char *local;              /* Name (relative) in this filesystem */
    char *remote;             /* Name (absolute) where subtree is mounted */
} RedirectNode;

static RedirectNode redirectList[MAXMOUNTPOINTS];
static int redirectCount = 0;


/*
 *----------------------------------------------------------------------
 *
 * NfsLookupTest --
 *
 *	Stub on top of NfsLookup to call it and exercise it.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints out the returned attributes of the file.
 *
 *----------------------------------------------------------------------
 */
void
NfsLookupTest(private, name)
    ClientData private;
    char *name;
{
    NfsState *nfsPtr = (NfsState *)private;
    diropokres dirResults;
    diropargs dirArgs;
    diropargs *dirArgsPtr = &dirArgs;
    fattr *attrPtr = &dirResults.attributes;
    int status;
    char component[NFS_MAXNAMLEN];
    Fs_RedirectInfo redirectInfo;

    dirArgs.name = component;
    status = NfsLookup(nfsPtr, NULL , name, FS_FOLLOW,
		       &dirResults, &dirArgsPtr, &redirectInfo);
    if (status == EREMOTE) {
	printf("REDIRECT => \"%s\"\n", redirectInfo.fileName);
    } else if (status == 0) {
	register int *intPtr;

	printf("Handle of %s:%s/%s\n\t", nfsPtr->host, nfsPtr->nfsName, name);
	NfsHandlePrint(&dirResults.file);
	printf("\n");
	printf("Parent handle\n\t");
	NfsHandlePrint(&dirArgsPtr->dir);
	printf("\n");

	printf("Attributes of %s:%s/%s\n", nfsPtr->host, nfsPtr->nfsName, name);
	printf("\tFileID %x FS_ID %x\n",
		attrPtr->fileid,
		attrPtr->fsid);
	printf("\tType %d mode 0%o links %d size %d\n",
		attrPtr->type,
		attrPtr->mode,
		attrPtr->nlink,
		attrPtr->size);
    } else {
	if (dirArgsPtr != (diropargs *)NULL) {
	    printf("Parent of %s\n\t", dirArgsPtr->name);
	    NfsHandlePrint(&dirArgsPtr->dir);
	    printf("\n");
	}
	printf("Lookup of %s:%s/%s returns <%d>\n",
		nfsPtr->host, nfsPtr->nfsName, name, status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NfsLookup --
 *
 *	Called to return a nfs_fh for a path inside NFS.  Because NFS only
 *	does component-at-a-time lookup we have to loop through the
 *	pathname ourselves.  This routine is modeled very closely on
 *	the Sprite kernel routine Fs_LocalLookup.
 * 
 * Results:
 *	This returns an error code from NFS, or -1 if NFS access failed
 *	entirely.  Upon NFS_OK return then *dirResultsPtr is filled in
 *	with the handle and attributes for the name looked up.  If
 *	dirArgsPtrPtr is non-null then this indirectly references storage
 *	for the last pathname component and the handle of its parent.
 *	This information is filled in if possible, otherwise the pointer
 *	is cleared to indicate the information isn't available.
 *	EREMOTE is returned if a symbolic link to the root was
 *	encountered, or the pathname left via ".." out the top of the
 *	NFS system.  *redirectInfoPtr gets filled in with the new
 *	pathname in this case.
 *
 * Side effects:
 *	None (yet).
 *
 *----------------------------------------------------------------------
 */
int
NfsLookup(nfsPtr, cwdFilePtr, name, useFlags, dirResultsPtr, dirArgsPtrPtr,
	redirectInfoPtr)
    NfsState *nfsPtr;		/* Top level state for NFS connection */
    NfsOpenFile *cwdFilePtr;	/* Handle for directory at start of path.
				 * If NULL the mount point is used. */
    char *name;			/* Name relative to cwdHandle */
    int useFlags;		/* Sprite Lookup flags.  FS_FOLLOW means
				 * follow symbolic links. */
    diropokres *dirResultsPtr;	/* Return - handle and attributes of name */
    diropargs **dirArgsPtrPtr;	/* If non-NULL **dirArgsPtrPtr is filled in
				 * with the last component of name and the
				 * handle for the parent directory.  This is
				 * used when creating, removing, etc. Note
				 * that (*dirArgsPtrPtr)->name must point
				 * to storage for the pathname component. */
    Fs_RedirectInfo *redirectInfoPtr; /* Returned name if a symbolic link to the
				 * root is encountered.  Return value of
				 * the procedure is EREMOTE in this case. */
{
    register char *curCharPtr;
    register int compLen = -1;
    register char *compPtr;
    register nfs_fh *cwdHandlePtr;
    register int status = (int) NFS_OK;
    int numLinks = 0;
    int haveGoodAttrs = 0;
    diropargs *returnDirArgsPtr;
    diropargs dirArgs;
    diropres dirResults;
    char component[NFS_MAXNAMLEN];
    char expandedPath[NFS_MAXNAMLEN];
    int isMountPoint;

    /*
     * Initialize lookup.  Skip leading slashs, get a handle for the
     * current starting point, fill in the diropargs that we'll be
     * passing to the NFS server.
     */
    redirectInfoPtr->prefixLength = 0;		/* no remote links in NFS */
    if (dirArgsPtrPtr != (diropargs **)NULL) {
	returnDirArgsPtr = *dirArgsPtrPtr;
	*dirArgsPtrPtr = (diropargs *)NULL;	/* to indicate no parent, yet */
    } else {
	returnDirArgsPtr = (diropargs *)NULL;
    }
    curCharPtr = name;
    while (*curCharPtr && *curCharPtr == '/') {
	curCharPtr++;
    }
    if (cwdFilePtr == (NfsOpenFile *)NULL) {
	cwdHandlePtr = nfsPtr->mountHandle;
    } else {
	cwdHandlePtr = cwdFilePtr->handlePtr;
    }
    dirArgs.name = component;
    /*
     * Loop through the pathname.
     */
    while (*curCharPtr != '\0') {
	/*
	 * Copy the next component and skip trailing slashes.
	 */
	compPtr = component;
	while (*curCharPtr != '/' && *curCharPtr != '\0') {
	    *compPtr++ = *curCharPtr++;
	}
	compLen = compPtr - component;
	if (compLen >= NFS_MAXNAMLEN) {
	    status = NFSERR_NAMETOOLONG;
	    goto exit;
	}
	*compPtr = '\0';
	bcopy((char *)cwdHandlePtr, (char *)&dirArgs.dir, sizeof(nfs_fh));

	isMountPoint = (bcmp((char *)cwdHandlePtr,
			  (char *)nfsPtr->mountHandle,
			  NFS_FHSIZE) == 0);

	if ((isMountPoint) &&
	    (CheckMountPoint(component,redirectInfoPtr->fileName) >= 0)) {
	    (void)strcat(redirectInfoPtr->fileName, curCharPtr);
	    return(EREMOTE);
	}

	while (*curCharPtr == '/') {
	    curCharPtr++;
	}
	if (compLen == 2 && component[0] == '.' && component[1] == '.') {
	    /*
	     * Going to the parent directory, "..".
	     */
	    if (isMountPoint) {
		/*
		 * We are falling off the top of the NFS system.  Copy the
		 * remaining part of the name, including the "../", into
		 * *newNamePtr and return that, instead of a handle.
		 */
		(void)strcpy(redirectInfoPtr->fileName, "../");
		(void)strcat(redirectInfoPtr->fileName, curCharPtr);
		return(EREMOTE);
	    }
	} else if (compLen == 1 && component[0] == '.') {
	    /*
	     * Already have a handle on ".", the current directory.
	     */
	    continue;
	}
	/*
	 * Lookup the next component via NFS.  Remember that the current
	 * handle has been copied into dirArgs, and the result handle
	 * will be copied into dirResults by the XDR routines.
	 */
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_LOOKUP, xdr_diropargs, &dirArgs,
			xdr_diropres, &dirResults, nfsTimeout) != RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NfsLookup: RPC nfsproc_lookup_2");
	    status = -1;
	    goto exit;
	} else if (dirResults.status != NFS_OK) {
	    /*
	     * Lookup failed.  If we failed on the last component we set up
	     * the returned dirArgs so they are useful for a subsequent
	     * create.  returnDirArgsPtr is the original pointer to
	     * good storage provided by our caller.  We set the pointer
	     * to this pointer to indicate the storage now has valid data.
	     */
	    status = (int)dirResults.status;
	    if (*curCharPtr == '\0' && returnDirArgsPtr != (diropargs *)NULL) {
		bcopy((char *)&dirArgs.dir, (char *)&returnDirArgsPtr->dir,
			sizeof(nfs_fh));
		strcpy(returnDirArgsPtr->name, component);
		*dirArgsPtrPtr = returnDirArgsPtr;
	    }
	    goto exit;
	} else {
	    /*
	     * Lookup succeeded.  The lookup result is cached for
	     * a brief period to optimize heavy traffic through top-level
	     * directories.
	     */
	    haveGoodAttrs = 1;
	    cwdHandlePtr = &dirArgs.dir;
	    LookupCacheInsert(cwdHandlePtr, component, &dirResults);

	    if ((*curCharPtr != '\0' || (useFlags & FS_FOLLOW)) &&
		dirResults.diropres_u.diropres.attributes.type == NFLNK) {
		/*
		 * Hit a symbolic link in the middle of a pathname or at
		 * the end are we are told to follow it.
		 */
		numLinks++;
		if (numLinks > FS_MAX_LINKS) {
		    status = ELOOP;
		    goto exit;
		} else {
		    int offset;		/* Distance of existing name from the
					 * start of its buffer */
		    if (numLinks == 1) {
			offset = (int)curCharPtr - (int)name;
		    } else {
			offset = (int)curCharPtr -
				 (int)(expandedPath);
		    }
		    status = ExpandLink(nfsPtr, &dirResults, curCharPtr, offset,
					expandedPath);
		    if (status != NFS_OK) {
			goto exit;
		    }
		    curCharPtr = expandedPath;
		}
		if (*curCharPtr == '/') {
		    /*
		     * A link back to the root has to be handled by the
		     * prefix table in the Sprite kernel.
		     */
		    strcpy(redirectInfoPtr->fileName, expandedPath);
		    return(EREMOTE);
		}
		/*
		 * The link was relative so we reset the current handle
		 * to point to the same place we found the link.
		 */
		cwdHandlePtr = &dirArgs.dir;
	     } else {
		 /*
		  * Not a symbolic link.
		  * Advance the current handle to the component we just found.
		  */
		 cwdHandlePtr = &dirResults.diropres_u.diropres.file;
	     }
	}
    }
    /*
     * Lookup complete.  Copy out our results.
     */
    if (haveGoodAttrs) {
	bcopy((char *)&dirResults.diropres_u.diropres, (char *)dirResultsPtr,
		sizeof(diropokres));
	if (returnDirArgsPtr != (diropargs *)NULL) {
	    /*
	     * Our caller wants information about the parent directory
	     * for a delete, rename, or link.  We restore the pointer
	     * to indicate that this information has been provided.
	     */
	    bcopy((char *)&dirArgs.dir, (char *)&returnDirArgsPtr->dir,
		    sizeof(nfs_fh));
	    strcpy(returnDirArgsPtr->name, component);
	    *dirArgsPtrPtr = returnDirArgsPtr;
	}
    } else {
	/*
	 * We didn't go throught the lookup loop because our caller asked
	 * for an empty name.  We do an explicit get attributes.
	 */
	register attrstat *tmpAttrPtr;
	tmpAttrPtr = nfsproc_getattr_2(cwdHandlePtr, nfsPtr->nfsClnt);
	if (tmpAttrPtr == (attrstat *)NULL) {
	    clnt_perror(nfsPtr->nfsClnt, "NfsLookup: RPC  nfsproc_getattr_2");
	    status = -1;
	    goto exit;
	} else if (tmpAttrPtr->status != NFS_OK) {
	    status = (int)tmpAttrPtr->status;
	    goto exit;
	} else {
	    bcopy((char *)cwdHandlePtr, (char *)&dirResultsPtr->file,
		    NFS_FHSIZE);
	    bcopy((char *)&tmpAttrPtr->attrstat_u.attributes,
		  (char *)&dirResultsPtr->attributes, sizeof(fattr));
	}
    }
exit:
    if (status == -1 && errno != 0) {
	status = errno;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * LookupCacheInsert --
 *
 *	Tuck away a <nfs_fh, component> => <nfs_fh> mapping.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds an entry into the component cache.
 *
 *----------------------------------------------------------------------
 */
static void
LookupCacheInsert(component, dirResultsPtr)
    char *component;		/* Pathname component just looked up. */
    diropres *dirResultsPtr;	/* Return info from NFS server */
{
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * ExpandLink --
 *	Expand a link by shifting the remaining pathname over and
 *	inserting the contents of the link file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Expands the link into nameBuffer.
 *
 *----------------------------------------------------------------------
 */
static int
ExpandLink(nfsPtr, dirResultsPtr, curCharPtr, offset, nameBuffer)
    NfsState	*nfsPtr;		/* Needed for ReadLink RPC */
    diropres	*dirResultsPtr;		/* Handle + attrs of the link file */
    char	*curCharPtr;		/* Points to beginning of the remaining
					 * name that has to be shifted */
    int		offset;			/* Offset of curCharPtr within its 
					 * buffer */
    char 	nameBuffer[];		/* New buffer for the expanded name */
{
    ReturnStatus 	status;
    int 		linkNameLength;	/* The length of the name in the
					 * link NOT including the Null */
    readlinkres		readLinkResult;
    char		savedC;
    register char 	*srcPtr;
    register char 	*dstPtr;

    linkNameLength = dirResultsPtr->diropres_u.diropres.attributes.size;
    if (*curCharPtr == '\0') {
	/*
	 * There is no pathname to shift around.
	 * Make sure the new name is null terminated
	 */
	nameBuffer[linkNameLength] = '\0';
    } else {
	/*
	 * Set up srcPtr to point to the start of the remaining pathname.
	 * Set up dstPtr to point to just after where the link will be,
	 * ie. just where the '/' is that separates the link value from
	 * the remaining pathname.
	 */
	srcPtr = curCharPtr;
	dstPtr = &nameBuffer[linkNameLength];
	if (linkNameLength < offset) {
	    /*
	     * The remaining name has to be shifted left.
	     * For example, /first is a link to /usr (or /users).  With the
	     * filename /first/abc, curCharPtr will point to the 'a' in "abc".
	     * dstPtr points to the '\0' after "/usr".
	     *
	     *  / f i r s t / a b c \0		{ current name, offset = 7 }
	     *  / u s r \0			{ link name, len = 4 }
	     *  / u s e r s \0			{   or len = 6 }
	     *  / u s r / a b c			{ final result }
	     *
	     * Insert the separating '/' first, then start at the beginning
	     * of the remaining name and copy it into the new buffer.
	     */
	    *dstPtr = '/';
	    dstPtr++;
	    for( ; ; ) {
		*dstPtr = *srcPtr;
	        if (*srcPtr == '\0') {
		    break;
		}
		dstPtr++;
		srcPtr++;
	    }
	} else {
	    /*
	     * The remaining name has to be shifted right.
	     * For example, /first is a link to /usr/tmp.  With the filename
	     * /first/abc, curCharPtr will point to the 'a' in "abc".
	     * dstPtr points to the '\0' after "/usr/tmp".
	     *
	     * / f i r s t / a b c \0		{ current name, offset = 7 }
	     * / u s r / t m p \0		{ link name, len = 8 }
	     * / u s r / t m p / a b c		{ final result }
	     * 
	     * Zoom to the end of the name (adjusting dstPtr to account
	     * for where the '/' will go) and then shift the name right.
	     */
	    dstPtr++;
	    while (*srcPtr != '\0') {
		srcPtr++;
		dstPtr++;
	    }
	    while (srcPtr >= curCharPtr) {
		*dstPtr = *srcPtr;
		dstPtr--;
		srcPtr--;
	    }
	    *dstPtr = '/';
	}
    }
    /*
     * Read and insert the link name in front of the remaining name
     * that was just shifted over.  By initializing the character string
     * pointer (data) before the RPC the XDR routines know to use
     * the existing buffer and not allocate a new one.  Also, the XDR routines
     * are going to null terminate the string and stomp on the character
     * after the link value, so we save and restore that.
     */
    readLinkResult.readlinkres_u.data = nameBuffer;
    savedC = nameBuffer[linkNameLength];
    if (clnt_call(nfsPtr->nfsClnt, NFSPROC_READLINK, xdr_nfs_fh,
		&dirResultsPtr->diropres_u.diropres.file, xdr_readlinkres,
		&readLinkResult, nfsTimeout) != RPC_SUCCESS) {
	clnt_perror(nfsPtr->nfsClnt, "nfsproc_lookup_2");
	if (errno != 0) {
	    return(errno);
	} else {
	    return(-1);
	}
    }
    nameBuffer[linkNameLength] = savedC;
    return(readLinkResult.status);
}

/*
 *----------------------------------------------------------------------
 *
 * NfsWriteTest --
 *
 *	Test harness for NFS writing.  This opens a NFS file and copies
 *	a Sprite file to an NFS file.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints out the returned attributes of the file.
 *
 *----------------------------------------------------------------------
 */
void
NfsWriteTest(private, spriteName, nfsName)
    ClientData private;
    char *spriteName;
    char *nfsName;
{
    NfsState *nfsPtr = (NfsState *)private;
    diropres longDirResults;
    diropokres *dirResultsPtr = &longDirResults.diropres_u.diropres;
    int status;
    char *newName;
    createargs createArgs;
    diropargs *wherePtr = &createArgs.where;
    sattr *sattrPtr = &createArgs.attributes;
    char component[NFS_MAXNAMLEN];
    Fs_RedirectInfo redirectInfo;

    wherePtr->name = component;
    status = NfsLookup(nfsPtr, NULL /* fix */, nfsName,
			FS_FOLLOW, dirResultsPtr, &wherePtr, &redirectInfo);
    if (status == EREMOTE) {
	printf("REDIRECT => \"%s\"\n", redirectInfo.fileName);
    } else if (status == NFSERR_NOENT && wherePtr != (diropargs *)NULL) {
	sattrPtr->mode = 0644;
	sattrPtr->uid = 1020;	/* non-existent! */
	sattrPtr->gid = 94;		/* non-existent! */
	sattrPtr->size = 0;
	sattrPtr->atime.seconds = -1;
	sattrPtr->atime.useconds = -1;
	sattrPtr->mtime.seconds = -1;
	sattrPtr->mtime.useconds = -1;
	if (clnt_call(nfsPtr->nfsClnt, NFSPROC_CREATE, xdr_createargs,
		&createArgs, xdr_diropres, &longDirResults, nfsTimeout)
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_CREATE");
	    return;
	}
	status = longDirResults.status;
	printf("Create of %s:%s/%s returns <%d>\n",
		nfsPtr->host, nfsPtr->nfsName, nfsName, status);

    } else {
	printf("Lookup of %s:%s/%s returns <%d>\n",
		nfsPtr->host, nfsPtr->nfsName, nfsName, status);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * NfsRecordMountPointProc --
 *
 *	Add a sub-mount point to our list.
 * 
 *      This routine is called by the Opt_Parse routine when it sees 
 *      a -m option.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      NFS allows one file system to be mounted within another NFS
 *      namespace.  This causes problems for Sprite since it expects
 *      to have a remote link at each mount point. The kluge of a
 *      solution is to tell nfsmount where the sub-mount points are
 *      in its namespace so that it can return EREMOTE when a lookup
 *      enters one of these subdomains.
 *
 *----------------------------------------------------------------------
 */

int
NfsRecordMountPointProc(option, argc, argv)
    char *option;             /* should point to 'm' */
    int argc;                 /* number of remaining args */
    char **argv;              /* arg pointers */
{
    int i;
    struct stat statBuf;

    if ((argc < 2) ||
	(*argv[0] == '-') ||
	(*argv[0] == '/') ||
	(*argv[1] != '/')) {
	fprintf(stderr, "Usage -m option: relative-local-name absolute-remote-name\n");
        exit(-1);
    }

    if (redirectCount >= MAXMOUNTPOINTS) {
	fprintf(stderr, "Too many mount points specified. (%d max)\n",
		MAXMOUNTPOINTS);
        exit(-1);
    }

    if (lstat(argv[1], &statBuf) < 0) {
	perror(argv[1]);
	exit(-1);
    }

    if ((statBuf.st_mode & S_IFMT) != S_IFRLNK) {
	fprintf(stderr,"Not a remote link: %s\n", argv[1]);
	exit(-1);
    }

    redirectList[redirectCount].local = argv[0];
    redirectList[redirectCount].remote = argv[1];
    redirectCount++;
    
    for (i=0,argc-=2; i<argc; i++) {
	argv[i] = argv[i+2];
    }

    /* tell Opt_Parse that we consumed 2 arguments */
    return argc;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckMountPoint --
 *
 *	Check a name against the mount point list.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Uses but does not modify the global redirectList.
 *
 *----------------------------------------------------------------------
 */

static int
CheckMountPoint(name, redirectPtr)
    char *name;               /* name of local file to verify */
    char *redirectPtr;        /* name to redirect to */
{
    int i;

    for (i=0; i<redirectCount; i++) {
	if (strcmp(redirectList[i].local,name) == 0) {
	    strcpy(redirectPtr, redirectList[i].remote);
	    return i;
	}
    }

    return -1;

}
