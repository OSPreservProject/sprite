/*
 * dev.h --
 *
 *     Types, constants, and macros exported by the device module.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEV
#define _DEV

#include "status.h"
#include "devTimer.h"
#include "devSyslog.h"
/* 
 * Machine dependent exported definitions.
 */
 
#include "devDependent.h"
"

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
