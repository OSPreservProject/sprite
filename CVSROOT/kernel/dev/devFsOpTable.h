/*
 * devFsOpTable.h --
 *
 *	The DEVICE operation switch is defined here.  This is the main
 *	interface between the file system and the device drivers.
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

#ifndef _DEVOPTABLE 
#define _DEVOPTABLE

#include "sprite.h"
#include "user/fs.h"
#include "devBlockDevice.h"

/*
 * Device type specific operations, calling sequence defined below.
 *	DeviceOpen
 *	DeviceRead
 *	DeviceWrite
 *	DeviceIOControl
 *	DeviceClose
 *	DeviceSelect
 *	BlockDeviceAttach
 *	DeviceReopen
 */

typedef struct DevFsTypeOps {
    int		 type;	/* One of the device types. See devNumbersInt.h */
    /*
     * Device Open - called during an open of a device file.
     *	(*openProc)(devicePtr, flags, notifyToken)
     *		Fs_Device *devicePtr;		(Identifies device)
     *		int flags;			(FS_READ, FS_WRITE, FS_APPEND)
     *		Fs_NotifyToken notifyToken;	(Handle on device used with
     *						(Fs_NotifyWriter/Reader calls)
     */
    ReturnStatus (*open)();
    /*
     * Device Read - called to get data from a device.
     * Device Write - called to pass data to a device.
     *
     * Both Read and Write take a Fs_IOParam record that indicates the
     * buffer, length, and offset of the transfer.  There is also
     * procID, familyID, and uid information for additional permission checks.
     * The length, buffer, and offset should be kept read-only because
     * they may be re-used by higher levels.
     * These also take a Fs_IOReply record that should be updated to
     * reflect the length transferred, and a signal/code to generate.
     * The length, signal, and code are all initialized to zero before
     * the call into the device driver.
     *
     *	(*readProc)(devicePtr, readPtr, replyPtr)
     *		Fs_Device *devicePtr;		(Identifies device)
     *		Fs_IOParam *readPtr;		(See above comments)
     *		Fs_IOReply *replyPtr;		(See above comments)
     *	(*writeProc)(devicePtr, writePtr, replyPtr)
     *		Fs_Device *devicePtr;		(Identifies device)
     *		Fs_IOParam *writePtr;		(See above comments)
     *		Fs_IOReply *replyPtr;		(See above comments)
     */
    ReturnStatus (*read)();
    ReturnStatus (*write)();
    /*
     * Device I/O Control - perform a device-specific operation
     * This takes an Fs_IOCParam record that specifies the inBuffer,
     * inBufSize, outBuffer, and outBufSize.  It also indicates
     * the command, byteOrder, procID, familyID, and uid.
     * The driver is responsible for fixing up the contents of the
     * inBuffer to match mach_ByteOrder, and fixing up the contents
     * of the outBuffer to match ioctlPtr->byteOrder.
     * The Fs_IOReply is used as in read and write.  The length is
     * initialized to ioctlPtr->outBufSize, so normally this doesn't
     * have to be modified.
     *
     *	(*ioctlProc)(devicePtr, ioctlPtr, replyPtr)
     *		Fs_Device *devicePtr;		(Identifies device)
     *		Fs_IOCParam *ioctlPtr;		(See above comments)
     *		Fs_IOReply *replyPtr;		(See above comments)
     */
    ReturnStatus (*ioctl)();
    /*
     * Device Close - close a stream to a device.
     *	(*closeProc)(devicePtr, flags, numUsers, numWriters)
     *		Fs_Device *devicePtr;		(Identifies device)
     *		int flags;			(Stream usage flags)
     *		int numUsers;			(Number of active streams left)
     *		int numWriters;			(Number of writers left)
     */
    ReturnStatus (*close)();
    /*
     * Device Select - poll a device for readiness
     *	(*selectProc)(devicePtr, readPtr, writePtr, exceptPtr)
     *		Fs_Device *devicePtr;		(Identifies device)
     *		int *readPtr;			(Readability bit)
     *		int *writePtr;			(Writability bit)
     *		int *exceptPtr;			(Exception bit)
     */
    ReturnStatus (*select)();
    /*
     * Block Device Attach - attach a block device at boot-time.
     *	(*attachProc)(devicePtr)
     *		Fs_Device *devicePtr;		(Identifies device)
     */
    DevBlockDeviceHandle *((*blockDevAttach)());
    /*
     * Reopen Device -  called during recovery to reestablish a stream
     *	(*reopenProc)(devicePtr, numUsers, numWriters, notifyToken)
     *		Fs_Device *devicePtr;		(Identifies device)
     *		int numUsers;			(Number of active streams)
     *		int numWriters;			(Number of writers)
     *		Fs_NotifyToken *notifyToken	(Handle on device used with
     *						(Fs_NotifyWriter/Reader calls)
     */
    ReturnStatus (*reopen)();
} DevFsTypeOps;

extern DevFsTypeOps devFsOpTable[];
extern int devNumDevices;

/*
 * DEV_TYPE_INDEX() - Compute the index into the devFsOpTable from the
 *		      type field from of the Fs_Device structure.
 */

#define	DEV_TYPE_INDEX(type)	((type)&0xff)
/*
 * A list of disk device Fs_Device structure that is used when probing for a
 * disk. Initialized in devConfig.c.
 */
extern Fs_Device devFsDefaultDiskPartitions[];
extern int devNumDefaultDiskPartitions;

#endif /* _DEVOPTABLE */
