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
 * Device types:
 *
 *	FS_DEV_CONSOLE		The console - basic character input/output
 *	FS_DEV_SYSLOG		The system log device
 *	FS_DEV_KEYBOARD		Keyboard
 *	FS_DEV_SCSI_DISK	Disk on the SCSI bus
 *	FS_DEV_SCSI_TAPE	Tape drive on the SCSI bus
 *	FS_DEV_MEMORY		Null device and kernel memory area.
 *	FS_DEV_XYLOGICS		Xylogics 450 controller
 *	FS_DEV_NET		Raw ethernet device - unit number is protocol.
 *	FS_DEV_SBC_DISK		Disk on Sun's "SCSI-3" host adaptor.
 *
 * NOTE: These numbers correspond to the major numbers for the devices
 * in /dev. Do not change them unless you redo makeDevice for all the devices
 * in /dev.
 *
 */

#define	FS_DEV_CONSOLE		0
#define	FS_DEV_SYSLOG		1
#define	FS_DEV_KEYBOARD		2
#define	FS_DEV_PLACEHOLDER_2	3
#define	FS_DEV_SCSI_DISK	4
#define	FS_DEV_SCSI_TAPE	5
#define	FS_DEV_MEMORY		6
#define	FS_DEV_XYLOGICS		7
#define	FS_DEV_NET		8
#define FS_DEV_SBC_DISK		9

/*
 * Device type specific operations.
 *	The arguments to the operations are commented below the
 *	macro definitions used to invoke them.
 */

typedef struct FsDeviceTypeOps {
    int		 type;	/* One of the device types. */
    ReturnStatus (*open)();
    ReturnStatus (*read)();
    ReturnStatus (*write)();
    ReturnStatus (*ioControl)();
    ReturnStatus (*close)();
    ReturnStatus (*select)();
} FsDeviceTypeOps;

extern FsDeviceTypeOps fsDeviceOpTable[];
extern int fsNumDevices;

/*
 * The filesystem device block I/O operation switch.
 */
typedef struct FsBlockOps {
    int 	deviceType;		/* Redundant device type info */
    ReturnStatus (*readWrite)();	/* Block read/write routine */
} FsBlockOps;

extern FsBlockOps fsBlockOpTable[];

/*
 * FsDeviceBlockIO --
 *	A device specific Block I/O routine is used to read or write
 *	filesystem blocks on a disk.  Its maps from filesystem
 *	block indexes to disk addresses and does the I/O.
 */
#define FsDeviceBlockIO(readWriteFlag, devicePtr, blockNumber, numBlocks, buf) \
	(*fsBlockOpTable[(devicePtr)->type].readWrite) \
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
    Boolean	readNotifyScheduled;	/* Used to optimize out notifies; */
    Boolean	writeNotifyScheduled;	/*  important for serial lines, etc. */
} FsDeviceIOHandle;			/* 80 BYTES */

/*
 * Data transferred when a local device stream migrates.
 */
typedef struct FsDeviceMigData {
    int foo;
} FsDeviceMigData;

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
extern ReturnStatus FsDeviceMigStart();
extern ReturnStatus FsDeviceMigEnd();
extern ReturnStatus FsDeviceMigrate();
extern void	    FsDeviceScavenge();
extern void	    FsDeviceClientKill();
extern ReturnStatus FsDeviceClose();

extern ReturnStatus FsRmtDeviceCltOpen();
extern FsHandleHeader *FsRmtDeviceVerify();
extern ReturnStatus FsRmtDeviceRead();
extern ReturnStatus FsRmtDeviceWrite();
extern ReturnStatus FsRmtDeviceIOControl();
extern ReturnStatus FsRmtDeviceSelect();
extern ReturnStatus FsRemoteIOMigStart();
extern ReturnStatus FsRemoteIOMigEnd();
extern ReturnStatus FsRmtDeviceMigrate();
extern ReturnStatus FsRmtDeviceReopen();
extern void	    FsRmtDeviceScavenge();
extern ReturnStatus FsRemoteIOClose();

#endif _FSDEVICE
