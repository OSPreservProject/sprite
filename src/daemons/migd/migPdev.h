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
 * $MigPdev: /sprite/lib/forms/RCS/proto.h,v 1.4 89/10/28 15:57:26 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _MIGPDEV
#define _MIGPDEV

extern int migPdev_Debug;

extern int MigPdev_OpenMaster();
extern void MigPdev_End();
extern void MigPdev_SignalClients();

#endif /* _MIGPDEV */
