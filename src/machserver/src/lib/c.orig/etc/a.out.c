/* 
 * a.out.c --
 *
 *	This file contains constants associated with a.out files,
 *	for example a table of page sizes for different machines.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/a.out.c,v 1.2 89/07/17 15:12:53 mgbaker Exp $ SPRITE (Berkeley)";
#endif not lint

/*
 * Page size for different machines.  These correspond to the machine
 * types defined in exec.h
 */

int Aout_PageSize[] = {
    0x0,			/* Undefined machine type. */
    0x1000,			/* M_68010 (Sun-2) */
    0x2000,			/* M_68020 (Sun-3) */
    0x2000			/* SPARC (Sun-4) */
};
