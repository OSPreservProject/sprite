/*
 * dev.h --
 *
 *     Types, constants, and macros exported by the device module.
 *
 * Copyright 1985, 1988 Regents of the University of California
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

#ifndef _DEV
#define _DEV

#include "status.h"
#ifdef KERNEL
#include "devSyslog.h"
#include "user/sysStats.h"
#else
#include <kernel/devSyslog.h>
#include <sysStats.h>
#endif
#ifndef _SPRITETIME
#include <spriteTime.h>
#endif

/*
 * The filesystem and the device module cooperate to translate from
 * filesystem block numbers to disk addresses.  Hence, this simple
 * type and the bytes per sector are exported.
 */
typedef struct Dev_DiskAddr {
    int cylinder;
    int head;
    int sector;
} Dev_DiskAddr;
/*
 *	DEV_BYTES_PER_SECTOR the common size for disk sectors.
 */
#define DEV_BYTES_PER_SECTOR	512

extern Time	dev_LastConsoleInput;

extern void Dev_ConsoleReset _ARGS_ ((int toConsole));
extern void Dev_Init _ARGS_((void));
extern void Dev_Config _ARGS_((void));

extern void Dev_GatherDiskStats _ARGS_((void));
extern int Dev_GetDiskStats _ARGS_((Sys_DiskStats *diskStatArr,int numEntries));
extern void Dev_RegisterConsoleCmd _ARGS_((char commandChar, void (*proc)(void),
    ClientData clientData));
extern void Dev_InvokeConsoleCmd _ARGS_((int commandChar));
extern int Dev_KbdQueueAttachProc _ARGS_((char character, void (*proc)(void),
    ClientData clientData));

#endif /* _DEV */
