/*
 * sii.h --
 *
 * 	SII registers.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWR
L)
 */

#ifndef _SII

#include "scsiDevice.h"
#include "machAddrs.h"

/*
 * SII registers
 */
typedef struct {
	unsigned short sdb;	/* SCSI Data Bus and Parity		*/
	unsigned short pad0;
	unsigned short sc1;	/* SCSI Control Signals One		*/
	unsigned short pad1;
	unsigned short sc2;	/* SCSI Control Signals Two		*/
	unsigned short pad2;
	unsigned short csr;	/* Control/Status register		*/
	unsigned short pad3;
	unsigned short id;		/* Bus ID register		*/
	unsigned short pad4;
	unsigned short slcsr;	/* Select Control and Status Register	*/
	unsigned short pad5;
	unsigned short destat;	/* Selection Detector Status Register	*/
	unsigned short pad6;
	unsigned short dstmo;	/* DSSI Timeout Register		*/
	unsigned short pad7;
	unsigned short data;	/* Data Register			*/
	unsigned short pad8;
	unsigned short dmctrl;	/* DMA Control Register			*/
	unsigned short pad9;
	unsigned short dmlotc;	/* DMA Length of Transfer Counter	*/
	unsigned short pad10;
	unsigned short dmaddrl;	/* DMA Address Register Low		*/
	unsigned short pad11;
	unsigned short dmaddrh;	/* DMA Address Register High		*/
	unsigned short pad12;
	unsigned short dmabyte;	/* DMA Initial Byte Register		*/
	unsigned short pad13;
	unsigned short stlp;	/* DSSI Short Target List Pointer	*/
	unsigned short pad14;
	unsigned short ltlp;	/* DSSI Long Target List Pointer	*/
	unsigned short pad15;
	unsigned short ilp;	/* DSSI Initiator List Pointer		*/
	unsigned short pad16;
	unsigned short dsctrl;	/* DSSI Control Register		*/
	unsigned short pad17;
	unsigned short cstat;	/* Connection Status Register		*/
	unsigned short pad18;
	unsigned short dstat;	/* Data Transfer Status Register	*/
	unsigned short pad19;
	unsigned short comm;	/* Command Register			*/
	unsigned short pad20;
	unsigned short dictrl;	/* Diagnostic Control Register		*/
	unsigned short pad21;
	unsigned short clock;	/* Diagnostic Clock Register		*/
	unsigned short pad22;
	unsigned short bhdiag;	/* Bus Handler Diagnostic Register	*/
	unsigned short pad23;
	unsigned short sidiag;	/* SCSI IO Diagnostic Register		*/
	unsigned short pad24;
	unsigned short dmdiag;	/* Data Mover Diagnostic Register	*/
	unsigned short pad25;
	unsigned short mcdiag;	/* Main Control Diagnostic Register	*/
	unsigned short pad26;
} SIIRegs;

#define SII_REG_BASE	(volatile SIIRegs *)MACH_SCSI_INTERFACE_ADDR
#define SII_BUF_BASE	(volatile char *)MACH_SCSI_BUFFER_ADDR
#define SII_REG_ADDR	(SII_REG_BASE)
#define SII_BUF_ADDR	(SII_BUF_BASE)

/*
 * SC1 - SCSI Control Signals One
 */
#define SII_SC1_MSK	0x1ff		/* All possible signals on the bus    */
#define SII_SC1_SEL	0x80		/* SCSI SEL signal active on bus      */
#define SII_SC1_ATN	0x08		/* SCSI ATN signal active on bus      */

/*
 * SC2 - SCSI Control Signals Two
 */
#define SII_SC2_IGS	0x8		/* SCSI drivers for initiator mode    */

/*
 * CSR - Control/Status Register
 */
#define SII_HPM	0x10			/* SII in on an arbitrated SCSI bus   */
#define	SII_RSE	0x08			/* 1 = respond to reselections	      */
#define SII_SLE	0x04			/* 1 = respond to selections	      */
#define SII_PCE	0x02			/* 1 = report parity errors	      */
#define SII_IE	0x01			/* 1 = enable interrupts	      */

/*
 * ID - Bus ID Register
 */
#define SII_ID_IO	0x8000		/* I/O 				      */

/*
 * DESTAT - Selection Detector Status Register
 */
#define SII_IDMSK	0x7		/* ID of target reselected the SII    */

/*
 * DMCTRL - DMA Control Register
 */
