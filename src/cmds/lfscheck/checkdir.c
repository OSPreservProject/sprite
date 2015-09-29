/* 
 * checkdir.c --
 *
 *	Routines to allow moving through a files block pointers.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/fscheck/RCS/checkdir.c,v 1.29 90/10/10 15:29:03 mendel Exp $ SPRITE (Berkeley)";
#endif not lint

#include <option.h>
#include "fscheck.h"
#include <list.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static  int 		lostFoundFileNum = -1;
static	char		pathName[FS_MAX_PATH_NAME_LENGTH];

int foundError;
int verbose = 1;
int debug = 0;
int tooBig = 0;
static int		diskFd;

Fsdm_FileDescriptor	*rootFDPtr, *lostFoundFDPtr;

static int MakePtrAccessible _ARGS_((register DirIndexInfo *indexInfoPtr));
static int GetFirstIndex _ARGS_((int blockNum, register DirIndexInfo *indexInfoPtr));
static int GetNextIndex _ARGS_((register DirIndexInfo *indexInfoPtr));
static void OpenDir _ARGS_((Fsdm_FileDescriptor *fdPtr, FdInfo *fdInfoPtr, DirIndexInfo *indexInfoPtr, Fslcl_DirEntry **dirEntryPtrPtr));
static void NextDirEntry _ARGS_((DirIndexInfo *indexInfoPtr, Fslcl_DirEntry **dirEntryPtrPtr));
static void CloseDir _ARGS_((DirIndexInfo *indexInfoPtr));
static void CheckDir _ARGS_((int fdNum, Fsdm_FileDescriptor *fdPtr, FdInfo *fdInfoPtr, int parentFdNum, int lostDirNum, int *dirOKPtr));

#define DIRFULL 2


/*
 *----------------------------------------------------------------------
 *
 * FetchFileDesc --
 *
 *	Return a file descriptor.
 *
 * Results:
 *	1 if the file descriptor is successfully read or 0 if the descriptor 
 *	was unreadable.
 *
 * Side effects:
 *	A file descriptor may be allocated.
 *
 *----------------------------------------------------------------------
 */
int
FetchFileDesc(fdNum, fdPtrPtr)
    int			fdNum;		/* Number of descriptor to fetch. */
    Fsdm_FileDescriptor	**fdPtrPtr;	/* Where to store ptr to descriptor. */
{
    int bufSize,  j;
    LfsFileDescriptor	*descPtr;
    char *descBuf;
    LfsDescMapEntry *descMapPtr;

    if ((fdNum < 0) || (fdNum >= superBlockPtr->descMap.maxDesc)) {
	return 0;
    }
    if (descInfoArray[fdNum].flags & FD_UNREADABLE) {
	return 0;
    }
    bufSize = superBlockPtr->fileLayout.descPerBlock * sizeof(*descPtr);

    descBuf = alloca(bufSize);

    descMapPtr = DescMapEntry(fdNum);
    if (descMapPtr->flags != LFS_DESC_MAP_ALLOCED) {
	Alloc(*fdPtrPtr,Fsdm_FileDescriptor,1);
	if (tooBig) {
	    return 0;
	}
	bzero((char *) *fdPtrPtr, sizeof(Fsdm_FileDescriptor));
	return 1;
    }
    if (DiskRead(diskFd, descMapPtr->blockAddress, bufSize, descBuf)
		    != bufSize) {
	return 0;
    }
    descPtr = (LfsFileDescriptor *)descBuf;
    for (j = 0; j < superBlockPtr->fileLayout.descPerBlock; j++) {
	if (!(descPtr->common.flags & FSDM_FD_ALLOC)) {
	    break;
	}
	if (descPtr->fileNumber == fdNum) {
	    break;
	}
	descPtr++;
    }
    if ((j >= superBlockPtr->fileLayout.descPerBlock) ||
	!(descPtr->common.flags & FSDM_FD_ALLOC) ||
	(descPtr->fileNumber != fdNum)) {
	return 0;
    }
    Alloc(*fdPtrPtr,Fsdm_FileDescriptor,1);
    if (tooBig) {
	return 0;
    }
    bcopy((Address)&(descPtr->common), (Address)*fdPtrPtr, 
	  sizeof(Fsdm_FileDescriptor));
    return(1);
}


/*
 *----------------------------------------------------------------------
 *
 * StoreFileDesc --
 *
 *	Store a file descriptor on disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An FdCheckInfo struct is allocated.
 *
 *----------------------------------------------------------------------
 */
void
StoreFileDesc(fdNum, fdPtr)
    int			fdNum;
    Fsdm_FileDescriptor	*fdPtr;
{
    extern void panic();
    panic("StoreFileDesc not implemented\n");
}

/*
 *----------------------------------------------------------------------
 *
 * MakePtrAccessible --
 *
 *	Make the pointer to the directory entry accessible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	*indexInfoPtr modified.
 *
 *----------------------------------------------------------------------
 */
static int
MakePtrAccessible(indexInfoPtr)
    register	DirIndexInfo *indexInfoPtr;
{
    register int 		*blockAddrPtr;
    register Fsdm_FileDescriptor	*fdPtr;

    fdPtr = indexInfoPtr->fdPtr;

    blockAddrPtr = &indexInfoPtr->blockAddr;

    if (indexInfoPtr->indexType == INDIRECT) {
	*blockAddrPtr = fdPtr->indirect[0];
    } else {
	*blockAddrPtr = fdPtr->indirect[1];
    }

    /*
     * Read first level block in.
     */
    if (indexInfoPtr->firstBlockNil) {
	if (*blockAddrPtr == FSDM_NIL_INDEX) {
	    return(1);
	}
	if (DiskRead(diskFd,
			  *blockAddrPtr, FS_BLOCK_SIZE,
			  indexInfoPtr->firstBlock) != FS_BLOCK_SIZE) {
	    fprintf(stderr,"MakePtrAccessible: Read (1) failed block %d\n",
			   *blockAddrPtr);
	    return(0);
	}
	indexInfoPtr->firstBlockNil = 0;
    }

    *blockAddrPtr = *(int *) (indexInfoPtr->firstBlock +
			      sizeof(int) * indexInfoPtr->firstIndex);
    if (indexInfoPtr->indexType == INDIRECT) {
	return(1);
    }

    /*
     * Read second level block in.
     */
    if (*blockAddrPtr != FSDM_NIL_INDEX) {
	if (DiskRead(diskFd, 
			  *blockAddrPtr, FS_BLOCK_SIZE,
			  indexInfoPtr->secondBlock) != FS_BLOCK_SIZE) {
	    fprintf(stderr,"MakePtrAccessible: Read (2) failed block %d\n",
			   *blockAddrPtr);
	    return(0);
	}
	indexInfoPtr->secondBlockNil = 0;
	*blockAddrPtr = *(int *) (indexInfoPtr->secondBlock +
				  sizeof(int) * indexInfoPtr->secondIndex);
    }
    return(1);
}


