/*
 * fspdev.h --
 *
 *	Declarations for pseudo-devices and pseudo-filesystems.
 *
 *	A pseudo-device is a file that acts as a communication channel
 *	between a user-level server process (hereafter called the "server"),
 *	and one or more client processes (hereafter called the "clients").
 *	Regular filesystem system calls (Fs_Read, Fs_Write, Fs_IOControl,
 *	Fs_Close) by a client process are forwarded to the server using
 *	a request-response procotol.  The server process can implement any
 *	sort of sementics for the file operations it wants to. The general
 *	format of Fs_IOControl, in particular, lets the server implement
 *	any remote procedure call it cares to define.
 *
 *	A pseudo-filesystem is a whole sub-tree of the filesystem that
 *	is controlled by a user-level server process.  The basic request
 *	response protocol is still used for communication.  In addition to
 *	file access operations, file naming operations are handled by
 *	a pseudo-filesystem server.  The pseudo-filesystem server can
 *	establish pseudo-device like connections for each pseudo-file
 *	that is opened, or it can open regular files and connect its
 *	clients to those files instead.
 *
 *	The user include file <dev/pdev.h> defines the request-response
 *	protocol as viewed by the user-level server process.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSPDEVX
#define _FSPDEVX

#include <fs.h>
#include <proc.h>
/*
 * Fspdev_State is returned from the SrvOpen routine to the CltOpen routine.
 * It is also sent via RPC from the remoteCltOpen routine to the localCltOpen
 * routine.  In this second case, the processID and uid of the remote client
 * is initialized.
 */
typedef struct Fspdev_State {
    Fs_FileID	ctrlFileID;	/* Control stream FileID */
    /*
     * The following fields are used when the client process is remote
     * from the server host.
     */
    Proc_PID	procID;		/* Process ID of remote client */
    int		uid;		/* User ID of remote client */
    Fs_FileID	streamID;	/* Client's stream ID used to set up a
				 * matching stream here on the server */
} Fspdev_State;

extern Boolean fspdev_Debug;

extern void Fspdev_Bin _ARGS_((void));
extern ReturnStatus FspdevPfsDomainInfo _ARGS_((Fs_FileID *fileIDPtr, 
			Fs_DomainInfo *domainInfoPtr));
extern void Fspdev_InitializeOps _ARGS_((void));

extern void Fspdev_PrintTrace _ARGS_((ClientData numRecs));

extern ReturnStatus Fspdev_PrintRec _ARGS_((ClientData clientData, int event,
			Boolean printHeaderFlag));

extern ReturnStatus Fspdev_TraceInit _ARGS_((void));


#endif _FSPDEVX
