/*
 * devDiskStats.h --
 *
 *	Declarations of routines for collecting statistics on Sprite disk 
 *	usage.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DISKSTATS
#define _DISKSTATS


/* procedures */

extern Sys_DiskStats *DevRegisterDisk();
extern void Dev_GatherDiskStats();
extern int Dev_GetDiskStats();

#endif /* _DISKSTATS */