#define SII_ASYNC	0x00		/* REQ/ACK Offset for async mode      */
#define SII_SYNC	0x03		/* REQ/ACK Offset for sync mode	      */

/*
 * DMLOTC - DMA Length Of Transfer Counter
 */
#define SII_TCMSK	0x1fff		/* transfer count mask		      */

/*
 * CSTAT - Connection Status Register
 */
#define	SII_CI		0x8000	/* composite interrupt bit for CSTAT	      */
#define SII_DI		0x4000	/* composite interrupt bit for DSTAT	      */
#define SII_RST_ONBUS	0x2000	/* 1 if reset is asserted on SCSI bus	      */
#define	SII_BER		0x1000	/* Bus error				      */
#define	SII_OBC		0x0800	/* Out_en Bit Cleared (DSSI mode)	      */
#define SII_TZ		0x0400	/* Target pointer Zero (STLP or LTLP is zero) */
#define	SII_BUF		0x0200	/* Buffer service - outbound pkt to non-DSSI  */
#define SII_LDN		0x0100	/* List element Done			      */
#define SII_SCH		0x0080	/* State Change				      */
#define SII_CON		0x0040	/* SII is Connected to another device	      */
#define SII_DST_ONBUS	0x0020	/* SII was Destination of current transfer    */
#define SII_TGT_ONBUS	0x0010	/* SII is operating as a Target		      */
#define SII_SWA		0x0008	/* Selected With Attention		      */
#define SII_SIP		0x0004	/* Selection In Progress		      */
#define SII_LST		0x0002	/* Lost arbitration			      */

/*
 * DSTAT - Data Transfer Status Register
 */
#define SII_DNE		0x2000	/* DMA transfer Done			      */
#define SII_TCZ		0x1000	/* Transfer Count register is Zero	      */
#define SII_TBE		0x0800	/* Transmit Buffer Empty		      */
#define SII_IBF		0x0400	/* Input Buffer Full			      */
#define SII_IPE		0x0200	/* Incoming Parity Error		      */
#define SII_OBB		0x0100	/* Odd Byte Boundry			      */
#define SII_MIS		0x0010	/* Phase Mismatch			      */
#define SII_ATN		0x0008	/* ATN set by initiator if in Target mode     */
#define SII_MSG		0x0004	/* current bus state of MSG		      */
#define SII_CD		0x0002	/* current bus state of C/D		      */
#define SII_IO		0x0001	/* current bus state of I/O		      */
#define SII_PHA_MSK	0x0007	/* Phase Mask				      */

/*
 * COMM - Command Register
 */
#define	SII_DMA		0x8000	/* DMA mode				      */
#define SII_RST		0x4000	/* Assert reset on SCSI bus for 25 usecs      */
#define SII_RSL		0x1000	/* 0 = select, 1 = reselect desired device    */

/* Commands 	I - Initiator, T - Target, D - Disconnected		      */
#define SII_INXFER	0x0800	/* Information Transfer command	(I,T)	      */
#define SII_SELECT	0x0400	/* Select command		(D)	      */
#define SII_REQDATA	0x0200	/* Request Data command		(T)	      */
#define	SII_DISCON	0x0100	/* Disconnect command		(I,T,D)	      */
#define SII_CHRESET	0x0080	/* Chip Reset command		(I,T,D)	      */
/* Chip state bits */
#define	SII_CON		0x0040	/* Connected				      */
#define SII_DST		0x0020	/* Destination				      */
#define SII_TGT		0x0010	/* Target				      */
#define SII_STATE_MSK  0x0070	/* State Mask				      */
/* SCSI control lines */
#define SII_ATN		0x0008	/* Assert the SCSI bus ATN signal	      */
#define	SII_MSG		0x0004	/* Assert the SCSI bus MSG signal	      */
#define	SII_CD		0x0002	/* Assert the SCSI bus C/D signal	      */
#define	SII_IO		0x0001	/* Assert the SCSI bus I/O signal	      */

/*
 * DICTRL - Diagnostic Control Register
 */
#define SII_PRE		0x4	/* Enable the SII to drive the SCSI bus	      */

#define SII_WAIT_COUNT		10000   /* Delay count used for the SII chip  */
#define SII_MAX_DMA_XFER_LENGTH	8192  	/* Max DMA transfer length for SII    */

ClientData	DevSIIInit();
Boolean		Dev_SIIIntr();
ScsiDevice	*DevSIIAttachDevice();

#endif _SII
