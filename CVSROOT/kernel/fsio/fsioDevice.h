/*
 * fsioDevice.h --
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

#include "fsio.h"
#include "fsioLock.h"

/*
 * The I/O descriptor for a local device: FSIO_LCL_DEVICE_STREAM
 */

typedef struct Fsio_DeviceIOHandle {
    Fs_HandleHeader	hdr;		/* Standard handle header. The
					 * 'major' field of the fileID is
					 * the device type.  The 'minor'
					 * field is the unit number. */
    List_Links		clientList;	/* List of clients of the device. */
    Fsutil_UseCounts		use;		/* Summary reference counts. */
    Fs_Device		device;		/* Device info passed to drivers.
					 * This includes a clientData field. */
    int			flags;		/* Flags returned by the device open.*/
    Fsio_LockState		lock;		/* User level lock state. */
    int			accessTime;	/* Cached version of access time */
    int			modifyTime;	/* Cached version of modify time */
    List_Links		readWaitList;	/* List of waiting reader processes. */
    List_Links		writeWaitList;	/* List of waiting writer processes. */
    List_Links		exceptWaitList;	/* List of process waiting for
					 * exceptions (is this needed?). */
    int			notifyFlags;	/* Bits set to optimize out notifies */
} Fsio_DeviceIOHandle;			/* 136 BYTES */

/*
 * Data transferred when a local device stream migrates.
 */
typedef struct Fsio_DeviceMigData {
    int foo;
} Fsio_DeviceMigData;

/*
 * The client data set up by the device pre-open routine on the server and
 * used by the device open routine on the client.
 */
typedef struct Fsio_DeviceState {
    int		accessTime;	/* Access time from disk descriptor */
    int		modifyTime;	/* Modify time from disk descriptor */
    Fs_FileID	streamID;	/* Used to set up client list */
} Fsio_DeviceState;

/*
 * Open operations.
 */
extern ReturnStatus	Fsio_DeviceNameOpen();
extern ReturnStatus	Fsio_DeviceClose();
extern ReturnStatus	FsDeviceDelete();

/*
 * Stream operations.
 */
extern ReturnStatus Fsio_DeviceIoOpen();
extern ReturnStatus Fsio_DeviceRead();
extern ReturnStatus Fsio_DeviceWrite();
extern ReturnStatus Fsio_DeviceIOControl();
extern ReturnStatus Fsio_DeviceSelect();
extern ReturnStatus Fsio_DeviceGetIOAttr();
extern ReturnStatus Fsio_DeviceSetIOAttr();
extern ReturnStatus Fsio_DeviceMigClose();
extern ReturnStatus Fsio_DeviceMigOpen();
extern ReturnStatus Fsio_DeviceMigrate();
extern Boolean	    Fsio_DeviceScavenge();
extern void	    Fsio_DeviceClientKill();
extern ReturnStatus Fsio_DeviceClose();


extern ReturnStatus Fsio_DeviceBlockIO();

#endif _FSDEVICE
