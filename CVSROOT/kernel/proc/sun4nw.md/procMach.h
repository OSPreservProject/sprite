/*
 * procMach.h --
 *
 *	The a.out format for an object file.
 *
 * Copyright (C) 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _PROCMACH
#define _PROCMACH

#include "sprite.h"

#define	NEW_PAGE_SIZE		0x2000
#ifdef sun4
#define	NEW_SEG_SIZE		0x40000
#else
#define	NEW_SEG_SIZE		0x20000
#endif

/*
 * Header prepended to each a.out file.
 */

typedef struct {
    unsigned short 	machineType;	/* machine type */
    unsigned short 	magic;		/* magic number */
    unsigned long	code;		/* Size of code segment */
    unsigned long	data;		/* Size of initialized data */
    unsigned long	bss;		/* Size of uninitialized data */
    unsigned long	syms;		/* Size of symbol table */
    unsigned long	entry;		/* Entry point */
    unsigned long	trsize;		/* Size of text relocation */
    unsigned long	drsize;		/* Size of data relocation */
} ProcExecHeader;

#define	PROC_OMAGIC	0407		/* Old impure format */
#define	PROC_NMAGIC	0410		/* Read-only text */
#define	PROC_ZMAGIC	0413		/* Demand load format */
#define PROC_MC68010	1		/* runs on '10 or '20 */
#define PROC_MC68020	2		/* runs on '20 only */
#define	PROC_SPARC	3		/* runs on SPARC only */

/*
 * Macros which take exec structures as arguments and tell whether
 * the file has a reasonable magic number or offsets to text|symbols|strings.
 */
#define	PROC_BAD_MAGIC_NUMBER(x) \
    (((x).magic)!=PROC_ZMAGIC)

#define	PROC_CODE_FILE_OFFSET(x) \
	((x).magic==PROC_ZMAGIC ? 0 : sizeof (ProcExecHeader))
#define	PROC_DATA_FILE_OFFSET(x) \
	(PROC_CODE_FILE_OFFSET(x) + (x).code)

/*
 * Macros which take exec structures as arguments and tell where the
 * various pieces will be loaded.
 */
#define PROC_CODE_LOAD_ADDR(x) NEW_PAGE_SIZE
#define PROC_DATA_LOAD_ADDR(x) \
	(((x).magic==PROC_OMAGIC)? (PROC_CODE_LOAD_ADDR(x)+(x).code) \
	: (NEW_SEG_SIZE+((PROC_CODE_LOAD_ADDR(x)+(x).code-1) & ~(NEW_SEG_SIZE-1))))
#define PROC_BSS_LOAD_ADDR(x)  (PROC_DATA_LOAD_ADDR(x)+(x).data)


#endif /* _PROCMACH */
