/*
 * devInt.h --
 *
 *	Internal globals and constants needed for the dev module.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVINT
#define _DEVINT

#include "vm.h"
/*
 * This keeps the state of the memory allocator for the kernel
 * virtual address range reserved for DMA.
 */
extern Vm_DevBuffer devIOBuffer;

/*
 * Disks contain a map that defines the way the disk is partitioned.
 * Each partition corresponds to a different device unit.  Partitions
 * are made up of complete cylinders because the disk layout and
 * allocation strategies are cylinder oriented.
 */
typedef struct DevDiskMap {
    int firstCylinder;		/* The first cylinder in the partition */
    int numCylinders;		/* The number of cylinders in the partition */
} DevDiskMap;

/*
 * There are generally 8 disk partitions defined for a disk.
 */
#define DEV_NUM_DISK_PARTS	8

/*
 * As with disks, 8 unit numbers are assigned to each tape drive.
 * the differences between the 8 are not yet defined, however.
 */
#define DEV_TAPES_PER_CNTRLR	8


/*
 * A configuration table that describes the controllers in the system.
 */
typedef struct DevConfigController {
    char *name;		/* Identifying string used in print statements */
    int address;	/* The address of the controller.  Correct
			 * interpretation of this depends on the space */
    int space;		/* DEV_MULTIBUS, DEV_VME_16D16, ...
			 * This is used to convert what the hardware thinks
			 * is its address to what the MMU of the system
			 * uses for those kinds of addresses.  For example,
			 * Sun2's have Multibus memory mapped into a
			 * particular range of kernel virtual addresses. */
    int controllerID;	/* Controller number: 0, 1, 2... */
    Boolean (*initProc)();	/* Initialization procedure */
    int vectorNumber;	/* Vector number for autovectored architectures */
    int (*intrProc)();	/* Interrupt handler called from autovector */
} DevConfigController;

/*
 * Definitions of address space types.
 * DEV_MEMORY	- on board memory
 * DEV_OBIO	- on board I/O devices.
 * DEV_MULTIBUS - the Multibus memory on the Sun2
 * DEV_MULTIBUS_IO - Multibus I/O space on the Sun2
 * DEV_VME_DxAx - The 6 sets of VME address spaces available on
 *	Sun3's.  Only D16A24 and D16A16 are available on VME based Sun2's.
 */
#define DEV_MEMORY	0
#define DEV_OBIO	1
#define DEV_MULTIBUS	22
#define DEV_MULTIBUS_IO	23
#define DEV_VME_D16A32	31
#define DEV_VME_D16A24	32
#define DEV_VME_D16A16	33
#define DEV_VME_D32A32	34
#define DEV_VME_D32A24	35
#define DEV_VME_D32A16	36

/*
 * A configuration table that describes the devices in the system.
 */
typedef struct DevConfigDevice {
    int controllerID;	/* Controller number: 0, 1, 2... */
    int slaveID;	/* Controller relative ID of device */
    int flags;		/* Device specific flags.  Used, for example,
			 * to distinquish tapes from disks on the SCSI bus. */
    Boolean (*initProc)();	/* Initialization procedure */
} DevConfigDevice;

/*
 * The controller configuration table.
 */
extern DevConfigController devCntrlr[];
extern int devNumConfigCntrlrs;

/*
 * The device configuration table.
 */
extern DevConfigDevice devDevice[];
extern int devNumConfigDevices;
#endif _DEVINT
