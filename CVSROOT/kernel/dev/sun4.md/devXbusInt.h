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

#ifndef	_DEV_XBUS_INT_
#define	_DEV_XBUS_INT_

#define	DEV_XBUS_STATE_OK		0x1
#define	DEV_XBUS_STATE_XOR_GOING	0x2
#define	DEV_XBUS_STATE_XOR_TEST		0x10
#define	DEV_XBUS_STATE_XOR_TEST_WAITING	0x20
#define	DEV_XBUS_STATE_TESTING		0x40

#define	DEV_XBUS_MAX_QUEUED_XORS	20

#define	DEV_XBUS_MALLOC_MIN_SIZE	12	/* 4 KB minimum size */
#define	DEV_XBUS_MALLOC_MAX_SIZE	25	/* 32 MB maximum size */
#define	DEV_XBUS_MALLOC_NUM_SIZES	(DEV_XBUS_MALLOC_MAX_SIZE - \
					 DEV_XBUS_MALLOC_MIN_SIZE + 1)
#define	DEV_XBUS_MALLOC_NUM_PTRS	100

typedef volatile unsigned int vuint;

typedef struct DevXbusXorInfo {
    ReturnStatus status;
    unsigned int	numBufs;
    unsigned int	bufLen;
    unsigned int	destBuf;
    unsigned int	buf[DEV_XBUS_MAX_XOR_BUFS];
    void (*callbackProc)();
    ClientData		clientData;
} DevXbusXorInfo;

typedef struct DevXbusFreeMem {
    struct DevXbusFreeMem *next;
    unsigned int	address;
} DevXbusFreeMem;

typedef struct DevXbusInfo {
    unsigned int state;
    DevXbusCtrlRegs *regs;
    char*	name;			/* name of device from devConfig.c */
    vuint	*hippidCtrlFifo;
    vuint	*hippisCtrlFifo;
    vuint	*xorCtrlFifo;
    unsigned int addressBase;
    unsigned int resetValue;
    Sync_Semaphore mutex;
    char	semName[30];
    int		boardId;
    DevXbusXorInfo xorQueue[DEV_XBUS_MAX_QUEUED_XORS];
    int		numInQ;
    DevXbusXorInfo* qHead;
    DevXbusXorInfo* qTail;
    DevXbusXorInfo* qEnd;
    DevXbusFreeMem	*freeList[DEV_XBUS_MALLOC_NUM_SIZES];
    DevXbusFreeMem	*freePtrList;
} DevXbusInfo;

extern void	DevXbusTestStart ();
extern void	DevXbusTestStop ();
extern void	DevXbusTestStat ();

#endif	/* _DEV_XBUS_INT_ */
