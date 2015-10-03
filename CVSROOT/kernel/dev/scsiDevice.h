/*
 * scsiDevice.h --
 *
 *	Declarations of Sprite interface to SCSI devices.  
 *	This file defines the interface to SCSI devices which consists 
 *	of three operations: Attach the device, Release the device, and
 *	send a SCSI command block to the device. 
 *
 *	INTERFACE SUMMARY: 
 *
 *	To interface to a SCSI device, the SCSI command formatter must first
 *	attach the device.  This is done by calling DevScsiAttachDevice() with
 *	the Fs_Device structure for the device. DevScsiAttachDevice 
 *	returns a ScsiDeviceHandle that contains the queue to insert 
 *	scsi command blocks for the device. The handle also 
 *	contains the procedure to call to release the device.
 *	   
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

#ifndef _SCSIDEVICE
#define _SCSIDEVICE

#include <list.h>
#include <devQueue.h>
#include <user/fs.h>
#include <fs.h>
#include <sys/scsi.h>

/*
 * The ScsiCmd data structure contains the information that a SCSI device 
 * to execute a SCSI command. It is the object enqueue on a SCSI device's 
 * request queue. Note that the bytes of the SCSI command block over 16 
 * bytes directly follow this data structure.
 */
struct ScsiCmd {
    List_Links	queuePtr;	/* List for queueing in HBA. Because we
				 * use the DevQueue routines this element
				 * MUST be the first in the structure.	 */
    Boolean	dataToDevice;	/* TRUE -> data is transferred to the device.
				 * FALSE -> data is transferred from the device.
				 * Meaningless if bufferLen is 0. */
    int		bufferLen;	/* The length of the data buffer in bytes. */
    Address	buffer;		/* The data buffer for this command. */
				/* Routine to called when command completes. */
    int		(*doneProc) _ARGS_((struct ScsiCmd *scsiCmdPtr, 
				ReturnStatus status, int statusByte, 
				int byteCount, int senseLength, 
				Address senseDataPtr));
    ClientData clientData;	/* A word of data available to the caller. 
				 * This item is not changed by the Device. */
    int	       commandBlockLen;	/* Length of the SCSI command block. The
				 * data of the SCSI command block immediately
				 * follows this data structure and continues
				 * for commandBlockLen bytes. */
    char	commandBlock[16]; /* The first 16 bytes of the SCSI command
				   * block.  The rest of the command directly
				   * follows the first 16 bytes. */
    int		senseLen;	/* Length of sense data. */
    char	senseBuffer[SCSI_MAX_SENSE_LEN]; /* Sense buffer. */
    int		statusByte;	/* Sense byte from scsi command. */
};
typedef struct ScsiCmd ScsiCmd;

/*
 * A device attached to a SCSI Bus is described by the following stucture. 
 * A pointer to a ScsiDevice is returned by the DevScsiAttachDevice
 * routine and contains the device queue to use to 
 * send commands to the device. The fields of the structure are 
 * initialize by the attached routine and should not be modified or copied
 * by the SCSI formatting routines. The pointer passed to the 
 * and releaseProc procedures should be the same value returned by the
 * attachProc. 
 */

typedef struct ScsiDevice {
    DevQueue	  devQueue;	     /* Queue to send request for this 
				      * device. Once the request has been
				      * processed, the caller 
				      * caller by calling function sepcified
				      * in the SCSICmd structure.  */
    char	 *locationName;	     /* A string used to identify the device's
				      * location in error messages produced
				      * the SCSI formatting routines. An 
				      * Example would be:
				      * "HBA 2 Bus 1 Target 6 LUN 2". */
    int		LUN;		      /* SCSI Logical unit number of device.
				       * This is stored here because the
				       * SCSI command formatter needs to 
				       * know the LUN to build command
				       * blocks. */
				     /* Routine to release the system 
				      * resources used by the device. */
    ReturnStatus (*releaseProc) _ARGS_((struct ScsiDevice *scsiDevicePtr));   
    int	 maxTransferSize;	     /* Maximum size of data transfer to 
				      * this device supported by the HBA. */
    int		 inquiryLength;	     /* Length in bytes of the inquiryData 
				      * pointed to by inquiryDataPtr. */
    char	 *inquiryDataPtr;    /* Data return by the INQUIRY command 
				      * sent to the device. */
    int		referenceCount;	     /* Count of number of references to this
				      * device handle. */
				     /* Routine to call if the statusByte
				      * returned by a command is non-zero. */
    ReturnStatus (*errorProc) _ARGS_((struct ScsiDevice *devPtr, 
				    ScsiCmd *scsiCmdPtr));     
    ClientData	clientData;	     /* Whatever you want it to be. */

} ScsiDevice;


