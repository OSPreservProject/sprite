/* 
 * fscheck.h
 *
 *	Types for the file system check program.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 * $Header: /sprite/src/cmds/fscheck/RCS/fscheck.h,v 1.17 90/10/10 15:29:07 mendel Exp $ SPRITE (Berkeley)
 */

#ifndef _FSCHECK
#define _FSCHECK

#include "disk.h"
#include <sys/types.h>
#include <varargs.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

/*
 *  fscheck Proc_Exit status codes.
 * 	Codes < 0 are hard errors. Codes > 0 are soft errors.
 *
 */
#define EXIT_OK		(char) 0

#define EXIT_SOFT_ERROR		(char) 1
#define EXIT_OUT_OF_MEMORY	(char) 2
#define EXIT_NOREBOOT		(char) 3
#define EXIT_REBOOT		(char) 4

#define EXIT_HARD_ERROR		(char) -1
#define EXIT_READ_FAILURE	(char) -2
#define EXIT_WRITE_FAILURE	(char) -3
#define EXIT_BAD_ARG		(char) -4
#define EXIT_MORE_MEMORY	(char) -5
#define EXIT_DISK_FULL		(char) -6

/*
 * Structure kept for each file descriptor.
 */
typedef struct FdInfo {
    short	flags;		/* Flags defined below. */
    short	origLinkCount;	/* Link count from the file descriptor. */
    short	newLinkCount;	/* Link computed by checkFS. */
} FdInfo;

/*
 * Flag values.
 *
 *	IS_A_DIRECTORY	This file is a directory.
 *	FD_REFERENCED	This file is referenced by a directory.
 *	FD_MODIFIED	This file descriptor has been modified.
 *	FD_SEEN		This file descriptor has already been checked.
 *	FD_ALLOCATED	This file descriptor is allocated.	
 *	ON_MOD_LIST	This file descriptor is on the modified list.
 *	FD_UNREADABLE	This file descriptor is in an unreadable sector.
 *	FD_RELOCATE	This file descriptor is in a readable sector in
 *			an unreadable block and is being relocated.
 */
#define	IS_A_DIRECTORY	0x01
#define	FD_REFERENCED	0x02
#define	FD_MODIFIED	0x04
#define	FD_SEEN		0x08
#define	FD_ALLOCATED	0x10
#define	ON_MOD_LIST	0x20
#define	FD_UNREADABLE	0x40
#define	FD_RELOCATE	0x80

/*
 * Structure for each element of the list of modified file descriptors.
 */
typedef struct ModListElement {
    List_Links		links;
    int			fdNum;
    Fsdm_FileDescriptor	*fdPtr;
} ModListElement;
extern	List_Links	modListHdr;
#define	modList &modListHdr

/*
 * Structure for each element of the list of relocating file descriptors.
 * Note that it looks like a ModListElement with an extra file descriptor
 * number at the end.  It can be inserted into the modList and written
 * in location newFdNum once newFdNum has been assigned.
 */
typedef struct RelocListElement {
    List_Links		links;
    int			newFdNum;
    Fsdm_FileDescriptor	*fdPtr;
    int			origFdNum;
} RelocListElement;
extern	List_Links	relocListHdr;
#define	relocList &relocListHdr

typedef enum {
    DIRECT, 
    INDIRECT, 
    DBL_INDIRECT
} BlockIndexType;

typedef enum {
    FD, 
    BLOCK, 
} ParentType;

/*
 * Structure for each element of the list of file blocks that need to be copied.
 */

typedef struct CopyListElement {
    List_Links		links;
    BlockIndexType	blockType;
    short		fragments;
    int			index;
    ParentType		parentType;
    int			parentNum;
    Fsdm_FileDescriptor	*fdPtr;
} CopyListElement;

extern	List_Links	copyListHdr;
#define	copyList &copyListHdr


/*
 * Structure to contain the current state about a block index.
 */


typedef struct DirIndexInfo {
    FdInfo	     *fdInfoPtr;	     /* Status info about the file 
					      * descriptor being read. */
    Fsdm_FileDescriptor *fdPtr;	     	     /* The file descriptor being
						read. */
    BlockIndexType indexType;		     /* Whether chasing direct, 
						indirect, or doubly indirect 
						blocks. */
    int		 blockNum;		     /* Block that is being read, 
						written, or allocated. */
    int		 blockAddr;		     /* Address of directory block
						to read. */
    int		 dirOffset;		     /* Offset of the directory entry 
						that we are currently examining 
						in the directory. */
    char	 dirBlock[FS_BLOCK_SIZE];    /* Where directory data is 
						stored. */
    int		 numFrags;		     /* Number of fragments stored in
						the directory entry. */
    int		 firstIndex;		     /* An index into either the direct
					        block pointers or into an 
					        indirect block. */
    int		 secondIndex;		     /* An index into a doubly indirect
					        block. */
    char 	 firstBlock[FS_BLOCK_SIZE];  /* First level indirect block. */
    int		 firstBlockNil;		     /* The first level block is 
						empty.*/
    char 	 secondBlock[FS_BLOCK_SIZE]; /* Second level indirect block. */
    int		 secondBlockNil;	     /* The second level block 
						is empty.*/
    int		 dirDirty;		     /* 1 if the directory block is
						dirty. */
} DirIndexInfo;

