/* 
 * fslclLookup.c --
 *
 *	The routines in the module manage the directory structure.
 *	The top level loop is in FslclLookup, and it is the workhorse
 *	of the Local Domain that is called by procedures like FslclOpen.
 *	Files and directories are also created, deleted, and renamed
 *	directly (or indirectly) through FslclLookup.
 *
 *	Support for heterogenous systems is done here by expanding "$MACHINE"
 *	in pathnames to a string like "sun3" or "spur".
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


#include <sprite.h>
#include <fs.h>
#include <fsconsist.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsprefix.h>
#include <fsdm.h>
#include <fslclInt.h>
#include <fslcl.h>
#include <fslclNameHash.h>
#include <fscache.h>
#include <fsutilTrace.h>
#include <fsStat.h>
#include <rpc.h>
#include <net.h>
#include <vm.h>
#include <string.h>
#include <proc.h>
#include <dbg.h>
#ifdef SOSP91
#include  <sospRecord.h>
#include <timer.h>
#endif

/*
 * Debugging flags.
 */
int fslclComponentTrace = FALSE;
extern int fsprefix_FileNameTrace;

int fsCompacts;		/* The number of times a directory block was so
			 * fragmented that we could have compacted it to
			 * make room for a new entry in the directory */
/*
 * A cache of recently seen pathname components is kept in a hash table.
 * The hash table gets initialized in Fsdm_AttachDisk after the first disk
 * gets attached.
 * The name caching can be disabled by setting the fslclNameCaching flag to FALSE.
 */

FslclHashTable fslclNameTable;
FslclHashTable *fslclNameTablePtr = (FslclHashTable *)NIL;
Boolean fslclNameCaching = TRUE;
int fslclNameHashSize = FSLCL_NAME_HASH_SIZE;

/*
 * Forward Declarations.
 */

static ReturnStatus FindComponent _ARGS_((Fsio_FileIOHandle *parentHandlePtr, 
		char *component, int compLen, Boolean isDotDot, 
		Fsio_FileIOHandle **curHandlePtrPtr, int *dirOffsetPtr));
static ReturnStatus InsertComponent _ARGS_((Fsio_FileIOHandle *curHandlePtr, 
		char *component, int compLen, int fileNumber,
		int *dirOffsetPtr));
static ReturnStatus DeleteComponent _ARGS_((Fsio_FileIOHandle *parentHandlePtr,
		char *component, int compLen, int *dirOffsetPtr));
static ReturnStatus ExpandLink _ARGS_((Fsio_FileIOHandle *curHandlePtr, 
		char *curCharPtr, int offset, char nameBuffer[]));
static ReturnStatus GetHandle _ARGS_((int fileNumber, 
		Fsdm_FileDescriptor *newDescPtr, 
		Fsio_FileIOHandle *curHandlePtr, char *name, 
		Fsio_FileIOHandle **newHandlePtrPtr));
static ReturnStatus CreateFile _ARGS_((Fsdm_Domain *domainPtr, 
		Fsio_FileIOHandle *parentHandlePtr, char *component, 
		int compLen, int fileNumber, int type, int permissions, 
		Fs_UserIDs *idPtr, Fsio_FileIOHandle **curHandlePtrPtr, 
		int *dirOffsetPtr));
static ReturnStatus WriteNewDirectory _ARGS_((Fsio_FileIOHandle *curHandlePtr,
		Fsio_FileIOHandle *parentHandlePtr));
static ReturnStatus LinkFile _ARGS_((Fsio_FileIOHandle *parentHandlePtr,
		char *component, int compLen, int fileNumber, int logOp, 
		Fsio_FileIOHandle **curHandlePtrPtr));
static ReturnStatus OkToMoveDirectory _ARGS_((
		Fsio_FileIOHandle *newParentHandlePtr, 
		Fsio_FileIOHandle *curHandlePtr));
static ReturnStatus MoveDirectory _ARGS_((Time *modTimePtr, 
		Fsio_FileIOHandle *newParentHandlePtr, 
		Fsio_FileIOHandle *curHandlePtr));
static ReturnStatus GetParentNumber _ARGS_((Fsio_FileIOHandle *curHandlePtr,
		int *parentNumberPtr));
static ReturnStatus SetParentNumber _ARGS_((Fsio_FileIOHandle *curHandlePtr, 
		int newParentNumber));
static ReturnStatus DeleteFileName _ARGS_((Fsio_FileIOHandle *parentHandlePtr,
		Fsio_FileIOHandle *curHandlePtr, char *component, 
		int compLen, int forRename, Fs_UserIDs *idPtr, int logOp));
static void	CloseDeletedFile _ARGS_((Fsio_FileIOHandle **parentHandlePtrPtr,
					Fsio_FileIOHandle **curHandlePtrPtr));
static Boolean DirectoryEmpty _ARGS_((Fsio_FileIOHandle *handlePtr));
static ReturnStatus CheckPermissions _ARGS_((Fsio_FileIOHandle *handlePtr, 
		int useFlags, Fs_UserIDs *idPtr, int type));
static ReturnStatus CacheDirBlockWrite _ARGS_((Fsio_FileIOHandle *handlePtr, 
		Fscache_Block *blockPtr, int blockNum, int length));


/*
 *----------------------------------------------------------------------
 *
 * FslclLookup --
 *
 *	The guts of local file name lookup.  This does a recursive
 *	directory lookup of a file pathname.  The success of the lookup
 *	depends on useFlags and the type.  The process needs to
 *	have read permission along the path, and other permissions on
 *	the target file itself according to useFlags. The type of the
 *	target file has to agree with the type parameter.
 *
 *	The major and minor fields of the Fs_FileID for local files correspond
 *	to the domain and fileNumber, respectively, of a file.  The domain
 *	is an index into the set of active domains (disks), and the fileNumber
 *	is an index into the array of file descriptors on disk.
 *
 * Results:
 *	If SUCCESS is returned then *handlePtrPtr contains a valid file
 *	handle.  This handle should be released with FsLocalClose.
 *	If FS_LOOKUP_REDIRECT is returned then **newNameInfoPtrPtr contains
 *	the new file name that the client takes back to its prefix table.
 *
 * Side effects:
 *	After a successful lookup the returned handle is locked and has
 *	another reference to it.  Also, the domain in which the file was
 *	found has an extra reference that needs to be released with
 *	Fsdm_DomainRelease as soon as our caller is finished with the handle.
 *
 *----------------------------------------------------------------------
 */
