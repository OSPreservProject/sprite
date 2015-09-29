/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)exec.h	7.1 (Berkeley) 6/4/86
 */

#ifndef _EXEC
#define _EXEC

/*
 * Header prepended to each a.out file.
 */
struct exec {
    unsigned int        a_dynamic       : 1;
    unsigned int        a_toolversion   : 7;
    unsigned char	a_machtype;	/* machine type */
    unsigned short	a_magic;	/* magic number */
    unsigned long	a_text;		/* size of text segment */
    unsigned long	a_data;		/* size of initialized data */
    unsigned long	a_bss;		/* size of uninitialized data */
    unsigned long	a_syms;		/* size of symbol table */
    unsigned long	a_entry;	/* entry point */
    unsigned long	a_trsize;	/* size of text relocation */
    unsigned long	a_drsize;	/* size of data relocation */
};

/*
 * Magic numbers currently defined:
 */

#define	OMAGIC	0407		/* old impure format */
#define	NMAGIC	0410		/* read-only text */
#define	ZMAGIC	0413		/* demand load format */

#define	SPRITE_ZMAGIC	0414		/* Uses Sprite pmegs */
#define	UNIX_ZMAGIC	0415		/* Unix binary compatible */

/*
 * Machine types currently defined.  Warning:  if you add new machine
 * types or change old ones, be sure to update the information in the
 * library file a.out.c.
 */

#define M_68010		1
#define M_68020		2
#define M_SPARC         3

#endif /* _EXEC */
