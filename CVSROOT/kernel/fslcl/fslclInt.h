/*
 * fslclInt.h --
 *
 *	Definitions of the parameters required for Local Domain operations.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSLOCALDOMAIN
#define _FSLOCALDOMAIN

#include "fscache.h"
#include "fsio.h"


/*
 * A directory entry:
 */
typedef struct FslclDirEntry {
    int fileNumber;		/* Index of the file descriptor for the file. */
    short recordLength;		/* How many bytes this directory entry is */
    short nameLength;		/* The length of the name in bytes */
    char fileName[FS_MAX_NAME_LENGTH+1];	/* The name itself */
} FslclDirEntry;
/*
 *	FSLCL_DIR_BLOCK_SIZE	Directory's grow in multiples of this constant,
 *		and records within a directory don't cross directory blocks.
 *	FSLCL_DIR_ENTRY_HEADER	The size of the header of a FslclDirEntry;
 *	FSLCL_REC_LEN_GRAIN	The number of bytes in a directory record
 *				are rounded up to a multiple of this constant.
 */
#define FSLCL_DIR_BLOCK_SIZE	512
#define FSLCL_DIR_ENTRY_HEADER	(sizeof(int) + 2 * sizeof(short))
#define FSLCL_REC_LEN_GRAIN	4

/*
 * FsDirRecLength --
 *	This computes the number of bytes needed for a directory entry.
 *	The argument should be the return of the String_Length function,
 *	ie, not include the terminating null in the count.
 */
#define FsDirRecLength(stringLength) \
    (FSLCL_DIR_ENTRY_HEADER + \
    ((stringLength / FSLCL_REC_LEN_GRAIN) + 1) * FSLCL_REC_LEN_GRAIN)
/*
 * Image of a new directory.
 */
extern char *fslclEmptyDirBlock;


/*
 * Declarations for the Local Domain lookup operations called via
 * the switch in Fsprefix_LookupOperation.  These are called with a pathname.
 */
ReturnStatus FslclExport();
ReturnStatus FslclOpen();
ReturnStatus FslclLookup();
ReturnStatus FslclGetAttrPath();
ReturnStatus FslclSetAttrPath();
ReturnStatus FslclMakeDevice();
ReturnStatus FslclMakeDir();
ReturnStatus FslclRemove();
ReturnStatus FslclRemoveDir();
ReturnStatus FslclRename();
ReturnStatus FslclHardLink();

/*
 * Declarations for the Local Domain attribute operations called via
 * the fsAttrOpsTable switch.  These are called with a fileID.
 */
ReturnStatus FslclGetAttr();
ReturnStatus FslclSetAttr();

extern void FslclAssignAttrs();

#endif /* _FSLOCALDOMAIN */
