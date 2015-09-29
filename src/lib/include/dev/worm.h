/*
 * worm.h --
 *
 *	Definitions and macros for Write-Once Read Many (worm) disk
 * 	manipulation.
 *
 * Copyright (C) 1987 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: worm.h,v 1.1 88/06/21 12:07:49 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _DEV_WORM
#define _DEV_WORM

/*   
 * worm specific commands:
 *
 *   IOC_WORM_COMMAND		Issue a worm specific command
 *   IOC_WORM_STATUS		Return status info from a worm
 */
#define IOC_WORM			(3 << 16)
#define IOC_WORM_COMMAND		(IOC_WORM | 0x1)
#define IOC_WORM_STATUS			(IOC_WORM | 0x2)

/*
 * Mag worm control, IOC_WORM_COMMAND
 * The one IN parameter specifies a specific
 * worm command and a repetition count.
 */
typedef struct Dev_WormCommand {
    int command;
    int count;
} Dev_WormCommand;

#define IOC_WORM_NO_OP			1

/*
 * Mag worm status, IOC_WORM_STATUS
 * This returns status info from drives.
 */
typedef struct Dev_WormStatus {
    int		statusReg;	/* Copy of device status register */
    int		residual;	/* Residual after last command */
    char	senseKey;	/* Sense key from last GetSense */
    char	code2;		/* Additional sense code from last GetSense */
    char	pad[2];
    int		location;	/* logical block addr corresponding to error */
} Dev_WormStatus;

/*
 * Stubs to interface to Fs_IOControl
 */
extern ReturnStatus Ioc_WormStatus();
extern ReturnStatus Ioc_WormCommand();

/*
 * Types for worm controllers.  Not used now, since there's only a single
 * type.
 */
#define DEV_WORM_RXT		0x1

#endif _DEV_WORM
