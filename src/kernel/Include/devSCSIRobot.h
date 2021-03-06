/*
 * devSCSIRobot.h --
 *
 * 	Definitions for the Exabyte EXB-120 robot on the SCSI I/O bus.
 *
 * Copyright 1992 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/kernel/Cvsroot/kernel/dev/devSCSIRobot.h,v 9.5 92/03/06 15:07:01 mani Exp $ SPRITE (Berkeley)
 */

#ifndef _SCSIEXBROBOT
#define _SCSIEXBROBOT

#include <dev/robot.h>
#include <scsiDevice.h>

typedef struct ScsiExbRobot {
    ScsiDevice	*devPtr;	/* SCSI Device we have attached. */
    int state;			/* State bits used to determine if it's
				 * open, really exists, etc. */
    unsigned int chmAddr;	/* Address of the robot arm itself.*/
    char *name;			/* Type name of robot device. */
} ScsiExbRobot;

/*
 * Robot state:
 *	SCSI_ROBOT_CLOSED	The device file is not open.
 *	SCSI_ROBOT_OPEN		The device file is open.
 */
#define SCSI_ROBOT_CLOSED	0x0
#define SCSI_ROBOT_OPEN		0x1


/*
 * The robot returns 18 bytes of sense.
 */
#define SCSI_EXB_ROBOT_SENSE_LEN	18

/*
 * Data Structures pertaining to the SCSI Mode Select command.
 */

typedef struct ExbRobotModeSelVendorUnique {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char reserved	:2;
    unsigned char pageCode	:6;	/* Identifies the vendor unique
					 * parameter list. Must be 0x0. */
    unsigned char paramListLength;	/* Length of the vendor unique
					 * parameter list. It is equal to
					 * 62. */
    unsigned char aInit		:1;	/* 0 == Perform INITIALIZE ELEMENT
					 *      STATUS (0xE7) only when the
					 *      command is issued.
					 * 1 == Perform INITIALIZE ELEMENT
					 *      STATUS after power-up. */
    unsigned char uInit		:1;	/* 0 == Do not perform INIT ELEMENT
					 *      STATUS on a cartridge each time
					 *      it is handled by the CHM.
					 * 1 == Perform INIT ELEMENT STATUS
					 *      each time. */
    unsigned char parity	:1;	/* 0 == Enable SCSI bus parity checking.
					 * 1 == Disable SCSI bus parity checking. */
    unsigned char reserved2	:2;	
    unsigned char notReadyDispCntrl:1;	/* 0 == "Not ready" message
					 *      is flashing.
					 * 1 == Message is steady. */
    unsigned char mesgDispCntrl	:2;	/* Determines how displayMessage
					 * is displayed.
					 * 0 == Flashing 8-char display.
					 * 1 == Steady 8-char display.
					 * 2 == Scrolling display (up to 60 chars). */
    unsigned char reserved3;
    char displayMessage[60];		/* Specifies the ready message that
					 * appears on the operator display when
					 * the robot is ready for operation. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char pageCode	:6;	/* Identifies the vendor unique
					 * parameter list. Must be 0x0. */
    unsigned char reserved	:2;

    unsigned char paramListLength;	/* Length of the vendor unique
					 * parameter list. It is equal to
					 * 62. */
    unsigned char mesgDispCntrl	:2;	/* Determines how displayMessage
					 * is displayed.
					 * 0 == Flashing 8-char display.
					 * 1 == Steady 8-char display.
					 * 2 == Scrolling display (up to 60 chars). */
    unsigned char notReadyDispCntrl	:1;	/* 0 == "Not ready" message
						 *      is flashing.
						 * 1 == Message is steady. */
    unsigned char reserved2	:2;
    unsigned char parity	:1;	/* 0 == Enable SCSI bus parity checking.
					 * 1 == Disable SCSI bus parity checking. */

    unsigned char uInit		:1;	/* 0 == Do not perform INIT ELEMENT
					 *      STATUS on a cartridge each time
					 *      it is handled by the CHM.
					 * 1 == Perform INIT ELEMENT STATUS
					 *      each time. */
    unsigned char aInit		:1;	/* 0 == Perform INITIALIZE ELEMENT
					 *      STATUS (0xE7) only when the
					 *      command is issued.
					 * 1 == Perform INITIALIZE ELEMENT
					 *      STATUS after power-up. */
    unsigned char reserved3;
    char displayMessage[60];		/* Specifies the ready message that
					 * appears on the operator display when
					 * the robot is ready for operation. */
#endif
} ExbRobotModeSelVendorUnique;


