/*
 * scsiHBA.h --
 *
 *	Declarations of the data structures and routines defining the
 *	interface between the scsiDevice code and SCSI HBAs.  
 *
 * 	An SCSI HBA must export a single routine that allows 
 *	SCSI devices to be attached. The attach routine should return
 *	a ScsiDevice structure for the device. All further activity 
 *	to the device is directed though this SssiDevice structure.
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

#ifndef _SCSIHBA
#define _SCSIHBA

/*
 * Get the ScsiCmd and ScsiDevice typedefs from scsiDevice.h
 */
#include "scsiDevice.h"

/*
 * The following fields of the ScsiDevice must be initialized by 
 * the HBA AttachDeviceProc
 *
 * Field: devQueue 
 *	Should be a device queue return by Dev_QueueCreate.
 *
 * Field: locationName
 *	Should be an ASCII character string specifing the SCSI device 
 *	location on this HBA. This is used in error message and should point
 *	the system admin at the device.
 *
 * Field: LUN
 *	The logical unit number of the device.
 * Field: maxTransferSize
 *	The maximum transfer size supported by the HBA.
 * Field: referenceCount
 *	To zero when the device is first attached.
 * Field: releaseProc 
 *	The release procedure for the device. Should be declared.
 *
 * 	ReturnStatus releaseProc(scsiDevicePtr)
 *	   ScsiDevice	*scsiDevicePtr;   -- Handle for the device to release.
 *
 */

/*
 * The following definitions should all the SCSI HBA needs know about
 * the format of SCSI command blocks.  
 */

/*
 * SCSI_CHECK_STATUS() - Macro that returns TRUE if a SCSI status byte
 *			 requests a REQUEST SENSE.
 */
#define	SCSI_CHECK_STATUS(statusBytes) (((statusBytes)&0x2) != 0)
#define	DEV_MAX_SENSE_BYTES	64
#define	DEV_MAX_INQUIRY_SIZE	254

/*
 * Definitions of SCSI messages.
 *
 * SCSI_COMMAND_COMPLETE - After a command has completed, successfully
 *	or not, this is returned to the host from the target.
 *
 * The following messages are used with connect/disconnect:
 * SCSI_SAVE_DATA_POINTER - Sent from target to host to request saving
 *	of current DMA address and count.   Indicates a pending dis-connect.
 * SCSI_RESTORE_POINTER - Sent from the target to the host to request
 *	restoring pointers saved before a disconnect
 * SCSI_DISCONNECT - Sent from the target to the host to disconnect.
 * SCSI_ABORT - Sent from the host to the target to abort current request.
 * SCSI_MESSAGE_REJECT -  Indicates reciept, by either host or target, of
 *	an unimplemented message.
 * SCSI_NO_OP - Sent from host to target if it has no real message to send.
 * SCSI_MESSAGE_PARITY_ERROR - Sent from host to target on message parity error
 * SCSI_BUS_RESET - Sent from host to target to reset all current I/O
 *
 * SCSI_IDENTIFY - The low order two bits of this message type indicate
 *	the Logical Unit of the Target which is requesting a reconnect.
 * SCSI_DIS_REC_IDENTIFY - Sent from the host to a target to indicate
 *	is supports connect/dis-connect
 *	
 */
#define SCSI_COMMAND_COMPLETE		0x00
#define SCSI_SAVE_DATA_POINTER		0x02
#define SCSI_RESTORE_POINTERS		0x03
#define SCSI_DISCONNECT			0x04
#define SCSI_ABORT			0x06
#define SCSI_MESSAGE_REJECT		0x07
#define SCSI_NO_OP			0x08
#define SCSI_MESSAGE_PARITY_ERROR	0x09
#define SCSI_BUS_RESET			0x0C

#define SCSI_IDENTIFY			0x80
#define SCSI_DIS_REC_IDENTIFY		0xc0


/*
 * devScsiAttachProcs - Table for attach procs indexed by SCSI HBA type.
 * This table is filled in  machine dependent 
 */
extern ScsiDevice *((*devScsiAttachProcs[])());
extern int	  devScsiNumHBATypes;

/*
 * This section describes the layout of the Fs_Device structure type and
 * unit number fields for SCSI devices.
 *
 * Type: 16 bits
 *   |  Bits 15 to  8   | Bits 7 to 0   |
 *   |  HBA  type       | device type   |
 * Unit:
 *  | Bits 15 to 10 | Bits 9 to 7 | Bits 6 to 4 |  Bits 3 to 0    |
 *  |   HBA Number  |    LUN      |   targetID  | Device specific |
 *
 * This limits a system to at most 256 different types of HBAs. Each HBA has
 * at most 64 instances each supporting a full complement of SCSI devices.
 *
 * Macros for SCSI devices:
 *
 * SCSI_HBA_TYPE()	 - 	 Compute the HBA type from the Fs_Device struct.
 * SCSI_HBA_NUMBER()	-	 Compute the HBA number from Fs_Device struct.
 * SCSI_TARGET_ID()	-	 Compute the SCSI device's targetID from the
 *				 Fs_Device struct.
 * SCSI_LUN()		-	 Compute the SCSI device Logical Unit number
 *				 the Fs_Device struct.
 * SCSI_DEVICE_BITS()	-	 Select the device dependent bits from the
 *				 Fs_Device struct.
 * SCSI_MAKE_DEVICE_TYPE() - 	 Build the type field of the Fs_Device structure
 *				 from the device type and HBA type.
 * SCSI_MAKE_DEVICE_UNIT() - 	 Build the unit field of the Fs_Device structure
 *				 from its compontends
 */
#define	SCSI_HBA_TYPE(devicePtr) 	((((devicePtr)->type)>>8)&0xff)
#define	SCSI_HBA_NUMBER(devicePtr) 	((((devicePtr)->unit)>>10)&0x3f)
#define	SCSI_TARGET_ID(devicePtr) 	((((devicePtr)->unit)>>4)&0x7)
#define	SCSI_LUN(devicePtr) 		((((devicePtr)->unit)>>7)&0x7)
#define	SCSI_DEVICE_BITS(devicePtr)	(((devicePtr)->unit)&0xf)
#define	SCSI_MAKE_DEVICE_TYPE(type, hbaType, ctrlNum, targetID, LUN, dBits) \
		(((hbaType)<<8)|(type))
#define	SCSI_MAKE_DEVICE_UNIT(type, hbaType, ctrlNum, targetID, LUN, dBits)  \
		(((ctrlNum)<<10)|((LUN)<<7)|((targetID)<<4)|(dBits))
/*
 * Routines.
 */
extern void DevScsiSenseCmd();

#endif /* _SCSIHBA */

