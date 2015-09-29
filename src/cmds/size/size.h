/*
 * size.h --
 *
 *	Declarations for the size program. To port this program to a new
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
 * $Header: /a/newcmds/size/RCS/size.h,v 1.3 89/07/26 23:45:34 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _SIZE
#define _SIZE

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <fmt.h>


#define HEADERSIZE	100

/*
 * A rather awkward way of determining the host machine type. There should
 * be one entry for each machine this program runs on.
 */
#if (defined(sun2) || defined(sun3))
#define HOST_FMT FMT_68K_FORMAT
#elif defined(spur)
#define HOST_FMT FMT_SPUR_FORMAT
#elif defined(ds3100)
#define HOST_FMT FMT_MIPS_FORMAT
#else
#define HOST_FMT FMT_68K_FORMAT
#endif

extern int hostFmt;

/*
 * Routines to print out size of object file.
 */
extern ReturnStatus	Print68k();
extern ReturnStatus	PrintSpur();
extern ReturnStatus     PrintMips();

#endif /* _SIZE */

