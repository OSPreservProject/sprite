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

#ifndef _FSPDEVINT
#define _FSPDEVINT

#include "trace.h"
#include "dev/pdev.h"
#include "fsprefix.h"
#include "fsrmt.h"
#include "fsioLock.h"
#include "fspdev.h"

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
typedef struct FspdevControlIOHandle {
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
} FspdevControlIOHandle;

/*
 * Because there are corresponding control handles on the file server,
 * which records which host has the pdev server, and on the pdev server
 * itself, we need to be able to reopen the control handle on the
 * file server after it reboots.
 */
typedef struct FspdevControlReopenParams {
    Fs_FileID	fileID;		/* FileID of the control handle */
    int		serverID;	/* ServerID recorded in control handle.
				 * This may be NIL if the server closes
				 * while the file server is down. */
    int		seed;		/* Used to create unique pseudo-stream fileIDs*/
} FspdevControlReopenParams;

/*
 * The following control messages are passed internally from the
 * ServerStreamCreate routine to the FspdevControlRead routine.
 * They contain a streamPtr for a new server stream
 * that gets converted to a user-level streamID in FspdevControlRead.
 */

typedef struct FspdevNotify {
    List_Links links;
    Fs_Stream *streamPtr;
} FspdevNotify;

/*
 * Circular buffers are used for a request buffer and a read data buffer.
 * These buffers are in the address space of the server process so the
 * server can access them without system calls.  The server uses I/O controls
 * to change the pointers.
 */
typedef struct FspdevCircBuffer {
    Address data;		/* Location of the buffer in user-space */
    int firstByte;		/* Byte index of first valid data in buffer.
				 * if -1 then the buffer is empty */
    int lastByte;		/* Byte index of last valid data in buffer. */
    int size;			/* Number of bytes in the circular buffer */
} FspdevCircBuffer;

/*
 * FspdevServerIOHandle has the main state for a client-server connection.
 * The client's handle is a stub which just has a pointer to this handle.
 */
typedef struct FspdevServerIOHandle {
    Fs_HandleHeader hdr;		/* Standard header, type FSIO_LCL_PSEUDO_STREAM */
    Sync_Lock lock;		/* Used to synchronize access to this struct. */
    int flags;			/* Flags bits are defined in fsPdev.c */
    int selectBits;		/* Select state of the pseudo-stream */
    Proc_PID serverPID;		/* Server's processID needed for copy out */
    Proc_PID clientPID;		/* Client's processID needed for copy out */
    FspdevCircBuffer	requestBuf;	/* Reference to server's request buffer.
				 * The kernel fills this buffer and the
				 * server takes the requests and data out */
    Address nextRequestBuffer;	/* The address of the next request buffer in
				 * the server's address space to use.  We let
				 * the server change buffers in mid-flight. */
    int nextRequestBufSize;	/* Size of the new request buffer */
    FspdevCircBuffer readBuf;		/* This buffer contains read-ahead data for
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
    FspdevControlIOHandle *ctrlHandlePtr;	/* Back pointer to control stream */
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
} FspdevServerIOHandle;

/*
 * The client side stream for a pseudo-device.  This keeps a reference
 * to the server's handle with all the state.
 */
typedef struct FspdevClientIOHandle {
    Fs_HandleHeader	hdr;
    FspdevServerIOHandle	*pdevHandlePtr;
    List_Links		clientList;
} FspdevClientIOHandle;

/*
 * The following types and macros are used to take pdev traces.
 */
typedef enum { PDEVT_NIL, PDEVT_SRV_OPEN, PDEVT_CLT_OPEN, PDEVT_READ_AHEAD,
	       PDEVT_SRV_CLOSE, PDEVT_CLT_CLOSE, PDEVT_READ_WAIT, 
	       PDEVT_WAKEUP, PDEVT_REQUEST, PDEVT_REPLY,
	       PDEVT_SRV_READ, PDEVT_SRV_READ_WAIT, PDEVT_SRV_WRITE,
	       PDEVT_SRV_SELECT, PDEVT_CNTL_READ, PDEVT_WAIT_LIST,
	       PDEVT_SELECT} FspdevTraceRecType ;

typedef struct FspdevTraceRecord {
    int index;
    Fs_FileID fileID;
    union {
	Pdev_RequestHdr	requestHdr;
	Pdev_Reply	reply;
	int		selectBits;
	struct WaitTrace {
	    int		selectBits;
	    Proc_PID	procID;
	} wait;
	Fsutil_UseCounts	use;
    } un;
} FspdevTraceRecord;

