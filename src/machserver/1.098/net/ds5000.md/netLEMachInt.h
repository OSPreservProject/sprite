/*
 * netLEMachInt.h --
 *
 *	Internal definitions for the ds5000-based LANCE controller.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef _NETLEMACHINT
#define _NETLEMACHINT

#include <netInt.h>
#include <machAddrs.h>

/*
 * Register address port and register data port definition.
 */

typedef struct NetLEMach_Reg {
	volatile unsigned short	dataPort;	/* RDP */
	unsigned short unused;
        volatile unsigned short	addrPort[1];	/* RAP */
} NetLEMach_Reg;

#define NET_LE_REG_SIZE	6

/*
 * Defined constants:
 */

/*
 * We have to copy the packets into the network buffer.
 */

#define NET_LE_COPY_PACKET		TRUE

/*
 * Misc addresses (offsets from slot address).
 */

#define NET_LE_MACH_BUFFER_OFFSET	0x0
#define NET_LE_MACH_BUFFER_SIZE		0x20000
#define NET_LE_MACH_RDP_OFFSET		0x100000
#define NET_LE_MACH_RAP_OFFSET		0x100004
#define NET_LE_MACH_DIAG_ROM_OFFSET	0x1c0000
#define NET_LE_MACH_ESAR_OFFSET		0x1c0000

/*
 * Macros for converting chip to cpu and cpu to chip address.
 * We always deal with chip addresses in two parts, the lower 16 bits
 * and the upper 8 bits.
 */
#define	NET_LE_FROM_CHIP_ADDR(statePtr, high,low)	\
		((Address) (statePtr->bufAddr + ((high) << 16) + (low)))
#define	NET_LE_TO_CHIP_ADDR_LOW(a) ( ((unsigned int) (a)) & 0xffff)

#define	NET_LE_TO_CHIP_ADDR_HIGH(a) ( (((unsigned int) (a)) >> 16) & 0x1)

/*
 * Macro to allocate a network buffer.  For the ds5000 we call the 
 * routine to allocate memory in the region accessible by the chip.
 */
#define BufAlloc(statePtr, numBytes) \
    NetLEMemAlloc((statePtr), (numBytes))

#endif /* _NETLEMACHINT */
