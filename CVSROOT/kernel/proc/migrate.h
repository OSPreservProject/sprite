/*
 * migrate.h --
 *
 *	Declarations of types for process migration used only by the proc
 * 	module.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Migrate: proto.h,v 1.4 86/03/20 14:00:21 andrew Exp $ SPRITE (Berkeley)
 */

#ifndef _MIGRATE
#define _MIGRATE

/*
 * Define the number of 4-byte fields transferred in a shot when updating user
 * information.  Note: Timer_Ticks count as multiple fields.  See the
 * comments in SendProcessState for a list of fields that are transferred.
 */

#define PROC_NUM_FLAGS 4
#define PROC_NUM_ID_FIELDS 5
#define PROC_NUM_BILLING_FIELDS (7 + 4 * (sizeof(Timer_Ticks) / sizeof(int)))
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