#ifdef SOSP91
Timer_Ticks totalNameTime = {0};
Fs_FileID NullFileID = {0};
#endif
ReturnStatus
FslclLookup(prefixHdrPtr, relativeName, rootIDPtr, useFlags, type, clientID,
	    idPtr, permissions, fileNumber, handlePtrPtr, newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHdrPtr;	/* Handle from the prefix table or
					 * the current working directory */
    char *relativeName;			/* Name to lookup relative to the
					 * file indicated by prefixHandlePtr */
    Fs_FileID *rootIDPtr;		/* File ID of the root of the domain */
    int useFlags;			/* FS_READ|FS_WRITE|FS_EXECUTE,
					 * FS_CREATE|FS_EXCLUSIVE, FS_OWNER,
					 * FS_LINK, FS_FOLLOW (links) */
    int type;				/* File type which to succeed on.  If
					 * this is FS_FILE, then any type will
					 * work. */
    int clientID;			/* Host ID of the client doing the open.
					 * Require to properly expand $MACHINE
					 * in pathnames */
    Fs_UserIDs *idPtr;			/* User and group IDs */
    int permissions;			/* Permission bits to use on a newly
					 * created file. */
    int fileNumber;			/* File number to link to if FS_LINK
					 * useFlag is present */
    Fsio_FileIOHandle **handlePtrPtr;	/* Result, the handle for the file.
					 * This is returned locked.  Also,
					 * its domain has a reference which
					 * needs to be released. */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* Redirect Result, the pathname left
					 * after it leaves our domain */
{
    register char 	*curCharPtr;	/* Pointer into the path name */
    Fsio_FileIOHandle *parentHandlePtr; /* Handle for parent dir. */
    register char	*compPtr;	/* Pointer into component. */
    register ReturnStatus status = SUCCESS;
    register int 	compLen = 0;	/* The length of component */
    Fsio_FileIOHandle	*curHandlePtr;	/* Handle for the current spot in
					 * the directory structure */
    Fsdm_Domain *domainPtr;		/* Domain of the lookup */
    char component[FS_MAX_NAME_LENGTH]; /* One component of the path name */
    char *newNameBuffer;		/* Extra buffer used after a symbolic
					 * link has been expanded */
    int numLinks = 0;			/* The number of links that have been
					 * expanded. Used to check against
					 * loops. */
    int	logOp;				/* Dir log operation. */
    int dirFileNumber;			/* File number od directory being 
					 * operated on. */
    int	dirOffset;			/* Offset of directory entry in the
					 * directory being operated on. */
    ClientData	logClientData;		/* Client data for directory change
					 * logging. */
#ifdef SOSP91
#define MAX_RECORDS 20
    char buf[SOSP_LOOKUP_OFFSET+MAX_RECORDS*sizeof(Fs_FileID)];
    Fs_FileID *sospTracePtr = (Fs_FileID*)(buf+SOSP_LOOKUP_OFFSET);
    Timer_Ticks startTicks, endTicks;
    int sospTraceCount = 0;
    Timer_GetCurrentTicks(&startTicks);
    bcopy((Address)prefixHdrPtr, (Address)sospTracePtr,
	    sizeof(Fs_FileID));
    sospTracePtr++;
    sospTraceCount++;
#endif

    /*
     * Get a handle on the domain of the file.  This is needed for disk I/O.
     * Remember that the <major> field of the fileID is a domain number.
     */
    domainPtr = Fsdm_DomainFetch(prefixHdrPtr->fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    if (prefixHdrPtr->fileID.type != FSIO_LCL_FILE_STREAM) {
	printf("FslclLookup, bad prefix handle type <%d> for {%s}%s\n",
	    prefixHdrPtr->fileID.type,
	    Fsutil_HandleName(prefixHdrPtr), relativeName);
	return(FS_DOMAIN_UNAVAILABLE);
    }
#ifdef SOSP91
    SOSP_IN_NAME_LOOKUP_FIELD = 1;
#endif
    /*
     * Duplicate the prefixHandle into the handle for the current point
     * in the directory.  This locks and ups the reference count on the handle.
     */
    curCharPtr = relativeName;
    curHandlePtr = Fsutil_HandleDupType(Fsio_FileIOHandle, prefixHdrPtr);
    parentHandlePtr = (Fsio_FileIOHandle *)NIL;
    newNameBuffer = (char *)NIL;
    /*
     * Loop through the pathname expanding links and checking permissions.
     * Creations and deletions are handled after this loop.
     */
    fs_Stats.lookup.number++;
    while (*curCharPtr != '\0' && status == SUCCESS) {
	status = CheckPermissions(curHandlePtr, FS_READ, idPtr, FS_DIRECTORY);
	if (status != SUCCESS) {
	    break;
	}
	/*
	 * Get the next component.  We make a special check here
	 * for "$MACHINE" embedded in the pathname.  This gets expanded
	 * to a machine type string, i.e. "sun3" or "spur", sort of like
	 * a symbolic link.  The value is dependent on the ID of the client
	 * doing the open.  For the local host we use a compiled in string so
	 * we can bootstrap ok, and for other clients we get the machine
	 * type string from the net module.  (why net?  why not...
	 * Net_InstallRoute installs a host's name and machine type.)
	 */
#define SPECIAL		"$MACHINE"
#define SPECIAL_LEN	8
	compPtr = component;
	while (*curCharPtr != '/' && *curCharPtr != '\0') {
	    if (*curCharPtr == '$' &&
		(strncmp(curCharPtr, SPECIAL, SPECIAL_LEN) == 0)) {
		char machTypeBuffer[32];
		char *machType;

		if (fslclComponentTrace) {
		    printf(" $MACHINE -> ");
		}
		fs_Stats.lookup.numSpecial++;
		if (clientID == rpc_SpriteID) {
		    /*
		     * Can't count on the net stuff being setup for ourselves
		     * as that is done via a user program way after bootting.
		     * Instead, use a compiled in string.  This is important
		     * when opening "/initSprite", which is a link to 
		     * "/initSprite.$MACHINE", when running on the root server.
		     */
		    machType = mach_MachineType;
		} else {
		    Net_SpriteIDToMachType(clientID, 32, machTypeBuffer);
		    if (*machTypeBuffer == '\0') {
			printf(
			 "FslclLookup, no machine type for client %d\n",
				clientID);
			machType = "unknown";
		    } else {
			machType = machTypeBuffer;
		    }
		}
		while (*machType != '\0') {
		    *compPtr++ = *machType++;
		    if (compPtr - component >= FS_MAX_NAME_LENGTH) {
			status = FS_FILE_NOT_FOUND;
			goto endScan;
		    }
		}
		curCharPtr += SPECIAL_LEN;
#undef SPECIAL
#undef SPECIAL_LEN
	    } else {
		*compPtr++ = *curCharPtr++;
	    }
	    if (compPtr - component >= FS_MAX_NAME_LENGTH) {
		status = FS_FILE_NOT_FOUND;
		goto endScan;
	    }
	}
	fs_Stats.lookup.numComponents++;
	*compPtr = '\0';
	compLen = compPtr - component;
	if (fslclComponentTrace) {
	    printf(" %s ", component);
	}
	/*
	 * Skip intermediate and trailing slashes so that *curCharPtr
	 * is Null when 'component' has the last component of the name.
	 */
	while (*curCharPtr == '/') {
	    curCharPtr++;
	}
	/*
	 * There are three cases for what the next component is:
	 * a sub-directory (or file), dot, and dot-dot.
	 */
	if ((compLen == 2) && component[0] == '.' && component[1] == '.') {
	    /* 
	     * Going to the parent directory ".."
	     */
	    if (curHandlePtr->hdr.fileID.minor == rootIDPtr->minor) {
		/*
		 * We are falling off the top of the domain.  Make the
		 * remaining part of the filename, including "../",
		 * available to our caller so it can go back to the prefix
		 * table.  Setting the prefixLength to zero indicates
		 * there is no prefix information in this LOOKUP_REDIRECT
		 */
		*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
		(*newNameInfoPtrPtr)->prefixLength = 0;
		(void)strcpy((*newNameInfoPtrPtr)->fileName, "../");
		(void)strcat((*newNameInfoPtrPtr)->fileName, curCharPtr);
		fs_Stats.lookup.parent++;
		status = FS_LOOKUP_REDIRECT;
	    } else {
		/*
		 * Advance curHandlePtr to the parent, "..".
		 * FindComponent will unlock its parentHandlePtr (the directory
		 * containing "..") after scanning it but before grabbing
		 * the handle for "..".  This prevents deadlock with another
		 * process descending the other way along the path.  
		 * We then nuke our own "parent" because it really isn't a
		 * parent anymore.
		 */
		if (parentHandlePtr != (Fsio_FileIOHandle *)NIL) {
		    Fsutil_HandleRelease(parentHandlePtr, TRUE);
		}
		parentHandlePtr = curHandlePtr;
		status = FindComponent(parentHandlePtr, component, compLen,
			    TRUE, &curHandlePtr, &dirOffset);
		/*
		 * Release the handle while being careful about its lock.
		 * The parent handle is normally aready unlocked by
		 * FindComponent, unless the directory is corrupted.
		 */
		Fsutil_HandleRelease(parentHandlePtr, (status != SUCCESS));
		parentHandlePtr = (Fsio_FileIOHandle *)NIL;
	    }
	} else if ((compLen == 1) && component[0] == '.') {
	    /*
	     * Just hang tight with . (dot) because we already
	     * have a locked handle for it.
	     */
	} else {
	    /*
	     * Advance to the next component and keep the handle on
	     * the parent locked so we can do deletes and creates.
	     */
	    if (parentHandlePtr != (Fsio_FileIOHandle *)NIL) {
		Fsutil_HandleRelease(parentHandlePtr, TRUE);
	    }
	    parentHandlePtr = curHandlePtr;
	    status = FindComponent(parentHandlePtr, component, compLen, FALSE,
			&curHandlePtr, &dirOffset);
	}
	/*
	 * At this point we have a locked handle on the current point
	 * in the lookup, and perhaps have a locked handle on the parent.
	 * Links are expanded now so we know whether or not the
	 * lookup is completed.  On the last component, we only
	 * expand the link if the FS_FOLLOW flag is present.
	 */
#ifdef SOSP91
	if (sospTraceCount<MAX_RECORDS && curHandlePtr !=
		(Fsio_FileIOHandle *)NIL) {
	    bcopy((Address)curHandlePtr, (Address)sospTracePtr,
		    sizeof(Fs_FileID));
	    sospTracePtr++;
	    sospTraceCount++;
	}
#endif
	if ((status == SUCCESS) &&
	    ((*curCharPtr != '\0') || (useFlags & FS_FOLLOW)) &&
	    ((curHandlePtr->descPtr->fileType == FS_SYMBOLIC_LINK ||
		curHandlePtr->descPtr->fileType == FS_REMOTE_LINK))) {
	    numLinks++;
	    fs_Stats.lookup.symlinks++;
	    if (numLinks > FS_MAX_LINKS) {
		status = FS_NAME_LOOP;
	    } else {
		/*
		 * An extra buffer is used because the caller probably only
		 * has a buffer just big enough for the name.
		 */
		int 	offset;		/* Distance of existing name from
					 * the start of its buffer */
		if (newNameBuffer == (char *)NIL) {
		    offset = (int)curCharPtr - (int)relativeName;
		    newNameBuffer = (char *)malloc(FS_MAX_PATH_NAME_LENGTH);
		} else {
		    offset = (int)curCharPtr - (int)newNameBuffer;
		}
		status = ExpandLink(curHandlePtr, curCharPtr, offset,
						    newNameBuffer);
		if (status == FS_FILE_NOT_FOUND) {
		    printf( "FslclLookup, empty link \"%s\"\n",
				relativeName);
		}
		curCharPtr = newNameBuffer;
	    }
	    if (status == SUCCESS) {
		/*
		 * (Note: One could enforce permissions on links here.)
		 * If the link is back to the root we have to REDIRECT,
		 * otherwise retreat the current point in the lookup to
		 * the parent directory before continuing.
		 */
		if (*curCharPtr == '/') {
		    *newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
		    (void)strcpy((*newNameInfoPtrPtr)->fileName, curCharPtr);
		    status = FS_LOOKUP_REDIRECT;
		    /*
		     * Return the length of the prefix indicated by
		     * a remote link, zero means no prefix.
		     */
		    if (curHandlePtr->descPtr->fileType == FS_REMOTE_LINK) {
			fs_Stats.lookup.remote++;
			(*newNameInfoPtrPtr)->prefixLength = 
					    curHandlePtr->descPtr->lastByte;
		    } else {
			fs_Stats.lookup.redirect++;
			(*newNameInfoPtrPtr)->prefixLength = 0;
		    }
		} else if (parentHandlePtr != (Fsio_FileIOHandle *)NIL) {
#ifdef SOSP91
		    /*
		     * Put an indication in the trace that we hit a link.
		     */
		    if (sospTraceCount<MAX_RECORDS && curHandlePtr !=
			    (Fsio_FileIOHandle *)NIL) {
			((int *)sospTracePtr)[0] = -1;
			((int *)sospTracePtr)[1] = -2;
			((int *)sospTracePtr)[2] = -3;
			((int *)sospTracePtr)[3] = -4;
			sospTracePtr++;
			sospTraceCount++;
		    }
#endif
		    Fsutil_HandleRelease(curHandlePtr, TRUE);
		    curHandlePtr = parentHandlePtr;
		    parentHandlePtr = (Fsio_FileIOHandle *)NIL;
		    status = SUCCESS;
		} else {
		    panic( "No parent after link");
		    status = FS_INVALID_ARG;
		}
	    }
	}
    }
endScan:
    if (useFlags & FSUTIL_TRACE_FLAG) {
	FSUTIL_TRACE_NAME(FSUTIL_TRACE_LOOKUP_DONE, relativeName);
    }
    if ((status == SUCCESS) ||
	((status == FS_FILE_NOT_FOUND) && (*curCharPtr == '\0'))) {
	/*
	 * Done with the lookup.  Determine the type of the file once
	 * we have a handle for it if its type is not already set from the
	 * file descriptor. Process creates, links, and deletes.
	 */
	switch(useFlags & (FS_CREATE|FS_DELETE|FS_LINK)) {
	    case 0:
		if (status == SUCCESS) {
		    /*
		     * Normal lookup completion.
		     */
		    status = CheckPermissions(curHandlePtr, useFlags, idPtr,
						type);
		}
		break;
	    case FS_CREATE:
		fs_Stats.lookup.forCreate++;
		if (status == SUCCESS && (useFlags & FS_EXCLUSIVE)) {
		    /*
		     * FS_EXCLUSIVE and FS_CREATE means that the file
		     * cannot already exist.
		     */
		    status = FS_FILE_EXISTS;
		} else if (status == FS_FILE_NOT_FOUND) {
		    /*
		     * 'component' is the last part of a pathname for a
		     * file we need to create and that doesn't exist.
		     * Check for write permission in the parent directory,
		     * choose a fileNumber for the new file, and then
		     * create the file itself.
		     */
		    int 	newFileNumber;
		    int 	nearbyFile;

		    status = CheckPermissions(parentHandlePtr, FS_WRITE, idPtr,
						    FS_DIRECTORY);
		    if (status == SUCCESS) {
			int logOp;
			dirFileNumber = parentHandlePtr->hdr.fileID.minor;
			newFileNumber = -1;
			dirOffset = -1;
			if (type == FS_DIRECTORY) {
			    logOp = FSDM_LOG_CREATE|FSDM_LOG_IS_DIRECTORY;
			    nearbyFile = -1;
			} else {
			    logOp = FSDM_LOG_CREATE;
			    nearbyFile = dirFileNumber;
			}
			logClientData = Fsdm_DirOpStart(logOp,
					  parentHandlePtr, dirOffset,
					  component, compLen,
					  newFileNumber, type,
					  (Fsdm_FileDescriptor *) NIL);
			status = Fsdm_GetNewFileNumber(domainPtr, nearbyFile,
							     &newFileNumber);
			if (status == SUCCESS) {
			    status = CreateFile(domainPtr, parentHandlePtr,
				     component, compLen, newFileNumber, type,
				     permissions, idPtr, &curHandlePtr,
				     &dirOffset);
			    if (status != SUCCESS) {
				(void)Fsdm_FreeFileNumber(domainPtr,
						newFileNumber);
			    } 
			}
			if (status == SUCCESS) { 
			    Fsdm_DirOpEnd(logOp, 
				    parentHandlePtr, dirOffset,
				    component, compLen, newFileNumber, type,
				    curHandlePtr->descPtr, logClientData, 
				    status);
			} else {
			    Fsdm_DirOpEnd(logOp, 
				     parentHandlePtr, dirOffset,
				     component, compLen, newFileNumber, type,
				     (Fsdm_FileDescriptor *) NIL, 
				     logClientData, status);
			}
		    }
		} else {
		    /*
		     * If the file exists, it's like a normal lookup completion
		     * and we have to check permissions.
		     */
		    status = CheckPermissions(curHandlePtr, useFlags, idPtr,
						type);
		}
		if (status == SUCCESS) {
		    (void)Fsdm_FileDescStore(curHandlePtr, FALSE);
		}
		break;
	    case FS_LINK: {
		Boolean 		fileDeleted = FALSE;
		Fsio_FileIOHandle	*deletedHandlePtr;
		/*
		 * The presence of FS_LINK means that curHandlePtr references
		 * a file that is being linked to.  If the file already exists
		 * it is deleted first.  Then link is made with LinkFile.
		 */
		if (useFlags & FS_RENAME) {
		    logOp = FSDM_LOG_RENAME_LINK;
		    fs_Stats.lookup.forRename++;
		} else {
		    logOp = FSDM_LOG_LINK;
		    fs_Stats.lookup.forLink++;
		}
		dirFileNumber = parentHandlePtr->hdr.fileID.minor;
		if (status == SUCCESS) {
		    /*
		     * Linking to an existing file.
		     * This can only be in preparation for a rename.
		     */
		    if ((useFlags & FS_RENAME) &&
			(curHandlePtr->descPtr->fileType == type)) {
			/*
			 * Try the delete, this fails on non-empty directories.
			 */
			deletedHandlePtr = curHandlePtr;
			status = DeleteFileName(parentHandlePtr,
			      curHandlePtr, component, compLen, FALSE, idPtr,
			      FSDM_LOG_RENAME_DELETE);
			if (status == SUCCESS) {
			    fileDeleted = TRUE;
			}
		    } else {
			/*
			 * Not ok to link to an existing file.
			 */
			status = FS_FILE_EXISTS;
		    }
		} else if (status == FS_FILE_NOT_FOUND) {
		    /*
		     * The file does not already exist.  Check write permission
		     * in the parent before making the link.
		     */
		    status = CheckPermissions(parentHandlePtr, FS_WRITE, idPtr,
						    FS_DIRECTORY);
		}
		if (status == SUCCESS) {
		    status = LinkFile(parentHandlePtr,
				component, compLen, fileNumber, logOp,
				&curHandlePtr);
		    if (status == SUCCESS) {
			(void)Fsdm_FileDescStore(curHandlePtr, FALSE);
		    }
		}
		if (fileDeleted) {
		    CloseDeletedFile(&parentHandlePtr, &deletedHandlePtr);
		}
		break;
	    }
	    case FS_DELETE:
		fs_Stats.lookup.forDelete++;
		if (status == SUCCESS) {
		    if ((curHandlePtr->descPtr->fileType != type) &&
			(type != FS_FILE)) {
			status = (type == FS_DIRECTORY) ? FS_NOT_DIRECTORY :
							  FS_WRONG_TYPE;
		    } else {
			logOp = (useFlags&FS_RENAME) ? FSDM_LOG_RENAME_UNLINK :
						       FSDM_LOG_UNLINK;
			status = DeleteFileName(parentHandlePtr, 
				curHandlePtr, component, compLen,
				(int) (useFlags & FS_RENAME), idPtr, logOp);
			if (status == SUCCESS) {
#ifdef SOSP91
			SOSP_ADD_DELETE_TRACE(clientID, 
			    SOSP_REMEMBERED_MIG,  
			    curHandlePtr->hdr.fileID,
			    curHandlePtr->cacheInfo.attr.modifyTime,
			    curHandlePtr->cacheInfo.attr.createTime,
			    curHandlePtr->cacheInfo.attr.lastByte + 1);
#endif

			    CloseDeletedFile(&parentHandlePtr,
					&curHandlePtr);
			}
		    }
		}
		break;
	}
    }

    /*
     * Clean up state and return a fileHandle to our caller.
     */
    if (newNameBuffer != (char *)NIL) {
	free(newNameBuffer);
    }
    if (parentHandlePtr != (Fsio_FileIOHandle *)NIL) {
	Fsutil_HandleRelease(parentHandlePtr, TRUE);
    }
    if (curHandlePtr != (Fsio_FileIOHandle *)NIL) {
	if (status != SUCCESS) {
	    Fsutil_HandleRelease(curHandlePtr, TRUE);
	    curHandlePtr = (Fsio_FileIOHandle *)NIL;
	} 
    }
    if (handlePtrPtr != (Fsio_FileIOHandle **)NIL) {
	/*
	 * Return a locked handle that has had its reference count bumped.
	 */
	*handlePtrPtr = curHandlePtr;
    } else if (curHandlePtr != (Fsio_FileIOHandle *)NIL) {
	printf( "FslclLookup: caller didn't want handle\n");
	Fsutil_HandleRelease(curHandlePtr, TRUE);
    }
    if ((status != SUCCESS) ||
	(handlePtrPtr == (Fsio_FileIOHandle **)NIL)) {
	Fsdm_DomainRelease(prefixHdrPtr->fileID.major);
    }
    if (fslclComponentTrace && !fsprefix_FileNameTrace) {
	printf(" <%x>\n", status);
    }
    if (status == FS_FILE_NOT_FOUND) {
	fs_Stats.lookup.notFound++;
    }
#ifdef SOSP91
    if (curHandlePtr != (Fsio_FileIOHandle *)NIL) {
	SOSP_ADD_LOOKUP(((int *)buf), clientID,
	    (*(Fs_FileID *)(&(curHandlePtr->hdr))), status, sospTraceCount,
	    SOSP_REMEMBERED_MIG,
	    SOSP_REMEMBERED_OP|(curHandlePtr->descPtr->fileType<<8));
    } else {
	SOSP_ADD_LOOKUP(((int *)buf), clientID,
	    NullFileID, status, sospTraceCount,
	    SOSP_REMEMBERED_MIG, SOSP_REMEMBERED_OP);
    }
    SOSP_REMEMBERED_CLIENT = -1;
    SOSP_REMEMBERED_MIG = -1;
    SOSP_REMEMBERED_OP = -1;
    SOSP_IN_NAME_LOOKUP_FIELD = 0;
    Timer_GetCurrentTicks(&endTicks);
    Timer_SubtractTicks(endTicks, startTicks, &endTicks);
    /* Should really have a lock here, but I'll trust that this works. */
    Timer_AddTicks(totalNameTime, endTicks, &totalNameTime);
#endif
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FindComponent --
 *
 *	Look for a pathname component in a directory.  This returns a locked
 *	file handle that has its reference count incremented.
 *
 * Results:
 *	SUCCESS or FS_FILE_NOT_FOUND
 *
 * Side effects:
 *	Disk I/O, installing and locking the handle.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
FindComponent(parentHandlePtr, component, compLen, isDotDot, curHandlePtrPtr,
	      dirOffsetPtr)
    Fsio_FileIOHandle	*parentHandlePtr;	/* Locked handle of current 
						 * directory */
    register char 	*component;		/* Name of path component to 
						 * find */
    register int 	compLen;		/* The number of characters in 
						 * component */
    Boolean		isDotDot;		/* TRUE if component is "..".
						 * In this case the handle for
						 * the parent is returned
						 * UNLOCKED. */
    Fsio_FileIOHandle	**curHandlePtrPtr;	/* Return, locked handle */
    int			*dirOffsetPtr;		/* OUT: Offset into directory
						 * off component. */
{
    register Fslcl_DirEntry *dirEntryPtr;	/* Reference to directory entry */
    register char	*s1;		/* Pointers into components used */
    register char	*s2;		/*   for fast in-line string compare */
    register int 	blockOffset;	/* Offset within the directory */
    ReturnStatus 	status;
    Fscache_Block	*cacheBlockPtr;	/* Cache block */
    int 		dirBlockNum;	/* Block number within directory */
    int 		length;		/* Length variable for read call */
    FslclHashEntry		*entryPtr;	/* Name cache entry */
    Fs_FileID		fileID;		/* Used when fetching handles */

    /*
     * Check in system-wide name cache here before scanning
     * the directory's data blocks.
     */
    entryPtr = FSLCL_HASH_LOOK_ONLY(fslclNameTablePtr, component, parentHandlePtr);
    if (entryPtr != (FslclHashEntry *)NIL) {
	if (entryPtr->hdrPtr->fileID.type != FSIO_LCL_FILE_STREAM) {
	    panic(
		      "FindComponent: got trashy handle from cache");
	} else {
	    if (isDotDot) {
		/*
		 * Unlock this directory before grabbing the handle for "..".
		 * This prevents deadlock with another lookup that is
		 * descending from our parent ("..") into this directory.
		 */
		Fsutil_HandleUnlock(parentHandlePtr);
	    }
	    *curHandlePtrPtr = Fsutil_HandleDupType(Fsio_FileIOHandle,
						entryPtr->hdrPtr);
	    return(SUCCESS);
	}
    }

    dirBlockNum = 0;
    do {
	status = Fscache_BlockRead(&parentHandlePtr->cacheInfo, dirBlockNum,
			&cacheBlockPtr, &length, FSCACHE_DIR_BLOCK, FALSE);
	if (status != SUCCESS || length == 0) {
	    *curHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
	    return(FS_FILE_NOT_FOUND);
	}
	dirEntryPtr = (Fslcl_DirEntry *)cacheBlockPtr->blockAddr;
	blockOffset = 0;
	while (blockOffset < length) {
	    if (dirEntryPtr->recordLength <= 0) {
		printf("Corrupted directory?");
		printf(" File ID <%d, %d, %d>",
				 parentHandlePtr->hdr.fileID.serverID,
				 parentHandlePtr->hdr.fileID.major,
				 parentHandlePtr->hdr.fileID.minor);
		printf(" dirBlockNum <%d>, blockOffset <%d>",
			     dirBlockNum, blockOffset);
		printf("\n");
		Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0,
				    FSCACHE_CLEAR_READ_AHEAD);
		*curHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
		return(FS_FILE_NOT_FOUND);
	    }

	    if (dirEntryPtr->fileNumber != 0) {
		/*
		 * A valid directory record.  If component and the directory
		 * entry are the same length then compare them for a match.
		 * This String Compare is in-lined for efficiency.
		 */
		if ((dirEntryPtr->nameLength == compLen)) {
		    s1 = component;
		    s2 = dirEntryPtr->fileName;
		    do {
			if (*s1 == '\0') {
			    /*
			     * The strings are the same length so hitting
			     * the end of component indicates a match.
			     */
			    if (isDotDot) {
				/*
				 * Unlock this directory before grabbing the
				 * handle for "..".  This prevents deadlock
				 * with another lookup that is descending
				 * from our parent ("..") into this directory.
				 */
				Fsutil_HandleUnlock(parentHandlePtr);
			    }
			    *dirOffsetPtr = blockOffset + 
					dirBlockNum * FS_BLOCK_SIZE;
			    /*
			     * Inlined call to GetHandle().
			     */
			    fileID.type = FSIO_LCL_FILE_STREAM;
			    fileID.serverID = rpc_SpriteID;
			    fileID.major = parentHandlePtr->hdr.fileID.major;
			    fileID.minor = dirEntryPtr->fileNumber;
			    status = Fsio_LocalFileHandleInit(&fileID, component,
					    (Fsdm_FileDescriptor *) NIL, FALSE,
					    curHandlePtrPtr);

			    Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0,
					       FSCACHE_CLEAR_READ_AHEAD);
			    if (status == SUCCESS) {
				FSLCL_HASH_INSERT(fslclNameTablePtr, component,
					     parentHandlePtr, *curHandlePtrPtr);
			    } else {
				/*
				 * It is possible that the file doesn't 
				 * exist anymore.  This happens when
				 * the parent directory of an open
				 * directory is deleted.  The ".." entry
				 * points to a deleted file.
				 */
				if (status == FS_FILE_REMOVED) {
				    status = FS_FILE_NOT_FOUND;
				} else {

				    printf(
		"FindComponent, no handle <0x%x> for \"%s\" fileNumber %d\n",
				    status, component, dirEntryPtr->fileNumber);
				}
				if (isDotDot) {
				    /*
				     * If we have an error, relock the handle
				     * because your caller will assume that it
				     * is locked.
				     */
				    Fsutil_HandleLock(parentHandlePtr);
				}
			    }
			    goto exit;	/* to quiet lint... */
#ifndef lint
			} else if (*s1++ != *s2++) {
			    break;
#else
			} else {
			    if (*s1 != *s2) {
				break;
			    }
			    s1++;
			    s2++;
#endif
			}
		    } while (TRUE);
		}
	    }
	    blockOffset += dirEntryPtr->recordLength;
	    dirEntryPtr = (Fslcl_DirEntry *)((int)dirEntryPtr +
					 dirEntryPtr->recordLength);
	}
	dirBlockNum++;
	Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, 0);
    } while(TRUE);
exit:
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * InsertComponent --
 *	Add a name to a directory.
 *
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	Adding the name to the directory, writing the modified directory block.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
InsertComponent(curHandlePtr, component, compLen, fileNumber, dirOffsetPtr)
    Fsio_FileIOHandle *curHandlePtr;	/* Locked handle of current directory */
    char *component;			/* Name of path component to insert */
    int compLen;			/* The length of component */
    int fileNumber;			/* File Number of inserted name */
    int	*dirOffsetPtr;  /* OUT: directory offset of component's entry.*/
{
    ReturnStatus 	status;
    int			dirBlockNum;	/* Directory block index */
    int 		blockOffset;	/* Offset within a directory block. */
    Fslcl_DirEntry 		*dirEntryPtr;	/* Reference to directory entry. */
    int 		length;		/* Length variable for read call. */
    int 		recordLength;	/* Length of directory entry for 
					 * component. */
    int 		freeSpace;	/* Total amount of free bytes in a 
					 * directory block. */
    int 		extraBytes;	/* The number of free bytes attached to
				 	 * a directory entry. */
    Fscache_Block	*cacheBlockPtr;	/* Cache block. */

    length = FS_BLOCK_SIZE;
    recordLength = Fslcl_DirRecLength(compLen);
    /*
     * Loop through the directory blocks looking for space of at least
     * recordLength in which to insert the new directory record.
     */
    dirBlockNum = 0;
    do {
	/*
	 * Read in a full data block.
	 */
	status = Fscache_BlockRead(&curHandlePtr->cacheInfo, dirBlockNum,
			&cacheBlockPtr, &length, FSCACHE_DIR_BLOCK, TRUE);
	if (status != SUCCESS) {
	    printf( "InsertComponent: Read failed\n");
	    return(status);
	} else if (length == 0) {
	    /*
	     * No more space, have to grow the directory.  First we
	     * need another cache block.
	     */
	    Boolean found = FALSE;

	    Fscache_FetchBlock(&curHandlePtr->cacheInfo, dirBlockNum,
				FSCACHE_DIR_BLOCK, &cacheBlockPtr, &found);
	    if (found) {
		panic( "InsertComponent found new dir block");
	    }
	    bzero(cacheBlockPtr->blockAddr, FS_BLOCK_SIZE);
	}

	dirEntryPtr = (Fslcl_DirEntry *)cacheBlockPtr->blockAddr;
	blockOffset = 0;
	freeSpace = 0;
	while (blockOffset < length) {
	    /*
	     * Scan a data block looking for deleted entries that are
	     * large enough to use, or for entries that contain enough extra
	     * bytes for the record we have to insert.  The amount of fragmented
	     * space is accumulated in freeSpace but compaction is not 
	     * implemented.
	     */
	    if (dirEntryPtr->fileNumber != 0) {
		/*
		 * A valid directory record.
		 * Check the left-over bytes attached to this record.
		 */
		extraBytes = dirEntryPtr->recordLength -
			     Fslcl_DirRecLength(dirEntryPtr->nameLength);
		if (extraBytes >= recordLength) {
		    /*
		     * Can fit new entry in the space left over.
		     */ 
		    goto haveASlot;
		}
		/*
		 * Count bytes that occur in fragments of 4 bytes or more.
		 */
		freeSpace += extraBytes & ~(FSLCL_REC_LEN_GRAIN-1);
	    } else {
		/*
		 * A deleted name in the directory.
		 */
		if (dirEntryPtr->recordLength >= recordLength) {
		    goto haveASlot;
		}
		freeSpace += dirEntryPtr->recordLength;
	    }
	    blockOffset += dirEntryPtr->recordLength;
	    dirEntryPtr = (Fslcl_DirEntry *)((int)dirEntryPtr +
					 dirEntryPtr->recordLength);
	}
	/*
	 * Finished scanning a directory block.  Note if the accumulated
	 * free bytes in the block could be used for a directory entry.
	 * (We aren't implementing compaction for a while yet...)
	 */
	if (freeSpace >= recordLength) {
	    fsCompacts++;
	}
	if (length < FS_BLOCK_SIZE) {
	    /*
	     * Scanned up to the end of the last directory data block.  Need
	     * to append to the end of the directory.
	     */
	    bzero(cacheBlockPtr->blockAddr + length, FSLCL_DIR_BLOCK_SIZE);
	    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
	    length += FSLCL_DIR_BLOCK_SIZE;
	    break;
	}

	dirBlockNum++;
	Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
    } while(TRUE);

haveASlot:

    /*
     * At this point dirEntryPtr references the slot we have to either re-use
     * or that has enough free bytes for us to use.
     */
    if (dirEntryPtr->fileNumber != 0) {
	/*
	 * Have to take space away from the end of a valid directory entry.
	 */
	int newRecordLength;	/* New length of the existing valid entry */
	Fslcl_DirEntry *tmpDirEntryPtr;	/* Pointer to new slot */

	newRecordLength = Fslcl_DirRecLength(dirEntryPtr->nameLength);
	tmpDirEntryPtr = (Fslcl_DirEntry *)((int)dirEntryPtr + newRecordLength);
	tmpDirEntryPtr->recordLength = dirEntryPtr->recordLength -
				       newRecordLength;
	dirEntryPtr->recordLength = newRecordLength;
	dirEntryPtr = tmpDirEntryPtr;
    }
    dirEntryPtr->fileNumber = fileNumber;
    dirEntryPtr->nameLength = compLen;
    (void)strcpy(dirEntryPtr->fileName, component);

    blockOffset = (((char *)dirEntryPtr) - (char *)(cacheBlockPtr->blockAddr));
    *dirOffsetPtr = dirBlockNum * FS_BLOCK_SIZE + blockOffset;
    status = CacheDirBlockWrite(curHandlePtr,cacheBlockPtr,dirBlockNum,length);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteComponent --
 *
 *	Delete a name from a directory.  The file descriptor itself
 *	is not modified.  It is assumed that the handle for the file that
 *	is being removed is already locked.
 *
 * Results:
 *	SUCCESS or FS_FILE_NOT_FOUND
 *
 * Side effects:
 *	Sets the fileNumber for the component to Zero (0) to mark as deleted.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
DeleteComponent(parentHandlePtr, component, compLen, dirOffsetPtr)
    Fsio_FileIOHandle	*parentHandlePtr;/* Locked handle of current dir. */
    char 		*component;	/* Name to delete */
    int 		compLen;	/* Length of the name */
    int			*dirOffsetPtr;	/* OUT: Directory offset of component.*/
{
    ReturnStatus	status;
    int 		blockOffset;	/* Offset within a directory block */
    Fslcl_DirEntry 	*dirEntryPtr;	/* Reference to directory entry */
    Fslcl_DirEntry 	*lastDirEntryPtr;/* Back pointer used when merging 
					  * adjacent entries after the delete */
    int 		length;		/* Length variable for read call */
    Fscache_Block	*cacheBlockPtr;	/* Cache block. */
    int			dirBlockNum;

    dirBlockNum = 0;
    do {
	status = Fscache_BlockRead(&parentHandlePtr->cacheInfo, dirBlockNum,
			  &cacheBlockPtr, &length, FSCACHE_DIR_BLOCK, FALSE);
	if (status != SUCCESS || length == 0) {
	    return(FS_FILE_NOT_FOUND);
	}
	blockOffset = 0;
	lastDirEntryPtr = (Fslcl_DirEntry *)NIL;
	dirEntryPtr = (Fslcl_DirEntry *)cacheBlockPtr->blockAddr;
	while (blockOffset < length) {
	    if ((dirEntryPtr->fileNumber != 0) &&
		(dirEntryPtr->nameLength == compLen) &&
		(strcmp(component, dirEntryPtr->fileName) == 0)) {
		/*
		 * Delete the entry from the name cache.
		 */
		*dirOffsetPtr = blockOffset + FS_BLOCK_SIZE*dirBlockNum;
		FSLCL_HASH_DELETE(fslclNameTablePtr, component,parentHandlePtr);
		dirEntryPtr->fileNumber = 0;
		if (lastDirEntryPtr != (Fslcl_DirEntry *)NIL) {
		    /*
		     * Grow the previous record so that it now includes
		     * this one.
		     */
		    lastDirEntryPtr->recordLength += dirEntryPtr->recordLength;
		}
		/*
		 * Write out the modified directory block.
		 */
		status = CacheDirBlockWrite(parentHandlePtr, cacheBlockPtr, 
					   dirBlockNum, length);
		return(status);
	    }
	    blockOffset += dirEntryPtr->recordLength;
	    if ((blockOffset & (FSLCL_DIR_BLOCK_SIZE - 1)) == 0) {
		 lastDirEntryPtr = (Fslcl_DirEntry *) NIL;
	    } else {
		 lastDirEntryPtr = dirEntryPtr;
	    }
	    dirEntryPtr = 
		(Fslcl_DirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
	}
	dirBlockNum++;
	Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
    } while(TRUE);
    /*NOTREACHED*/
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
static ReturnStatus
ExpandLink(curHandlePtr, curCharPtr, offset, nameBuffer)
    Fsio_FileIOHandle	*curHandlePtr;	/* Handle on the link file */
    char	*curCharPtr;		/* Points to beginning of the remaining
					 * name that has to be shifted */
    int		offset;			/* Offset of curCharPtr within its 
					 * buffer */
    char 	nameBuffer[];		/* New buffer for the expanded name */
{
    ReturnStatus 	status;
    int 		linkNameLength;	/* The length of the name in the
					 * link NOT including the Null */
    register char 	*srcPtr;
    register char 	*dstPtr;

    linkNameLength = curHandlePtr->descPtr->lastByte;	/* + 1 */
    if (linkNameLength < 0) {				/* <= */
	return(FS_FILE_NOT_FOUND);
    }
    if (*curCharPtr == '\0') {
	/*
	 * There is no pathname, just make sure the new name is Null terminated
	 */
	nameBuffer[linkNameLength] = '\0';		/* still ok */
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
     * that was just shifted over.
     */
    status = Fscache_Read(&curHandlePtr->cacheInfo, 0, nameBuffer, 0,
		    &linkNameLength, (Sync_RemoteWaiter *)NIL);
    if ((status == SUCCESS) || (linkNameLength > 0)) { 
	(void)Fsdm_UpdateDescAttr(curHandlePtr, &curHandlePtr->cacheInfo.attr, 
			FSDM_FD_ACCESSTIME_DIRTY);
    }

    /*
     * FIX HERE to handle old sprite links that include a null.
     */
#ifdef notdef
    if (nameBuffer[linkNameLength-1] == '\0') {
	/*
	 * Old Sprite link with trailing NULL.  Shift remaining name over
	 * to the left one place.
	 */
	dstPtr = &nameBuffer[linkNameLength-1];
	srcPtr = dstPtr + 1;
	while (*srcPtr != '\0') {
	    *dstPtr++ = *srcPtr++;
	}
	*dstPtr = '\0'
    }
#endif /* notdef */
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * GetHandle --
 *
 *	Given a file number and the handle on the parent directotry,
 *	this routine returns a locked handle for the file.  This is a
 *	small layer on top of Fsio_LocalFileHandleInit that is oriented
 *	towards the needs of the lookup routines.
 *
 * Results:
 *	A return code.  If SUCCESS the returned handle is locked.
 *
 * Side effects:
 *	Calls Fsio_LocalFileHandleInit to set up the handle.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
GetHandle(fileNumber, newDescPtr, curHandlePtr, name, newHandlePtrPtr)
    int		fileNumber;		/* Number of file to get handle for */
    Fsdm_FileDescriptor *newDescPtr;
    Fsio_FileIOHandle	*curHandlePtr;	/* Handle on file in the same domain */
    char	*name;			/* File name for error msgs */
    Fsio_FileIOHandle	**newHandlePtrPtr;/* Return, ref. to installed handle */
{
    register ReturnStatus status;
    Fs_FileID fileID;

    fileID.type = FSIO_LCL_FILE_STREAM;
    fileID.serverID = rpc_SpriteID;
    fileID.major = curHandlePtr->hdr.fileID.major;
    fileID.minor = fileNumber;
    status = Fsio_LocalFileHandleInit(&fileID, name, newDescPtr, FALSE,
		newHandlePtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * CreateFile --
 *      Create a file by adding it do a directory and and initializing its
 *      file descriptor.  The caller has already allocated the fileNumber
 *      for the file.  The file descriptor is written to disk before the
 *	name is inserted into the directory.
 *
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	Calls InsertComponent, Fsdm_FileDescInit.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
CreateFile(domainPtr, parentHandlePtr, component, compLen, fileNumber, type,
	   permissions, idPtr, curHandlePtrPtr, dirOffsetPtr)
    Fsdm_Domain	*domainPtr;		/* Domain of the file */
    Fsio_FileIOHandle	*parentHandlePtr;/* Handle of directory in which to add 
					 * file. */
    char	*component;		/* Name of the file */
    int		compLen;		/* The length of component */
    int		fileNumber;		/* Domain relative file number */
    int		type;			/* Type of the file */
    int		permissions;		/* Permission bits on the file */
    Fs_UserIDs	*idPtr;			/* User ID of calling process */
    Fsio_FileIOHandle	**curHandlePtrPtr;/* Return, handle for the new file */
    int		*dirOffsetPtr;  /* OUT: directory offset of component's entry.*/
{
    ReturnStatus	status;
    Fsdm_FileDescriptor	*parentDescPtr;	/* Descriptor for the parent */
    Fsdm_FileDescriptor	*newDescPtr;	/* Descriptor for the new file */

    /*
     * Set up the file descriptor using the group ID from the parent directory.
     */
    parentDescPtr = parentHandlePtr->descPtr;
    newDescPtr = (Fsdm_FileDescriptor *)malloc(sizeof(Fsdm_FileDescriptor));
    status = Fsdm_FileDescInit(domainPtr, fileNumber, type, permissions,
			    idPtr->user, parentDescPtr->gid, newDescPtr);
    if (status == SUCCESS) {
	if (type == FS_DIRECTORY) {
	    /*
	     * Both the parent and the directory itself reference it.
	     */
	    newDescPtr->numLinks = 2;
	    newDescPtr->flags |= FSDM_FD_LINKS_DIRTY;
	}
	/*
	 * GetHandle does extra work because we already have the
	 * file descriptor all set up...
	 */
	status = GetHandle(fileNumber, newDescPtr, parentHandlePtr, component,
			    curHandlePtrPtr);
	if (status != SUCCESS) {
	    /*
	     * GetHandle shouldn't fail because we just wrote out
	     * the file descriptor.
	     */
	    panic( "CreateFile: GetHandle failed\n");
	} else {
	    if (type == FS_DIRECTORY) {
		status = WriteNewDirectory(*curHandlePtrPtr,
					    parentHandlePtr);
	    }
	    if (status == SUCCESS) {
		/*
		 * Commit by adding the name to the directory.  If we crash
		 * after this the file will show up and have a good
		 * descriptor for itself on disk.
		 */
		status = InsertComponent(parentHandlePtr, component,
					  compLen, fileNumber, dirOffsetPtr);
		if (type == FS_DIRECTORY) {
		    if (status == SUCCESS) {
			/*
			 * Update the parent directory to reflect
			 * the addition of a new sub-directory.
			 */
			parentDescPtr->numLinks++;
			parentDescPtr->descModifyTime = Fsutil_TimeInSeconds();
			parentDescPtr->flags |= FSDM_FD_LINKS_DIRTY;
		       (void)Fsdm_FileDescStore(parentHandlePtr, FALSE);
		    }
		}
	    }
	    if (status != SUCCESS) {
		/*
		 * Couldn't add to the directory, no disk space perhaps.
		 * Unwind by marking the file descriptor as free and
		 * releasing the handle we've created.
		 */
		printf("CreateFile: aborting create of %d (%s) in %d\n",
			fileNumber, component, 
			parentHandlePtr->hdr.fileID.minor);
		newDescPtr->flags = FSDM_FD_FREE;
		Fsutil_HandleRelease(*curHandlePtrPtr, TRUE);
		*curHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
	    }
	}
    }
    if (status != SUCCESS)  { 
	free((Address) newDescPtr);
    } else {
       (void)Fsdm_FileDescStore(parentHandlePtr, FALSE);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * WriteNewDirectory --
 *      Write a new sub-directory.  Set the file numbers on the canned image
 *      of a new directory and write the block to the directory.
 *
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	Disk I/O to write the directory data block.
 *
 *----------------------------------------------------------------------
 */
static	ReturnStatus
WriteNewDirectory(curHandlePtr, parentHandlePtr)
    Fsio_FileIOHandle *curHandlePtr;		/* Handle of file to delete */
    Fsio_FileIOHandle *parentHandlePtr;	/* Handle of directory in which
						 * to delete file*/
{
    ReturnStatus	status;
    int			offset;
    int			length;
    register Fslcl_DirEntry *dirEntryPtr;
    char		*dirBlock;

    /*
     * The malloc and Byte_Copy could be avoided by puting this routine
     * into its own monitor so that it could write directly onto fslclEmptyDirBlock
     * fslclEmptyDirBlock is already set up with ".", "..", and the correct
     * nameLengths and recordLengths.
     */
    dirBlock = (char *)malloc(FSLCL_DIR_BLOCK_SIZE);
    bcopy((Address)fslclEmptyDirBlock, (Address)dirBlock, FSLCL_DIR_BLOCK_SIZE);
    dirEntryPtr = (Fslcl_DirEntry *)dirBlock;
    dirEntryPtr->fileNumber = curHandlePtr->hdr.fileID.minor;
    dirEntryPtr = (Fslcl_DirEntry *)((int)dirEntryPtr +
				 dirEntryPtr->recordLength);
    dirEntryPtr->fileNumber = parentHandlePtr->hdr.fileID.minor;
    offset = 0;
    length = FSLCL_DIR_BLOCK_SIZE;
    status = Fscache_Write(&curHandlePtr->cacheInfo, 0, (Address)dirBlock,
		offset, &length, (Sync_RemoteWaiter *)NIL);
    if (status == SUCCESS) {
	(void)Fsdm_UpdateDescAttr(curHandlePtr, &curHandlePtr->cacheInfo.attr, 
			FSDM_FD_MODTIME_DIRTY);
    }
    free(dirBlock);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * LinkFile --
 *	Create an entry in a directory that references an existing file.
 *	This understands that links to directories are made as part of
 *	a rename and checks that no circularities in the directory structure
 *	result from moving a directory.  Other than that it is left to
 *	the caller to check permissions.
 *
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	Calls InsertComponent, bumps the link count on the existing file.
 *	Calls MoveDirectory in the case of directories.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
LinkFile(parentHandlePtr, component, compLen, fileNumber, logOp,curHandlePtrPtr)
    Fsio_FileIOHandle	*parentHandlePtr;/* Handle of directory in which to add 
					 * file. */
    char	*component;		/* Name of the file */
    int		compLen;		/* The length of component */
    int		fileNumber;		/* Domain relative file number */
    int		logOp;
    Fsio_FileIOHandle	**curHandlePtrPtr;/* Return, handle for the new file */
{
    ReturnStatus	status;
    Fsdm_FileDescriptor	*linkDescPtr;	/* Descriptor for the existing file */
    Time 		modTime;	/* Descriptors are modified */
    ClientData		logClientData;
    int			dirOffset;

    if (fileNumber == parentHandlePtr->hdr.fileID.minor) {
	/*
	 * Trying to move a directory into itself, ie. % mv subdir subdir
	 */
	return(GEN_INVALID_ARG);
    }
    status = GetHandle(fileNumber, (Fsdm_FileDescriptor *) NIL,
			parentHandlePtr, component, curHandlePtrPtr);
    if (status != SUCCESS) {
	printf( "LinkFile: can't get existing file handle\n");
	return(FAILURE);
    } else if ((*curHandlePtrPtr)->descPtr->fileType == FS_DIRECTORY) {
	logOp |= FSDM_LOG_IS_DIRECTORY;
	status = OkToMoveDirectory(parentHandlePtr, *curHandlePtrPtr);
    }
    if (status == SUCCESS) {
	linkDescPtr = (*curHandlePtrPtr)->descPtr;
	logClientData = Fsdm_DirOpStart(logOp, parentHandlePtr, -1,
				component, compLen, fileNumber, 
					linkDescPtr->fileType,
					linkDescPtr);
	linkDescPtr->numLinks++;
	linkDescPtr->descModifyTime = Fsutil_TimeInSeconds();
	linkDescPtr->flags |= FSDM_FD_LINKS_DIRTY;
	status = Fsdm_FileDescStore(*curHandlePtrPtr, FALSE);
	if (status == SUCCESS) {
	    /*
	     * Commit by adding the name to the directory.
	     */
	    status = InsertComponent(parentHandlePtr, component, compLen,
						fileNumber, &dirOffset);
	    if ((*curHandlePtrPtr)->descPtr->fileType == FS_DIRECTORY &&
		status == SUCCESS) {
		/*
		 * This link is part of a rename (links to directories are
		 * only allowed at this time), and the ".." entry in the
		 * directory may have to be fixed.
		 */
		modTime.seconds = Fsutil_TimeInSeconds();
		status = MoveDirectory(&modTime, parentHandlePtr,
						    *curHandlePtrPtr);
		if (status != SUCCESS) {
		    (void)DeleteComponent(parentHandlePtr, component,
						    compLen, &dirOffset);
		}
	    }
	    if (status != SUCCESS) {
		/*
		 * Couldn't add to the directory because of I/O probably.
		 * Unwind by reducing the link count.  This gets the cache
		 * consistent with the cached directory image anyway.
		 */
		linkDescPtr->numLinks--;
		linkDescPtr->flags |= FSDM_FD_LINKS_DIRTY;
		(void)Fsdm_FileDescStore(*curHandlePtrPtr, FALSE);
		Fsutil_HandleRelease(*curHandlePtrPtr, TRUE);
		*curHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
	    } else {
		(void)Fsdm_FileDescStore(parentHandlePtr, FALSE);
	    }
	} else {
	    linkDescPtr->numLinks--;
	    linkDescPtr->flags &= ~FSDM_FD_LINKS_DIRTY;
	}
	Fsdm_DirOpEnd(logOp, parentHandlePtr, dirOffset, 
		  component, compLen, fileNumber, linkDescPtr->fileType,
		  linkDescPtr, logClientData, status);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * OkToMoveDirectory --
 *
 *	Check that it is ok to move a directory.  Dis-connected trees and
 *	loops in directory structure are prevented here.
 *
 * Results:
 *	A return code.
 *
 * Side effects:
 *	File handles are momentarily locked on the route from the new
 *	position in the hierarchy up to the root to check against illegal moves.
 *	This action might cause deadlock with another process desending along
 *	same route, descenders hold a lock on the parent while they try for
 *	a lock on the next sub-directory.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
OkToMoveDirectory(newParentHandlePtr, curHandlePtr)
    Fsio_FileIOHandle *newParentHandlePtr;	/* New parent directory for
						 * curHandlePtr */
    Fsio_FileIOHandle *curHandlePtr;		/* Directory being moved */
{
    ReturnStatus	status;
    int 		oldParentFileNumber;	/* File number of original 
						 * parent directory. */
    int			newParentNumber;	/* File number of new 
						 * parent directory. */

    status = GetParentNumber(curHandlePtr, &oldParentFileNumber);
    if (status != SUCCESS) {
	return(status);
    }
    newParentNumber = newParentHandlePtr->hdr.fileID.minor;
    if (oldParentFileNumber == newParentNumber) {
	/*
	 * ".." entry is ok because the rename is within the same directory.
	 */
	status = SUCCESS;
    } else {
	/*
	 * Have to trace up from the new parent to make sure we don't find
	 * the current file.  If we let that happen then you create
	 * dis-connected loops in the directory structure.
	 */
	Fsio_FileIOHandle *parentHandlePtr;
	int parentNumber;

	for (parentNumber = newParentNumber; status == SUCCESS; ) {
	    if (parentNumber == curHandlePtr->hdr.fileID.minor) {
		status = FS_INVALID_ARG;
	    } else if (parentNumber == FSDM_ROOT_FILE_NUMBER ||
		       parentNumber == oldParentFileNumber) {
		break;
	    } else {
		/*
		 * Advance parentNumber upwards towards the root.  The
		 * handles are locked because of the I/O required to get
		 * the parent file number.  We have to be careful when
		 * locking the parents because the first parent
		 * (newParentHandlePtr) is already locked by us.
		 *
		 * It still seems like there is a possibility of deadlock if
		 * someone is desending into newParentHandlePtr and has a lock
		 * on the directory above that.  FIX ME
		 */
		if (parentNumber != newParentNumber) {
		    status = GetHandle(parentNumber,
		       (Fsdm_FileDescriptor *) NIL,   curHandlePtr, (char *)NIL,
					&parentHandlePtr);
		} else {
		    parentHandlePtr = newParentHandlePtr;
		}
		if (status == SUCCESS) {
		    status = GetParentNumber(parentHandlePtr, &parentNumber);
		}
		if (parentHandlePtr != newParentHandlePtr) {
		    (void)Fsutil_HandleRelease(parentHandlePtr, TRUE);
		}
	    }
	}
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * MoveDirectory --
 *
 *	Complete the moving of a directory by updating its reference to
 *	its parent and incrementing the link count on its new parent.
 *
 * Results:
 *	A return code.
 *
 * Side effects:
 *	Sets the directory's parent entry to reference the parent passed in.
 *	Increments the link count on the new parent.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
MoveDirectory( modTimePtr, newParentHandlePtr, curHandlePtr)
    Time	*modTimePtr;			/* Modify time for parent */
    Fsio_FileIOHandle	*newParentHandlePtr;	/* New parent directory for 
						 * curHandlePtr */
    Fsio_FileIOHandle	*curHandlePtr;		/* Directory being moved */
{
    ReturnStatus	status;
    int			oldParentFileNumber;	/* File number of original 
						 * parent directory */
    int			newParentNumber;	/* File number of new 
						 * parent directory */

    status = GetParentNumber(curHandlePtr, &oldParentFileNumber);
    if (status != SUCCESS) {
	printf(
		  "MoveDirectory: Can't get parent's file number\n");
    } else {
	newParentNumber = newParentHandlePtr->hdr.fileID.minor;
	if (oldParentFileNumber != newParentNumber) {
	    /*
	     * Patch the directory entry for ".."
	     */
	    FSLCL_HASH_DELETE(fslclNameTablePtr, "..", curHandlePtr);
	    status = SetParentNumber(curHandlePtr, newParentNumber);
	}
	if (status == SUCCESS) {
	    /*
	     * Increment the link count on the new parent.  The old parent's
	     * link count gets decremented when the orignial instance of the
	     * directory is removed.  This is done even if the move is within
	     * the same directory because deleting the original name later will
	     * reduce the link count.
	     */
	    register Fsdm_FileDescriptor *parentDescPtr;

	    parentDescPtr = newParentHandlePtr->descPtr;
	    parentDescPtr->numLinks++;
	    parentDescPtr->descModifyTime = modTimePtr->seconds;
	    parentDescPtr->flags |= FSDM_FD_LINKS_DIRTY;
	    (void)Fsdm_FileDescStore(newParentHandlePtr, FALSE);
	}
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * GetParentNumber --
 *	Read a directory to determine the file number of the parent.
 *
 * Results:
 *	SUCCESS or an error code, sets *parentNumberPtr to be the file number
 *	from the ".." entry in the directory.
 *
 * Side effects:
 *	None, except for the I/O to read the directory block.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GetParentNumber(curHandlePtr, parentNumberPtr)
    Fsio_FileIOHandle	*curHandlePtr;	/* Handle for current directory */
    int		*parentNumberPtr;	/* Result, the file number of the parent
					 * of curHandlePtr */
{
    ReturnStatus 	status;
    int 		length;
    register Fslcl_DirEntry *dirEntryPtr;
    Fscache_Block	*cacheBlockPtr;

    status = Fscache_BlockRead(&curHandlePtr->cacheInfo, 0, &cacheBlockPtr,
				&length, FSCACHE_DIR_BLOCK, FALSE);
    if (status != SUCCESS) {
	return(status);
    } else if (length == 0) {
	return(FAILURE);
    }

    dirEntryPtr = (Fslcl_DirEntry *)cacheBlockPtr->blockAddr;
    if (dirEntryPtr->nameLength != 1 ||
	dirEntryPtr->fileName[0] != '.' ||
	dirEntryPtr->fileName[1] != '\0') {
	printf(
		  "GetParentNumber: \".\", corrupted directory\n");
	status = FAILURE;
    } else {
	dirEntryPtr = 
		(Fslcl_DirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
	if (dirEntryPtr->nameLength != 2 ||
	    dirEntryPtr->fileName[0] != '.' ||
	    dirEntryPtr->fileName[1] != '.' ||
	    dirEntryPtr->fileName[2] != '\0') {
	    printf(
		      "GetParentNumber: \"..\", corrupted directory\n");
	    status = FAILURE;
	} else {
	    *parentNumberPtr = dirEntryPtr->fileNumber;
	}
    }
    Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * SetParentNumber --
 *	Patch the file number for the ".." entry of a directory.
 *
 * Results:
 *	SUCCESS or an error code from the I/O.
 *
 * Side effects:
 *	Changes the parent file number of the ".." entry.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
SetParentNumber(curHandlePtr, newParentNumber)
    Fsio_FileIOHandle	*curHandlePtr;	/* Handle for current directory */
    int 		newParentNumber;/* The new file number of the parent
					 * of curHandlePtr */
{
    ReturnStatus	status;
    int 		length;
    register Fslcl_DirEntry *dirEntryPtr;
    Fscache_Block	*cacheBlockPtr;

    status = Fscache_BlockRead(&curHandlePtr->cacheInfo, 0, &cacheBlockPtr,
				&length, FSCACHE_DIR_BLOCK, FALSE);
    if (status != SUCCESS) {
	return(status);
    } else if (length == 0) {
	return(FAILURE);
    }
    dirEntryPtr = (Fslcl_DirEntry *)cacheBlockPtr->blockAddr;
    if (dirEntryPtr->nameLength != 1 ||
	dirEntryPtr->fileName[0] != '.' ||
	dirEntryPtr->fileName[1] != '\0') {
	printf(
		  "SetParentNumber: \".\", corrupted directory\n");
	Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
	return(FAILURE);
    }
    dirEntryPtr = (Fslcl_DirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
    if (dirEntryPtr->nameLength != 2 ||
	dirEntryPtr->fileName[0] != '.' ||
	dirEntryPtr->fileName[1] != '.' ||
	dirEntryPtr->fileName[2] != '\0') {
	printf("SetParentNumber: \"..\", corrupted directory\n");
	Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
	return(FAILURE);
    }
    dirEntryPtr->fileNumber = newParentNumber;
    status = CacheDirBlockWrite(curHandlePtr, cacheBlockPtr, 0, length);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteFileName --
 *
 *      Delete a file by clearing its fileNumber in the directory and
 *	reducing its link count.  
 *
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	Log entry may be written.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
DeleteFileName(parentHandlePtr, curHandlePtr, component,
	     compLen, forRename, idPtr, logOp)
    Fsio_FileIOHandle *parentHandlePtr;	/* Handle of directory in
						 * which to delete file*/
    Fsio_FileIOHandle *curHandlePtr;	/* Handle of file to delete */
    char *component;				/* Name of the file to delte */
    int compLen;				/* The length of component */
    int forRename;		/* if FS_RENAME, then the file being delted
				 * is being renamed.  This allows non-empty
				 * directories to be deleted */
    Fs_UserIDs *idPtr;		/* User and group IDs */
    int		logOp;		/* Directory log operation.; */
{
    ReturnStatus status;
    Fsdm_FileDescriptor *parentDescPtr;	/* Descriptor for parent */
    Fsdm_FileDescriptor *curDescPtr;	/* Descriptor for the file to delete */
    int type;				/* Type of the file */
    int	fileNumber;			/* Number of file being deleted. */
    ClientData	logClientData;		/* ClientData returned from 
					 * DirOpStart. */
    int dirOffset;

    type = curHandlePtr->descPtr->fileType;
    if (parentHandlePtr == (Fsio_FileIOHandle *)NIL) {
	/*
	 * There is no handle on the parent because we have just
	 * gone up via "..".  You can't delete the parent.
	 */
	status = FS_NO_ACCESS;
    } else if ((compLen == 1) && (component[0] == '.')) {
	/*
	 * Disallow removing dot.
	 */
	status = FS_NO_ACCESS;
    } else if (type == FS_DIRECTORY && (!forRename) &&
		!DirectoryEmpty(curHandlePtr)) {
	status = FS_DIR_NOT_EMPTY;
    } else {
	/*
	 * One needs write permission in the parent to do the delete.
	 */
	status = CheckPermissions(parentHandlePtr, FS_WRITE, idPtr,
					FS_DIRECTORY);
    }
    if (status != SUCCESS) {
	return(status);
    }
    curDescPtr = curHandlePtr->descPtr;
    parentDescPtr = parentHandlePtr->descPtr;
    fileNumber = curHandlePtr->hdr.fileID.minor;
    dirOffset = -1;

    if (type == FS_DIRECTORY) {
	logOp |= FSDM_LOG_IS_DIRECTORY;
    }
    logClientData = Fsdm_DirOpStart(logOp, parentHandlePtr, dirOffset, 
		    component, compLen, fileNumber, type,
		    curHandlePtr->descPtr);
    /*
     * Remove the name from the directory first.
     */
    status = DeleteComponent(parentHandlePtr, component, compLen, &dirOffset);
    if (status == SUCCESS) {
	curDescPtr->numLinks--;
	curDescPtr->flags |= FSDM_FD_LINKS_DIRTY;
	if (type == FS_DIRECTORY) {
	    /*
	     * The directory might have been known in the hash table as
	     * someone's "..". Besure that this entry is gone.
	     */
	    FSLCL_HASH_DELETE(fslclNameTablePtr, "..", curHandlePtr);
	    if (!forRename) {
		/*
		 * Directories have an extra link because they reference 
		 * themselves.
		 */
		curDescPtr->numLinks--;
		if (curDescPtr->numLinks > 0) {
		    printf("DeleteFileName: extra links on directory\n");
		}
	    }
	}
	curDescPtr->descModifyTime = Fsutil_TimeInSeconds();
	status = Fsdm_FileDescStore(curHandlePtr, FALSE);
	if (status != SUCCESS) {
	    printf("DeleteFileName: (1) Couldn't store descriptor\n");
#ifdef notdef
	    return(status);
#endif
	}
	if (type == FS_DIRECTORY) {
	    /*
	     * A directory's link count reflects the number of subdirectories
	     * it has (they each have a ".." that references it.)  Here
	     * it is decremented because the subdirectory is going away.
	     */
	    parentDescPtr->numLinks--;
	    parentDescPtr->descModifyTime = Fsutil_TimeInSeconds();
	    parentDescPtr->flags |= FSDM_FD_LINKS_DIRTY;
	}
	status = Fsdm_FileDescStore(parentHandlePtr, FALSE);
	if (status != SUCCESS) {
	    printf("DeleteFileName: (2) Couldn't store descriptor\n");
#ifdef notdef
	    return(status);
#endif
	}
	if ((curDescPtr->numLinks <= 0) && (curHandlePtr->use.ref != 0)) {
	    logOp |= FSDM_LOG_STILL_OPEN;
	}
    } 
    Fsdm_DirOpEnd(logOp, parentHandlePtr, dirOffset,
		        component, compLen, fileNumber, type,
			curDescPtr, logClientData, status);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * CloseDeletedFile --
 *
 *	This routine is called to close a deleted file.
 *	If there are no links left then the
 *	file's handle is marked as deleted.  Finally, if this routine
 *	has the last reference on the handle then the file's data blocks
 *	are truncated away and the file descriptor is marked as free.
 *
 *	This is a separate routine from DeleteFileName because the
 *	file cannot be closed unless the parent handle is unlocked
 *	(to prevent deadlock while doing the consistency callback),
 *	and the code to do a rename deletes the existing file, then
 *	does a link.  This would break if the parent handle was
 *	released during the delete.
 *
 * Results:
 *	SUCCESS or error code.
 *
 * Side effects:
 *	The parent and/or current handles may be released and set to NIL.
 *
 *----------------------------------------------------------------------
 */

static void
CloseDeletedFile(parentHandlePtrPtr, curHandlePtrPtr)
    Fsio_FileIOHandle **parentHandlePtrPtr;	/* Handle of parent of
						 * deleted file. */
    Fsio_FileIOHandle **curHandlePtrPtr;	/* Handle of deleted file. */

{
    register Fsio_FileIOHandle *curHandlePtr;	/* Local copy */

    curHandlePtr = *curHandlePtrPtr;
    if (curHandlePtr->descPtr->numLinks <= 0) {
	/*
	 * At this point curHandlePtr is potentially the last reference
	 * to the file.  If there are no users then do the delete, otherwise
	 * mark the handle as deleted and Fsio_FileClose will take care of 
	 * it.
	 */
	curHandlePtr->flags |= FSIO_FILE_NAME_DELETED;
	if (curHandlePtr->use.ref == 0) {
	    /*
	     * Handle the deletion and clean up the handle.
	     * We set the the clientID to us and specify client
	     * call-backs so that any other clients will be notified.
	     */
	    Fsutil_HandleRelease(*parentHandlePtrPtr, TRUE);
	    *parentHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
	    (void)Fsio_FileCloseInt(curHandlePtr, 0, 0, 0, rpc_SpriteID, 
		    TRUE);
	    *curHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
	} else {
	    Fsutil_HandleRelease(curHandlePtr, TRUE);
	    *curHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
	}
    } else {
	Fsutil_HandleRelease(curHandlePtr, TRUE);
	*curHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fslcl_DeleteFileDesc --
 *	Delete a file from disk given its file handle.  It is assumed that the
 *	file's name has already been deleted from the directory structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Delete the data blocks and mark file descriptor as free.  The
 *	handle remains locked during this call.  The DESCRIPTOR referenced
 *	by the handle is FREED because the file handle is going to
 *	be removed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fslcl_DeleteFileDesc(handlePtr)
    Fsio_FileIOHandle *handlePtr;
{
    ReturnStatus status;
    Fsdm_Domain *domainPtr;

    if (handlePtr->descPtr->fileType == FS_DIRECTORY) {
	/*
	 * Remove .. from the name cache so we don't end up with
	 * a bad cache entry later when this directory is re-created.
	 */
	FSLCL_HASH_DELETE(fslclNameTablePtr, "..", handlePtr);
    }

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    /*
     * The ordering of the deletion is as follows:
     * 1. Mark the descriptor on disk as free so if we crash the
     *		disk scavenger will free the blocks for us.
     * 2. Truncate the blocks out of the cache and from the descriptor.
     * 3. Mark the file descriptor as available in the bitmask.
     */
    FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_DELETE, ((Fs_HandleHeader *)handlePtr));
    handlePtr->descPtr->flags = FSDM_FD_FREE | FSDM_FD_DIRTY;
    status = Fsdm_FileDescStore(handlePtr,TRUE);
    if (status != SUCCESS) {
	printf("Fslcl_DeleteFileDesc: Can't mark descriptor as free\n");
    } else {
	status = Fsio_FileTrunc(handlePtr, 0, FSCACHE_TRUNC_DELETE);
	if (status != SUCCESS) {
	    printf("Fslcl_DeleteFileDesc: Can't truncate file <%d,%d> \"%s\"\n",
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		    Fsutil_HandleName(handlePtr));
	} else {
	    (void)Fsdm_FreeFileNumber(domainPtr, handlePtr->hdr.fileID.minor);
	}
	free((Address)handlePtr->descPtr);
	handlePtr->descPtr = (Fsdm_FileDescriptor *)NIL;
    }
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DirectoryEmpty --
 *	Scan a directory and determine if there are any files left
 *	in it other than "." and "..".
 *
 * Results:
 *	TRUE if the directory is empty, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
DirectoryEmpty(handlePtr)
    Fsio_FileIOHandle *handlePtr;	/* Handle of directory to check */
{
    ReturnStatus	status;
    int 		blockOffset;	/* Offset within a directory block */
    Fslcl_DirEntry 		*dirEntryPtr;	/* Reference to directory entry */
    int 		length;		/* Length for read call */
    int			dirBlockNum;
    Fscache_Block	*cacheBlockPtr;

    dirBlockNum = 0;
    do {
	status = Fscache_BlockRead(&handlePtr->cacheInfo, dirBlockNum,
		      &cacheBlockPtr, &length, FSCACHE_DIR_BLOCK, FALSE);
	if (status != SUCCESS || length == 0) {
	    /*
	     * Have run out of the directory and not found anything.
	     */
	    return(TRUE);
	}
	blockOffset = 0;
	dirEntryPtr = (Fslcl_DirEntry *)cacheBlockPtr->blockAddr;
	while (blockOffset < length) {
	    if (dirEntryPtr->fileNumber != 0) {
		/*
		 * A valid directory record.
		 */
		if ((strcmp(".", dirEntryPtr->fileName) == 0) ||
		    (strcmp("..", dirEntryPtr->fileName) == 0)) {
		    /*
		     * "." and ".." are ok
		     */
		} else {
		    Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, 
					FSCACHE_CLEAR_READ_AHEAD);
		    return(FALSE);
		}
	    }
	    blockOffset += dirEntryPtr->recordLength;
	    dirEntryPtr = 
		(Fslcl_DirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
	}
	dirBlockNum++;
	Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
    } while(TRUE);
    /*NOTREACHED*/
}

/*
 *----------------------------------------------------------------------
 *
 * CheckPermissions --
 *
 *	Check permissions on a file during lookup.  This just looks
 *	at the uid, groupIDs, and the permission bits on the file.
 *	
 *	Some semantic checking is done:
 *		type indicates what kind of file to accept.
 *		Execution of files with no execute bit is prevented for uid = 0
 *		Execution of directories is prevented.
 *	Some semantic checking is not done:
 *		Doesn't check against writing to directories.  This is
 *		done later in the FileSrvOpen routine.
 *
 * Results:
 *	FS_NO_ACCESS if the useFlags include a permission that does
 *	not fit with the uid/groupIDs of the file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
CheckPermissions(handlePtr, useFlags, idPtr, type)
    Fsio_FileIOHandle		*handlePtr;
    register int		useFlags;
    register Fs_UserIDs		*idPtr;
    int 			type;
{
    register Fsdm_FileDescriptor	*descPtr;
    register int		*groupPtr;
    register unsigned int 	permBits;
    register int 		index;
    register int		uid = idPtr->user;
    ReturnStatus 		status;

    if (handlePtr->hdr.fileID.type != FSIO_LCL_FILE_STREAM) {
	panic( "CheckPermissions on non-local file\n");
	return(FAILURE);
    }
    descPtr = handlePtr->descPtr;
    /*
     * Make sure the file type matches.  FS_FILE means any type, otherwise
     * it should match exactly.
     */
    if (type != FS_FILE && type != descPtr->fileType) {
	/*
	 * Patch around the fact that FS_REMOTE_LINK and FS_SYMBOLIC_LINK
	 * get or'ed together, but they are not-proper bit fields.
	 * (They equal 3 and 2, respectively.)
	 * Hence we allow a regular symbolic link to satisfy a
	 * request for a remote link.
	 */
	if ((type == (FS_REMOTE_LINK|FS_SYMBOLIC_LINK)) &&
	    ((descPtr->fileType == FS_SYMBOLIC_LINK) ||
	     (descPtr->fileType == FS_REMOTE_LINK))) {
/*	    printf( "Allowing a symlink for a remote link\n");*/
	} else {
	    switch(type) {
		case FS_DIRECTORY:
		    return(FS_NOT_DIRECTORY);
		default:
		    return(FS_WRONG_TYPE);
	    }
	}
    }
    /*
     * Dis-allow execution of directories...
     */
    if ((type == FS_FILE) && (useFlags & FS_EXECUTE) &&
	(descPtr->fileType != FS_FILE)) {
	return(FS_WRONG_TYPE);
    }
    /*
     * Reset SETUID/SETGID bit when writing a file.
     */
    if ((useFlags & FS_WRITE) && (descPtr->fileType == FS_FILE) &&
	(descPtr->permissions & (FS_SET_UID|FS_SET_GID))) {
	descPtr->permissions &= ~(FS_SET_UID|FS_SET_GID);
	descPtr->flags |= FSDM_FD_PERMISSIONS_DIRTY;
    }
    /*
     * Check for ownership permission.  This probably redundant with
     * respect to the checking done by FslclSetAttr.
     */
#ifdef notdef
    if (useFlags & FS_OWNERSHIP) {
	if ((uid != descPtr->uid) && (uid != 0)) {
	    return(FS_NOT_OWNER);
	}
    }
#endif notdef
    /*
     * Check read/write/exec permissions against one of the owner bits,
     * the group bits, or the world bits.  'permBits' is set to
     * be the corresponding bits from the file descriptor and then
     * shifted over so the comparisions are against the WORLD bits.
     */
    if (uid == 0) {
	/*
	 * For normal files, only check for execute permission.  This
	 * prevents root from being able to execute ordinary files by
	 * accident.  However, root has complete access to directories.
	 */
	if (descPtr->fileType == FS_DIRECTORY) {
	    return(SUCCESS);
	}
	useFlags &= FS_EXECUTE;
    }
    if (uid == descPtr->uid) {
	permBits = (descPtr->permissions >> 6) & 07;
    } else {
	for (index = idPtr->numGroupIDs, groupPtr = idPtr->group;
	     index > 0;
	     index--, groupPtr++) {
	    if (*groupPtr == descPtr->gid) {
		permBits = (descPtr->permissions >> 3) & 07;
		goto havePermBits;
	    }
	}
	permBits = descPtr->permissions & 07;
    }
havePermBits:
    if (((useFlags & FS_READ) && ((permBits & FS_WORLD_READ) == 0)) ||
	((useFlags & FS_WRITE) && ((permBits & FS_WORLD_WRITE) == 0)) ||
	((useFlags & FS_EXECUTE) && ((permBits & FS_WORLD_EXEC) == 0))) {
	/*
	 * The file's permission don't include what is needed.
	 */
	status = FS_NO_ACCESS;
    } else {
	status = SUCCESS;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * CacheDirBlockWrite --
 *
 *	Write into a cache block returned from Fscache_BlockRead.  Used only
 *	for writing directories.
 *
 * Results:
 *	SUCCESS unless error when allocating disk space.
 *
 * Side effects:
 *	The cache block is unlocked.  It is deleted if the offset is the
 *	beginning of the block and the disk allocation failed.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
CacheDirBlockWrite(handlePtr, blockPtr, blockNum, length)
    register	Fsio_FileIOHandle	*handlePtr;	/* Handle for file. */
    register	Fscache_Block	*blockPtr;	/* Cache block. */
    int				blockNum;	/* Block number. */
    int				length;		/* Number of valid bytes in
						 * the block. */
{
    ReturnStatus	status = SUCCESS;
    int			blockAddr = FSDM_NIL_INDEX;
    Boolean		newBlock;
    int			flags = FSCACHE_CLEAR_READ_AHEAD;
    int			offset;
    int			newLastByte;
    int			blockSize;
#ifdef SOSP91
    Boolean		isForeign = FALSE;	/* Due to migration? */
#endif SOSP91

#ifdef SOSP91
    if (proc_RunningProcesses[0] != (Proc_ControlBlock *) NIL) {
	if ((proc_RunningProcesses[0]->state == PROC_MIGRATED) ||
		(proc_RunningProcesses[0]->genFlags &
		(PROC_FOREIGN | PROC_MIGRATING))) {
	    isForeign = TRUE;
	}
    }
#endif SOSP91
    offset =  blockNum * FS_BLOCK_SIZE;
    newLastByte = offset + length - 1;
    (void) (handlePtr->cacheInfo.backendPtr->ioProcs.allocate)
	((Fs_HandleHeader *)handlePtr, offset, length, 0,
		&blockAddr, &newBlock);
#ifdef lint
    (void) Fsdm_BlockAllocate((Fs_HandleHeader *)handlePtr, offset, length,
			0, &blockAddr, &newBlock);
    (void) FsrmtBlockAllocate((Fs_HandleHeader *)handlePtr, offset, length,
			0, &blockAddr, &newBlock);
#endif /* lint */
    if (blockAddr == FSDM_NIL_INDEX) {
	status = FS_NO_DISK_SPACE;
	if (handlePtr->descPtr->lastByte + 1 < offset) {
	    /*
	     * Delete the block if are appending and this was a new cache
	     * block.
	     */
	    flags = FSCACHE_DELETE_BLOCK;
	}
    }
    Fscache_UpdateDirSize(&handlePtr->cacheInfo, newLastByte);
    handlePtr->descPtr->dataModifyTime = Fsutil_TimeInSeconds();
    handlePtr->descPtr->descModifyTime = handlePtr->descPtr->dataModifyTime;
    handlePtr->descPtr->flags |= FSDM_FD_MODTIME_DIRTY;
    fs_Stats.blockCache.dirBytesWritten += FSLCL_DIR_BLOCK_SIZE;
    fs_Stats.blockCache.dirBlockWrites++;
#ifdef SOSP91
	if (isForeign) {
	    fs_SospMigStats.blockCache.dirBytesWritten += FSLCL_DIR_BLOCK_SIZE;
	    fs_SospMigStats.blockCache.dirBlockWrites++;
	}
#endif SOSP91
    blockSize = handlePtr->descPtr->lastByte + 1 - (blockNum * FS_BLOCK_SIZE);
    if (blockSize > FS_BLOCK_SIZE) {
	blockSize = FS_BLOCK_SIZE;
    }
    Fscache_UnlockBlock(blockPtr, (unsigned int)Fsutil_TimeInSeconds(), blockAddr,
			blockSize, flags);
    return(status);
}