/*
 *----------------------------------------------------------------------
 *
 * GetFirstIndex --
 *
 *	Initialize the index structure.  This will set up the index info
 *	structure so that it contains a pointer to the desired block pointer.
 *
 * Results:
 *	1 if could set up the index, 0 if could not.
 *
 * Side effects:
 *	The index structure is initialized.
 *
 *----------------------------------------------------------------------
 */
static int
GetFirstIndex(blockNum, indexInfoPtr)
    int			      blockNum;      /* Where to start indexing. */
    register DirIndexInfo     *indexInfoPtr; /* Index structure to initialize.*/
{
    int			indirectBlock;
    Fsdm_FileDescriptor	*fdPtr;

    indexInfoPtr->firstBlockNil = 1;
    indexInfoPtr->secondBlockNil = 1;
    indexInfoPtr->blockNum = blockNum;
    indexInfoPtr->dirDirty = 0;

    fdPtr = indexInfoPtr->fdPtr;

    if (blockNum < FSDM_NUM_DIRECT_BLOCKS) {
	/*
	 * This is a direct block.
	 */
	indexInfoPtr->indexType = DIRECT;
	indexInfoPtr->firstIndex = blockNum;
	indexInfoPtr->blockAddr = fdPtr->direct[blockNum];
	return(1);
    }

    /*
     * Is an indirect block.
     */
    blockNum -= FSDM_NUM_DIRECT_BLOCKS;
    indirectBlock = blockNum / FSDM_INDICES_PER_BLOCK;
    if (indirectBlock == 0) {
	/*
	 * This is a singly indirect block.
	 */
	indexInfoPtr->indexType = INDIRECT;
	indexInfoPtr->firstIndex = blockNum;
    } else {
	/*
	 * This a doubly indirect block.
	 */
	indexInfoPtr->indexType = DBL_INDIRECT;
	indexInfoPtr->firstIndex = indirectBlock - 1;
	indexInfoPtr->secondIndex = blockNum -
				    indirectBlock * FSDM_INDICES_PER_BLOCK;
    }

    /*
     * Finish off by making the block pointer accessible.  This may include
     * reading indirect blocks into the cache.
     */
    return(MakePtrAccessible(indexInfoPtr));
}


/*
 *----------------------------------------------------------------------
 *
 * GetNextIndex --
 *
 *	Put the correct pointers in the index structure to access the
 *	block after the current block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The allocation structure is modified.
 *
 *----------------------------------------------------------------------
 */