typedef struct ExbRobotAddrAssignPg {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char reserved1	:2;
    unsigned char pageCode	:6;	/* Identifies the element address
					 * assignment parameter list. The
					 * value of this field must by 0x1d
					 * (for exb-120).
					 */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char pageCode	:6;
    unsigned char reserved1	:2;
#endif    
    unsigned char paramListLength;	/* Length of the element address
					 * assignment parameter list. The
					 * valid value for this field is
					 * 0x12, which indicates that there
					 * are an additional 18 bytes of
					 * parameter data that follow this
					 * byte.
					 */
    unsigned char transportElemAddr[2]; /* Address of the transport
					 * mechanism.
					 */
    unsigned char transportElemCount[2];/* Number of such mechanisms. The
					 * EXB-120 only has 1.
					 */
    unsigned char firstStorElemAddr[2];	/* Address of the first tape position. */
    unsigned char storElemCount[2];	/* Number of such elements. */
    unsigned char firstEePortAddr[2];
    unsigned char eePortAddrCount[2];	/* Self-explanatory. */
    unsigned char firstDriveAddr[2];	/* Address of the first data transfer
					 * element (i.e., tape drive).
					 */
    unsigned char driveCount[2];	/* Number of such drives. */
    unsigned char reserved2[2];
} ExbRobotAddrAssignPg;


typedef struct ExbRobotModeSelectData {
    unsigned char header[4];	/* Reserved */
    ExbRobotModeSelVendorUnique vendorUnique;
} ExbRobotModeSelectData;

typedef struct ModeSenseCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		/* 0x1a for Mode Sense. */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char reserved	:1;
    unsigned char disableBlkDescrptr:1;	/* Not used on Exabyte EXB-120. */
    unsigned char reserved2	:3;
    unsigned char pageControl	:2;	/* Defines the type of parameters
					 * that are to be returned.
					 * 0 == Current values.
					 * 1 == Values which as changeable.
					 * 2 == Default values.
					 * 3 == Saved values. */
    unsigned char pageCode	:6;	/* Specifies which pages are to be
					 * returned.
					 * 0x1d == Element Address Assgnmt Pg.
					 * 0x1e == Transport Geometry Pg.
					 * 0x1f == Device Capabilities Pg.
					 * 0x0  == Vendor Unique Pg.
					 * 0x3f == All pages (in the above order). */
    unsigned char reserved3;
    unsigned char allocLength;		/* Specifies the maximum length
					 * of the parameter list to be
					 * returned. Max == 112 (0x70). */
    unsigned char vendorUnique	:2;	
    unsigned char reserved4	:4;
    unsigned char flag		:1;	/* Interrupt after linked command. */
    unsigned char link		:1;	/* Another command follows. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;		/* 0x1a for Mode Sense. */
    unsigned char reserved2	:3;
    unsigned char disableBlkDescrptr:1;	/* Not used on Exabyte EXB-120. */    
    unsigned char reserved	:1;
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char pageCode	:6;	/* Specifies which pages are to be
					 * returned.
					 * 0x1d == Element Address Assgnmt Pg.
					 * 0x1e == Transport Geometry Pg.
					 * 0x1f == Device Capabilities Pg.
					 * 0x0  == Vendor Unique Pg.
					 * 0x3f == All pages (in the above order). */
    unsigned char pageControl	:2;	/* Defines the type of parameters
					 * that are to be returned.
					 * 0 == Current values.
					 * 1 == Values which as changeable.
					 * 2 == Default values.
					 * 3 == Saved values. */
    unsigned char reserved3;
    unsigned char allocLength;		/* Specifies the maximum length
					 * of the parameter list to be
					 * returned. Max == 112 (0x70). */
    unsigned char link		:1;	/* Another command follows. */
    unsigned char flag		:1;	/* Interrupt after linked command. */
    unsigned char reserved4	:4;
    unsigned char vendorUnique	:2;
#endif
} ModeSenseCommand;


typedef struct MoveMediumCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;
    unsigned char unitNumber	:3;
    unsigned char reserved	:5;
    unsigned char transpElemAddr[2];
    unsigned char sourceAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char reserved3	:7;
    unsigned char invert	:1;
    unsigned char eePos		:2;
    unsigned char reserved4	:4;
    unsigned char finalBits	:2; 	/* These are fixed at 0. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;
    unsigned char reserved	:5;
    unsigned char unitNumber	:3;
    unsigned char transpElemAddr[2];
    unsigned char sourceAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char invert	:1;
    unsigned char reserved3	:7;
    unsigned char finalBits	:2;	/* These are fixed at 0. */
    unsigned char reserved4	:4;
    unsigned char eePos		:2;
#endif
} MoveMediumCommand;


typedef struct PositionRobotCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;
    unsigned char unitNumber		:3;
    unsigned char reserved		:5;
    unsigned char transpElemAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char reserved3		:7;
    unsigned char invert		:1;
    unsigned char reserved4;
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;
    unsigned char reserved		:5;
    unsigned char unitNumber		:3;
    unsigned char transpElemAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char invert		:1;
    unsigned char reserved3		:7;
    unsigned char reserved4;
#endif
} PositionRobotCommand;

