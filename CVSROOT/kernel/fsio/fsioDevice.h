/*
 * fsDevice.h --
 *
 *	Declarations for device access.  The DEVICE operation switch is
 *	defined here.  The I/O handle formas for devices is defined here.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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

#ifndef _FSDEVICE
#define _FSDEVICE

#include "fsRecovery.h"

/*
 * Include the device switch declaration from dev.
 */
#include "devFsOpTable.h"

/*
 * FsDeviceBlockIO --
 *	A device specific Block I/O routine is used to read or write
 *	filesystem blocks on a disk.  Its maps from filesystem
 *	block indexes to disk addresses and does the I/O.
 */
#define FsDeviceBlockIO(readWriteFlag, devicePtr, blockNumber, numBlocks, buf) \
	(*devFsBlockOpTable[(devicePtr)->type].readWrite) \
	(readWriteFlag, devicePtr, blockNumber, numBlocks, buf)
#ifdef comment
    int readWriteFlag;		/* FS_READ or FS_WRITE */
    Fs_Device *devicePtr;	/* Specifies device type to do I/O with */
    int blockNumber;		/* CAREFUL, fragment index, not block index.
				 * This is relative to start of device. */
    int numBlocks;		/* CAREFUL, number of fragments, not blocks */
    Address buf;		/* I/O buffer */
#endif comment



/*
 * The I/O descriptor for a local device: FS_LCL_DEVICE_STREAM
 */

typedef struct FsDeviceIOHandle {
    FsHandleHeader	hdr;		/* Standard handle header. The
					 * 'major' field of the fileID is
					 * the device type.  The 'minor'
					 * field is the unit number. */
    List_Links		clientList;	/* List of clients of the device. */
    FsUseCounts		use;		/* Summary reference counts. */
    Fs_Device		device;		/* Device info passed to drivers.
					 * This includes a clientData field. */
    FsLockState		lock;		/* User level lock state. */
    int			accessTime;	/* Cached version of access time */
    int			modifyTime;	/* Cached version of modify time */
    List_Links		readWaitList;	/* List of waiting reader processes. */
    List_Links		writeWaitList;	/* List of waiting writer processes. */
    List_Links		exceptWaitList;	/* List of process waiting for
					 * exceptions (is this needed?). */
    int			notifyFlags;	/* Bits set to optimize out notifies */
} FsDeviceIOHandle;			/* 136 BYTES */

/*
 * Data transferred when a local device stream migrates.
 */
typedef struct FsDeviceMigData {
    int foo;
} FsDeviceMigData;

/*
 * The client data set up by the device pre-open routine on the server and
 * used by the device open routine on the client.
 */
typedef struct FsDeviceState {
    int		accessTime;	/* Access time from disk descriptor */
    int		modifyTime;	/* Modify time from disk descriptor */
    Fs_FileID	streamID;	/* Used to set up client list */
} FsDeviceState;

/*
 * Open operations.
 */
extern ReturnStatus	FsDeviceSrvOpen();
extern ReturnStatus	FsDeviceClose();
extern ReturnStatus	FsDeviceDelete();

/*
 * Stream operations.
 */
extern ReturnStatus FsDeviceCltOpen();
extern ReturnStatus FsDeviceRead();
extern ReturnStatus FsDeviceWrite();
extern ReturnStatus FsDeviceIOControl();
extern ReturnStatus FsDeviceSelect();
extern ReturnStatus FsDeviceGetIOAttr();
extern ReturnStatus FsDeviceSetIOAttr();
extern ReturnStatus FsDeviceRelease();
extern ReturnStatus FsDeviceMigEnd();
extern ReturnStatus FsDeviceMigrate();
extern Boolean	    FsDeviceScavenge();
extern void	    FsDeviceClientKill();
extern ReturnStatus FsDeviceClose();

extern ReturnStatus FsRmtDeviceCltOpen();
extern FsHandleHeader *FsRmtDeviceVerify();
extern ReturnStatus FsRemoteIORelease();
extern ReturnStatus FsRemoteIOMigEnd();
extern ReturnStatus FsRmtDeviceMigrate();
extern ReturnStatus FsRmtDeviceReopen();
extern ReturnStatus FsRemoteIOClose();

#endif _FSDEVICE