static int
GetNextIndex(indexInfoPtr)
    register DirIndexInfo *indexInfoPtr; /* Index structure to set up. */
{
    int			accessible = 0;
    register Fsdm_FileDescriptor	*fdPtr;

    fdPtr = indexInfoPtr->fdPtr;

    indexInfoPtr->blockNum++;
    indexInfoPtr->dirDirty = 0;

    /*
     * Determine whether we are now in direct, indirect or doubly indirect
     * blocks.
     */
    switch (indexInfoPtr->indexType) {
	case DIRECT:
	    if (indexInfoPtr->blockNum < FSDM_NUM_DIRECT_BLOCKS) {
		/*
		 * Still in the direct blocks.
		 */
		indexInfoPtr->firstIndex++;
		indexInfoPtr->blockAddr = 
				fdPtr->direct[indexInfoPtr->firstIndex];
		accessible = 1;
	    } else {
		/*
		 * Moved into indirect blocks.
		 */
		indexInfoPtr->indexType = INDIRECT;
		indexInfoPtr->firstIndex = 0;
	    }
	    break;
	case INDIRECT:
	    if (indexInfoPtr->blockNum < FSDM_NUM_DIRECT_BLOCKS +
			FSDM_INDICES_PER_BLOCK) {
		/*
		 * Still in singly indirect blocks.
		 */
		indexInfoPtr->firstIndex++;
		indexInfoPtr->blockAddr = *(int *) (indexInfoPtr->firstBlock +
				  sizeof(int) * indexInfoPtr->firstIndex);
		accessible = 1;
		break;
	   } else {
		/*
		 * Moved into doubly indirect blocks.
		 */
		indexInfoPtr->firstIndex = 0;
		indexInfoPtr->secondIndex = 0;
		indexInfoPtr->indexType = DBL_INDIRECT;
		/*
		 * Free up the pointer block.
		 */
		indexInfoPtr->firstBlockNil = 1;
	    }
	    break;
	case DBL_INDIRECT:
	    indexInfoPtr->secondIndex++;
	    if (indexInfoPtr->secondIndex == FSDM_INDICES_PER_BLOCK) {
		indexInfoPtr->firstIndex++;
		indexInfoPtr->secondIndex = 0;
		indexInfoPtr->secondBlockNil = 1;
	    } else {
		indexInfoPtr->blockAddr = *(int *) (indexInfoPtr->secondBlock +
			      sizeof(int) * indexInfoPtr->secondIndex);
		accessible = 1;
	    }
	    break;
    }

    /*
     * Make the block pointers accessible if necessary.
     */
    if (!accessible) {
	return(MakePtrAccessible(indexInfoPtr));
    } else {
	return(1);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * OpenDir --
 *
 *	Set up the structure to allow moving through the given directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The index structure is set up and *dirEntryPtrPtr set to point to
 *	the first directory entry.
 *
 *----------------------------------------------------------------------
 */
static void
OpenDir(fdPtr, fdInfoPtr, indexInfoPtr, dirEntryPtrPtr)
    Fsdm_FileDescriptor	*fdPtr;		/* The file descriptor for the
					 * directory. */
    FdInfo		*fdInfoPtr;	/* Descriptor status info for the
					 * directory. */
    DirIndexInfo 	*indexInfoPtr;	/* Index info struct */
    Fslcl_DirEntry		**dirEntryPtrPtr; /* Where to return a pointer to
					   * the first directory entry. */
{
    int			fragsToRead;

    if (fdPtr->lastByte == -1) {
	/*
	 * Empty directory.
	 */
	*dirEntryPtrPtr = (Fslcl_DirEntry *) NULL;
	return;
    } else if ((fdPtr->lastByte + 1) % FSLCL_DIR_BLOCK_SIZE != 0) {
	fprintf(stderr,
    "Directory not multiple of directory block size. Directory corrected.\n");
	foundError = 1;
	fdPtr->lastByte = (fdPtr->lastByte & ~(FSLCL_DIR_BLOCK_SIZE - 1)) +
			    FSLCL_DIR_BLOCK_SIZE - 1;
	fdInfoPtr->flags |= FD_MODIFIED;
    }
    /*
     * Initialize the index structure.
     */
    indexInfoPtr->fdPtr = fdPtr;
    indexInfoPtr->fdInfoPtr = fdInfoPtr;
    if (!GetFirstIndex(0, indexInfoPtr)) {
	fprintf(stderr, "OpenDir: Error setting up index\n");
	*dirEntryPtrPtr = (Fslcl_DirEntry *) NULL;
	return;
    }
    /*
     * Read in the first directory block.
     */
    if (fdPtr->lastByte >= FS_BLOCK_SIZE - 1) {
	fragsToRead = FS_FRAGMENTS_PER_BLOCK;
    } else {
	fragsToRead = fdPtr->lastByte / FS_FRAGMENT_SIZE + 1;
    }
    indexInfoPtr->numFrags = fragsToRead;
    if (DiskRead(diskFd, 
		      indexInfoPtr->blockAddr, 
		      fragsToRead*FS_FRAGMENT_SIZE, indexInfoPtr->dirBlock) 
		      != fragsToRead*FS_FRAGMENT_SIZE) {
	fprintf(stderr, "OpenDir: Read failed block %d\n",
		       indexInfoPtr->blockAddr); 
	*dirEntryPtrPtr = (Fslcl_DirEntry *) NULL;
	return;
    } 
    indexInfoPtr->dirOffset = 0;
    *dirEntryPtrPtr = (Fslcl_DirEntry *) indexInfoPtr->dirBlock;
}


/*
 *----------------------------------------------------------------------
 *
 * NextDirEntry --
 *
 *	Return a pointer to the next directory entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The index structure is modified and *dirEntryPtrPtr set to point
 *	to the next directory entry.
 *
 *----------------------------------------------------------------------
 */
static void
NextDirEntry(indexInfoPtr, dirEntryPtrPtr)
    DirIndexInfo 	*indexInfoPtr;
    Fslcl_DirEntry		**dirEntryPtrPtr;
{
    int			firstDirByte;
    int			lastByte;
    Fslcl_DirEntry		*dirEntryPtr;
    int			fragsToRead;

    dirEntryPtr = *dirEntryPtrPtr;
    indexInfoPtr->dirOffset += dirEntryPtr->recordLength;
    lastByte = indexInfoPtr->fdPtr->lastByte;
    firstDirByte = 
	 indexInfoPtr->dirOffset + indexInfoPtr->blockNum * FS_BLOCK_SIZE;
    if (firstDirByte == lastByte + 1) {
	/*
	 * We reached the end of the directory.  Write out the directory
	 * block if necessary.
	 */
	*dirEntryPtrPtr = (Fslcl_DirEntry *) NULL;
	return;
    }

    if (indexInfoPtr->dirOffset < FS_BLOCK_SIZE) {
	/*
	 * The next directory entry is in the current block.
	 */
	dirEntryPtr = 
	     (Fslcl_DirEntry *) &(indexInfoPtr->dirBlock[indexInfoPtr->dirOffset]);
    } else {
	/*
	 * Have to move to the next directory block.  Write out the current
	 * block if necessary.
	 */
#ifdef notdef
	if (indexInfoPtr->dirDirty && writeDisk) {
		*dirEntryPtrPtr = (Fslcl_DirEntry *) NULL;
	    }
	}
#endif
	if (!GetNextIndex(indexInfoPtr)) {
	    fprintf(stderr, "NextDirEntry: Get index failed\n");
	    *dirEntryPtrPtr = (Fslcl_DirEntry *) NULL;
	    return;
	}
	if (lastByte - firstDirByte + 1 >= FS_BLOCK_SIZE ||
	    indexInfoPtr->blockNum >= FSDM_NUM_DIRECT_BLOCKS) {
	    fragsToRead = FS_FRAGMENTS_PER_BLOCK;
	} else {
	    fragsToRead = (lastByte - firstDirByte) / FS_FRAGMENT_SIZE + 1;
	}
	indexInfoPtr->numFrags = fragsToRead;
	if (DiskRead(diskFd,   indexInfoPtr->blockAddr,
			  fragsToRead*FS_FRAGMENT_SIZE, 
			  indexInfoPtr->dirBlock) != fragsToRead*FS_FRAGMENT_SIZE) {
	    fprintf(stderr, "NextDirEntry: Read failed block %d\n",
			    indexInfoPtr->blockAddr); 
	    *dirEntryPtrPtr = (Fslcl_DirEntry *) NULL;
	    return;
	}
	indexInfoPtr->dirOffset = 0;
	dirEntryPtr = (Fslcl_DirEntry *) indexInfoPtr->dirBlock;
    }

    *dirEntryPtrPtr = dirEntryPtr;
}

static	DirIndexInfo	lostDirIndex;
static  Fslcl_DirEntry	*lostDirEntryPtr;

static	List_Links	orphanDirListHdr;
static  List_Links	*orphanDirList = &orphanDirListHdr;
typedef struct DirList {
    List_Links	links;
    int		dirNumber;
} DirList;

/*
 *----------------------------------------------------------------------
 *
 * CloseDir --
 *
 *	Flushes the current directory block to disk, if necessary.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The index structure is modified and *dirEntryPtrPtr set to point
 *	to the next directory entry.
 *
 *----------------------------------------------------------------------
 */
static void
CloseDir(indexInfoPtr)
    DirIndexInfo 	*indexInfoPtr;
{

#ifdef notdef
    if (indexInfoPtr->dirDirty && writeDisk) {
	    return;
    }
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * CheckDirTree --
 *
 *	Traverse the directory tree taking care of unreferenced files and
 *	ensuring that link counts are correct.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
CheckDirTree(diskId)
    int			diskId;
{
    register	int	i;
    register FdInfo	*fdInfoPtr;
    Fslcl_DirEntry		*dirEntryPtr;
    DirIndexInfo	dirIndex;
    int			lostRootDirNum = -1;
    int			unrefFiles = 0;
    int			linkCountsCorrected = 0;
    int			entryNum;
    int			notAdded;


    diskFd = diskId;
    /*
     * Check the root directory for consistency.
     */
    if (!FetchFileDesc(FSDM_ROOT_FILE_NUMBER, &rootFDPtr)) {
	fprintf(stderr, "Unable to fetch file descriptor for root");
	exit(EXIT_HARD_ERROR);
    }

    if ((rootFDPtr->flags & FSDM_FD_FREE) ||
	rootFDPtr->fileType != FS_DIRECTORY ||
	rootFDPtr->magic != FSDM_FD_MAGIC ||
	rootFDPtr->lastByte == -1) {

	fprintf(stderr, "Root directory corrupted\n");
	    exit(EXIT_HARD_ERROR);
    }
    OpenDir(rootFDPtr, &descInfoArray[FSDM_ROOT_FILE_NUMBER], &dirIndex, 
	    &dirEntryPtr);
    if (dirEntryPtr == (Fslcl_DirEntry *)NULL) {
	exit(EXIT_HARD_ERROR);
    }

    (void)strcpy(pathName, "/");
    entryNum = 0;
    while (dirEntryPtr != (Fslcl_DirEntry *) NULL) {
	/*
	 * Go through the root directory checking each directory entry.
	 */
	CheckDirEntry(entryNum, &dirIndex, dirEntryPtr);
	if (dirEntryPtr->nameLength > 0 && 
	    strncmp("lost+found", dirEntryPtr->fileName,
		    dirEntryPtr->nameLength)  == 0) {
	    lostFoundFileNum = dirEntryPtr->fileNumber;
	}
	NextDirEntry(&dirIndex, &dirEntryPtr);
	entryNum++;
    }
    CloseDir(&dirIndex);
    if (tooBig) {
	return;
    }

    if (lostFoundFileNum == -1) {
	fprintf(stderr, "lost+found missing from root\n");
	lostFoundFileNum = -1;
    } else {
	/*
	 * Make sure that lost and found is consistent.
	 */
	if (!FetchFileDesc(lostFoundFileNum, &lostFoundFDPtr)) {
	    fprintf(stderr, "Unable to fetch file descriptor for lost+found");
	    exit(EXIT_HARD_ERROR);
	}

	(void)strcpy(pathName, "/lost+found/");
	fdInfoPtr = &descInfoArray[lostFoundFileNum];
	if (lostFoundFDPtr->fileType != FS_DIRECTORY) {
	    fprintf(stderr,
	     "Lost+found isn't a directory!  Should remove and recreate.\n");
	    lostFoundFileNum = -1;
	} else {
	    int	dirOK;

	    CheckDir(lostFoundFileNum, lostFoundFDPtr, fdInfoPtr,
			FSDM_ROOT_FILE_NUMBER, lostRootDirNum,	&dirOK);
	    if (dirOK) {
		OpenDir(lostFoundFDPtr, fdInfoPtr, &lostDirIndex,
				 &lostDirEntryPtr);
		if (lostDirEntryPtr == (Fslcl_DirEntry *)NULL) {
		    fprintf(stderr, "Could not open lost+found\n");
		    lostFoundFileNum = -1;
		} else {
		    descInfoArray[lostFoundFileNum].flags |= FD_SEEN;
		}
	    } else {
		lostFoundFileNum = -1;
	    }
	}
    }

    if (tooBig) {
	return;
    }
    /*
     * Check all file descriptors.  If we are re-creating the root this
     * has a side-effect of preparing a list of the orphans of the root.
     */
    for (i = 0, fdInfoPtr = descInfoArray;
	 i < superBlockPtr->descMap.maxDesc; 
	 i++, fdInfoPtr++) {
	int			dirOK;
	Fsdm_FileDescriptor	*newFDPtr;

	if (!(fdInfoPtr->flags & FD_ALLOCATED) ||
	     (fdInfoPtr->flags & FD_SEEN) ||
	    !(fdInfoPtr->flags & IS_A_DIRECTORY)) {
	    continue;
	}
	pathName[0] = '\0';
	if (!FetchFileDesc(i, &newFDPtr)) {
	    fprintf(stderr,
		    "Unable to fetch file descriptor for directory <%d>.\n", i);
	    continue;
	}
	CheckDir(i, newFDPtr, fdInfoPtr, FSDM_ROOT_FILE_NUMBER,
	    lostRootDirNum, &dirOK);
#ifdef notdef
	StoreFileDesc(i, newFDPtr);
#endif
    }
    if (tooBig) {
	return;
    }
    /*
     * Now go through the file descriptors again this time putting all
     * unreferenced files in the lost and found directory and correcting
     * link counts.
     */
    notAdded = 0;
    for (i = 0, fdInfoPtr = descInfoArray;
	 i < superBlockPtr->descMap.maxDesc; 
	 i++, fdInfoPtr++) {

	/*
	 * We have to do lost+found last because its link count is changed
	 * when directories are added to it.
	 */

	if (i == lostFoundFileNum) {
	    continue;
	} else if (i == superBlockPtr->descMap.maxDesc-1) {
	    if (lostFoundFileNum == -1) {
		break;
	    }
	    i = lostFoundFileNum;
	    fdInfoPtr = &descInfoArray[lostFoundFileNum];
	}
	if (!(fdInfoPtr->flags & FD_ALLOCATED) ||
	    (fdInfoPtr->flags & FD_UNREADABLE) ||
	    i == FSDM_BAD_BLOCK_FILE_NUMBER) {
	    continue;
	}

	if (!(fdInfoPtr->flags & FD_REFERENCED) && i != FSDM_ROOT_FILE_NUMBER) {
	    if (verbose) {
		fprintf(stderr, "File %d is unreferenced\n", i);
	    }
	    unrefFiles++;
	    foundError = 1;
	    fflush(stderr);
#ifdef notdef
	    if (lostFoundFileNum != -1) {
		(void) sprintf(newFileName,"%d",i);

		if (AddToDirectory(lostFoundFileNum, lostFoundFDPtr, 
				&lostDirIndex, &lostDirEntryPtr, i, 
				newFileName, fdInfoPtr) == DIRFULL) {
		    if (notAdded == 0) {
			fprintf(stderr, 
			    "Directory #%d full.  Couldn't insert file.\n", 
			    lostFoundFileNum);
		    }
		    notAdded++;
		}
	    }
#endif
	} else if (fdInfoPtr->newLinkCount != fdInfoPtr->origLinkCount) {
	    if (verbose) {
		fprintf(stderr,
		    "Link count corrected for file %d.  Is %d should be %d.\n", 
		    i, fdInfoPtr->origLinkCount, fdInfoPtr->newLinkCount);
	    }
	    linkCountsCorrected++;
	    foundError = 1;
	    fflush(stderr);
	    if (i == FSDM_ROOT_FILE_NUMBER) {
		rootFDPtr->numLinks = fdInfoPtr->newLinkCount;
		fdInfoPtr->flags |= FD_MODIFIED;
	    } else if (i == lostFoundFileNum) {
		lostFoundFDPtr->numLinks = fdInfoPtr->newLinkCount;
		fdInfoPtr->flags |= FD_MODIFIED;
	    } else {
		Fsdm_FileDescriptor	*fdPtr;
		if (!FetchFileDesc(i, &fdPtr)) {
		    fprintf(stderr,
      "Unable to fetch file descriptor for file <%d> to update link count", i);
		} else {
		    fdPtr->numLinks = fdInfoPtr->newLinkCount;
		    fdInfoPtr->flags |= FD_MODIFIED;
#ifdef notdef
		    StoreFileDesc(i, fdPtr);
#endif
		}
	    }
	}
	if (i == lostFoundFileNum) {
	    break;
	}
    }
    if (notAdded > 0) {
	fprintf(stderr, "%d files not added to directory %d.\n", notAdded,
	    lostFoundFileNum);
    }

    if (unrefFiles > 0) {
	fprintf(stderr, "%d unreferenced files\n", unrefFiles);
    }
    if (linkCountsCorrected) {
	fprintf(stderr, "%d links counts corrected\n",
		       linkCountsCorrected);
    }

#ifdef notdef
    if (lostFoundFileNum != - 1 && lostDirIndex.dirDirty && writeDisk) {
	if (Disk_FragWrite(partFID, domainPtr,
			lostDirIndex.blockAddr + domainPtr->dataOffset * 
				FS_FRAGMENTS_PER_BLOCK,
			    lostDirIndex.numFrags, 
			    lostDirIndex.dirBlock) < 0) {
	    OutputPerror("CheckDirTree: Write failed");
	    exit(EXIT_WRITE_FAILURE);
	}
    }

    StoreFileDesc(FSDM_ROOT_FILE_NUMBER, rootFDPtr);
    if (lostFoundFileNum != -1) {
	StoreFileDesc(lostFoundFileNum, lostFoundFDPtr);
    }
#endif
}


/*
 *----------------------------------------------------------------------
 *
* CheckDir --
 *
 *	Descend the directory tree starting from the given file descriptor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CheckDir(fdNum, fdPtr, fdInfoPtr, parentFdNum, lostDirNum, dirOKPtr)
    int			fdNum;	/* Which file descriptor we are looking
					   at. */
    Fsdm_FileDescriptor	*fdPtr;		/* Pointer the file descriptor that
					 * we are looking at. */
    FdInfo		*fdInfoPtr;	/* Status of the file desc that we are
					   looking at. */
    int			parentFdNum;	/* File descriptor of parent directory.
					 */
    int			lostDirNum;	/* For re-creating trashed directories.
					 * This routine will add to a list
					 * of directories that were ref'ed
					 * by this 'lost directory' */
    int			*dirOKPtr;	/* Return 1 if the directory is
					 * not corrupted. */
{
    Fslcl_DirEntry	 *dirEntryPtr;
    FdInfo	 *newFDInfoPtr;
    DirIndexInfo dirIndex;
    int		 entryNum;
    int		nullIndex;

    fdInfoPtr->flags |= FD_SEEN;
    *dirOKPtr = 0;

    /*
     * Open the directory.
     */
    OpenDir(fdPtr, fdInfoPtr, &dirIndex, &dirEntryPtr);
    nullIndex = strlen(pathName);
    if (dirEntryPtr == (Fslcl_DirEntry *) NULL) {	
	fprintf(stderr, "Empty directory %d %s changed to a file.\n",
		        fdNum, pathName);
	foundError = 1;
	fdPtr->fileType = FS_FILE;
	fdInfoPtr->flags |= FD_MODIFIED;
	return;
    }
    /*
     * Go through the directory.
     */
    entryNum = 0;
    if (debug) {
	fprintf(stderr,"Working on %s\n",pathName);
    }
    do {
	CheckDirEntry(entryNum, &dirIndex, dirEntryPtr);
	if (entryNum == 0) {
	    /*
	     * This should be "." and should point to the directory that
	     * we are checking.
	     */
	    if (dirEntryPtr->fileNumber == 0 ||
		strncmp(".", dirEntryPtr->fileName, 
			dirEntryPtr->nameLength)  != 0) {
		fprintf(stderr,
			". missing in directory %d %s.  Changed to a file.\n",
			       fdNum, pathName);
		foundError = 1;
		fdPtr->fileType = FS_FILE;
		fdInfoPtr->flags |= FD_MODIFIED;
		fdInfoPtr->flags &= ~IS_A_DIRECTORY;
		return;
	    }
	    if (dirEntryPtr->fileNumber != fdNum) {
		fprintf(stderr, 
			    ". does not point to self for directory %d %s\n",
			    fdNum, pathName);
		foundError = 1;
		dirEntryPtr->fileNumber = fdNum;
		dirIndex.dirDirty = 1;
	    }
	    fdInfoPtr->newLinkCount++;
	} else if (entryNum == 1) {
	    /*
	     * This should be ".."
	     */
	    if (dirEntryPtr->fileNumber == 0 ||
		strncmp("..", dirEntryPtr->fileName, 
			dirEntryPtr->nameLength) != 0) {
		fprintf(stderr,
			".. missing in directory %d %s.  Changed to a file.\n",
			       fdNum, pathName);
		foundError = 1;
		fdPtr->fileType = FS_FILE;
		fdInfoPtr->flags |= FD_MODIFIED;
		fdInfoPtr->flags &= ~IS_A_DIRECTORY;
		return;
	    }
	    if (dirEntryPtr->fileNumber != parentFdNum) {
		fprintf(stderr, 
		    ".. in directory %d %s pointed to %d, changed to %d.\n",
			fdNum, pathName, dirEntryPtr->fileNumber, parentFdNum);
		foundError = 1;
		dirEntryPtr->fileNumber = parentFdNum;
		dirIndex.dirDirty = 1;
	    }
	    /*
	     * Look for orphans of lost directories.
	     */
	    if (lostDirNum > 0 &&
		dirEntryPtr->fileNumber == lostDirNum &&
		(fdNum != FSDM_ROOT_FILE_NUMBER ||
		lostDirNum != FSDM_ROOT_FILE_NUMBER)) {
		DirList *dirListPtr;
		Alloc(dirListPtr,DirList,1);
		if (!tooBig) {
		    dirListPtr->dirNumber = fdNum;
		    fprintf(stderr, "Found #%d, an orphan of dir #%d\n", 
			    fdNum, lostDirNum);
		    List_Insert((List_Links *)dirListPtr,
				LIST_ATREAR(orphanDirList));
		    fdInfoPtr->flags |= FD_REFERENCED;
		    fdInfoPtr->newLinkCount++;
		}
	    } else {
		descInfoArray[dirEntryPtr->fileNumber].newLinkCount++;
	    }
	} else if (dirEntryPtr->fileNumber != 0) {
	    newFDInfoPtr = &descInfoArray[dirEntryPtr->fileNumber];
	    newFDInfoPtr->newLinkCount++;
	    newFDInfoPtr->flags |= FD_REFERENCED;
	    if (!(newFDInfoPtr->flags & FD_SEEN) &&
	       (newFDInfoPtr->flags & IS_A_DIRECTORY)) {
		int	dirOK;
		Fsdm_FileDescriptor	*newFDPtr;

		/*
		 * Recurse on the directory.
		 */
		(void)strncat(pathName, dirEntryPtr->fileName,
			      dirEntryPtr->nameLength);
		(void)strcat(pathName, "/");
		if (!FetchFileDesc(dirEntryPtr->fileNumber, &newFDPtr)) {
		    fprintf(stderr,
		    "Unable to fetch file descriptor for directory <%d>.\n",
			      dirEntryPtr->fileNumber);
		} else {
		    CheckDir(dirEntryPtr->fileNumber, newFDPtr, 
			     newFDInfoPtr, fdNum, lostDirNum, &dirOK);
		    pathName[nullIndex] = '\0';
#ifdef notdef
		    StoreFileDesc(dirEntryPtr->fileNumber, newFDPtr);
#endif
		}
	    } 
	}
	NextDirEntry(&dirIndex, &dirEntryPtr);
	entryNum++;
    } while (dirEntryPtr != (Fslcl_DirEntry *) NULL);

    *dirOKPtr = 1;
}


/*
 *----------------------------------------------------------------------
 *
 * CheckDirEntry --
 *
 *	Perform checks on the current directory entry to make sure that
 *	it is has not been trashed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The directory may be modified.
 *
 *----------------------------------------------------------------------
 */

void
CheckDirEntry(entryNum, dirIndexPtr, dirEntryPtr)
    int		entryNum;	
    register	DirIndexInfo	*dirIndexPtr;
    register	Fslcl_DirEntry	*dirEntryPtr;
{
    int			dirBlockOffset;
    int			lastDirByte;
    char		buf[FS_MAX_NAME_LENGTH + 1];
    int			nameLength;
    register	char	*strPtr;
    int hadError = 0;

    dirBlockOffset = dirIndexPtr->dirOffset & (FSLCL_DIR_BLOCK_SIZE - 1);
    lastDirByte = dirBlockOffset + dirEntryPtr->recordLength;

    if (dirEntryPtr->fileNumber == 0) {
	nameLength = 0;
    } else {
	nameLength = dirEntryPtr->nameLength;
    }

    /*
     * First check the record length.
     */
    if (lastDirByte > FSLCL_DIR_BLOCK_SIZE ||
	lastDirByte % FSLCL_REC_LEN_GRAIN != 0 || 
	(FSLCL_DIR_BLOCK_SIZE - lastDirByte < FSLCL_DIR_ENTRY_HEADER &&
	 lastDirByte != FSLCL_DIR_BLOCK_SIZE) ||
	dirEntryPtr->recordLength < Fslcl_DirRecLength(nameLength) ||
	dirEntryPtr->recordLength < 0) {
	fprintf(stderr,
	"Bad record length in directory.  Directory entry deleted from %s\n",
		       pathName);
	foundError = 1;
	hadError = 1;
	/*
	 * If the record length is screwed up, extend this record to the end
	 * of the directory block and zap the file number.
	 */
	dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - dirBlockOffset;
	dirEntryPtr->fileNumber = 0;
	dirIndexPtr->dirDirty = 1;
	return;
    }
    /*
     * Check the name length.
     */
    if (dirEntryPtr->fileNumber != 0) {
	int	nameError = 0;
	nameLength = strnlen(dirEntryPtr->fileName,
			     dirEntryPtr->recordLength - FSLCL_DIR_ENTRY_HEADER);
	if (nameLength != dirEntryPtr->nameLength) {
	    bcopy(dirEntryPtr->fileName, buf, nameLength);
	    buf[nameLength] = '\0';
	    fprintf(stderr,
	    "Name length %d wrong for directory entry: %s. Should be %d.\n", 
		dirEntryPtr->nameLength, buf, nameLength);
	    foundError = 1;
	    hadError = 1;
	    dirEntryPtr->nameLength = nameLength;
	    dirIndexPtr->dirDirty = 1;
	    dirEntryPtr->recordLength = Fslcl_DirRecLength(nameLength);
	}
	/*
	 * Make sure that the name contains printable characters.
	 */
	strPtr = dirEntryPtr->fileName;
	while (*strPtr != '\0') {
	    if (*strPtr < 0 || !isprint(*strPtr)) {
		*strPtr = '?';
		nameError = 1;
	    }
	    strPtr++;
	}
	if (nameError) {
	    dirIndexPtr->dirDirty = 1;
	    fprintf(stderr,
	      "Non-printable characters in name for file %d in directory %s\n",
	      dirEntryPtr->fileNumber, pathName);
	    foundError = 1;
	    hadError = 1;
	}
    }

    /*
     * Now check the file number.
     */
    if (dirEntryPtr->fileNumber < 0 ||
        dirEntryPtr->fileNumber >= superBlockPtr->descMap.maxDesc) {
	fprintf(stderr, 
	"Bad file number in directory.  Directory entry deleted from %s.\n", 
		       pathName);
	foundError = 1;
	hadError = 1;
	dirEntryPtr->fileNumber = 0;
	dirIndexPtr->dirDirty = 1;
    /*
     * Here we want to allow the ".." entry (entryNum = 0) to reference a
     * non-allocated file descriptor. What we have is an orphan directory so
     * we shouldn't nuke it just because something bad happened to its parent.
     */
    } else if (dirEntryPtr->fileNumber != 0 &&
              !(descInfoArray[dirEntryPtr->fileNumber].flags & FD_ALLOCATED) &&
	      entryNum != 1)
	      {
	fprintf(stderr, 
	    "File %s%s references non-allocated descriptor %d. File Deleted.\n",
	    pathName, dirEntryPtr->fileName,dirEntryPtr->fileNumber);
	foundError = 1;
	hadError = 1;
	dirEntryPtr->fileNumber = 0;
	dirIndexPtr->dirDirty = 1;
    }
    if (!hadError && debug) {
	fprintf(stderr,"Entry %s ok.\n",dirEntryPtr->fileName);
    }
    if (hadError) {
	fprintf(stderr,
    "Entry %s (%d) now has nameLength %d, recordLength %d, fileNumber %d.\n",
	dirEntryPtr->fileName, entryNum, dirEntryPtr->nameLength, 
	dirEntryPtr->recordLength, dirEntryPtr->fileNumber);
    }
}
#ifdef notdef


/*
 *----------------------------------------------------------------------
 *
 * SetDotDot --
 *
 *	Make ".." in the given directory point to a given directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	".." in the given directory is set to point to lost and found.
 *
 *----------------------------------------------------------------------
 */
static int
SetDotDot(dirNumber, dirFDPtr, fdPtr, fdInfoPtr)
    int			dirNumber;
    Fsdm_FileDescriptor	*dirFDPtr;
    Fsdm_FileDescriptor	*fdPtr;
    FdInfo		*fdInfoPtr;
{
    Fslcl_DirEntry	 	*dirEntryPtr;
    DirIndexInfo 	dirIndex;

    /*
     * Open the directory.
     */
    OpenDir(fdPtr, fdInfoPtr, &dirIndex, &dirEntryPtr);
    if (dirEntryPtr == (Fslcl_DirEntry *)NULL) {
	fprintf(stderr, "SetDotDot: Could not open dir\n");
	return(0);
    }
    /*
     * Move past "." to "..".  Note that it is assume that this directory
     * has been checked and thus has both "." and "..".
     */
    NextDirEntry(&dirIndex, &dirEntryPtr);
    if (dirEntryPtr == (Fslcl_DirEntry *)NULL) {
	fprintf(stderr, "SetDotDot: Could not move from . to ..\n");
	return(0);
    }

    descInfoArray[dirNumber].newLinkCount++;
    descInfoArray[dirNumber].flags |= FD_MODIFIED;
    dirEntryPtr->fileNumber = dirNumber;
    if (writeDisk) {
	if (Disk_FragWrite(partFID, domainPtr, 
	   dirIndex.blockAddr + domainPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
	   dirIndex.numFrags, dirIndex.dirBlock) < 0) {
	    fprintf(stderr, "SetDotDot: Write failed block %d\n",
		    dirIndex.blockAddr + 
		        domainPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK);
	    return(0);
	}
    }
    return(1);
}

/*
 *----------------------------------------------------------------------
 *
 * AddToDirectory --
 *
 *	Add the file descriptor to a directory.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The directory is modified to contain the orphaned file.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
AddToDirectory(dirNumber, dirFDPtr, dirIndexPtr, dirEntryPtrPtr, fileNumber,
    fileName, fdInfoPtr)
    int			dirNumber;
    Fsdm_FileDescriptor	*dirFDPtr;
    DirIndexInfo	*dirIndexPtr;
    Fslcl_DirEntry		**dirEntryPtrPtr;
    int		 	fileNumber;
    char 		*fileName;
    register	FdInfo	*fdInfoPtr;
{
    int		 	nameLength;
    int		 	recordLength;
    int		 	leftOver;
    int			oldRecLength;
    Fslcl_DirEntry		*dirEntryPtr;
    Fsdm_FileDescriptor	*fdPtr;

    adding = 1;
    nameLength = strlen(fileName);
    recordLength = Fslcl_DirRecLength(nameLength);

    dirEntryPtr = *dirEntryPtrPtr;
    while (dirEntryPtr != (Fslcl_DirEntry *) NULL) {
	if (dirEntryPtr->fileNumber != 0) {
	    oldRecLength = Fslcl_DirRecLength(dirEntryPtr->nameLength);
	    leftOver = dirEntryPtr->recordLength - oldRecLength;
	    if (leftOver >= recordLength) {
		dirEntryPtr->recordLength = oldRecLength;
		dirEntryPtr = 
			(Fslcl_DirEntry *) ((int) dirEntryPtr + oldRecLength);
		dirEntryPtr->recordLength = leftOver;
		dirIndexPtr->dirOffset += oldRecLength;
	    } else {
		NextDirEntry(dirIndexPtr, &dirEntryPtr);
		continue;
	    }
	} else if (dirEntryPtr->recordLength < recordLength) {
	    NextDirEntry(dirIndexPtr, &dirEntryPtr);
	    continue;
	}

	if (!FetchFileDesc(fileNumber, &fdPtr)) {
	    fprintf(stderr,
     "Unable to fetch file descriptor for file <%d> to add to directory <%d>\n",
		      fileNumber, dirNumber);
	    return FAILURE;
	}
	if (fdInfoPtr->flags & IS_A_DIRECTORY) {
	    if (!SetDotDot(dirNumber, dirFDPtr, fdPtr, fdInfoPtr)) {
		*dirEntryPtrPtr = dirEntryPtr;
		return FAILURE;
	    }
	}
	fdPtr->numLinks = fdInfoPtr->newLinkCount + 1;
	fdInfoPtr->flags |= FD_MODIFIED;
	StoreFileDesc(fileNumber, fdPtr);

	dirEntryPtr->fileNumber = fileNumber;
	dirEntryPtr->nameLength = nameLength;
	dirIndexPtr->dirDirty = 1;
	(void)strcpy(dirEntryPtr->fileName, fileName);
	leftOver = dirEntryPtr->recordLength - recordLength;
	if (leftOver > FSLCL_DIR_ENTRY_HEADER) {
	    dirEntryPtr->recordLength = recordLength;
	    dirEntryPtr =(Fslcl_DirEntry *) ((int) dirEntryPtr + recordLength);
	    dirEntryPtr->fileNumber = 0;
	    dirEntryPtr->recordLength = leftOver;
	    dirIndexPtr->dirOffset += recordLength;
	} else {
	    NextDirEntry(dirIndexPtr, &dirEntryPtr);
	}
	*dirEntryPtrPtr = dirEntryPtr;
	return SUCCESS;
    }
    return DIRFULL;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeRoot --
 *
 *	Set up the file descriptor for the root directory.
 *
 * Results:
 *	Fill in the file descriptor.
 *
 * Side effects:
 *	Marks block 0 in the bitmap as in use.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
MakeRoot(domainPtr, bitmapPtr, fdPtr)
    Ofs_DomainHeader		*domainPtr;	/* Ptr to domain info */
    u_char			*bitmapPtr;	/* Ptr to cylinder data block
						 * bitmap */
    Fsdm_FileDescriptor		*fdPtr;
{
    Time 	time;
    int 	index;
    u_char 	*bytePtr;

    fdPtr->flags = FSDM_FD_ALLOC;
    fdPtr->fileType = FS_DIRECTORY;
    fdPtr->permissions = 0755;
    fdPtr->uid = 0;
    fdPtr->gid = 0;
    fdPtr->lastByte = FS_BLOCK_SIZE-1;
    fdPtr->firstByte = -1;
    fdPtr->numLinks = 3;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fdPtr->devServerID  -1;
    fdPtr->devType = -1;
    fdPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fdPtr->createTime = time.seconds;
    fdPtr->accessTime = 0;
    fdPtr->descModifyTime = time.seconds;
    fdPtr->dataModifyTime = time.seconds;

    /*
     * Place the data in the first filesystem block.
     */
    fdPtr->direct[0] = 0;
    bytePtr = GetBitmapPtr(domainPtr, bitmapPtr, 0);
    *bytePtr |= 0xf0;
    for (index = 1; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fdPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fdPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fdPtr->numKbytes = 4;
    fdPtr->version = 1;
    return SUCCESS;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * strnlen --
 *
 *	This is identical to strlen except that it will return N
 *	if the string length reaches N.
 *
 * Results:
 *	The return value is the number of characters in the
 *	string, not including the terminating zero byte.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
strnlen(string, numChars)
    register char *string;		/* String whose length is wanted. */
    int		   numChars;		/* Maximum number of chars to check. */
{
    register int result = -1;

    do {
	result += 1;
    } while (result < numChars && *string++ != 0);
    return(result);
}
