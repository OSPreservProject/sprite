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
#define MACH_UNCACHED_MEMORY_ADDR	0xa0000000
#define MACH_KSEG2_ADDR			0xc0000000

/*
 * IO slot addresses.
 */

#define MACH_IO_SLOT_BASE		0xbe000000
#define MACH_IO_SLOT_SIZE		0x400000
#define MACH_IO_SLOT_ADDR(slot)	        (MACH_IO_SLOT_BASE + \
					(MACH_IO_SLOT_SIZE * (slot)))
/*
 * The standard TURBOchannel option information is located at these offsets
 * from the start of the option rom.  See page 13  of the TURBOchannel
 * Hardware Specification.  These offsets must be added to the option rom
 * address, as specified in the documentation for the option.
 */

#define MACH_IO_ROM_OFFSET		0x3e0
#define MACH_IO_ROM_INFO_OFFSET		0x400

/*
 * Addresses within the system interface (slot 7).
 */

#define MACH_ROM_ADDR			MACH_IO_SLOT_ADDR(7)
#define MACH_CHKSYN_ADDR		(MACH_ROM_ADDR + 0x100000)
#define MACH_ERRADR_ADDR		(MACH_ROM_ADDR + 0x180000)
#define MACH_DZ_ADDR			(MACH_ROM_ADDR + 0x200000)
#define MACH_RTC_ADDR			(MACH_ROM_ADDR + 0x280000)
#define MACH_CSR_ADDR			(MACH_ROM_ADDR + 0x300000)

/*
 * Other addresses.
 */

#define MACH_SERIAL_INTERFACE_ADDR 0x1fe00000	/* Serial interface address. */

#endif /* _MACHADDRS */
