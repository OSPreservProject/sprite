/*
 * devInt.h --
 *
 *	Internal globals and constants needed for the dev module.
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

#ifndef _DEVINT
#define _DEVINT

/*
 * A configuration table that describes the controllers in the system.
 */
typedef struct DevConfigController {
    char *name;		/* Identifying string used in print statements */
    int address;	/* The address of the controller.  Correct
			 * interpretation of this depends on the space */
    int controllerID;	/* Controller number: 0, 1, 2... */
    ClientData (*initProc)();	/* Initialization procedure */
    int (*intrProc)();	/* Interrupt handler called from autovector */
} DevConfigController;

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