typedef struct ReadElemStatCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		/* 0xb8 for this Command. */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char volTag	:1;	/* 0 == don't report volume tag info.
					 * 1 == do report it. */
    unsigned char elemTypeCode	:4;	/* 0 == report all types.
					 * 1 == report transport elements.
					 * 2 == report storage elements.
					 * 3 == report entry/exit elements.
					 * 4 == report data transfer elements. */
    unsigned char startElemAddr[2];	/* Min elem address to report. */
    unsigned char numElements[2];	/* Max number of descriptors to
					 * be transferred. */
    unsigned char reserved1;
    unsigned char allocLength[3];	/* Bytes of space allocated
					 * for data. */
    unsigned char vendorUnique	:2; 	/* Vendor Unique bits. */
    unsigned char reserved3	:4;
    unsigned char flag		:1; 	/* Interrupt after linked command. */
    unsigned char link		:1; 	/* Another command follows. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;		/* 0xb8 for this Command. */
    unsigned char elemTypeCode	:4;	/* 0 == report all types.
					 * 1 == report transport elements.
					 * 2 == report storage elements.
					 * 3 == report entry/exit elements.
					 * 4 == report data transfer elements. */
    unsigned char volTag	:1;	/* 0 == don't report volume tag info.
					 * 1 == do report it. */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char startElemAddr[2];	/* Min elem address to report. */
    unsigned char numElements[2];	/* Max number of descriptors to
					 * be transferred. */
    unsigned char reserved1;
    unsigned char allocLength[3];	/* Bytes of space allocated
					 * for data. */
    unsigned char link		:1; 	/* Another command follows. */
    unsigned char flag		:1; 	/* Interrupt after linked command. */
    unsigned char reserved2	:4;
    unsigned char vendorUnique	:2; 	/* Vendor Unique bits. */
#endif
} ReadElemStatCommand;


typedef struct InitElemStatCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		/* 0xe7 for this Command. */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char reserved1	:4;	
    unsigned char range		:1;	/* 0 == init all elements.
					 * 1 == init elements specified. */
    unsigned char elemAddr[2];		/* Specifies starting address
					 * of the series of elements
					 * to be scanned. Ignored when
					 * range == 0. */
    unsigned char reserved2[2];	
    unsigned char numElements[2];	/* Maximum number of elements
					 * to be scanned. ignored when
					 * range == 0. */
    unsigned char reserved3[2];
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;		/* 0xe7 for this Command. */
    unsigned char range		:1;	/* 0 == init all elements.
					 * 1 == init elements specified. */
    unsigned char reserved1	:4;	
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char elemAddr[2];		/* Specifies starting address
					 * of the series of elements
					 * to be scanned. Ignored when
					 * range == 0. */
    unsigned char reserved2[2];	
    unsigned char numElements[2];	/* Maximum number of elements
					 * to be scanned. ignored when
					 * range == 0. */
    unsigned char reserved3[2];
#endif
} InitElemStatCommand;


/* Function Prototypes */

extern ReturnStatus DevSCSIExbRobotError _ARGS_((ScsiDevice *devPtr,
     ScsiCmd *scsiCmdPtr));						 
extern ReturnStatus DevSCSIExbRobotOpen _ARGS_((Fs_Device *devicePtr,
     int useFlags, Fs_NotifyToken token, int *flagsPtr));
extern ReturnStatus DevSCSIExbRobotClose _ARGS_((Fs_Device *devicePtr,
     int useFlags, int openCount, int writerCount));
extern ReturnStatus DevSCSIExbRobotIOControl _ARGS_((Fs_Device *devicePtr,
     Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));

extern void ExbRobotInitElemStatus _ARGS_((ScsiCmd *scsiRobotCmdPtr,
     Dev_RobotCommand *cmdPtr));
extern void ExbRobotInquiry _ARGS_((ScsiCmd *scsiRobotCmdPtr, 
     ExbRobotInquiryData *inquiryDataPtr));
extern void ExbModeSelect _ARGS_((ScsiCmd *scsiRobotCmdPtr, 
     Dev_RobotCommand *cmdPtr, Address dataPtr,
     unsigned int dataLength));				  
extern void ExbMoveMedium _ARGS_((ScsiCmd *scsiRobotCmdPtr,
     ScsiExbRobot *robotPtr, Dev_RobotCommand *cmdPtr));
extern void ExbPosElem _ARGS_((ScsiCmd *scsiRobotCmdPtr,
     ScsiExbRobot *robotPtr, Dev_RobotCommand *cmdPtr));
extern void ExbPrevRemoval _ARGS_((ScsiCmd *scsiRobotCmdPtr, 
     Dev_RobotCommand *cmdPtr));
extern void ExbReqSense _ARGS_((ScsiCmd *scsiRobotCmdPtr, 
     ExbRobotSenseData *senseDataPtr));
     
#endif /* _SCSIEXBROBOT */
