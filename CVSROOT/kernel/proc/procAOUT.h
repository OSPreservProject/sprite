/*
 * procAOUT.h --
 *
 *	The a.out format for an object file.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _PROCAOUT
#define _PROCAOUT

#include "sprite.h"
#include "vm.h"

#define	NEW_PAGE_SIZE		0x2000
#define	NEW_SEG_SIZE		0x20000

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
} Proc_AOUT;

#define	PROC_OMAGIC	0407		/* Old impure format */
#define	PROC_NMAGIC	0410		/* Read-only text */
#define	PROC_ZMAGIC	0413		/* Demand load format */
#define PROC_MC68010	1		/* runs on '10 or '20 */
#define PROC_MC68020	2		/* runs on '20 only */

/*
 * Macros which take exec structures as arguments and tell whether
 * the file has a reasonable magic number or offsets to text|symbols|strings.
 */
#define	PROC_BAD_MAGIC_NUMBER(x) \
    (((x).magic)!=PROC_OMAGIC && ((x).magic)!=PROC_NMAGIC && ((x).magic)!=PROC_ZMAGIC)

#define	PROC_CODE_FILE_OFFSET(x) \
	((x).magic==PROC_ZMAGIC ? 0 : sizeof (Proc_AOUT))
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

#endif _PROCAOUT
