/*
 * procAOUT.h --
 *
 *	The a.out format for an object file for SPUR. This file is based on
 * a.out.h and sys/exec.h for SPUR.
 *
 * Copyright (C)  1988 Regents of the University of California
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

#ifndef _PROCAOUT
#define _PROCAOUT

#include "sprite.h"
#include "vm.h"

#define	NEW_PAGE_SIZE		0x1000
#define	NEW_SEG_SIZE		0x20000

/*
 * Header prepended to each a.out file. Taken from sys/exec.h
 */

typedef struct {
    unsigned long 	magic;		/* magic number */
    unsigned long	bytord;		/* byte order indicator. */
    unsigned long	code;		/* Size of code segment */
    unsigned long	data;		/* Size of initialized data */
    unsigned long	sdata;		/* Size of shared initialized data */
    unsigned long	bss;		/* Size of uninitialized data */
    unsigned long	sbss;		/* Size of shared uninitialized data */
    unsigned long	syms;		/* Size of symbol table */
    unsigned long	entry;		/* Entry point */
    unsigned long	rsize;		/* Size of relocation area */
    unsigned long	expsize;	/* Size of load-time expressions */
    unsigned long	a_padding;	
} Proc_AOUT;

#define	PROC_OMAGIC	0507		/* impure format */
#define	PROC_ZMAGIC	0513		/* Demand load format */

/*
 * Macros which take exec structures as arguments and tell whether
 * the file has a reasonable magic number or offsets to text|symbols|strings.
 */
#define	PROC_BAD_MAGIC_NUMBER(x) \
    (((x).magic)!=PROC_ZMAGIC)

#define	PROC_CODE_FILE_OFFSET(x) \
	((x).magic==PROC_ZMAGIC ? 0 : sizeof (Proc_AOUT))
#define	PROC_DATA_FILE_OFFSET(x) \
	(PROC_CODE_FILE_OFFSET(x) + (x).code)

/*
 * Macros which take exec structures as arguments and tell where the
 * various pieces will be loaded.
 */
#define PROC_CODE_LOAD_ADDR(x) 0x40000000
#define PROC_DATA_LOAD_ADDR(x) \
	(((x).magic==PROC_OMAGIC)? (PROC_CODE_LOAD_ADDR(x)+(x).code) \
	: (0xc0000000))
#define PROC_BSS_LOAD_ADDR(x)  (PROC_DATA_LOAD_ADDR(x)+(x).data)

#endif _PROCAOUT
