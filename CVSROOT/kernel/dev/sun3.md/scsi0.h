/*
 * devSCSIRegs.h
 *
 * Definitions for the first version of the Sun Host Adaptor registers.
 * This information is derived from Sun's "Manual for Sun SCSI
 * Programmers", which details the layout of the this implementation
 * of the Host Adaptor, the device that interfaces to the SCSI Bus.
 *
 * Copyright 1986 Regents of the University of California
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

#ifndef _DEVSCSIREGS
#define _DEVSCSIREGS

/*
 * The device registers for the original Sun SCSI Host Adaptor.
 * Through these registers the SCSI bus is controlled.  There are the
 * usual status and control bits, and there are also registers through
 * which command blocks and status blocks are transmitted.  This format
 * is defined on Page 10. of Sun's SCSI Programmers' Manual.
 */
typedef struct DevSCSIRegs {
    unsigned char data;		/* Data register.  Contains the ID of the
				 * SCSI "target", or controller, for the 
				 * SELECT phase. Also, leftover odd bytes
				 * are left here after a read. */
    unsigned char pad1;		/* The other half of the data register which
				 * is never used by us */
    unsigned char commandStatus;/* Command and status blocks are passed
				 * in and out through this */
    unsigned char pad2;		/* The other half of the commandStatus register
				 * which never contains useful information */
    unsigned short control;	/* The SCSI interface control register.
				 * Bits are defined below */
    unsigned short pad3;
    unsigned int dmaAddress;	/* Target address for DMA */
    short 	dmaCount;	/* Number of bytes for DMA.  Initialize this
				 * this to minus the byte count minus 1 more,
				 * and the device increments to -1. If this
				 * is 0 or 1 after a transfer then there was
				 * a DMA overrun. */
    unsigned char pad4;
    unsigned char intrVector;	/* For VME, Index into autovector */
} DevSCSIRegs;

/*
 * Control bits in the SCSI Host Interface control register.
 *
 *	SCSI_PARITY_ERROR There was a parity error on the SCSI bus.
 *	SCSI_BUS_ERROR	There was a bus error on the SCSI bus.
 *	SCSI_ODD_LENGTH An odd byte is left over in the data register after
 *			a read or write.
 *	SCSI_INTERRUPT_REQUEST bit checked by polling routine.  If a command
 *			block is sent and the SCSI_INTERRUPT_ENABLE bit is
 *			NOT set, then the appropriate thing to do is to
 *			wait around (poll) until this bit is set.
 *	SCSI_REQUEST	Set by controller to start byte passing handshake.
 *	SCSI_MESSAGE	Set by a controller during message phase.
 *	SCSI_COMMAND	Set during the command, status, and messages phase.
 *	SCSI_INPUT	If set means data (or commandStatus) set by device.
 *	SCSI_PARITY	Used to test the parity checking hardware.
 *	SCSI_BUSY	Set by controller after it has been selected.
 *  The following bits can be set by the CPU.
 *	SCSI_SELECT	Set by the host when it want to select a controller.
 *	SCSI_RESET	Set by the host when it want to reset the SCSI bus.
 *	SCSI_PARITY_ENABLE	Enable parity checking on transfer
 *	SCSI_WORD_MODE		Send 2 bytes at a time
 *	SCSI_DMA_ENABLE		Do DMA, always used.
 *	SCSI_INTERRUPT_ENABLE	Interrupt upon completion.
 */
#define SCSI_PARITY_ERROR		0x8000
#define SCSI_BUS_ERROR			0x4000
#define SCSI_ODD_LENGTH			0x2000
#define SCSI_INTERRUPT_REQUEST		0x1000
#define SCSI_REQUEST			0x0800
#define SCSI_MESSAGE			0x0400
#define SCSI_COMMAND			0x0200
#define SCSI_INPUT			0x0100
#define SCSI_PARITY			0x0080
#define SCSI_BUSY			0x0040
#define SCSI_SELECT			0x0020
#define SCSI_RESET			0x0010
#define SCSI_PARITY_ENABLE		0x0008
#define SCSI_WORD_MODE			0x0004
#define SCSI_DMA_ENABLE			0x0002
#define SCSI_INTERRUPT_ENABLE		0x0001

#endif _DEVSCSIREGS