/*
 * Upon command completion the routine specified by doneProc in the
 * ScsiCmd data structure is called with the following arguments:
 *	(*doneProc)(scsiCmdPtr, errorCode, scsiStatusByte, 
 *			amountTransferred, senseDataLen, senseDataPtr);
 *	where
 * ScsiCmd   *scsiCmdPtr  - The scsiCmdPtr argument passed to sendCmdProc.
 * ReturnStatus errorCode  - The error code from the HBA.
 * unsigned char scsiStatusByte - The scsi status byte as returned by the SCSI 
 *			          command.
 * int	 amountTransferred - Number of bytes of data transferred by this 
 *			     command.
 * int   senseDataLen -   The length in bytes of the SCSI sense info returned.
 *			  This number will be zero useless the scsiStatusByte
 *			  has the SCSI check condition bit set.
 * char *senseDataPtr -   Pointer to scsi sense blocks.  This block will no
 *			  longer be valid once the call back routine returns.
 *			  (i.e. You must copy it if you want to save it.)
 * 
 * scsiDoneProc may be called at interrupt level so it should use
 * appropriate locking and be relatively simple. Also, scsiDoneProc
 * maybe called before the sendCmdProc proc returns.
 */


/*
 * DevScsiSendCmd is normally encoded as macros for speed and reduced
 * calling depth.
 * If lint is being run we use the real routines in devScsiDevice.c to 
 * permit type checking. Documentation on the calling sequence is found
 * in devScsiDevice.c.
 */
#ifndef lint
#define	DevScsiSendCmd(handlePtr, scsiCmdPtr) \
    (Dev_QueueInsert((handlePtr)->devQueue,(List_Links *) (scsiCmdPtr)))
#else
extern void		  DevScsiSendCmd();
#endif

#define	MAX_SCSI_ERROR_STRING	128
extern int devScsiNumErrors[];
extern char **devScsiErrors[];

extern ScsiDevice *DevScsiAttachDevice _ARGS_((Fs_Device *devicePtr,
    void (*insertProc)()));
extern ReturnStatus DevScsiSendCmdSync _ARGS_((ScsiDevice *scsiDevicePtr,
    ScsiCmd *scsiCmdPtr, int *amountTransferredPtr));
extern ReturnStatus DevScsiReleaseDevice _ARGS_((ScsiDevice *scsiDevicePtr));
extern ReturnStatus DevScsiTestReady _ARGS_((ScsiDevice *scsiDevicePtr));
extern ReturnStatus DevScsiReadBlockLimits _ARGS_((ScsiDevice *scsiDevicePtr,
    int *minPtr, int *maxPtr));
extern ReturnStatus DevScsiStartStopUnit _ARGS_((ScsiDevice *scsiDevicePtr,
    Boolean start));
extern ReturnStatus DevScsiModeSense _ARGS_((ScsiDevice *scsiDevicePtr,
    int disableBlockDesc, int pageControl, int pageCode, int vendor, 
    int *sizePtr, char *bufferPtr));
extern ReturnStatus DevScsiRequestSense _ARGS_((ScsiDevice *scsiDevicePtr,
    int clearCount, int vendor, int *sizePtr, char *bufferPtr));
extern ReturnStatus DevScsiReadPosition _ARGS_((ScsiDevice *scsiDevicePtr,
    int blockType, ScsiReadPositionResult *positionPtr));
extern ReturnStatus DevScsiIOControl _ARGS_((ScsiDevice *devPtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ScsiDevice *DevNoHBAAttachDevice _ARGS_((Fs_Device *devicePtr,
    void (*insertProc)()));
extern Boolean DevScsiMapClass7Sense _ARGS_((int senseLength, char *senseDataPtr, ReturnStatus *statusPtr, char *errorString));
extern ReturnStatus DevScsiGroup0Cmd _ARGS_((ScsiDevice *devPtr, int cmd, unsigned int blockNumber, unsigned int countNumber, register ScsiCmd *scsiCmdPtr));

#endif /* _SCSIDEVICE */

