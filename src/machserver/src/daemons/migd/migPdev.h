/*
 * migPdev.h --
 *
 *	Declarations of functions and variables for the migration
 *	daemon pseudo-device management.
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
 * $Header: /user5/kupfer/spriteserver/src/daemons/migd/RCS/migPdev.h,v 1.3 92/04/29 22:30:59 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _MIGPDEV
#define _MIGPDEV

extern int migPdev_Debug;

extern void MigPdev_CloseAllStreams();
extern void MigPdev_Init();
extern int MigPdev_OpenMaster();
extern void MigPdev_End();
extern void MigPdev_SignalClients();

#endif /* _MIGPDEV */
