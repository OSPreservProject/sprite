/* 
 * devBlockDevice.h --
 *
 *	Declarations for block devices drivers.  A block device is any device
 *	that provides random access to some number of fixed size units
 *	called blocks. Block device are normally used to build file system
 *	on but may also be accessed like a file. 
 *
 *	The model used for block devices is that devices are first "attached"
 *	to the system.  This is normally done when the device is
 *	opened with Fs_Open. Attaching a device first 
 *	verifies the existence and readiness (i.e. Is it powered on?)
 *	of the device.  If the device exists and is accessible, the
 *	attach routine returns a pointer to a handle use to 
 *	access the device. 
 *
 *	Because of intended usage and the speeds of some
 *	block devices, the I/O interface to block devices is asynchronous.
 *	Input and output operations are started by calling Dev_BlockDeviceIO
 *	that starts the operation and returns before the operation 
 *	completes. The initiator of block input/output operations is
 *	notified of completion by a call back to a function specified
 *	in the request.
 *
 * Copyright 1989 Regents of the University of California
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

#ifndef _DEVBLOCKDEVICE
#define _DEVBLOCKDEVICE

/*
 * MAX_BLOCK_DEV_CTRL_WORDS - Number of sizeof(int) words reserved for 
 * 			      controller use in a block device request.
 *			      This memory is used by the controller for
 *			      queuing and building controller control blocks.
 */

#define	MAX_BLOCK_DEV_CTRL_WORDS (64/sizeof(int))

/*
 * The DevBlockDeviceRequest structure is passed to block IO devices to 
 * request a range of address to be read or written.  This startAddress and
 * range must be fit within the constraint specified in the 
 * DevBlockDeviceHandle.
 */
typedef struct DevBlockDeviceRequest {
    int			operation;	/* Operation  FS_READ or FS_WRITE. */
    unsigned int	startAddress;	/* Byte offset into device to start
					 * operation. */
    unsigned int	startAddrHigh;	/* High order 32 bits of byte offset
					 * starting address. */
    int			bufferLen;	/* Length of the data buffer for 
					 * request in bytes. */
    Address		buffer;		/* The data buffer. */
    void		(*doneProc)();	/* Procedure to call upon completion.*/
    ClientData		clientData;	/* Word of client data available to the 
					 * caller.  */
				/*
				 * The rest of the bytes in this structure are
				 * for block device controller information. 
				 * It is declared to be an array of ints to 
				 * guarantee it will start on a word 
				 * boundary. */
    int			ctrlData[MAX_BLOCK_DEV_CTRL_WORDS];
} DevBlockDeviceRequest;

/* 
 * An active block device is described by a pointer to it's block device 
 * handle. A block device handle must start with the a DevBlockDeviceHandle 
 * structure. The call sequence of routines in DevBlockDeviceHandle is 
 * defined in the macro specified in the comment following the structure's
 * field names.
 */

typedef struct DevBlockDeviceHandle {
    ReturnStatus  (*blockIOProc)();	/* Start a block read or write
					 * operation. 
					 * See below for calling sequence. */
    ReturnStatus  (*IOControlProc)();	/* Perform an IO Control operation on
					 * the device. 
					 * See below for calling sequence. */
    ReturnStatus  (*releaseProc)();	/* Release the device and free any 
					 * resources held. 
					 * See below for calling sequence. */
    int	 	minTransferUnit;	/* Smallest unit of transfer to or
					 * from the device. All request must
					 * at least this size and be 
					 * a multiple of this size. */
    int		maxTransferSize;	/* Largest unit of transfer to and 
					 * from the device. */
    ClientData	clientData;		/* A word of clientData usable by the
					 * caller. */
} DevBlockDeviceHandle;


