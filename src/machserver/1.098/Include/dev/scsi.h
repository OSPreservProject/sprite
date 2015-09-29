/*
 * scsi.h --
 *
 *	Declarations of the user level interface to SCSI devices. The
 *	interface defined in this file allows user level programs to 
 *	send arbitrary SCSI commands to SCSI devices.  The interface
 *	users Sprite Io_Controls to send the SCSI command block and 
 *	a data buffer to the device. The SCSI status byte, command data, 
 *	and any sense data is returned using the
 *	out buffer of the Io_Control. The IO_COntrols defined in this
 *	file should work with any SCSI device (ie SCSI Disk, SCSI Tape, 
 *	SCSI Worm, etc) as well as the HBA device driver.
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
 * $Header: /sprite/src/lib/include/dev/RCS/scsi.h,v 1.1 89/04/12 21:03:58 mendel Exp Locker: mendel $ SPRITE (Berkeley)
 */

#ifndef _DEVSCSI
#define _DEVSCSI

/*   
 * Scsi-device specific commands:
 *
 *   IOC_SCSI_COMMAND		Issue a SCSI command
 */
#define IOC_SCSI			(7 << 16)
#define IOC_SCSI_COMMAND		(IOC_SCSI | 0x1)

/*
 * IOC_SCSI_COMMAND
 * The one IN parameter specifies the SCSI command block, data buffer,
 * and data transfer direction.
 */
typedef struct Dev_ScsiCommand {
    int		bufferLen;	/* The length of the data buffer in bytes. */
    int		dataOffset;	/* Offset into the in IO Control input buffer
				 * of the start of the data buffer. If 
				 * dataOffset equals the size of the input
				 * buffer then the commands is assumed to 
				 * receive data and the data is returned 
				 * in the output buffer. */
    int		commandLen;	/* Length in bytes of the SCSI command block. */
    /*
     * The SCSI command block immediately follows the Dev_ScsiCommand 
     * structure and extends for commandLen bytes.  The input data for
     * the command starts at dataOffset-sizeof(Dev_ScsiCommand) and
     * extends to the end of the input buffer. The input data should start
     * on a 32 bit word boundry.
     */
} Dev_ScsiCommand;

/*
 * IOC_SCSI_COMMAND returns in the output parameter the scsi Status
 * block.
 */

typedef struct Dev_ScsiStatus {
    int		statusByte;	/* Scsi status byte as returned by the
				 * device. */
    int  amountTransferred;	/* Number of data bytes transferred by the
				 * command. If the command received data
				 * then the data follows the Dev_ScsiStatus
				 * structure. */
    int	   senseDataLen;	/* The number of bytes of sense data returned
				 * by the device.  The sense data immediately
				 * follows the received data in
				 * the output buffer. */
} Dev_ScsiStatus;


#endif /* _DEVSCSI */
