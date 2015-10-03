/*
 * devNull.h --
 *
 *	Declarations of  procedures for the /dev/null device.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVNULL
#define _DEVNULL

/*
 * Forward Declarations.
 */

/*  extern ReturnStatus Dev_NullOpen(); */
/*  extern ReturnStatus Dev_NullClose(); */

extern ReturnStatus Dev_NullRead _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_NullWrite _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_NullIOControl _ARGS_((Fs_Device *devicePtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_NullSelect _ARGS_((Fs_Device *devicePtr, int *readPtr,
    int *writePtr, int *exceptPtr));

#endif /* _DEVNULL */