/*
 * DevBlockDeviceHandle routine releaseProc calling sequence:
 *    
 * releaseProc -
 *
 *	ReturnStatus releaseProc(blockDevHandlePtr)
 *	    DevBlockDeviceHandle *blockDevHandlePtr; / * The device handle as
 *						       * returned by the 
 *						       * attach routine. * /
 *	Release the resources in use by the specified device. 
 *	Once released, the device must be attached again with 
 *	attachProc before any operation can be performed.
 *	Note that device attaches do not nest so the caller is 
 *	responsible for insuring all users of a device are finished
 *	before releasing the device. releaseProc returns SUCCESS if
 *	the operation succeeds or a Sprite error code otherwise.
 *
 *
 * DevBlockDeviceHandle routine blockIOProc calling sequence:
 *
 *  ReturnStatus blockIOProc(blockDevHandlePtr, requestPtr)
 *  	 DevBlockDeviceHandle	*blockDevHandlePtr;
 *				    / * Handle of the device to operate on. * /
 *	 DevBlockDeviceRequest	*requestPtr; / * Request to be performed. * /
 *
 *	Enqueue a block IO request for the specified device. Upon operation
 *	completion doneProc specified in the requestPtr is called. 
 *	blockIOProc should return SUCCESS if the operation is 
 *	successfully enqueued or a Sprite error code otherwise. 
 *	When the enqueued operation finishes, 
 *	doneProc is called with the following arguments:
 *
 *	(*doneProc)(requestPtr, returnStatus, amountTransferred)
 *		DevBlockDeviceRequest *requestPtr;
 *					       / * The requestPtr passed to
 *						 * blockIOProc. * /
 *		ReturnStatus	returnStatus;  / * The error status of the
 *						 * command. SUCCESS if no
 *						 * error occurred. * /
 *		int	amountTransferred;     / * The number of bytes
 *						 * transferred by the 
 *						 * operation. * /
 *	
 *	Note that doneProc may be called before blockIOProc finishes and
 *	may also be called running at interrupt level. 
 *
 * IOControlProc calling sequences:
 * 	ReturnStatus IOControlProc(blockDevHandlePtr, command, byteOrder,
 *				inBufSize, inBuffer,  outBufSize, outBuffer);
 *
 *  	 DevBlockDeviceHandle	*blockDevHandlePtr;
 *				    / * Handle of the device to operate on. * /
 *	 int	command;	/ * IO control to be performed. * /
 *	 int	byteOrder;	/ * Caller's byte ordering. * /
 *	 int	inBufSize;	/ * Size of the input buffer - inBuffer. * /
 *	 Address inBuffer;	/ * The input buffer. * /
 *	 int	outBufSize;	/ * Size of the output buffer - outBuffer. * /
 *	 Address outBuffer;	/ * The output buffer. * /
 *	
 *	Execute an IO control operation on the specified device. The
 *	return value is the Sprite error code for the IOControl.
 *
 *
 *----------------------------------------------------------------------
 */

/*
 * DEV_NO_ATTACH_PROC -  Special attach procedure entry in the
 *			 devFsOpTable array  specifying no attach proc 
 *			 available.
 */
#define	DEV_NO_ATTACH_PROC	((DevBlockDeviceHandle *(*)())0)

/* procedures */

extern DevBlockDeviceHandle	*Dev_BlockDeviceAttach();
extern ReturnStatus Dev_BlockDeviceRelease();
extern ReturnStatus Dev_BlockDeviceIOSync();
#ifdef lint
extern ReturnStatus Dev_BlockDeviceIO();
extern ReturnStatus Dev_BlockDeviceIOControl();
#else
/*
 * For speed, we code Dev_BlockDeviceIO and Dev_BlockDeviceIOControl() as 
 * macros. See the routines in devBlockDevice.c for documentation. If 
 * lint is being run we keep around the routines for type checking.
 */
#define	Dev_BlockDeviceIO(handlePtr, requestPtr) \
       (((handlePtr)->blockIOProc)((handlePtr),(requestPtr)))

#define	Dev_BlockDeviceIOControl(handle, cmd, byteOrder, isize, ibuf, osize, obuf) \
       (((handle)->IOControlProc)((handle),(cmd),(byteOrder),(isize),(ibuf),(osize),(obuf)))


#endif /* lint */
#endif
