/*
 * devSCSITape.h --
 *
 *	External definitions for the SCSI Tape.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSITAPE
#define _DEVSCSITAPE

/*
 * Forward Declarations.
 */
ReturnStatus Dev_SCSITapeOpen();
ReturnStatus Dev_SCSITapeRead();
ReturnStatus Dev_SCSITapeWrite();
ReturnStatus Dev_SCSITapeIOControl();
ReturnStatus Dev_SCSITapeClose();

ReturnStatus Dev_SCSITapeBlockIOInit();
ReturnStatus Dev_SCSITapeBlockIO();

#endif _DEVSCSITAPE
