/*
 * fsLocalDomain.h --
 *
 *	Definitions of the parameters required for Local Domain operations.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSLCL
#define _FSLCL

#include "fscache.h"
#include "fsio.h"


/*
 * A directory entry:  Note that this is compatible with 4.3BSD 'struct direct'
 */
typedef struct Fslcl_DirEntry {
    int fileNumber;		/* Index of the file descriptor for the file. */
    short recordLength;		/* How many bytes this directory entry is */
    short nameLength;		/* The length of the name in bytes */
    char fileName[FS_MAX_NAME_LENGTH+1];	/* The name itself */
} Fslcl_DirEntry;
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
 * Fslcl_DirRecLength --
 *	This computes the number of bytes needed for a directory entry.
 *	The argument should be the return of the String_Length function,
 *	ie, not include the terminating null in the count.
 */
#define Fslcl_DirRecLength(stringLength) \
    (FSLCL_DIR_ENTRY_HEADER + \
    ((stringLength / FSLCL_REC_LEN_GRAIN) + 1) * FSLCL_REC_LEN_GRAIN)

/*
 * Misc. routines.
 */
extern void	     Fslcl_DomainInit();
extern ReturnStatus  Fslcl_DeleteFileDesc();
extern void Fslcl_NameInitializeOps();



#endif /* _FSLCL */
