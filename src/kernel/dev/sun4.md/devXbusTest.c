/* 
 * devXbusTest.c --
 *
 *	Routines for testing the xbusboard.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun4.md/devXbusTest.c,v 9.1 92/10/23 15:04:33 elm Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#ifndef TESTING
#include "stdio.h"
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "stdlib.h"
#include "vmMach.h"
#include "dev/xbus.h"
#include "devXbusInt.h"
#include "devXbus.h"
#include "dbg.h"
#include "sync.h"
#endif

#ifdef TESTING
typedef int Sync_Semaphore;

#define Sync_SemInitStatic(name)	1
#define FS_WRITE	2
#endif

extern int	devXbusDebug;
static int	xbusID = 0xd;
static int	hppiID = 0;

static int	hppiQueue[1000];
static int	numInHppiQueue = 0;
static int	hppiInUse = 0;
static Sync_Semaphore hppiMutex = Sync_SemInitStatic("MyHppiIO\n");

static int	bufStartAddr;
static int	bufEndAddr;
static int	suSize;
static int	numDisk;
static int	bufSize;
static int	dataBufSize;
static int	numBuf;
static int	sectorsPerStripe;

static Sync_Semaphore	mutex = Sync_SemInitStatic("devXbusTest.c");
static int	hppiQueued = 0;
static int	xorQueued = 0;
static int	diskQueued = 0;
static int	numReq = 0;

#define DataAddress(reqID) (bufStartAddr + (reqID) * bufSize)
#define ParityAddress(reqID) (DataAddress(reqID) + dataBufSize)

static void myHppiCallback();
static void hppiCallback();
static void xorCallback();
static void diskCallback();

#ifdef TESTING
int raidArray[1];

static int masterLocked = 0;

MASTER_LOCK(sema)
    Sync_Semaphore	*sema;
{
    if (masterLocked) {
	panic("MASTER_LOCK while lock held\n");
    }
    masterLocked = 1;
}
MASTER_UNLOCK(sema)
    Sync_Semaphore	*sema;
{
    if (!masterLocked) {
	panic("MASTER_UNLOCK while lock NOT held\n");
    }
    masterLocked = 0;
}

void
Dev_XbusHippiBuffer()
{
}

void
Net_HppiLoopback(id, size, callbackProc, clientData)
    int		id;
    int		size;
    void	(*callbackProc)();
    ClientData	clientData;
{
    printf("Net_HppiLoopback: id=%d size=%d\n", id, size);
    callbackProc(clientData, SUCCESS);
}

void
Dev_XbusXor(id, destBuf, numBuf, bufs, size, callbackProc, clientData)
    int		id;
    int		destBuf;
    int		numBuf;
    int		bufs[];
    int		size;
    void	(*callbackProc)();
    ClientData	clientData;
{
    int i;
    printf("Dev_XbusXor: id=%d destBuf=%d numBuf=%d size=%d",
	    id, destBuf, numBuf, size);
    printf(" bufs =");
    for (i = 0; i < numBuf; i++) {
	printf(" %d", bufs[i]);
    }
    printf("\n");
    callbackProc(clientData, SUCCESS);
}

void
Raid_InitiateSimpleStripeIOs(id, operation, startSector, nthSector,
	buffer, callbackProc, clientData, ctrlData)
    int		id;
    int		operation;
    int		startSector;
    int		nthSector;
    int		buffer;
    void	(*callbackProc)();
    ClientData	clientData;
    int		ctrlData;
{
    printf("Raid_InitiateSimpleStripeIOs: id=%d operation=%d startSector=%d nthSector=%d buffer=%d\n", id, operation, startSector, nthSector, buffer);
    callbackProc(clientData, SUCCESS);
}

#endif TESTING


static void
Net_HppiIO(id, fromBuf, toBuf, size, callbackProc, clientData)
    int		id;
    int		fromBuf, toBuf;
    int		size;
    void	(*callbackProc)();
    ClientData	clientData;
{
    ReturnStatus status;
    extern void	Net_HppiLoopback();
    extern void	Net_HppiSrcPattern();
    
    if (devXbusDebug) {
	printf("Net_HppiIO(%d,%x,%x,%d,%x,%x)\n",
	    id, fromBuf, toBuf, size, callbackProc, clientData);
    }
    status = Dev_XbusHippiBuffer(xbusID, 1, size, fromBuf);
    if (devXbusDebug) {
	printf("Dev_XbusHippiBuffer(%d,%d,%d,%x)\n", id, 1, size, fromBuf);
    }
    if (status != SUCCESS) {
	printf("ERROR: Dev_XbusHippiBuffer(1) failed, status=%x\n", status);
    }
    status = Dev_XbusHippiBuffer(xbusID, -1, size, toBuf);
    if (status != SUCCESS) {
	printf("ERROR: Dev_XbusHippiBuffer(-1) failed, status=%x\n", status);
    }
    if (devXbusDebug) {
	printf("Dev_XbusHippiBuffer(%d,%d,%d,%x)\n", id, -1, size, toBuf);
    }
    Net_HppiLoopback(id, size, callbackProc, clientData);
}

static void
MyHppiIO(reqID)
    int		reqID;
{
    if (devXbusDebug) {
	printf("MyHppiIO(%d)\n", reqID);
    }
    MASTER_LOCK(&hppiMutex);
    if (numInHppiQueue == 1000) {
	MASTER_UNLOCK(&hppiMutex);
	panic("MyHppiIO queue full.\n");
    }
    hppiQueue[numInHppiQueue++] = reqID;
    if (!hppiInUse) {
	hppiInUse = 1;
	reqID = hppiQueue[--numInHppiQueue];
	MASTER_UNLOCK(&hppiMutex);
	Net_HppiIO(hppiID,
		DataAddress(reqID),DataAddress((reqID-1+numBuf)%numBuf),
		dataBufSize, myHppiCallback, reqID);
    } else {
	MASTER_UNLOCK(&hppiMutex);
    }
}

static void
myHppiCallback(reqID, status)
    int			reqID;
    ReturnStatus	status;
{
    if (devXbusDebug) {
	printf("myHppiCallback(%d,%x)\n", reqID, status);
    }
    MASTER_LOCK(&hppiMutex);
    hppiInUse = 0;
    if (numInHppiQueue > 0) {
	int	newReqID;
	hppiInUse = 1;
	newReqID = hppiQueue[--numInHppiQueue];
	MASTER_UNLOCK(&hppiMutex);
	Net_HppiIO(hppiID,
		DataAddress(newReqID), DataAddress((newReqID-1+numBuf)%numBuf),
		dataBufSize, myHppiCallback, newReqID);
    } else {
	MASTER_UNLOCK(&hppiMutex);
    }
    hppiCallback(reqID, status);
}


static int	isEnd = 0;
static int	maxNumReq;

void
DevXbusTestStart(bufStartAddrArg, bufEndAddrArg, suSizeArg, numDiskArg,
	maxNumReqArg)
    int		bufStartAddrArg, bufEndAddrArg;
    int		suSizeArg;
    int		numDiskArg;
    int		maxNumReqArg;
{
    int		reqID;

    printf("DevXbusTestStart: bufStartAddr=0x%x bufEndAddr=0x%x suSize=%d numDisk=%d, maxNumReq=%d\n", bufStartAddrArg, bufEndAddrArg, suSizeArg, numDiskArg, maxNumReqArg);

    numInHppiQueue = 0;
    hppiInUse = 0;
    hppiQueued = 0;
    xorQueued = 0;
    diskQueued = 0;
    numReq = 0;
    isEnd = 0;

    bufStartAddr = bufStartAddrArg;
    bufEndAddr = bufEndAddrArg;
    suSize = suSizeArg;
    numDisk = numDiskArg;
    maxNumReq = maxNumReqArg;
    bufSize = suSize * numDisk;
    dataBufSize = suSize * (numDisk -1);
    numBuf = (bufEndAddr - bufStartAddr) / bufSize;
    sectorsPerStripe = (suSize/512) * numDisk;

    for (reqID = 0; reqID < numBuf; reqID++) {
	MASTER_LOCK(&mutex);
	hppiQueued++;
	MASTER_UNLOCK(&mutex);
	MyHppiIO(reqID);
    }
}

void
DevXbusTestStop()
{
    isEnd = 1;
}

void
DevXbusTestStat(buf)
    char	*buf;
{
    sprintf(buf, "DevXbusTest: maxNumReq=%d numReq=%d hppiInUse=%d hppiQueued=%d xorQueued=%d diskQueued=%d\n", maxNumReq, numReq, hppiInUse, hppiQueued, xorQueued, diskQueued);
}


static void
hppiCallback(reqID, status)
    int			reqID;
    ReturnStatus	status;
{
    int			buf[20];
    int			suID;

    if (devXbusDebug) {
	printf("hppiCallback(%d,%x)\n", reqID, status);
    }
    if (status != SUCCESS) {
	printf("ERROR: hppi request %d failed, status=0x%x\n", reqID, status);
    }
    MASTER_LOCK(&mutex);
    hppiQueued--;
    xorQueued++;
    MASTER_UNLOCK(&mutex);
    for (suID = 0; suID < numDisk-1; suID++) {
	buf[suID] = DataAddress(reqID) + suID * suSize;
    }
    Dev_XbusXor(xbusID, ParityAddress(reqID), numDisk-1, buf, suSize,
	    xorCallback, (ClientData) reqID);
}

static void
xorCallback(reqID, status)
    int			reqID;
    ReturnStatus	status;
{
    extern int		raidArray[];
    extern void		Raid_InitiateSimpleStripeIOs();

    if (devXbusDebug) {
	printf("xorCallback(%d,%x)\n", reqID, status);
    }
    if (status != SUCCESS) {
	printf("ERROR: xor request %d failed, status=0x%x\n", reqID, status);
    }
    MASTER_LOCK(&mutex);
    xorQueued--;
    diskQueued++;
    MASTER_UNLOCK(&mutex);
    Raid_InitiateSimpleStripeIOs(&raidArray[0], FS_WRITE,
	    reqID * sectorsPerStripe, (reqID+1) * sectorsPerStripe,
	    DataAddress(reqID), diskCallback, reqID, 0);
}

static void
diskCallback(reqID, status)
    int			reqID;
    ReturnStatus	status;
{
    if (devXbusDebug) {
	printf("diskCallback(%d,%x)\n", reqID, status);
    }
    if (status != SUCCESS) {
	printf("ERROR: disk request %d failed, status=0x%x\n", reqID, status);
    }
    MASTER_LOCK(&mutex);
    numReq++;
    if (numReq >= maxNumReq) {
	isEnd = 1;
    }
    diskQueued--;
    if (!isEnd) {
	hppiQueued++;
    }
    MASTER_UNLOCK(&mutex);
    if (!isEnd) {
	MyHppiIO(reqID);
    }
    if (numReq >= maxNumReq) {
	printf("Ending test, numReq=%d\n", numReq);
    }
}

#ifdef TESTING
main()
{
    DevXbusTestStart(8, 512000, 1024, 10);
}
#endif
