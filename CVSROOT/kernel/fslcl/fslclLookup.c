/* 
 * fsLocalLookup.c --
 *
 *	The routines in the module manage the directory structure.
 *	The top level loop is in FsLocalLookup, and it is the workhorse
 *	of the Local Domain that is called by procedures like FsLocalOpen.
 *	Files and directories are also created, deleted, and renamed
 *	directly (or indirectly) through FsLocalLookup.
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


#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsNameOps.h"
#include "fsPrefix.h"
#include "fsLocalDomain.h"
#include "fsNameHash.h"
#include "fsOpTable.h"
#include "fsConsist.h"
#include "fsCacheOps.h"
#include "fsTrace.h"
#include "fsStat.h"
#include "rpc.h"
#include "vm.h"
#include "string.h"
#include "proc.h"
#include "dbg.h"

/*
 * Debugging flags.
 */
int fsComponentTrace = FALSE;
extern int fsFileNameTrace;

int fsCompacts;		/* The number of times a directory block was so
			 * fragmented that we could have compacted it to
			 * make room for a new entry in the directory */
/*
 * A cache of recently seen pathname components is kept in a hash table.
 * The hash table gets initialized in FsAttachDisk after the first disk
 * gets attached.
 * The name caching can be disabled by setting the fsNameCaching flag to FALSE.
 */

FsHashTable fsNameTable;
FsHashTable *fsNameTablePtr = (FsHashTable *)NIL;
Boolean fsNameCaching = TRUE;
int fsNameHashSize = FS_NAME_HASH_SIZE;

/*
 * Forward Declarations.
 */
ReturnStatus	ExpandLink();
ReturnStatus	FindComponent();
ReturnStatus 	InsertComponent();
Boolean		DirectoryEmpty();