extern int	numBlocks;
extern int	numFiles;
extern int	numBadDesc;
extern int	numFrags;
extern int	foundError;
extern int	errorType;
extern int	fdBitmapError;
extern Boolean	tooBig;
extern int	noCopy;
extern int	debug;
extern char	end[];
extern FILE 	*outputFile;
extern int	patchHeader;
extern int	writeDisk;
extern int	verbose;
extern int	silent;
extern int	clearDomainNumber;
extern int	recoveryCheck;
extern int	badBlockInit;
extern int	patchRoot;
extern int	flushBuffer;
extern int	maxHeapSize;
extern int	bufferSize;
extern int	heapSize;
extern int	lastErrorFD;
extern int	num1KBlocks;
extern int	bytesPerCylinder;
extern List_Links    modListHdr;
extern List_Links    relocListHdr;
extern unsigned char *fdBitmapPtr;
extern unsigned char *cylBitmapPtr;
extern unsigned char bitmasks[];
extern int	rawOutput;
extern int	attached;
extern int	outputFileNum;
extern char	*outputFileName;
extern Ofs_DomainHeader  *domainPtr;
extern int		partFID;
extern int	bitmapVerbose;
extern int	fixCount;
extern int	numReboot;
extern int	clearFixCount;
extern int	blockToFind;
extern int	fileToPrint;
extern int	setCheckedBit;

extern int Output();	
extern void OutputPerror();	
extern void WriteOutputFile();	
extern int CloseOutputFile();	
extern void ExitHandler();
extern char *sbrk();
extern void ClearFd();
extern void UnmarkBitmap();
extern int MarkBitmap();
extern void AddToCopyList();

/*
 * Macro to get a pointer into the bit map for a particular block.
 */
#define BlockToCylinder(domainPtr, blockNum) \
    (blockNum) / (domainPtr)->geometry.blocksPerCylinder

#define GetBitmapPtr(domainPtr, bitmapPtr, blockNum) \
  &((bitmapPtr)[BlockToCylinder(domainPtr, blockNum) * \
  bytesPerCylinder + (blockNum) % (domainPtr)->geometry.blocksPerCylinder / 2])

/*
 * Macros to convirt physical block numbers to virtual block numbers. All direct
 * blocks are virtual, indirect blocks are physical.
 */
#define VirtToPhys(domainPtr,blockNum) \
    ((blockNum) + (domainPtr)->dataOffset * FS_FRAGMENTS_PER_BLOCK)

#define PhysToVirt(domainPtr,blockNum) \
    ((blockNum) - (domainPtr)->dataOffset * FS_FRAGMENTS_PER_BLOCK)

#define MarkFDBitmap(num,bitmapPtr) \
    (bitmapPtr)[(num) >> 3] |= (1 << (7 -((num)  & 7))) 

#define UnmarkFDBitmap(num,bitmapPtr) \
    (bitmapPtr)[(num) >> 3] &= ~(1 << (7 -((num)  & 7))) 

/*
 * Number of file descriptors in a sector, if we have to go through
 * the sectors individually.
 */
#define FILE_DESC_PER_SECTOR (FSDM_FILE_DESC_PER_BLOCK / DISK_SECTORS_PER_BLOCK)



#define Alloc(ptr,type,number) AllocByte((ptr),type,sizeof(type) * (number))

#define AllocByte(ptr,type,numBytes) { \
	int oldHeapSize = heapSize; \
	(ptr) = (type *) malloc((unsigned) (numBytes)); \
	heapSize = (int) (sbrk(0) - (char *) end); \
	if ((heapSize != oldHeapSize) && (debug)) {\
	    Output(stderr,"Heapsize now %d.\n",heapSize); \
	}\
	if ((maxHeapSize > 0) && (heapSize > maxHeapSize) && (!tooBig)) { \
	    tooBig = TRUE; \
	    Output(stderr,"Heap limit exceeded.\n"); \
	    (ptr) = NULL; \
	    foundError = 1;\
	    errorType = EXIT_OUT_OF_MEMORY;\
	} }

#define min(a,b) (((a) < (b)) ? (a) : (b) )

#endif _FSCHECK
