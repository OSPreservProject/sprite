/*
 * rawBlockDev.h --
 *
 *	Declarations of routines for raw mode access to block devices.
 *	This routines should be accessed thru the file system device
 *	switch.
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

#ifndef _RAWBLOCKDEV
#define _RAWBLOCKDEV

#include "user/fs.h"
#include "fs.h"

extern ReturnStatus DevRawBlockDevOpen _ARGS_((Fs_Device *devicePtr,
    int useFlags, Fs_NotifyToken token, int *flagsPtr));
extern ReturnStatus DevRawBlockDevReopen _ARGS_((Fs_Device *devicePtr,
    int refs, int writers, Fs_NotifyToken token, int *flagsPtr));
extern ReturnStatus DevRawBlockDevRead _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
extern ReturnStatus DevRawBlockDevWrite _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
extern ReturnStatus DevRawBlockDevClose _ARGS_((Fs_Device *devicePtr,
    int useFlags, int openCount, int writerCount));
extern ReturnStatus DevRawBlockDevIOControl _ARGS_((Fs_Device *devicePtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));

#endif /* _RAWBLOCKDEV */