#ifndef CLEAN

#define PDEV_TRACE(fileIDPtr, event) \
    if (fspdevTracing) { \
	FspdevTraceRecord rec; \
	rec.index = ++fspdevTraceIndex; \
	rec.fileID = *(fileIDPtr); \
	Trace_Insert(fspdevTraceHdrPtr, event, (ClientData)&rec);	\
    }

#define PDEV_REQUEST(fileIDPtr, requestHdrPtr) \
    if (fspdevTracing) {							\
	FspdevTraceRecord rec; \
	rec.index = ++fspdevTraceIndex; \
	rec.fileID = *(fileIDPtr); \
	rec.un.requestHdr = *(requestHdrPtr); \
	Trace_Insert(fspdevTraceHdrPtr, PDEVT_REQUEST, (ClientData)&rec);\
    }

#define PDEV_REPLY(fileIDPtr, replyPtr) \
    if (fspdevTracing) {							\
	FspdevTraceRecord rec; \
	rec.index = ++fspdevTraceIndex; \
	rec.fileID = *(fileIDPtr); \
	rec.un.reply = *(replyPtr); \
	Trace_Insert(fspdevTraceHdrPtr, PDEVT_REPLY, (ClientData)&rec);\
    }

#define PDEV_TSELECT(fileIDPtr, read, write, except) \
    if (fspdevTracing) {							\
	FspdevTraceRecord rec; \
	rec.index = ++fspdevTraceIndex; \
	rec.fileID = *(fileIDPtr); \
	rec.un.selectBits = 0; \
	if (read) { rec.un.selectBits |= FS_READABLE; } \
	if (write) { rec.un.selectBits |= FS_WRITABLE; } \
	if (except) { rec.un.selectBits |= FS_EXCEPTION; } \
	Trace_Insert(fspdevTraceHdrPtr, PDEVT_SELECT, (ClientData)&rec);\
    }

#define PDEV_WAKEUP(fileIDPtr, zprocID, zselectBits) \
    if (fspdevTracing) {							\
	FspdevTraceRecord rec; \
	rec.index = ++fspdevTraceIndex; \
	rec.fileID = *(fileIDPtr); \
	rec.un.wait.selectBits = zselectBits; \
	rec.un.wait.procID = zprocID; \
	Trace_Insert(fspdevTraceHdrPtr, PDEVT_WAKEUP, (ClientData)&rec);\
    }
#define DBG_PRINT(fmt)	if (fspdev_Debug) { printf fmt ; }

#define PDEV_REQUEST_PRINT(fileIDPtr, requestHdrPtr) \
    switch(requestHdrPtr->operation) {	\
	case PDEV_OPEN:	\
	    DBG_PRINT( ("Pdev %d,%d: Open  ", (fileIDPtr)->major, \
					     (fileIDPtr)->minor) ); \
	    break;	\
	case PDEV_READ:	\
	    DBG_PRINT( ("Pdev %d,%d: Read  ", (fileIDPtr)->major, \
					     (fileIDPtr)->minor) ); \
	    break;	\
	case PDEV_WRITE:	\
	    DBG_PRINT( ("Pdev %d,%d: Write ", (fileIDPtr)->major, \
					     (fileIDPtr)->minor) ); \
	    break;	\
	case PDEV_CLOSE:	\
	    DBG_PRINT( ("Pdev %d,%d: Close ", (fileIDPtr)->major, \
					     (fileIDPtr)->minor) ); \
	    break;	\
	case PDEV_IOCTL:	\
	    DBG_PRINT( ("Pdev %d,%d: Ioctl ", (fileIDPtr)->major, \
					     (fileIDPtr)->minor) ); \
	    break;	\
	default:	\
	    DBG_PRINT( ("Pdev %d,%d: ?!?   ", (fileIDPtr)->major, \
					     (fileIDPtr)->minor) ); \
	    break;	\
    }
#else
/*
 * Compiling with -DCLEAN will zap the if statements and procedure
 * calls defined by the above macros
 */
#define DBG_PRINT(fmt)

#define PDEV_TRACE(fileIDPtr, event)
#define PDEV_REQUEST(fileIDPtr, requestHdrPtr)
#define PDEV_REPLY(fileIDPtr, replyPtr)
#define PDEV_TSELECT(fileIDPtr, read, write, except)
#define PDEV_WAKEUP(fileIDPtr, waitInfoPtr, selectBits)

#endif not CLEAN

/*
 * Internal Pdev routines
 */
