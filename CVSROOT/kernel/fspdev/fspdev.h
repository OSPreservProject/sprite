/*
 * fsPdev.h --
 *
 *	Declaration of FsPdevState.  This is passed back from the name server
 *	as the generic streamData of a open.  It gets passed into the
 *	pseudo-device client open routine.  This is in a separate header
 *	to avoid a circularity inclusions caused if fsNameOps.h includes
 *	the main fsPdev.h header file, which includes dev/pdev.h, which
 *	includes fsNameOps.h, which...
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSPDEVSTATE
#define _FSPDEVSTATE

#include "fs.h"
#include "proc.h"

/*
 * FsPdevState is returned from the SrvOpen routine to the CltOpen routine.
 * It is also sent via RPC from the remoteCltOpen routine to the localCltOpen
 * routine.  In this second case, the processID and uid of the remote client
 * is initialized.
 */
typedef struct FsPdevState {
    Fs_FileID	ctrlFileID;	/* Control stream FileID */
    /*
     * The following fields are used when the client process is remote
     * from the server host.
     */
    Proc_PID	procID;		/* Process ID of remote client */
    int		uid;		/* User ID of remote client */
    Fs_FileID	streamID;	/* Client's stream ID used to set up a
				 * matching stream here on the server */
} FsPdevState;

#endif /* _FSPDEVSTATE */
