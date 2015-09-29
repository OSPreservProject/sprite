/*
 * devSmem.h --
 *
 *	Declarations of  procedures for the /dev/smem device.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header: /sprite/src/kernel/Cvsroot/kernel/dev/sun4c.md/devSmem.h,v 1.1 92/08/13 15:54:02 secor Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVSMEM
#define _DEVSMEM

/*
 * Forward Declarations.
 */

extern ReturnStatus Dev_SmemRead _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_SmemWrite _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_SmemIOControl _ARGS_((Fs_Device *devicePtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_SmemSelect _ARGS_((Fs_Device *devicePtr, int *readPtr,
    int *writePtr, int *exceptPtr));

#endif /* _DEVSMEM */
