/*
 * machAddrs.h --
 *
 *     	Addresses to various pieces of the machine.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#ifndef _MACHADDRS
#define _MACHADDRS

#define MACH_KUSEG_ADDR			0x0
#define MACH_CACHED_MEMORY_ADDR		0x80000000
#define MACH_CACHED_FRAME_BUFFER_ADDR	0x8fc00000
#define MACH_UNCACHED_MEMORY_ADDR	0xa0000000
#define MACH_UNCACHED_FRAME_BUFFER_ADDR	0xafc00000
#define MACH_PLANE_MASK_ADDR		0xb0000000
#define MACH_CURSOR_REG_ADDR		0xb1000000
#define MACH_COLOR_MAP_ADDR		0xb2000000
#define MACH_VDAC_ADDR			0xb2000000
#define MACH_NETWORK_INTERFACE_ADDR	0xb8000000
#define MACH_NETWORK_BUFFER_ADDR	0xb9000000
#define MACH_SCSI_INTERFACE_ADDR	0xba000000
#define MACH_SCSI_BUFFER_ADDR		0xbb000000
#define MACH_SERIAL_INTERFACE_ADDR	0xbc000000
#define MACH_CLOCK_ADDR			0xbd000000
#define MACH_SYS_CSR_ADDR		0xbe000000
#define MACH_ROM_ADDR			0xbf000000
#define MACH_KSEG2_ADDR			0xc0000000

#endif _MACHADDRS
