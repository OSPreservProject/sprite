/*
 * devfb.h --
 *
 *	Declarations of local stuff for frame buffer device.
 *
 * Copyright 1990 Regents of the University of California
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

#ifndef _DEVFB
#define _DEVFB
#include <sys/fb.h>

/* procedures */

extern ReturnStatus DevFBOpen _ARGS_((Fs_Device *devicePtr, int useFlags,
    Fs_NotifyToken token, int *flagsPtr));
extern ReturnStatus DevFBIOControl _ARGS_((Fs_Device *devicePtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus DevFBClose _ARGS_((Fs_Device *devicePtr, int useFlags,
    int openCount, int writerCount));
extern ReturnStatus DevFBMMap _ARGS_((Fs_Device *devicePtr,
    Address startAddr, int length, int offset, Address *newAddrPtr));

typedef fbtype  FBType;
typedef fbinfo  FBInfo;
typedef fbcmap  FBCMap;
typedef fbsattr FBSAttr;
typedef fbgattr FBGAttr;

#endif /* _DEVFB */
