/*
 * boot.h --
 *
 *	Boot program definitions.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: boot.h,v 1.2 87/05/08 17:43:52 brent Exp $ SPRITE (Berkeley)
 */

#ifndef _BOOT
#define _BOOT

/*
 * This affects what devices get configured in.
 */
#define SCSI_DISK_BOOT
/*
 * Do SCSI3. 
*/
#ifdef notdef
#define SCSI3_BOOT
#define SCSI3_ONBOARD 
#endif
#define SCSI0_BOOT 
int (*boot_Poll)();
int (*boot_SendSCSICommand)();

#endif _BOOT
