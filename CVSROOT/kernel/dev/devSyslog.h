/*
 * devSyslog.h --
 *
 *	Declarations of data and procedures for the system log device.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSYSLOG
#define _DEVSYSLOG

#include "user/fs.h"
#include "fs.h"

/*
 * Forward Declarations.
 */

extern ReturnStatus Dev_SyslogOpen _ARGS_((Fs_Device *devicePtr, int useFlags,
    Fs_NotifyToken token, int *flagsPtr));
extern ReturnStatus Dev_SyslogReopen _ARGS_((Fs_Device *devicePtr, int refs,
    int writers, Fs_NotifyToken token, int *flagsPtr));
extern ReturnStatus Dev_SyslogRead _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_SyslogWrite _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
extern void Dev_SyslogPutChar _ARGS_((int ch));
extern ReturnStatus Dev_SyslogClose _ARGS_((Fs_Device *devicePtr, int useFlags,
    int openCount, int writerCount));
extern ReturnStatus Dev_SyslogIOControl _ARGS_((Fs_Device *devicePtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Dev_SyslogSelect _ARGS_((Fs_Device *devicePtr,
    int *readPtr, int *writePtr, int *exceptPtr));
extern void Dev_SyslogDebug _ARGS_((Boolean stopLog));
extern void Dev_SyslogReturnBuffer _ARGS_((char **bufPtrPtr,
    int **firstIndexPtrPtr, int **lastIndexPtrPtr, int *bufSizePtr));

#endif /* _DEVSYSLOG */
