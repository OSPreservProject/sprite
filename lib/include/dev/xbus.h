/*
 * xbus.h --
 *
 *	Declarations of interface to the xbus board.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/xbus.h,v 1.1 92/07/09 18:35:10 elm Exp Locker: elm $ SPRITE (Berkeley)
 */

#ifndef _XBUS
#define _XBUS

#define	IOC_XBUS			(21 << 16)
#define	IOC_XBUS_RESET			(IOC_XBUS | 1)
#define	IOC_XBUS_DEBUG_ON		(IOC_XBUS | 2)
#define	IOC_XBUS_DEBUG_OFF		(IOC_XBUS | 3)
#define	IOC_XBUS_READ_REG		(IOC_XBUS | 4)
#define	IOC_XBUS_WRITE_REG		(IOC_XBUS | 5)
#define	IOC_XBUS_DO_XOR			(IOC_XBUS | 6)
#define	IOC_XBUS_CHECK_PARITY		(IOC_XBUS | 7)
#define	IOC_XBUS_LOCK_VME		(IOC_XBUS | 8)
#define	IOC_XBUS_HIPPI_SRC_BUFFER	(IOC_XBUS | 10)
#define	IOC_XBUS_HIPPI_DST_BUFFER	(IOC_XBUS | 11)
#define	IOC_XBUS_HIPPI_DST_FLUSH	(IOC_XBUS | 12)
#define	IOC_XBUS_USE_INTR_STATUS	(IOC_XBUS | 13)
#define	IOC_XBUS_TEST_START		(IOC_XBUS | 1000)
#define	IOC_XBUS_TEST_STOP		(IOC_XBUS | 1001)
#define	IOC_XBUS_TEST_STATS		(IOC_XBUS | 1002)


/*
 * This isn't a hard limit, but it's nice to include it here (so user
 * programs can see it).
 */
#define	DEV_XBUS_MAX_XOR_BUFS		20

/*
 * The maximum size of a string returned from IOC_XBUS_TEST_STATS.
 */
#define	DEV_XBUS_TEST_MAX_STAT_STR	1000

typedef struct DevXbusRegisterAccess {
    unsigned int	registerNum;
    unsigned int	value;
} DevXbusRegisterAccess;

#define	DEV_XBUS_MAX_BOARDS		16

/*
 * Sun4 address space the link occupies (2=d16, 3=d32)
 */
#define DEV_XBUS_ADDR_SPACE		3

/*
 * This is how far the board ID must be shifted to get the base address.
 */
#define	DEV_XBUS_ID_ADDR_SHIFT		28

/*
 * This is the start of memory relative to the start of XBUS address space.
 */
#define	DEV_XBUS_MEMORY_OFFSET		0x08000000

#define	DEV_XBUS_REG_RESET		0x000
#define	DEV_XBUS_REG_ATC0_PAR_ADDR	0x080
#define	DEV_XBUS_REG_ATC0_PAR_DATA	0x084
#define	DEV_XBUS_REG_ATC1_PAR_ADDR	0x088
#define	DEV_XBUS_REG_ATC1_PAR_DATA	0x08c
#define	DEV_XBUS_REG_ATC2_PAR_ADDR	0x090
#define	DEV_XBUS_REG_ATC2_PAR_DATA	0x094
#define	DEV_XBUS_REG_ATC3_PAR_ADDR	0x098
#define	DEV_XBUS_REG_ATC3_PAR_DATA	0x09c
#define	DEV_XBUS_REG_SERVER_PAR_ADDR	0x0a0
#define	DEV_XBUS_REG_SERVER_PAR_DATA	0x0c0
#define	DEV_XBUS_REG_STATUS		0x0e0
#define	DEV_XBUS_REG_HIPPID_CTRL_FIFO	0x400
#define	DEV_XBUS_REG_HIPPIS_CTRL_FIFO	0x800
#define	DEV_XBUS_REG_XOR_CTRL_FIFO	0xc00

#define	DEV_XBUS_XOR_GO			0x80000000

#define	DEV_XBUS_RESETREG_FREEZE	(1 << 0)
#define	DEV_XBUS_RESETREG_MEMORY	(1 << 1)
#define	DEV_XBUS_RESETREG_HIPPID_ADDR_FLUSH	(1 << 2)
#define	DEV_XBUS_RESETREG_XOR		(1 << 3)
#define	DEV_XBUS_RESETREG_HIPPID	(1 << 4)
#define	DEV_XBUS_RESETREG_HIPPIS	(1 << 5)
#define	DEV_XBUS_RESETREG_ATC0		(1 << 6)
#define	DEV_XBUS_RESETREG_ATC1		(1 << 7)
#define	DEV_XBUS_RESETREG_ATC2		(1 << 8)
#define	DEV_XBUS_RESETREG_ATC3		(1 << 9)
#define	DEV_XBUS_RESETREG_XBUS		(1 << 11)

#define	DEV_XBUS_RESETREG_CLEAR_PARITY	(1 << 12)
#define	DEV_XBUS_RESETREG_CHECK_PARITY	(1 << 13)
#define	DEV_XBUS_RESETREG_CLEAR_XOR_BIT	(1 << 15)
#define	DEV_XBUS_RESETREG_RESET		0x0
#define	DEV_XBUS_RESETREG_NORMAL	0xffff

#define	DEV_XBUS_STATUS_ATC0_PARITY_ERR		(1 << 0)
#define	DEV_XBUS_STATUS_ATC1_PARITY_ERR		(1 << 1)
#define	DEV_XBUS_STATUS_ATC2_PARITY_ERR		(1 << 2)
#define	DEV_XBUS_STATUS_ATC3_PARITY_ERR		(1 << 3)
#define	DEV_XBUS_STATUS_SERVER_PARITY_ERR	(1 << 4)
#define	DEV_XBUS_STATUS_XOR_INTERRUPT		(1 << 5)
#define	DEV_XBUS_STATUS_HIPPID_FIFO_NONEMPTY	(1 << 6)
#define	DEV_XBUS_STATUS_HIPPIS_FIFO_NONEMPTY	(1 << 7)

typedef struct DevXbusParityErrorInfo {
    volatile unsigned int	data;
    volatile unsigned int	addr;
} DevXbusParityErrorInfo;

typedef struct DevXbusCtrlRegs {
    volatile unsigned int	reset;
    volatile unsigned int	pad1[0x1f];
    DevXbusParityErrorInfo atcParity[4];
    volatile unsigned int	serverParityAddr;
    volatile unsigned int	pad2[0x7];
    volatile unsigned int	serverParityData;
    volatile unsigned int	pad3[0x7];
    volatile unsigned int	status;
} DevXbusCtrlRegs;

#endif	/* _XBUS */
