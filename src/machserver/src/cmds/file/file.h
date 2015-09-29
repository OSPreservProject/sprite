/*
 * file.h --
 *
 *	Declarations for the file program. To port this program to a new
 *	machine you have to define HOST_FMT to be the format 
 *	of the new host. Look at fmt.h for supported host formats.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/attcmds/file/RCS/file.h,v 1.5 90/10/19 15:25:43 jhh Exp Locker: shirriff $ 
 */

#ifdef KERNEL
#ifndef NULL
#define NULL 0
#endif
#ifndef BUFSIZ
#define BUFSIZ 4096
#endif
#else
#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#endif
#include <fmt.h>

#define const

#define HEADERSIZE	BUFSIZ

/*
 * A rather awkward way of determining the host machine type. There should
 * be one entry for each machine this program runs on.
 */
#if (defined(sun2) || defined(sun3))
#define HOST_FMT FMT_68K_FORMAT
#else
#if defined(sun4)
#define HOST_FMT FMT_SPARC_FORMAT
#else
#if defined(ds3100)
#define HOST_FMT FMT_MIPS_FORMAT
#else
#if defined(symm)
#define HOST_FMT FMT_SYM_FORMAT
#else
#define HOST_FMT FMT_68K_FORMAT
#endif /* symm */
#endif /* ds3100 */
#endif /* sun4 */
#endif /* sun2/sun3 */

extern int hostFmt;

/*
 * Routines to print out size of object file.
 */
#ifdef __STDC__
extern char *machType68k(int bsize, char *buf, int *m, int *sym,
	char **other);
extern char *machTypeSparc(int bsize, char *buf, int *m, int *sym,
	char **other);
extern char *machTypeMips(int bsize, char *buf, int *m, int *sym,
	char **other);
extern char *machTypeSymm(int bsize, char *buf, int *m, int *sym,
	char **other);
#else
extern char *machType68k();
extern char *machTypeSparc();
extern char *machTypeMips();
extern char *machTypeSymm();
#endif
