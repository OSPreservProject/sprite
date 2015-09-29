/* $Header: exec.h,v 2.1 88/06/13 17:45:43 hilfingr Exp $ */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 * Modifications copyright (c) 1987 by The Regents of the University of
 * California.
 */

#ifndef _EXEC_
#define _EXEC_

/*
 * format of the exec header
 * known by kernel and by user programs
 */
struct exec {
    unsigned long   a_magic;    /* magic number */
    unsigned long   a_bytord;   /* byte order indicator */
    unsigned long   a_text;     /* size of text segment */
    unsigned long   a_data;     /* size of private initialized data */
    unsigned long   a_sdata;    /* size of shared initialized data */
    unsigned long   a_bss;      /* size of private uninitialized data */
    unsigned long   a_sbss;     /* size of shared uninitialized data */
    unsigned long   a_syms;     /* size of symbol table */
    unsigned long   a_entry;    /* entry point (memory address of */
                                /* starting address for execution) */
    unsigned long   a_rsize;    /* size of relocation area */
    unsigned long   a_expsize;  /* size of load-time expressions */
    unsigned long   a_padding;
};

#define	ZMAGIC	0513		/* demand load format. Read-only text */
#define OMAGIC	0507		/* impure format: also used for */
			        /* non-executable load files. */

#endif _EXEC_
