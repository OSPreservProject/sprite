/*
 * devStdFBInt.h --
 *
 *	Declarations of the standard frame buffer device.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSTDFBINT
#define _DEVSTDFBINT

#include <fs.h>

extern	ReturnStatus	DevStdFBOpen _ARGS_ ((Fs_Device *devicePtr, 
				int useFlags, Fs_NotifyToken token,
				int *flagsPtr));
extern	ReturnStatus	DevStdFBMMap _ARGS_ ((Fs_Device *devicePtr, 
				Address startAddr, int length, int offset,
				Address *newAddrPtr));
extern	ReturnStatus	DevStdFBIOControl _ARGS_ ((Fs_Device *devicePtr, 
				Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern	ReturnStatus	DevStdFBClose _ARGS_ ((Fs_Device *devicePtr, 
				int useFlags, int openCount, int writerCount));

#endif /* _DEVSTDFBINT */

