/*
 * miscutil.h --
 *
 *	Declarations of ...
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
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/raid/miscutil.h,v 1.1 92/06/25 17:20:49 eklee Exp $ SPRITE (Berkeley)
 */

#ifndef _MISCUTIL
#define _MISCUTIL

#define RoundUp(size, modulus)	((((size)+(modulus)-1)/(modulus))*(modulus))

typedef struct {
    Sync_Semaphore	mutex;
    Sync_Condition	wait;
    int			numIO;
    ReturnStatus	status;
} SyncControl;

typedef struct {
    Sync_Semaphore	mutex;
    Sync_Condition	wait;
    int			numIO;
    ReturnStatus	status;
    void		(*doneProc)();
    int			clientData;
} AsyncControl;


extern void Raid_InitSyncControl _ARGS_((SyncControl *syncControlPtr));
extern void Raid_StartSyncIO _ARGS_((SyncControl *syncControlPtr));
extern ReturnStatus Raid_WaitSyncIO _ARGS_((SyncControl *syncControlPtr));
extern int Raid_SyncDoneProc _ARGS_((SyncControl *syncControlPtr, ReturnStatus status));
extern ReturnStatus Raid_DevReadSync _ARGS_((DevBlockDeviceHandle *devHandlePtr, int devOffset, char *buffer, int bufSize));
extern ReturnStatus Raid_DevWriteSync _ARGS_((DevBlockDeviceHandle *devHandlePtr, int devOffset, char *buffer, int bufSize));
extern ReturnStatus Raid_DevWriteInt();
extern ReturnStatus Raid_DevReadInt();
extern void Raid_WaitTime _ARGS_((int ms));

#endif /* _MISCUTIL */
