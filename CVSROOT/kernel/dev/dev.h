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
#include "devTimer.h"
#include "devSyslog.h"
#include "devConsole.h"
#include "devKeyboard.h"
#include "devDependent.h"
#else
#include <kernel/devTimer.h>
#include <kernel/devSyslog.h>
#include <kernel/devConsole.h>
#include <kernel/devKeyboard.h>
#include <kernel/devDependent.h>
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


extern void Dev_Init();
extern void Dev_Config();

#endif _DEV
