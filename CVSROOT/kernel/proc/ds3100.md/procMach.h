/*
 * procAOUT.h --
 *
 *	The a.out format for an object file.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _PROCMACH
#define _PROCMACH

#include "sprite.h"

/*
 * File header magic number.
 */
#define	PROC_OBJ_MAGIC	0x0162

/*
 * A.out header magic number.
 */
#define	PROC_OMAGIC	0407		/* Impure format */
#define	PROC_NMAGIC	0410		/* Shared text format */
#define	PROC_ZMAGIC	0413		/* Demand load format */
#define	PROC_LIBMAGIC	0443		/* Share library format */

/*
 * Description of the file.
 */
typedef struct {
    unsigned short	magic;		/* The magic number. */
    unsigned short	numSections;	/* The number of sections. */
    long		timeDateStamp;	/* Time and date stamp. */		
    long		symPtr;		/* File pointer to symbolic header. */	
    long		numSyms;	/* Size of symbolic header. */
    unsigned short	optHeader;	/* Size of optional header. */
    unsigned short	flags;		/* Flags. */
} ProcFileHeader;

/*
 * A.out header.
 */
typedef struct {
    short		magic;		/* Magic number. */
    short		verStamp;	/* Version stamp. */
    long		codeSize;	/* Code size in bytes. */
    long		heapSize;	/* Initialized data size in bytes. */
    long		bssSize;	/* Uninitialized data size in bytes. */
    Address		entry;		/* Entry point. */
    Address		codeStart;	/* Base of code used for this file. */
    Address		heapStart;	/* Base of heap used for this file. */
    Address		bssStart;	/* Base of bss used for this file. */
    long		gprMask;	/* General purpose register mask. */
    long		cprMask[4];	/* Co-processor register masks. */
    long		gpValue;	/* The gp value for this object. */
} ProcAOUTHeader;

/*
 * Section header.
 */
typedef struct {
    char		name[8];	/* Section name. */
    long		physAddr;	/* Section physical address. */
    long		virtAddr;	/* Section virtual address. */
    long		size;		/* Section size. */
    long		sectionPtr;	/* File pointer to section data.  */
    long		relocPtr;	/* File pointer to relocation data.  */
    long		lnnoPtr;	/* File pointer to gp tables. */
    unsigned short	numReloc;	/* Number of relocation entries. */
    unsigned short	numLnno;	/* Numberof gp tables. */
    long		flags;		/* Section flags. */
} ProcSectionHeader;

/*
 * The header at the beginning of each file.
 */
typedef struct {
    ProcFileHeader	fileHeader;
    ProcAOUTHeader	aoutHeader;
} ProcExecHeader;

/*
 * Default place for a text segment.
 */
#define DEFAULT_TEXT	0x00400000

#endif /* _PROCMACH */
