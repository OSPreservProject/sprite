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
#include <fsprefix.h>
#include <fsrmt.h>
#include <dev/pdev.h>
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

/*
 * Both the pseudo-device and pseudo-filesystem implementation use a
 * control stream to note what host runs the master, and to keep a seed.
 * With pseudo-devices the control IO handle is hooked to a stream that
 * is returned to the server, and the server reads new streamIDs off
 * of this stream.  With pseudo-filesystems the server gets new streamIDs
 * via IOC_PFS_OPEN, instead, and the control stream is used to keep
 * a pointer to the prefix table entry that represents the pseudo-filesystem.
 *
 * There is a control handle kept on both on the file server and on the
 * host running the server process.  The one on the file server is used
 * by the SrvOpen routine to detect if a master exists, and the one on
 * the server's host is used for control messages, and it also is used
 * to detect if the server process is still alive (by looking at serverID).
 */
typedef struct Fspdev_ControlIOHandle {
    Fsrmt_IOHandle rmt;	/* FSIO_CONTROL_STREAM or FSIO_PFS_CONTROL_STREAM.
				 * This is a remote I/O handle in order to do
				 * a remote close to the name server so
				 * the serverID field gets cleaned up right. */
    int serverID;		/* Host ID of server process.  If NIL it
				 * means there is no server.  This is kept */
    int	seed;			/* Used to make FileIDs for client handles */
    /*
     * These fields are used to implement reading from a pdev control stream.
     */
    List_Links	queueHdr;	/* Control message queue, pdev's only */
    List_Links readWaitList;	/* So the server can wait for control msgs */
    Fsio_LockState lock;		/* So the server can flock() the pdev file */
    /*
     * Cached I/O attributes.
     */
    int		accessTime;	/* Time of last write operation */
    int		modifyTime;	/* Time of last read operation */
    /*
     *  IOC_SET/GET_OWNER support.
     */
    Ioc_Owner	owner;		/* Owning process or family */
    /*
     * This pointer is used to clean up the prefix table entry that the
     * naming request-response stream is hooked to (pseudo-filesystems)
     */
    Fsprefix *prefixPtr;	/* Prefix of pseudo-filesystem */
} Fspdev_ControlIOHandle;

/*
 * Circular buffers are used for a request buffer and a read data buffer.
 * These buffers are in the address space of the server process so the
 * server can access them without system calls.  The server uses I/O controls
 * to change the pointers.
 */
typedef struct Fspdev_CircBuffer {
    Address data;		/* Location of the buffer in user-space */
    int firstByte;		/* Byte index of first valid data in buffer.
				 * if -1 then the buffer is empty */
    int lastByte;		/* Byte index of last valid data in buffer. */
    int size;			/* Number of bytes in the circular buffer */
} Fspdev_CircBuffer;

/*
 * Fspdev_ServerIOHandle has the main state for a client-server connection.
 * The client's handle is a stub which just has a pointer to this handle.
 */
typedef struct Fspdev_ServerIOHandle {
    Fs_HandleHeader hdr;		/* Standard header, type FSIO_LCL_PSEUDO_STREAM */
    Sync_Lock lock;		/* Used to synchronize access to this struct. */
    int flags;			/* Flags bits are defined in fsPdev.c */
    int selectBits;		/* Select state of the pseudo-stream */
    Proc_PID serverPID;		/* Server's processID needed for copy out */
    Proc_PID clientPID;		/* Client's processID needed for copy out */
    Fspdev_CircBuffer	requestBuf;	/* Reference to server's request buffer.
				 * The kernel fills this buffer and the
				 * server takes the requests and data out */
    Address nextRequestBuffer;	/* The address of the next request buffer in
				 * the server's address space to use.  We let
				 * the server change buffers in mid-flight. */
    int nextRequestBufSize;	/* Size of the new request buffer */
    Fspdev_CircBuffer readBuf;		/* This buffer contains read-ahead data for
				 * the pseudo-device.  The server process puts
				 * data here and the kernel removes it to
				 * satisfy client reads. If non-existent,
				 * the kernel asks the server explicitly for
				 * read data with PDEV_READ requests */
    Pdev_Op operation;		/* Current operation.  Checked when handling
				 * the reply. */
    Pdev_Reply reply;		/* Server's reply message */
    Address replyBuf;		/* Pointer to reply data buffer.  This is in
				 * the client's address space if the
				 * FS_USER flag is set */
    int replySize;		/* Amount of data the client expects returned */
    Sync_Condition setup;	/* This is notified after the server has set
				 * up buffer space for us.  A pseudo stream
				 * can't be used until this is done. */
    Sync_Condition access;	/* Notified after a RequestResponse to indicate
				 * that another client process can use the
				 * pseudo-stream. */
    Sync_Condition caughtUp;	/* This is notified after the server has read
				 * or set the buffer pointers.  The kernel
				 * waits for the server to catch up
				 * before safely resetting the pointers to
				 * the beginning of the buffer */
    Sync_Condition replyReady;	/* Notified after the server has replied */
    List_Links srvReadWaitList;	/* To remember the server process waiting
				 * to read new pointer values. */
    Sync_RemoteWaiter clientWait;/* Client process info for I/O waiting */
    List_Links cltReadWaitList;	/* These lists are used to remember clients */
    List_Links cltWriteWaitList;/*   waiting to read, write, or detect */
    List_Links cltExceptWaitList;/*   exceptions on the pseudo-stream. */
    Fspdev_ControlIOHandle *ctrlHandlePtr;	/* Back pointer to control stream */
    /*
     * The following fields support pseudo-filesystems.
     */
    Fs_FileID	userLevelID;	/* User-defined FileID for connections to
				 * pseudo-filesystem servers.  This is passed
				 * as 'prefixID' of name operation arguments
				 * to represent lookup starting points. */
    struct {			/* Info needed to set up new pdev connection */
	int clientID;		/* Host ID of client doing PFS_OPEN */
	int useFlags;		/* Usage flags of the open */
	char *name;		/* Name of pseudo-file, for handle headers */
    } open;
} Fspdev_ServerIOHandle;

/*
 * The client side stream for a pseudo-device.  This keeps a reference
 * to the server's handle with all the state.
 */
typedef struct Fspdev_ClientIOHandle {
    Fs_HandleHeader	hdr;
    Fspdev_ServerIOHandle	*pdevHandlePtr;
    List_Links		clientList;
    struct Vm_Segment	*segPtr;	/* Reference to code segment needed
					 * to flush VM cache. JMS */
} Fspdev_ClientIOHandle;

extern void Fspdev_Bin _ARGS_((void));
extern ReturnStatus FspdevPfsDomainInfo _ARGS_((Fs_FileID *fileIDPtr, 
			Fs_DomainInfo *domainInfoPtr));
extern void Fspdev_InitializeOps _ARGS_((void));

extern ReturnStatus Fspdev_PrintRec _ARGS_((ClientData clientData, int event,
			Boolean printHeaderFlag));

#endif /* _FSPDEVX */
