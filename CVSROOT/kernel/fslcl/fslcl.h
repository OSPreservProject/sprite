/*
 * fslcl.h --
 *
 *	Definitions of the parameters required for Local Domain operations.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSLCL
#define _FSLCL

#include <fscache.h>
#include <fsio.h>
#include <fsconsist.h>
#include <fsioFile.h>


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
extern void Fslcl_DomainInit _ARGS_((void));
extern ReturnStatus Fslcl_DeleteFileDesc _ARGS_((Fsio_FileIOHandle *handlePtr));
extern void Fslcl_NameInitializeOps _ARGS_((void));
extern void Fslcl_NameHashInit _ARGS_((void));

#endif /* _FSLCL */
