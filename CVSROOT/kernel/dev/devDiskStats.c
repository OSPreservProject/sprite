/* 
 * devDiskStats.c --
 *
 *	Routines supporting statistics on Sprite Disk usage.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "sync.h"
#include "sysStats.h"
#include "devDiskStats.h"
#include "user/fs.h"
#include "stdlib.h"
#include "list.h"

/*
 * The disk stats modules cleans a linked list of registers disk to implment
 * the Idle counts of devices. This list is composed of devices.
 */

typedef struct Device {
    List_Links	links;		  /* Used by the List_Links library routines.*/
    Boolean	(*idleCheck)();	  /* Routine to check device's state. */
    ClientData clientData; 	  /* ClientData argument to  idleCheck. */
    int		type;		  /* Fs_Device type of this disk. */
    int		unit;	    	  /* Fs_Device unit of this disk. */
    Sys_DiskStats diskStats;  	  /* Stat structure of device. */
} Device;

/*
 * If idleCheck functions are kept in a list pointed to by deviceListHdr and
 * protected by deviceListMutex. The variable initialized set to TRUE 
 * indicates the list has been initialized.
 */
static Sync_Semaphore deviceListMutex = Sync_SemInitStatic("devDiskStatMutex");
static List_Links	deviceListHdr;
static Boolean		initialized = FALSE;

/*
 *----------------------------------------------------------------------
 *
 * Dev_GatherDiskStats --
 *
 *	Determine which disks are idle. This routine should be called 
 *	periodically to obtain an estimate of the idle percentage of 
 *	a disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Dev_GatherDiskStats()
{
    register Device *devicePtr;

    MASTER_LOCK(&deviceListMutex);

    /*
     * If any device has been registered do an idle check on all 
     * registers devices. 
     */
    if (initialized) {
	LIST_FORALL(&deviceListHdr, (List_Links *) devicePtr) {
	    register Sys_DiskStats *stats = &(devicePtr->diskStats);

	    stats->numSamples++;
	    if ((devicePtr->idleCheck)(devicePtr->clientData)) {
		stats->idleCount++;
	    }
	}
    }
    MASTER_UNLOCK(&deviceListMutex);

}


/*
 *----------------------------------------------------------------------
 *
 * Dev_GetDiskStats --
 *
 *	Return statistics about the different disks.
 *
 * Results:
 *	Number of statistics entries returned.
 *
 * Side effects:
 *	Entries in *diskStatPtr filled in.
 *
 *----------------------------------------------------------------------
 */
int
Dev_GetDiskStats(diskStatArr, numEntries)
    Sys_DiskStats *diskStatArr;	/* Where to store the disk  stats. */
    int		   numEntries;	/* The number of elements in diskStatArr. */
{
    Device *devicePtr;
    int	   index;

    MASTER_LOCK(&deviceListMutex);

    /*
     * Return Sys_DiskStats for all registers devices being careful not to
     * overrun the callers buffer.
     */
    index = 0;
    if (initialized) {
	LIST_FORALL(&deviceListHdr, (List_Links *) devicePtr) {
	    if (index >= numEntries) {
		break;
	    }
	    diskStatArr[index] = devicePtr->diskStats;
	    index += 1;
	}
    }
    MASTER_UNLOCK(&deviceListMutex);
    return index;
}


/*
 *----------------------------------------------------------------------
 *
 * DevRegisterDisk--
 *
 *	Register a disk with the disk stat module so that its idle 
 * 	percentage can be computed from period sampling and its diskStats
 *	structure may available to the Dev_GetDiskStats routine.
 *
 * Results:
 *	The initialized Sys_DiskStats structure for the device.
 *
 * Side effects:
 *	The idleCheck function will be called when periodcally and should
 *	return TRUE if the disk is idle. It is should be declared as follows:
 *
 *		Boolean idleCheck(clientData)
 *			ClientData clientData  -- The clientData argument passed
 *						  to DevRegisterDevice.
 *
 *----------------------------------------------------------------------
 */

Sys_DiskStats *
DevRegisterDisk(devicePtr, deviceName, idleCheck, clientData)
    Fs_Device	*devicePtr;	/* Fs_Device for disk. */
    char	*deviceName;	/* Printable name for this device. */
    Boolean	(*idleCheck)();	/* Function returning TRUE if the device
				 * is idle. */
    ClientData	clientData;	/* ClientData argument passed to idleCheck
				 * to indicate which device. */
{
    Device	*newDevice, *devPtr;
    Boolean	found = FALSE;


    /*
     * Allocated, initialized, and add to the callback list a Device structure
     * for this device.
     */
    newDevice = (Device *) malloc(sizeof(Device));
    List_InitElement((List_Links *) newDevice);
    newDevice->idleCheck = idleCheck;
    newDevice->clientData = clientData;
    newDevice->type = devicePtr->type;
    newDevice->unit = devicePtr->unit;
    bzero((char *) &(newDevice->diskStats), sizeof(Sys_DiskStats));
    strncpy(newDevice->diskStats.name, deviceName, SYS_DISK_NAME_LENGTH);
    MASTER_LOCK(&deviceListMutex);
    if (!initialized) {
	List_Init(&deviceListHdr);
	initialized = TRUE;
    }
    LIST_FORALL(&deviceListHdr, (List_Links *) devPtr) {
	if ((devPtr->unit == devicePtr->unit) &&
	    (devPtr->type == devicePtr->type)) {
	   found = TRUE;
	   break;
	}
    }
    if (found) {
	devPtr->idleCheck = idleCheck;
	devPtr->clientData = clientData;
    } else { 
	List_Insert((List_Links *) newDevice, LIST_ATREAR(&deviceListHdr));
	devPtr = newDevice;
    }
    MASTER_UNLOCK(&deviceListMutex);
    if (found) {
	free((char *)newDevice);
    }
    return &(devPtr->diskStats);
}

