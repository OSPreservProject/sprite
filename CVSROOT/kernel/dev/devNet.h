/*
 * devNet.h --
 *
 *	This defines the interface to the file system net device.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVNET
#define _DEVNET

#include "sprite.h"
#include "user/fs.h"
#include "fs.h"

/*
 * Forward routines.
 */

extern ReturnStatus DevNet_FsOpen _ARGS_((Fs_Device *devicePtr, int useFlags,
    Fs_NotifyToken data, int *flagsPtr));
extern ReturnStatus DevNet_FsReopen _ARGS_((Fs_Device *devicePtr, int refs,
    int writers, Fs_NotifyToken data, int *flagsPtr));
extern ReturnStatus DevNet_FsRead _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
extern ReturnStatus DevNet_FsWrite _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
extern ReturnStatus DevNet_FsClose _ARGS_((Fs_Device *devicePtr, int useFlags,
    int openCount, int writerCount));
extern ReturnStatus DevNet_FsSelect _ARGS_((Fs_Device *devicePtr, int *readPtr,
    int *writePtr, int *exceptPtr));
extern ReturnStatus DevNet_FsIOControl _ARGS_((Fs_Device *devicePtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));

extern void DevNetEtherHandler _ARGS_((Address packetPtr, int size));

#endif /* _DEVNET */

