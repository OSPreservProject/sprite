/*
 * devXbusInt.h --
 *
 *	Internal declarations of interface to the xbus board.
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
 * $Header$ SPRITE (Berkeley)
 */

typedef volatile unsigned int vuint;

typedef struct DevXbusInfo {
    unsigned int state;
    DevXbusCtrlRegs *regs;
    char*	name;			/* name of device from devConfig.c */
    vuint	*hippidCtrlFifo;
    vuint	*hippisCtrlFifo;
    vuint	*xorCtrlFifo;
    Sync_Semaphore mutex;
} DevXbusInfo;

#define	DEV_XBUS_STATE_OK		0x1