extern ReturnStatus	FspdevSignalOwner();
extern FspdevClientIOHandle *FspdevConnect();

/*
 * Definitions for a trace of the request-response protocol.
 */
extern Trace_Header fspdevTraceHdr;
extern Trace_Header *fspdevTraceHdrPtr;
extern int fspdevTraceLength;
extern Boolean fspdevTracing;
extern int fspdevMaxTraceDataSize;
extern int fspdevTraceIndex;
/*
 * File server open-time routines.
 */
extern ReturnStatus	FspdevNameOpen();
extern ReturnStatus	FspdevRmtLinkNameOpen();
/*
 * Control Stream routines.
 */
extern FspdevControlIOHandle *FspdevControlHandleInit();
extern ReturnStatus	FspdevControlIoOpen();
extern ReturnStatus	FspdevControlRead();
extern ReturnStatus	FspdevControlIOControl();
extern ReturnStatus	FspdevControlSelect();
extern ReturnStatus	FspdevControlGetIOAttr();
extern ReturnStatus	FspdevControlSetIOAttr();
extern Fs_HandleHeader  *FspdevControlVerify();
extern ReturnStatus	FspdevControlReopen();
extern Boolean		FspdevControlScavenge();
extern void		FspdevControlClientKill();
extern ReturnStatus	FspdevControlClose();
/*
 * Pfs Control Stream routines.
 */
extern ReturnStatus	FspdevPfsIoOpen();
extern Fs_HandleHeader  *FspdevPfsControlVerify();
/*
 * Server stream routines.
 */
extern ReturnStatus	FspdevServerStreamRead();
extern ReturnStatus	FspdevServerStreamIOControl();
extern ReturnStatus	FspdevServerStreamSelect();
extern ReturnStatus	FspdevServerStreamClose();
/*
 * Pseudo-device (client-side) streams
 */
extern ReturnStatus	FspdevPseudoStreamIoOpen();
extern ReturnStatus	FspdevPseudoStreamOpen();
extern ReturnStatus	FspdevPseudoStreamRead();
extern ReturnStatus	FspdevPseudoStreamWrite();
extern ReturnStatus	FspdevPseudoStreamIOControl();
extern ReturnStatus	FspdevPseudoStreamSelect();
extern ReturnStatus	FspdevPseudoStreamGetIOAttr();
extern ReturnStatus	FspdevPseudoStreamSetIOAttr();
extern ReturnStatus	FspdevPseudoStreamMigClose();
extern ReturnStatus	FspdevPseudoStreamMigOpen();
extern ReturnStatus	FspdevPseudoStreamMigrate();
extern ReturnStatus	FspdevPseudoStreamClose();
extern void		FspdevPseudoStreamCloseInt();

extern FspdevServerIOHandle *FspdevServerStreamCreate();

/*
 * Remote pseudo-device streams
 */
extern ReturnStatus	FspdevRmtPseudoStreamIoOpen();
extern Fs_HandleHeader  *FspdevRmtPseudoStreamVerify();
extern ReturnStatus	FspdevRmtPseudoStreamMigrate();
/*
 * Local and remote pseudo-device streams to pseudo-file-systems
 */
extern ReturnStatus	FspdevPfsStreamIoOpen();
extern ReturnStatus	FspdevRmtPfsStreamIoOpen();
/*
 * Naming Stream routines.
 */
extern ReturnStatus	FspdevPfsExport();
extern ReturnStatus	FspdevPfsNamingIoOpen();
/*
 * Pseudo-file-system naming routines.
 */
extern ReturnStatus	FspdevPfsOpen();
extern ReturnStatus	FspdevPfsGetAttrPath();
extern ReturnStatus	FspdevPfsSetAttrPath();
extern ReturnStatus	FspdevPfsMakeDir();
extern ReturnStatus	FspdevPfsMakeDevice();
extern ReturnStatus	FspdevPfsRemove();
extern ReturnStatus	FspdevPfsRemoveDir();
extern ReturnStatus	FspdevPfsRename();
extern ReturnStatus	FspdevPfsHardLink();
extern ReturnStatus FspdevPseudoStream2Path();
extern ReturnStatus FspdevPseudoStreamLookup();

/*
 * Pseudo-file-system routines given an open file.
 */
extern ReturnStatus	FspdevPseudoGetAttr();
extern ReturnStatus	FspdevPseudoSetAttr();

extern Boolean		FspdevPdevServerOK();

extern ReturnStatus FspdevPassStream();

extern int FspdevPfsOpenConnection();


#endif _FSPDEVINT