/*
 *----------------------------------------------------------------------
 *
 * FsLocalLookup --
 *
 *	The guts of local file name lookup.  This does a recursive
 *	directory lookup of a file pathname.  The success of the lookup
 *	depends on useFlags and the type.  The process needs to
 *	have read permission along the path, and other permissions on
 *	the target file itself according to useFlags. The type of the
 *	target file has to agree with the type parameter.
 *
 *	The major and minor fields of the FsFileID for local files correspond
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
 *	FsDomainRelease as soon as our caller is finished with the handle.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsLocalLookup(prefixHdrPtr, relativeName, useFlags, type, clientID, idPtr,
		permissions, fileNumber, handlePtrPtr, newNameInfoPtrPtr)
    FsHandleHeader *prefixHdrPtr;	/* Handle from the prefix table or
					 * the current working directory */
    char *relativeName;			/* Name to lookup relative to the
					 * file indicated by prefixHandlePtr */
    int useFlags;			/* FS_READ|FS_WRITE|FS_EXECUTE,
					 * FS_CREATE|FS_EXCLUSIVE, FS_OWNER,
					 * FS_LINK, FS_FOLLOW (links) */
    int type;				/* File type which to succeed on.  If
					 * this is FS_FILE, then any type will
					 * work. */
    int clientID;			/* Host ID of the client doing the open.
					 * Require to properly expand $MACHINE
					 * in pathnames */
    FsUserIDs *idPtr;			/* User and group IDs */
    int permissions;			/* Permission bits to use on a newly
					 * created file. */
    int fileNumber;			/* File number to link to if FS_LINK
					 * useFlag is present */
    FsLocalFileIOHandle **handlePtrPtr;	/* Result, the handle for the file.
					 * This is returned locked.  Also,
					 * its domain has a reference which
					 * needs to be released. */
    FsRedirectInfo **newNameInfoPtrPtr;	/* Redirect Result, the pathname left
					 * after it leaves our domain */
{
    register char 	*curCharPtr;	/* Pointer into the path name */
    register FsLocalFileIOHandle *parentHandlePtr; /* Handle for parent dir. */
    register char	*compPtr;	/* Pointer into component. */
    register ReturnStatus status = SUCCESS;
    register int 	compLen;	/* The length of component */
    FsLocalFileIOHandle	*curHandlePtr;	/* Handle for the current spot in
					 * the directory structure */
    FsDomain *domainPtr;		/* Domain of the lookup */
    char component[FS_MAX_NAME_LENGTH]; /* One component of the path name */
    char *newNameBuffer;		/* Extra buffer used after a symbolic
					 * link has been expanded */
    int numLinks = 0;			/* The number of links that have been
					 * expanded. Used to check against
					 * loops. */

    /*
     * Get a handle on the domain of the file.  This is needed for disk I/O.
     * Remember that the <major> field of the fileID is a domain number.
     */
    domainPtr = FsDomainFetch(prefixHdrPtr->fileID.major, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

   /*
     * Duplicate the prefixHandle into the handle for the current point
     * in the directory.  This locks and ups the reference count on the handle.
     */
    curCharPtr = relativeName;
    curHandlePtr = FsHandleDupType(FsLocalFileIOHandle, prefixHdrPtr);
    parentHandlePtr = (FsLocalFileIOHandle *)NIL;
    newNameBuffer = (char *)NIL;
    if (curHandlePtr->hdr.fileID.type != FS_LCL_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "FsLocalLookup, bad prefix handle type <%d>\n",
	    curHandlePtr->hdr.fileID.type);
    }
    /*
     * Loop through the pathname expanding links and checking permissions.
     * Creations and deletions are handled after this loop.
     */
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
	 * doing the open.  For this host we use a compiled in string so
	 * we can bootstrap ok, and for other clients we get the machine
	 * type string from the net module.  (why net?  why not...
	 * Net_InstallRoute installs a host's name and machine type.)
	 */
#define SPECIAL		"$MACHINE"
#define SPECIAL_LEN	8
	compPtr = component;
	while (*curCharPtr != '/' && *curCharPtr != '\0') {
	    if (*curCharPtr == '$' &&
		(String_NCompare(SPECIAL_LEN, curCharPtr, SPECIAL) == 0)) {
		char *machType;

		if (fsComponentTrace) {
		    Sys_Printf(" $MACHINE -> ");
		}
		if (clientID == rpc_SpriteID) {
		    /*
		     * Can't count on the net stuff being setup for ourselves
		     * as that is done via a user program way after bootting.
		     * Instead, use a compiled in string.  This is important
		     * when opening "/initSprite", which is a link to 
		     * "/initSprite.$MACHINE", when running on the root server.
		     */
		    extern char *etc_MachineType;	/* XXX */
		    machType = etc_MachineType;
		} else {
		    machType = (char *)Net_SpriteIDToMachType(clientID);
		    if (machType == (char *)NIL) {
			Sys_Panic(SYS_WARNING,
			 "FsLocalLookup, no machine type for client %d\n",
				clientID);
			machType = "unknown";
		    }
		}
		while (*machType != '\0') {
		    *compPtr++ = *machType++;
		}
		curCharPtr += SPECIAL_LEN;
#undef SPECIAL
#undef SPECIAL_LEN
	    } else {
		*compPtr++ = *curCharPtr++;
	    }
	}
	*compPtr = '\0';
	compLen = compPtr - component;
	if (fsComponentTrace) {
	    Sys_Printf(" %s ", component);
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
	     * Remember that the fileID <minor> field is the fileNumber.
	     * Compare with the fileNumber of the prefix to check against
	     * falling out of the top of the domain.
	     *
	     * FIX HERE to allow exporting non-roots.
	     */
	    if (curHandlePtr->hdr.fileID.minor == FS_ROOT_FILE_NUMBER) {
		/*
		 * We are falling off the top of the domain.  Make the
		 * remaining part of the filename, including "../",
		 * available to our caller so it can go back to the prefix
		 * table.  Setting the prefixLength to zero indicates
		 * there is no prefix information in this LOOKUP_REDIRECT
		 */
		*newNameInfoPtrPtr = 
			(FsRedirectInfo *) Mem_Alloc(sizeof(FsRedirectInfo));
		(*newNameInfoPtrPtr)->prefixLength = 0;
		String_Copy("../", (*newNameInfoPtrPtr)->fileName);
		String_Cat(curCharPtr, (*newNameInfoPtrPtr)->fileName);
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
		if (parentHandlePtr != (FsLocalFileIOHandle *)NIL) {
		    FsHandleRelease(parentHandlePtr, TRUE);
		}
		parentHandlePtr = curHandlePtr;
		status = FindComponent(parentHandlePtr, component, compLen,
			    TRUE, &curHandlePtr);
		/*
		 * Release the handle while being careful about its lock.
		 * The parent handle is normally aready unlocked by
		 * FindComponent, unless the directory is corrupted.
		 */
		FsHandleRelease(parentHandlePtr, (status != SUCCESS));
		parentHandlePtr = (FsLocalFileIOHandle *)NIL;
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
	    if (parentHandlePtr != (FsLocalFileIOHandle *)NIL) {
		FsHandleRelease(parentHandlePtr, TRUE);
	    }
	    parentHandlePtr = curHandlePtr;
	    status = FindComponent(parentHandlePtr, component, compLen, FALSE,
			&curHandlePtr);
	}
	/*
	 * At this point we have a locked handle on the current point
	 * in the lookup, and perhaps have a locked handle on the parent.
	 * Links are expanded now so we know whether or not the
	 * lookup is completed.  On the last component, we only
	 * expand the link if the FS_FOLLOW flag is present.
	 */
	if ((status == SUCCESS) &&
	    ((*curCharPtr != '\0') || (useFlags & FS_FOLLOW)) &&
	    ((curHandlePtr->descPtr->fileType == FS_SYMBOLIC_LINK ||
		curHandlePtr->descPtr->fileType == FS_REMOTE_LINK))) {
	    numLinks++;
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
		    newNameBuffer = (char *)Mem_Alloc(FS_MAX_PATH_NAME_LENGTH);
		} else {
		    offset = (int)curCharPtr - (int)newNameBuffer;
		}
		status = ExpandLink(curHandlePtr, curCharPtr, offset,
						    newNameBuffer);
		if (status == FS_FILE_NOT_FOUND) {
		    Sys_Panic(SYS_WARNING, "FsLocalLookup, empty link \"%s\"\n",
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
		    *newNameInfoPtrPtr = 
			(FsRedirectInfo *) Mem_Alloc(sizeof(FsRedirectInfo));
		    String_Copy(curCharPtr, (*newNameInfoPtrPtr)->fileName);
		    status = FS_LOOKUP_REDIRECT;
		    /*
		     * Return the length of the prefix indicated by
		     * a remote link, zero means no prefix.
		     */
		    if (curHandlePtr->descPtr->fileType == FS_REMOTE_LINK) {
			(*newNameInfoPtrPtr)->prefixLength = 
					    curHandlePtr->descPtr->lastByte;
		    } else {
			(*newNameInfoPtrPtr)->prefixLength = 0;
		    }
		} else if (parentHandlePtr != (FsLocalFileIOHandle *)NIL) {
		    FsHandleRelease(curHandlePtr, TRUE);
		    curHandlePtr = parentHandlePtr;
		    parentHandlePtr = (FsLocalFileIOHandle *)NIL;
		    status = SUCCESS;
		} else {
		    Sys_Panic(SYS_FATAL, "No parent after link");
		    status = FS_INVALID_ARG;
		}
	    }
	}
    }
    if (useFlags & FS_TRACE_FLAG) {
	FS_TRACE_NAME(FS_TRACE_LOOKUP_DONE, relativeName);
    }
    if ((status == SUCCESS) ||
	((status == FS_FILE_NOT_FOUND) && (*curCharPtr == '\0'))) {
	/*
	 * Done with the lookup.  Process creates, links, and deletes.
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
		    int 	fileNumber;
		    int 	nearbyFile;
    
		    status = CheckPermissions(parentHandlePtr, FS_WRITE, idPtr,
						    FS_DIRECTORY);
		    if (status == SUCCESS) {
			nearbyFile = (type == FS_DIRECTORY) ? -1 :
				     parentHandlePtr->hdr.fileID.minor;
			status = FsGetNewFileNumber(domainPtr, nearbyFile,
							     &fileNumber);
			if (status == SUCCESS) {
			    status = CreateFile(domainPtr, parentHandlePtr,
				     component, compLen, fileNumber, type,
				     permissions, idPtr, &curHandlePtr);
			    if (status != SUCCESS) {
				FsFreeFileNumber(domainPtr, fileNumber);
			    }
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
		break;
	    case FS_LINK:
		/*
		 * The presense of FS_LINK means that curHandlePtr references
		 * a file that is being linked to.  If the file already exists
		 * it is deleted first.  Then link is made with LinkFile.
		 */
		if (status == SUCCESS) {
		    /*
		     * Linking to an existing file.  Make sure the types match,
		     * and only allow links to directories if this is in
		     * preparation for a rename.
		     */
		    if (curHandlePtr->descPtr->fileType != type) {
			if (curHandlePtr->descPtr->fileType == FS_DIRECTORY) {
			    status = FS_IS_DIRECTORY;
			} else if (type == FS_DIRECTORY) {
			    status = FS_NOT_DIRECTORY;
			} else {
			    status = FS_FILE_EXISTS;
			}
		    } else if ((type == FS_DIRECTORY) &&
			       (useFlags & FS_RENAME) == 0) {
			status = FS_NO_ACCESS;
		    } else {
			/* Ok to link to the file */
		    }
		    if (status == SUCCESS) {
			/*
			 * Try the delete, this fails on non-empty directories.
			 */
			status = DeleteFileName(domainPtr, parentHandlePtr,
			      &curHandlePtr, component, compLen, FALSE,
			      clientID, idPtr);
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
		    status = LinkFile(domainPtr, parentHandlePtr,
				component, compLen, fileNumber, &curHandlePtr);
		}
		break;
	    case FS_DELETE:
		if (status == SUCCESS) {
		    if ((curHandlePtr->descPtr->fileType != type) &&
			(type != FS_FILE)) {
			status = (type == FS_DIRECTORY) ? FS_NOT_DIRECTORY :
							  FS_WRONG_TYPE;
		    } else {
			status = DeleteFileName(domainPtr, parentHandlePtr, 
				&curHandlePtr, component, compLen,
				(int) (useFlags & FS_RENAME), clientID, idPtr);
		    }
		}
		break;
	}
    }

    /*
     * Clean up state and return a fileHandle to our caller.
     */
    if (newNameBuffer != (char *)NIL) {
	Mem_Free(newNameBuffer);
    }
    if (parentHandlePtr != (FsLocalFileIOHandle *)NIL) {
	FsHandleRelease(parentHandlePtr, TRUE);
    }
    if (curHandlePtr != (FsLocalFileIOHandle *)NIL) {
	if (status != SUCCESS) {
	    FsHandleRelease(curHandlePtr, TRUE);
	    curHandlePtr = (FsLocalFileIOHandle *)NIL;
	} 
    }
    if (handlePtrPtr != (FsLocalFileIOHandle **)NIL) {
	/*
	 * Return a locked handle that has had its reference count bumped.
	 */
	*handlePtrPtr = curHandlePtr;
    } else if (curHandlePtr != (FsLocalFileIOHandle *)NIL) {
	Sys_Panic(SYS_WARNING, "FsLocalLookup: caller didn't want handle\n");
	FsHandleRelease(curHandlePtr, TRUE);
    }
    if ((status != SUCCESS) ||
	(handlePtrPtr == (FsLocalFileIOHandle **)NIL)) {
	FsDomainRelease(prefixHdrPtr->fileID.major);
    }
    if (fsComponentTrace && !fsFileNameTrace) {
	Sys_Printf(" <%x>\n", status);
    }
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
FindComponent(parentHandlePtr, component, compLen, isDotDot, curHandlePtrPtr)
    FsLocalFileIOHandle	*parentHandlePtr;	/* Locked handle of current 
						 * directory */
    register char 	*component;		/* Name of path component to 
						 * find */
    register int 	compLen;		/* The number of characters in 
						 * component */
    Boolean		isDotDot;		/* TRUE if component is "..".
						 * In this case the handle for
						 * the parent is returned
						 * UNLOCKED. */
    FsLocalFileIOHandle	**curHandlePtrPtr;	/* Return, locked handle */
{
    register FsDirEntry *dirEntryPtr;	/* Reference to directory entry */
    register char	*s1;		/* Pointers into components used */
    register char	*s2;		/*   for fast in-line string compare */
    register int 	blockOffset;	/* Offset within the directory */
    ReturnStatus 	status;
    FsCacheBlock	*cacheBlockPtr;	/* Cache block */
    int 		dirBlockNum;	/* Block number within directory */
    int 		length;		/* Length variable for read call */
    FsHashEntry		*entryPtr;	/* Name cache entry */
    FsFileID		fileID;		/* Used when fetching handles */

    /*
     * Check in system-wide name cache here before scanning
     * the directory's data blocks.
     */
    entryPtr = FS_HASH_LOOK_ONLY(fsNameTablePtr, component, parentHandlePtr);
    if (entryPtr != (FsHashEntry *)NIL) {
	if (entryPtr->hdrPtr->fileID.type != FS_LCL_FILE_STREAM) {
	    Sys_Panic(SYS_FATAL,
		      "FindComponent: got trashy handle from cache");
	} else {
	    if (isDotDot) {
		/*
		 * Unlock this directory before grabbing the handle for "..".
		 * This prevents deadlock with another lookup that is
		 * descending from our parent ("..") into this directory.
		 */
		FsHandleUnlock(parentHandlePtr);
	    }
	    *curHandlePtrPtr = FsHandleDupType(FsLocalFileIOHandle,
						entryPtr->hdrPtr);
	    return(SUCCESS);
	}
    }

    dirBlockNum = 0;
    do {
	status = FsCacheBlockRead(&parentHandlePtr->cacheInfo, dirBlockNum,
			&cacheBlockPtr, &length, FS_DIR_CACHE_BLOCK, FALSE);
	if (status != SUCCESS || length == 0) {
	    *curHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
	    return(FS_FILE_NOT_FOUND);
	}
	dirEntryPtr = (FsDirEntry *)cacheBlockPtr->blockAddr;
	blockOffset = 0;
	while (blockOffset < length) {
	    if (dirEntryPtr->recordLength <= 0) {
		Sys_Printf("Corrupted directory?");
		Sys_Printf(" File ID <%d, %d, %d>",
				 parentHandlePtr->hdr.fileID.serverID,
				 parentHandlePtr->hdr.fileID.major,
				 parentHandlePtr->hdr.fileID.minor);
		Sys_Printf(" dirBlockNum <%d>, blockOffset <%d>",
			     dirBlockNum, blockOffset);
		Sys_Printf("\n");
		FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0,
				    FS_CLEAR_READ_AHEAD);
		*curHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
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
				FsHandleUnlock(parentHandlePtr);
			    }
			    /*
			     * Inlined call to GetHandle().
			     */
			    fileID.type = FS_LCL_FILE_STREAM;
			    fileID.serverID = rpc_SpriteID;
			    fileID.major = parentHandlePtr->hdr.fileID.major;
			    fileID.minor = dirEntryPtr->fileNumber;
			    status = FsLocalFileHandleInit(&fileID,
					    curHandlePtrPtr);

			    FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0,
					       FS_CLEAR_READ_AHEAD);
			    if (status == SUCCESS) {
				FS_HASH_INSERT(fsNameTablePtr, component,
					     parentHandlePtr, *curHandlePtrPtr);
			    } else {
				Sys_Panic(SYS_WARNING,
		"FindComponent, no handle <0x%x> for \"%s\" fileNumber %d\n",
				    status, component, dirEntryPtr->fileNumber);
			    }
			    goto exit;	/* to quiet lint... */
			} else if (*s1++ != *s2++) {
			    break;
			}
		    } while (TRUE);
		}
	    }
	    blockOffset += dirEntryPtr->recordLength;
	    dirEntryPtr = (FsDirEntry *)((int)dirEntryPtr +
					 dirEntryPtr->recordLength);
	}
	dirBlockNum++;
	FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, 0);
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
InsertComponent(curHandlePtr, component, compLen, fileNumber)
    FsLocalFileIOHandle *curHandlePtr;	/* Locked handle of current directory */
    char *component;			/* Name of path component to insert */
    int compLen;			/* The length of component */
    int fileNumber;			/* File Number of inserted name */
{
    ReturnStatus 	status;
    int			dirBlockNum;	/* Directory block index */
    int 		blockOffset;	/* Offset within a directory block. */
    FsDirEntry 		*dirEntryPtr;	/* Reference to directory entry. */
    int 		length;		/* Length variable for read call. */
    int 		recordLength;	/* Length of directory entry for 
					 * component. */
    int 		freeSpace;	/* Total amount of free bytes in a 
					 * directory block. */
    int 		extraBytes;	/* The number of free bytes attached to
				 	 * a directory entry. */
    FsCacheBlock	*cacheBlockPtr;	/* Cache block. */

    length = FS_BLOCK_SIZE;
    recordLength = FsDirRecLength(compLen);
    /*
     * Loop through the directory blocks looking for space of at least
     * recordLength in which to insert the new directory record.
     */
    dirBlockNum = 0;
    do {
	/*
	 * Read in a full data block.
	 */
	status = FsCacheBlockRead(&curHandlePtr->cacheInfo, dirBlockNum,
			&cacheBlockPtr, &length, FS_DIR_CACHE_BLOCK, TRUE);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING, "InsertComponent: Read failed\n");
	    return(status);
	} else if (length == 0) {
	    /*
	     * No more space, have to grow the directory.  First we
	     * need another cache block.
	     */
	    Boolean found = FALSE;

	    FsCacheFetchBlock(&curHandlePtr->cacheInfo, dirBlockNum,
				FS_DIR_CACHE_BLOCK, &cacheBlockPtr, &found);
	    if (found) {
		Sys_Panic(SYS_FATAL, "InsertComponent found new dir block");
	    }
	    Byte_Zero(FS_BLOCK_SIZE, cacheBlockPtr->blockAddr);
	}

	dirEntryPtr = (FsDirEntry *)cacheBlockPtr->blockAddr;
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
			     FsDirRecLength(dirEntryPtr->nameLength);
		if (extraBytes >= recordLength) {
		    /*
		     * Can fit new entry in the space left over.
		     */ 
		    goto haveASlot;
		}
		/*
		 * Count bytes that occur in fragments of 4 bytes or more.
		 */
		freeSpace += extraBytes & ~(FS_REC_LEN_GRAIN-1);
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
	    dirEntryPtr = (FsDirEntry *)((int)dirEntryPtr +
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
	    Byte_Zero(FS_DIR_BLOCK_SIZE, cacheBlockPtr->blockAddr + length);
	    dirEntryPtr->recordLength = FS_DIR_BLOCK_SIZE;
	    length += FS_DIR_BLOCK_SIZE;
	    break;
	}

	dirBlockNum++;
	FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);
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
	FsDirEntry *tmpDirEntryPtr;	/* Pointer to new slot */

	newRecordLength = FsDirRecLength(dirEntryPtr->nameLength);
	tmpDirEntryPtr = (FsDirEntry *)((int)dirEntryPtr + newRecordLength);
	tmpDirEntryPtr->recordLength = dirEntryPtr->recordLength -
				       newRecordLength;
	dirEntryPtr->recordLength = newRecordLength;
	dirEntryPtr = tmpDirEntryPtr;
    }
    dirEntryPtr->fileNumber = fileNumber;
    dirEntryPtr->nameLength = compLen;
    String_Copy(component, dirEntryPtr->fileName);

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
DeleteComponent(parentHandlePtr, component, compLen)
    FsLocalFileIOHandle	*parentHandlePtr;/* Locked handle of current dir. */
    char 		*component;	/* Name to delete */
    int 		compLen;	/* Length of the name */
{
    ReturnStatus	status;
    int 		blockOffset;	/* Offset within a directory block */
    FsDirEntry 		*dirEntryPtr;	/* Reference to directory entry */
    FsDirEntry 		*lastDirEntryPtr;/* Back pointer used when merging 
					  * adjacent entries after the delete */
    int 		length;		/* Length variable for read call */
    FsCacheBlock	*cacheBlockPtr;	/* Cache block. */
    int			dirBlockNum;

    dirBlockNum = 0;
    do {
	status = FsCacheBlockRead(&parentHandlePtr->cacheInfo, dirBlockNum,
			  &cacheBlockPtr, &length, FS_DIR_CACHE_BLOCK, FALSE);
	if (status != SUCCESS || length == 0) {
	    return(FS_FILE_NOT_FOUND);
	}
	blockOffset = 0;
	lastDirEntryPtr = (FsDirEntry *)NIL;
	dirEntryPtr = (FsDirEntry *)cacheBlockPtr->blockAddr;
	while (blockOffset < length) {
	    if ((dirEntryPtr->fileNumber != 0) &&
		(dirEntryPtr->nameLength == compLen) &&
		(String_Compare(component, dirEntryPtr->fileName) == 0)) {
		/*
		 * Delete the entry from the name cache.
		 */
		FS_HASH_DELETE(fsNameTablePtr, component, parentHandlePtr);
		dirEntryPtr->fileNumber = 0;
		if (lastDirEntryPtr != (FsDirEntry *)NIL) {
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
	    if ((blockOffset & (FS_DIR_BLOCK_SIZE - 1)) == 0) {
		 lastDirEntryPtr = (FsDirEntry *) NIL;
	    } else {
		 lastDirEntryPtr = dirEntryPtr;
	    }
	    dirEntryPtr = 
		(FsDirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
	}
	dirBlockNum++;
	FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);
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
    FsLocalFileIOHandle	*curHandlePtr;	/* Handle on the link file */
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

    linkNameLength = curHandlePtr->descPtr->lastByte;
    if (linkNameLength < 0) {
	return(FS_FILE_NOT_FOUND);
    }
    if (*curCharPtr == '\0') {
	/*
	 * There is no pathname, just make sure the new name is Null terminated
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
     * that was just shifted over.
     */
    status = FsCacheRead(&curHandlePtr->cacheInfo, 0, nameBuffer, 0,
		    &linkNameLength, (Sync_RemoteWaiter *)NIL);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * GetHandle --
 *
 *	Given a file number and the handle on the parent directotry,
 *	this routine returns a locked handle for the file.  This is a
 *	small layer on top of FsLocalFileHandleInit that is oriented
 *	towards the needs of the lookup routines.
 *
 * Results:
 *	A return code.  If SUCCESS the returned handle is locked.
 *
 * Side effects:
 *	Calls FsLocalFileHandleInit to set up the handle.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
GetHandle(fileNumber, curHandlePtr, newHandlePtrPtr)
    int		fileNumber;		/* Number of file to get handle for */
    FsLocalFileIOHandle	*curHandlePtr;	/* Handle on file in the same domain */
    FsLocalFileIOHandle	**newHandlePtrPtr;/* Return, ref. to installed handle */
{
    register ReturnStatus status;
    FsFileID fileID;

    fileID.type = FS_LCL_FILE_STREAM;
    fileID.serverID = rpc_SpriteID;
    fileID.major = curHandlePtr->hdr.fileID.major;
    fileID.minor = fileNumber;
    status = FsLocalFileHandleInit(&fileID, newHandlePtrPtr);
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
 *	Calls InsertComponent, FsInitFileDesc.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
CreateFile(domainPtr, parentHandlePtr, component, compLen, fileNumber, type,
	   permissions, idPtr, curHandlePtrPtr)
    FsDomain	*domainPtr;		/* Domain of the file */
    FsLocalFileIOHandle	*parentHandlePtr;/* Handle of directory in which to add 
					 * file. */
    char	*component;		/* Name of the file */
    int		compLen;		/* The length of component */
    int		fileNumber;		/* Domain relative file number */
    int		type;			/* Type of the file */
    int		permissions;		/* Permission bits on the file */
    FsUserIDs	*idPtr;			/* User ID of calling process */
    FsLocalFileIOHandle	**curHandlePtrPtr;/* Return, handle for the new file */
{
    ReturnStatus	status;
    FsFileDescriptor	*parentDescPtr;	/* Descriptor for the parent */
    FsFileDescriptor	*newDescPtr;	/* Descriptor for the new file */

    /*
     * Set up the file descriptor using the group ID from the parent directory.
     */
    parentDescPtr = parentHandlePtr->descPtr;
    newDescPtr = (FsFileDescriptor *)Mem_Alloc(sizeof(FsFileDescriptor));
    status = FsInitFileDesc(domainPtr, fileNumber, type, permissions,
			    idPtr->user, parentDescPtr->gid, newDescPtr);
    if (status == SUCCESS) {
	if (type == FS_DIRECTORY) {
	    /*
	     * Both the parent and the directory itself reference it.
	     */
	    newDescPtr->numLinks = 2;
	}
	status = FsStoreFileDesc(domainPtr, fileNumber, newDescPtr);
	if (status == SUCCESS) {
	    /*
	     * GetHandle does extra work because we already have the
	     * file descriptor all set up...
	     */
	    status = GetHandle(fileNumber, parentHandlePtr, curHandlePtrPtr);
	    if (status != SUCCESS) {
		/*
		 * GetHandle shouldn't fail because we just wrote out
		 * the file descriptor.
		 */
		Sys_Panic(SYS_FATAL, "CreateFile: GetHandle failed\n");
		newDescPtr->flags = FS_FD_FREE;
		(void)FsStoreFileDesc(domainPtr, fileNumber, newDescPtr);
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
							compLen, fileNumber);
		    if (type == FS_DIRECTORY) {
			if (status == SUCCESS) {
			    /*
			     * Update the parent directory to reflect
			     * the addition of a new sub-directory.
			     */
			    parentDescPtr->numLinks++;
			    parentDescPtr->descModifyTime = fsTimeInSeconds;
			    (void)FsStoreFileDesc(domainPtr,
				      parentHandlePtr->hdr.fileID.minor,
					  parentDescPtr);
			}
		    }
		}
		if (status != SUCCESS) {
		    /*
		     * Couldn't add to the directory, no disk space perhaps.
		     * Unwind by marking the file descriptor as free and
		     * releasing the handle we've created.
		     */
		    Sys_Panic(SYS_WARNING, "CreateFile: unwinding\n");
		    newDescPtr->flags = FS_FD_FREE;
		    (void)FsStoreFileDesc(domainPtr, fileNumber, newDescPtr);
		    FsHandleRelease(*curHandlePtrPtr, TRUE);
		    *curHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
		}
	    }
	}
    }
    Mem_Free((Address) newDescPtr);
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
    FsLocalFileIOHandle *curHandlePtr;		/* Handle of file to delete */
    FsLocalFileIOHandle *parentHandlePtr;	/* Handle of directory in which
						 * to delete file*/
{
    ReturnStatus	status;
    int			offset;
    int			length;
    register FsDirEntry *dirEntryPtr;
    char		*dirBlock;

    /*
     * The Mem_Alloc and Byte_Copy could be avoided by puting this routine
     * into its own monitor so that it could write directly onto fsEmptyDirBlock
     * fsEmptyDirBlock is already set up with ".", "..", and the correct
     * nameLengths and recordLengths.
     */
    dirBlock = (char *)Mem_Alloc(FS_DIR_BLOCK_SIZE);
    Byte_Copy(FS_DIR_BLOCK_SIZE, (Address)fsEmptyDirBlock,
				 (Address)dirBlock);
    dirEntryPtr = (FsDirEntry *)dirBlock;
    dirEntryPtr->fileNumber = curHandlePtr->hdr.fileID.minor;
    dirEntryPtr = (FsDirEntry *)((int)dirEntryPtr +
				 dirEntryPtr->recordLength);
    dirEntryPtr->fileNumber = parentHandlePtr->hdr.fileID.minor;
    offset = 0;
    length = FS_DIR_BLOCK_SIZE;
    status = FsCacheWrite(&curHandlePtr->cacheInfo, 0, (Address)dirBlock,
		offset, &length, (Sync_RemoteWaiter *)NIL);
    Mem_Free(dirBlock);
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
LinkFile(domainPtr, parentHandlePtr, component, compLen, fileNumber,
	 curHandlePtrPtr)
    FsDomain	*domainPtr;		/* Domain of the file */
    FsLocalFileIOHandle	*parentHandlePtr;/* Handle of directory in which to add 
					 * file. */
    char	*component;		/* Name of the file */
    int		compLen;		/* The length of component */
    int		fileNumber;		/* Domain relative file number */
    FsLocalFileIOHandle	**curHandlePtrPtr;/* Return, handle for the new file */
{
    ReturnStatus	status;
    FsFileDescriptor	*linkDescPtr;	/* Descriptor for the existing file */
    Time 		modTime;	/* Descriptors are modified */

    if (fileNumber == parentHandlePtr->hdr.fileID.minor) {
	/*
	 * Trying to move a directory into itself, ie. % mv subdir subdir
	 */
	return(GEN_INVALID_ARG);
    }
    status = GetHandle(fileNumber, parentHandlePtr, curHandlePtrPtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "LinkFile: can't get existing file handle\n");
	return(FAILURE);
    } else if ((*curHandlePtrPtr)->descPtr->fileType == FS_DIRECTORY) {
	status = OkToMoveDirectory(parentHandlePtr, *curHandlePtrPtr);
    }
    if (status == SUCCESS) {
	linkDescPtr = (*curHandlePtrPtr)->descPtr;
	linkDescPtr->numLinks++;
	linkDescPtr->descModifyTime = fsTimeInSeconds;
	status = FsStoreFileDesc(domainPtr, fileNumber, linkDescPtr);
	if (status == SUCCESS) {
	    /*
	     * Commit by adding the name to the directory.
	     */
	    status = InsertComponent(parentHandlePtr, component, compLen,
							fileNumber);
	    if ((*curHandlePtrPtr)->descPtr->fileType == FS_DIRECTORY &&
		status == SUCCESS) {
		/*
		 * This link is part of a rename (links to directories are
		 * only allowed at this time), and the ".." entry in the
		 * directory may have to be fixed.
		 */
		modTime.seconds = fsTimeInSeconds;
		status = MoveDirectory(domainPtr, &modTime, parentHandlePtr,
						    *curHandlePtrPtr);
		if (status != SUCCESS) {
		    (void)DeleteComponent(parentHandlePtr, component,
							     compLen);
		}
	    }
	    if (status != SUCCESS) {
		/*
		 * Couldn't add to the directory because of I/O probably.
		 * Unwind by reducing the link count.  This gets the cache
		 * consistent with the cached directory image anyway.
		 */
		linkDescPtr->numLinks--;
		(void)FsStoreFileDesc(domainPtr, fileNumber, linkDescPtr);
		FsHandleRelease(*curHandlePtrPtr, TRUE);
		*curHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
	    }
	} else {
	    linkDescPtr->numLinks--;
	}
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
    FsLocalFileIOHandle *newParentHandlePtr;	/* New parent directory for
						 * curHandlePtr */
    FsLocalFileIOHandle *curHandlePtr;		/* Directory being moved */
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
	status == SUCCESS;
    } else {
	/*
	 * Have to trace up from the new parent to make sure we don't find
	 * the current file.  If we let that happen then you create
	 * dis-connected loops in the directory structure.
	 */
	FsLocalFileIOHandle *parentHandlePtr;
	int parentNumber;

	for (parentNumber = newParentNumber; status == SUCCESS; ) {
	    if (parentNumber == curHandlePtr->hdr.fileID.minor) {
		status = FS_INVALID_ARG;
	    } else if (parentNumber == FS_ROOT_FILE_NUMBER ||
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
		    status = GetHandle(parentNumber, curHandlePtr,
					&parentHandlePtr);
		} else {
		    parentHandlePtr = newParentHandlePtr;
		}
		if (status == SUCCESS) {
		    status = GetParentNumber(parentHandlePtr, &parentNumber);
		}
		if (parentHandlePtr != newParentHandlePtr) {
		    (void)FsHandleRelease(parentHandlePtr, TRUE);
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
MoveDirectory(domainPtr, modTimePtr, newParentHandlePtr, curHandlePtr)
    FsDomain	*domainPtr;			/* Domain of operation */
    Time	*modTimePtr;			/* Modify time for parent */
    FsLocalFileIOHandle	*newParentHandlePtr;	/* New parent directory for 
						 * curHandlePtr */
    FsLocalFileIOHandle	*curHandlePtr;		/* Directory being moved */
{
    ReturnStatus	status;
    int			oldParentFileNumber;	/* File number of original 
						 * parent directory */
    int			newParentNumber;	/* File number of new 
						 * parent directory */

    status = GetParentNumber(curHandlePtr, &oldParentFileNumber);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "MoveDirectory: Can't get parent's file number\n");
    } else {
	newParentNumber = newParentHandlePtr->hdr.fileID.minor;
	if (oldParentFileNumber != newParentNumber) {
	    /*
	     * Patch the directory entry for ".."
	     */
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
	    register FsFileDescriptor *parentDescPtr;

	    parentDescPtr = newParentHandlePtr->descPtr;
	    parentDescPtr->numLinks++;
	    parentDescPtr->descModifyTime = modTimePtr->seconds;
	    (void)FsStoreFileDesc(domainPtr, newParentNumber, parentDescPtr);
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
    FsLocalFileIOHandle	*curHandlePtr;	/* Handle for current directory */
    int		*parentNumberPtr;	/* Result, the file number of the parent
					 * of curHandlePtr */
{
    ReturnStatus 	status;
    int 		length;
    register FsDirEntry *dirEntryPtr;
    FsCacheBlock	*cacheBlockPtr;

    status = FsCacheBlockRead(&curHandlePtr->cacheInfo, 0, &cacheBlockPtr,
				&length, FS_DIR_CACHE_BLOCK, FALSE);
    if (status != SUCCESS) {
	return(status);
    } else if (length == 0) {
	return(FAILURE);
    }

    dirEntryPtr = (FsDirEntry *)cacheBlockPtr->blockAddr;
    if (dirEntryPtr->nameLength != 1 ||
	dirEntryPtr->fileName[0] != '.' ||
	dirEntryPtr->fileName[1] != '\0') {
	Sys_Panic(SYS_WARNING,
		  "GetParentNumber: \".\", corrupted directory\n");
	status = FAILURE;
    } else {
	dirEntryPtr = 
		(FsDirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
	if (dirEntryPtr->nameLength != 2 ||
	    dirEntryPtr->fileName[0] != '.' ||
	    dirEntryPtr->fileName[1] != '.' ||
	    dirEntryPtr->fileName[2] != '\0') {
	    Sys_Panic(SYS_WARNING,
		      "GetParentNumber: \"..\", corrupted directory\n");
	    status = FAILURE;
	} else {
	    *parentNumberPtr = dirEntryPtr->fileNumber;
	}
    }
    FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);

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
    FsLocalFileIOHandle	*curHandlePtr;	/* Handle for current directory */
    int 		newParentNumber;/* The new file number of the parent
					 * of curHandlePtr */
{
    ReturnStatus	status;
    int 		length;
    register FsDirEntry *dirEntryPtr;
    FsCacheBlock	*cacheBlockPtr;

    status = FsCacheBlockRead(&curHandlePtr->cacheInfo, 0, &cacheBlockPtr,
				&length, FS_DIR_CACHE_BLOCK, FALSE);
    if (status != SUCCESS) {
	return(status);
    } else if (length == 0) {
	return(FAILURE);
    }
    dirEntryPtr = (FsDirEntry *)cacheBlockPtr->blockAddr;
    if (dirEntryPtr->nameLength != 1 ||
	dirEntryPtr->fileName[0] != '.' ||
	dirEntryPtr->fileName[1] != '\0') {
	Sys_Panic(SYS_WARNING,
		  "SetParentNumber: \".\", corrupted directory\n");
	FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);
	return(FAILURE);
    }
    dirEntryPtr = (FsDirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
    if (dirEntryPtr->nameLength != 2 ||
	dirEntryPtr->fileName[0] != '.' ||
	dirEntryPtr->fileName[1] != '.' ||
	dirEntryPtr->fileName[2] != '\0') {
	Sys_Panic(SYS_WARNING,"SetParentNumber: \"..\", corrupted directory\n");
	FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);
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
 *      Delete a file by clearing it's fileNumber in the directory and
 *	reducing its link count.  If there are no links left then the
 *	file's handle is marked as deleted.  Finally, if this routine
 *	has the last reference on the handle then the file's data blocks
 *	are truncated away and the file descriptor is marked as free.
 *
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	May call FsDeleteFileDesc to remove the file and its data.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
DeleteFileName(domainPtr, parentHandlePtr, curHandlePtrPtr, component,
	       compLen, forRename, clientID, idPtr)
    FsDomain *domainPtr;			/* Domain of the file */
    FsLocalFileIOHandle *parentHandlePtr;	/* Handle of directory in
						 * which to delete file*/
    FsLocalFileIOHandle **curHandlePtrPtr;	/* Handle of file to delete */
    char *component;				/* Name of the file to delte */
    int compLen;				/* The length of component */
    int forRename;		/* if FS_RENAME, then the file being delted
				 * is being renamed.  This allows non-empty
				 * directories to be deleted */
    int clientID;		/* Host doing lookup.  Used to prevent call-
				 * backs to this host. */
    FsUserIDs *idPtr;		/* User and group IDs */
{
    ReturnStatus status;
    register FsLocalFileIOHandle *curHandlePtr;	/* Local copy */
    FsFileDescriptor *parentDescPtr;	/* Descriptor for parent */
    FsFileDescriptor *curDescPtr;	/* Descriptor for the file to delete */
    int type;				/* Type of the file */

    curHandlePtr = *curHandlePtrPtr;
    type = curHandlePtr->descPtr->fileType;
    if (parentHandlePtr == (FsLocalFileIOHandle *)NIL) {
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
	 * One needs write permission in the parent to do the delte.
	 */
	status = CheckPermissions(parentHandlePtr, FS_WRITE, idPtr,
					FS_DIRECTORY);
    }
    if (status != SUCCESS) {
	return(status);
    }
    curDescPtr = curHandlePtr->descPtr;
    parentDescPtr = parentHandlePtr->descPtr;
    /*
     * Remove the name from the directory first.
     */
    status = DeleteComponent(parentHandlePtr, component, compLen);
    if (status == SUCCESS) {
	curDescPtr->numLinks--;
	if (type == FS_DIRECTORY && !forRename) {
	    /*
	     * Directories have an extra link because they reference themselves.
	     */
	    curDescPtr->numLinks--;
	    if (curDescPtr->numLinks > 0) {
	    Sys_Panic(SYS_WARNING,
		      "DeleteFileName: extra links on directory\n");
	    }
	}
	curDescPtr->descModifyTime = fsTimeInSeconds;
	status = FsStoreFileDesc(domainPtr, curHandlePtr->hdr.fileID.minor,
				 curDescPtr);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING,
		      "DeleteFileName: (1) Couldn't store descriptor\n");
	    return(status);
	}
	if (type == FS_DIRECTORY) {
	    /*
	     * A directory's link count reflects the number of subdirectories
	     * it has (they each have a ".." that references it.)  Here
	     * it is decremented because the subdirectory is going away.
	     */
	    parentDescPtr->numLinks--;
	    parentDescPtr->descModifyTime = fsTimeInSeconds;
	    status = FsStoreFileDesc(domainPtr,
			 parentHandlePtr->hdr.fileID.minor, parentDescPtr);
	    if (status != SUCCESS) {
		Sys_Panic(SYS_WARNING,
			  "DeleteFileName: (2) Couldn't store descriptor\n");
		return(status);
	    }
	}
	if (curDescPtr->numLinks <= 0) {
	    /*
	     * At this point curHandlePtr is potentially the last reference
	     * to the file.  If there are no users then do the delete, otherwise
	     * mark the handle as deleted and FsFileClose will take care of it.
	     */
	    curHandlePtr->flags |= FS_FILE_DELETED;
	    if (curHandlePtr->use.ref == 0) {
		/*
		 * Tell other clients (only the last writer) that the
		 * file has been deleted.  Call with our own hostID
		 * order to guarantee a call-back to all clients.
		 */
		FsClientRemoveCallback(&curHandlePtr->consist, rpc_SpriteID);
		/*
		 * Delete the file from disk.
		 */
		status = FsDeleteFileDesc(curHandlePtr);
		/*
		 * Wipe out the handle.
		 */
		FsHandleRelease(curHandlePtr, TRUE);
		FsHandleRemove(curHandlePtr);
		*curHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
	    } else {
		FsHandleRelease(curHandlePtr, TRUE);
		*curHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
	    }
	} else {
	    FsHandleRelease(curHandlePtr, TRUE);
	    *curHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
	}
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsDeleteFileDesc --
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
FsDeleteFileDesc(handlePtr)
    FsLocalFileIOHandle *handlePtr;
{
    ReturnStatus status;
    FsDomain *domainPtr;

    if (handlePtr->descPtr->fileType == FS_DIRECTORY) {
	/*
	 * Remove .. from the name cache so we don't end up with
	 * a bad cache entry later when this directory is re-created.
	 */
	FS_HASH_DELETE(fsNameTablePtr, "..", handlePtr);
    }

    domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    FS_TRACE_HANDLE(FS_TRACE_DELETE, ((FsHandleHeader *)handlePtr));
    status = FsFileTrunc(handlePtr, 0, FS_TRUNC_DELETE);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "FsDeleteFileDesc: Can't truncate file\n");
    } else {
	handlePtr->descPtr->flags = FS_FD_FREE;
	status = FsStoreFileDesc(domainPtr, handlePtr->hdr.fileID.minor,
					    handlePtr->descPtr);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING, "FsDeleteFileDesc: Can't free descriptor\n");
	} else {
	    FsFreeFileNumber(domainPtr, handlePtr->hdr.fileID.minor);
	}
	Mem_Free((Address)handlePtr->descPtr);
	handlePtr->descPtr = (FsFileDescriptor *)NIL;
    }
    FsDomainRelease(handlePtr->hdr.fileID.major);
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
    FsLocalFileIOHandle *handlePtr;	/* Handle of directory to check */
{
    ReturnStatus	status;
    int 		blockOffset;	/* Offset within a directory block */
    FsDirEntry 		*dirEntryPtr;	/* Reference to directory entry */
    int 		length;		/* Length for read call */
    int			dirBlockNum;
    FsCacheBlock	*cacheBlockPtr;

    dirBlockNum = 0;
    do {
	status = FsCacheBlockRead(&handlePtr->cacheInfo, dirBlockNum,
		      &cacheBlockPtr, &length, FS_DIR_CACHE_BLOCK, FALSE);
	if (status != SUCCESS || length == 0) {
	    /*
	     * Have run out of the directory and not found anything.
	     */
	    return(TRUE);
	}
	blockOffset = 0;
	dirEntryPtr = (FsDirEntry *)cacheBlockPtr->blockAddr;
	while (blockOffset < length) {
	    if (dirEntryPtr->fileNumber != 0) {
		/*
		 * A valid directory record.
		 */
		if ((String_Compare(".", dirEntryPtr->fileName) == 0) ||
		    (String_Compare("..", dirEntryPtr->fileName) == 0)) {
		    /*
		     * "." and ".." are ok
		     */
		} else {
		    FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, 
					FS_CLEAR_READ_AHEAD);
		    return(FALSE);
		}
	    }
	    blockOffset += dirEntryPtr->recordLength;
	    dirEntryPtr = 
		(FsDirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
	}
	dirBlockNum++;
	FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);
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
 *	There is no high level semantic checking like preventing
 *	directories from being written by users.
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
    FsLocalFileIOHandle		*handlePtr;
    register int		useFlags;
    register FsUserIDs		*idPtr;
    int 			type;
{
    register FsFileDescriptor	*descPtr;
    register int		*groupPtr;
    register unsigned int 	permBits;
    register int 		index;
    register int		uid = idPtr->user;
    ReturnStatus 		status;

    if (handlePtr->hdr.fileID.type != FS_LCL_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "CheckPermissions on non-local file\n");
	return(FAILURE);
    }
    descPtr = handlePtr->descPtr;
    /*
     * Make sure the file type matches.  FS_FILE means any time, otherwise
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
/*	    Sys_Panic(SYS_WARNING, "Allowing a symlink for a remote link\n");*/
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
     * Check for ownership permission first.
     */
    if (useFlags & FS_OWNERSHIP) {
	if ((uid != descPtr->uid) && (uid != 0)) {
	    return(FS_NOT_OWNER);
	}
    }
    /*
     * Check read/write/exec permissions against one of the owner bits,
     * the group bits, or the world bits.  'permBits' is set to
     * be the corresponding bits from the file descriptor and then
     * shifted over so the comparisions are against the WORLD bits.
     */
    if (uid == 0) {
	/*
	 * Let uid 0 do anything, regardless of the file's permBits
	 */
	status = SUCCESS;
    } else {
	if (uid == descPtr->uid) {
	    permBits = (descPtr->permissions >> 6) & 07;
	} else {
	    for (index = idPtr->numGroupIDs, groupPtr = idPtr->group;
	         index > 0;	/* index is just a counter */
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
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * CacheDirBlockWrite --
 *
 *	Write into a cache block returned from FsCacheBlockRead.  Used only
 *	for writing directories.
 *
 *	THIS USES THE descPtr FIELDS.  CHECK cacheInfo.attr FIELDS
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
    register	FsLocalFileIOHandle	*handlePtr;	/* Handle for file. */
    register	FsCacheBlock	*blockPtr;	/* Cache block. */
    int				blockNum;	/* Block number. */
    int				length;		/* Number of valid bytes in
						 * the block. */
{
    ReturnStatus	status = SUCCESS;
    int			blockAddr = FS_NIL_INDEX;
    Boolean		newBlock;
    int			flags = FS_CLEAR_READ_AHEAD;
    int			offset;
    int			newLastByte;
    int			blockSize;

    offset =  blockNum * FS_BLOCK_SIZE;
    newLastByte = offset + length - 1;
    (void) (*fsStreamOpTable[handlePtr->hdr.fileID.type].allocate)
	((FsHandleHeader *)handlePtr, offset, length, &blockAddr, &newBlock);
    if (blockAddr == FS_NIL_INDEX) {
	status = FS_NO_DISK_SPACE;
	if (handlePtr->descPtr->lastByte + 1 < offset) {
	    /*
	     * Delete the block if are appending and this was a new cache
	     * block.
	     */
	    flags = FS_DELETE_BLOCK;
	}
    }
    FsUpdateDirSize(&handlePtr->cacheInfo, newLastByte);
    handlePtr->descPtr->dataModifyTime = fsTimeInSeconds;
    fsStats.blockCache.dirBytesWritten += FS_DIR_BLOCK_SIZE;
    fsStats.blockCache.dirBlockWrites++;
    blockSize = handlePtr->descPtr->lastByte + 1 - (blockNum * FS_BLOCK_SIZE);
    if (blockSize > FS_BLOCK_SIZE) {
	blockSize = FS_BLOCK_SIZE;
    }
    FsCacheUnlockBlock(blockPtr, (unsigned int)fsTimeInSeconds, blockAddr,
			blockSize, flags);
    return(status);
}
