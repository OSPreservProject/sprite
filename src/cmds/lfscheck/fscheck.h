/* 
 * fscheck.h
 *
 *	Types for the file system check program.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 * $Header: /user2/mendel/lfs/src/cmds/fscheck/RCS/fscheck.h,v 1.17 90/10/10 15:29:07 mendel Exp Locker: mendel $ SPRITE (Berkeley)
 */

#ifndef _FSCHECK
#define _FSCHECK

#ifdef __STDC__
#define _HAS_PROTOTYPES
#endif
#include <cfuncproto.h>
#include <disk.h>
#include <sys/types.h>
#include <varargs.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <alloca.h>
#include <bstring.h>

#include <kernel/lfsDesc.h>
#include <kernel/lfsDescMap.h>
#include <kernel/lfsFileLayout.h>
#include <kernel/lfsDirOpLog.h>
#include <kernel/lfsSegLayout.h>
#include <kernel/lfsStableMem.h>
#include <kernel/lfsSuperBlock.h>
#include <kernel/lfsUsageArray.h>
#include <kernel/lfsStats.h>

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

extern int	foundError;
extern Boolean	tooBig;
extern int	debug;
extern int	writeDisk;
extern int	verbose;
extern	FdInfo		*descInfoArray;
extern int DiskRead _ARGS_((int diskFd, int blockOffset, int bufferSize, 
			char *bufferPtr));
extern char *GetStableMemEntry _ARGS_((ClientData clientData, int entryNumber));

extern int FetchFileDesc _ARGS_((int fdNum, Fsdm_FileDescriptor **fdPtrPtr));
extern void StoreFileDesc _ARGS_((int fdNum, Fsdm_FileDescriptor *fdPtr));
extern void CheckDirTree _ARGS_((int diskFd));
extern void CheckDirEntry _ARGS_((int entryNum, register DirIndexInfo *dirIndexPtr, register Fslcl_DirEntry *dirEntryPtr));
extern ReturnStatus MakeRoot _ARGS_((Ofs_DomainHeader *domainPtr, u_char *bitmapPtr, Fsdm_FileDescriptor *fdPtr));
extern int strnlen _ARGS_((register char *string, int numChars));
extern int MarkBitmap _ARGS_((int fdNum, int blockNum, unsigned char *bitmapPtr, int numFrags, Ofs_DomainHeader *domainPtr));
extern int Output _ARGS_((int __builtin_va_alist));
extern void OutputPerror _ARGS_((int __builtin_va_alist));
extern void WriteOutputFile _ARGS_((FILE *stream, int flush));
extern int CloseOutputFile _ARGS_((FILE *stream));
extern void ExitHandler _ARGS_((void));
extern void ClearFd _ARGS_((int flags, Fsdm_FileDescriptor *fdPtr));
extern void AddToCopyList _ARGS_((ParentType parentType, Fsdm_FileDescriptor *fdPtr, int fdNum, int blockNum, int index, BlockIndexType blockType, int fragments, Boolean *copyUsedPtr));
extern int main _ARGS_((int argc, char *argv[]));
extern void CheckFilesystem _ARGS_((int firstPartFID, int partFID, int partition));
extern void CheckBlocks _ARGS_((int partFID, Ofs_DomainHeader *domainPtr, int fdNum, Fsdm_FileDescriptor *fdPtr, unsigned char *newCylBitmapPtr, int *modifiedPtr, Boolean *copyUsedPtr));
extern int ProcessIndirectBlock _ARGS_((int fdNum, int partFID, int lastBlock, Boolean duplicate, register Fsdm_FileDescriptor *fdPtr, int *blockNumPtr, unsigned char *newCylBitmapPtr, int *fileBlockNumPtr, int *dirtyPtr, int *lastRealBlockPtr, int *modifiedPtr, int *blockCountPtr, Boolean *copyUsedPtr));
extern void RelocateFD _ARGS_((register Ofs_DomainHeader *domainPtr, FdInfo *descInfoPtr, RelocListElement *relocPtr));
extern int CopyBlock _ARGS_((register Ofs_DomainHeader *domainPtr, FdInfo *descInfoPtr, int partFID, u_char *bitmapPtr, CopyListElement *copyPtr));
extern int FillNewBlock _ARGS_((int blockNum, BlockIndexType blockType, int fragments, Fsdm_FileDescriptor *fdPtr, Ofs_DomainHeader *domainPtr, FdInfo *descInfoPtr, int partFID, u_char *bitmapPtr, int *newBlockNumPtr));
extern Boolean LoadUsageArray _ARGS_((int diskFd, int checkPointSize, char *checkPointPtr));
extern Boolean LoadDescMap _ARGS_((int diskFd, int checkPointSize, char *checkPointPtr));
extern char *GetUsageState _ARGS_((LfsSegUsageEntry *entryPtr));
extern int DiskRead _ARGS_((int diskFd, int blockOffset, int bufferSize, char *bufferPtr));
extern char *GetStableMemEntry _ARGS_((ClientData clientData, int entryNumber));

extern char *sbrk();



#define Alloc(ptr,type,number) AllocByte((ptr),type,sizeof(type) * (number))

#define AllocByte(ptr,type,numBytes) { \
	(ptr) = (type *) malloc((unsigned) (numBytes)); }

#define min(a,b) (((a) < (b)) ? (a) : (b) )

extern LfsSuperBlock	*superBlockPtr;
extern ClientData descMapDataPtr;	    /* Descriptor map of file system. */
extern LfsDescMapCheckPoint *descMapCheckPointPtr; /* Most current descriptor map 
					     * check point. */

ClientData   usageArrayDataPtr;	/* Data of usage map. */

/*
 * BlockToSegmentNum() - Macro to convert from file system block numbers to 
 * 		     	 segment numbers.
 */
#define	BlockToSegmentNum(block) (((block)-superBlockPtr->hdr.logStartOffset)/\
			(superBlockPtr->usageArray.segmentSize/blockSize))

/*
 *	DescMapEntry() - Return the descriptor map entry for a file number.
 *	UsageArrayEntry() - Return an usage array entry for a segment number.
 *	DescMapBlockIndex() - Return the block index of a desc map block.
 *	UsageArrayBlockIndex() - Return the block index of a usage array block.
 *	
 */

#define	DescMapEntry(fileNumber) ((LfsDescMapEntry *) GetStableMemEntry(descMapDataPtr, fileNumber))
#define	UsageArrayEntry(segNo) ((LfsSegUsageEntry *) GetStableMemEntry(usageArrayDataPtr, segNo))

#define DescMapBlockIndex(blockNum) GetStableMemBlockIndex(descMapDataPtr, blockNum)
#define UsageArrayBlockIndex(blockNum) GetStableMemBlockIndex(usageArrayDataPtr, blockNum)

#endif _FSCHECK
