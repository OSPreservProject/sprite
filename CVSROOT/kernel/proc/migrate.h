/*
 * migrate.h --
 *
 *	Declarations of types for process migration used only by the proc
 * 	module.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Migrate: proto.h,v 1.4 86/03/20 14:00:21 andrew Exp $ SPRITE (Berkeley)
 */

#ifndef _MIGRATE
#define _MIGRATE

/*
 * Define the number of fields transferred in a shot when updating user
 * information.
 */

#define PROC_NUM_FLAGS 4
#define PROC_NUM_ID_FIELDS 4
#define PROC_NUM_BILLING_FIELDS 4
#define PROC_NUM_USER_INFO_FIELDS 3

/*
 * Size of various fields of the PCB structure copied upon migration.
 */

#define SIG_INFO_SIZE (((3 * SIG_NUM_SIGNALS) + 3) * sizeof(int))

/*
 * Parameters for a remote Proc_Wait.
 */

typedef struct {
    Proc_PID 	pid;	    /* ID of process doing the wait */
    int 	numPids;    /* number of pids in array */
    Boolean 	flags;      /* Flags to Proc_Wait. */
    int 	token;      /* token to use for remote notify call */
} ProcRemoteWaitCmd;

#endif _MIGRATE
