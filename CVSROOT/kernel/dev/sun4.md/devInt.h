/*
 * devInt.h --
 *
 *	Internal globals and constants needed for the dev module.
 *
 * Copyright 1989 Regents of the University of California
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

#ifndef _DEVINT
#define _DEVINT

#include "vm.h"


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
    ClientData (*initProc) _ARGS_((struct DevConfigController *ctrlLocPtr));
			/* Initialization procedure */
    int vectorNumber;	/* Vector number for autovectored architectures */
    Boolean (*intrProc) _ARGS_((ClientData  clientData));
			/* Interrupt handler called from autovector */
} DevConfigController;

/*
 * Definitions of address space types.
 * DEV_OBMEM	- on board memory
 * DEV_OBIO	- on board I/O devices.
 * DEV_MULTIBUS - the Multibus memory on the Sun2
 * DEV_MULTIBUS_IO - Multibus I/O space on the Sun2
 * DEV_VME_DxAx - The 6 sets of VME address spaces available on
 *	Sun3's.  Only D16A24 and D16A16 are available on VME based Sun2's.
 */
#define DEV_OBMEM	0
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
 * Special valued returned from Controller init procedures indicating 
 * the controller doesn't exists.
 */

#define	DEV_NO_CONTROLLER	((ClientData) 0)

/*
 * The controller configuration table.
 */
extern DevConfigController devCntrlr[];
extern int devNumConfigCntrlrs;

#endif /* _DEVINT */
