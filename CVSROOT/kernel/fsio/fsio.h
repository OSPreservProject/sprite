/*
 * fsio.h --
 *
 *	Declarations of stream IO routines.
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

#ifndef _FSIO
#define _FSIO

#include "fs.h"

/*
 * Stream Types:
 *	FSIO_STREAM		Top level type for stream with offset.  Streams
 *				have an ID and are in the handle table to
 *				support migration and shared stream offsets.
 * The remaining types are for I/O handles
 *	FSIO_LCL_FILE_STREAM	For a regular disk file stored locally.
 *	FSIO_RMT_FILE_STREAM	For a remote Sprite file.
 *	FSIO_LCL_DEVICE_STREAM	For a device on this host.
 *	FSIO_RMT_DEVICE_STREAM	For a remote device.
 *	FSIO_LCL_PIPE_STREAM	For an anonymous pipe buffered on this host.
 *	FSIO_RMT_PIPE_STREAM	For an anonymous pipe bufferd on a remote host.
 *				This type arises from process migration.
 *	FSIO_CONTROL_STREAM	This is the stream used by the server for
 *				a pseudo device to listen for new clients.
 *	FSIO_SERVER_STREAM	The main state for a pdev request-response
 *				connection.  Refereneced by the server's stream.
 *	FSIO_LCL_PSEUDO_STREAM	A client's handle on a request-response
 *				connection to a local pdev server.
 *	FSIO_RMT_PSEUDO_STREAM	As above, but when the pdev server is remote.
 *	FSIO_PFS_CONTROL_STREAM	Control stream for pseudo-filesystems.
 *	FSIO_PFS_NAMING_STREAM	The request-response stream used for naming
 *				operations in a pseudo-filesystem.  This
 *				I/O handle is hung off the prefix table.
 *	FSIO_LCL_PFS_STREAM	A clients' handle on a request-response
 *				connection to a local pfs server.
 *	FSIO_RMT_PFS_STREAM	As above, but when the pfs server is remote.
 *	FSIO_RMT_CONTROL_STREAM	Needed only during get/set I/O attributes of
 *				a pseudo-device whose server is remote.
 *	FSIO_PASSING_STREAM	Used to pass streams from a pseudo-device
 *				server to its client in response to an open.
 *		Internet Protocols
 *	FSIO_RAW_IP_STREAM	Raw Internet Protocol stream.
 *	FSIO_UDP_STREAM		UDP protocol stream.
 *	FSIO_TCP_STREAM		TCP protocol stream.
 *
 * The following streams are not implemented
 *	FS_RMT_NFS_STREAM	NFS access implemented in kernel.
 *	FS_RMT_UNIX_STREAM	For files on the old hybrid unix/sprite server.
 *	FS_LCL_NAMED_PIPE_STREAM Stream to a named pipe whose backing file
 *				is on the local host.
 *	FS_RMT_NAMED_PIPE_STREAM Stream to a named pipe whose backing file
 *				is remote. 
 */
#define FSIO_STREAM			0
#define FSIO_LCL_FILE_STREAM		1
#define FSIO_RMT_FILE_STREAM		2
#define FSIO_LCL_DEVICE_STREAM		3
#define FSIO_RMT_DEVICE_STREAM		4
#define FSIO_LCL_PIPE_STREAM		5
#define FSIO_RMT_PIPE_STREAM		6
#define FSIO_CONTROL_STREAM		7
#define FSIO_SERVER_STREAM		8
#define FSIO_LCL_PSEUDO_STREAM		9
#define FSIO_RMT_PSEUDO_STREAM		10
#define FSIO_PFS_CONTROL_STREAM		11
#define FSIO_PFS_NAMING_STREAM		12
#define FSIO_LCL_PFS_STREAM		13
#define FSIO_RMT_PFS_STREAM		14
#define FSIO_RMT_CONTROL_STREAM		15
#define FSIO_PASSING_STREAM		16
#define FSIO_RAW_IP_STREAM		17
#define FSIO_UDP_STREAM			18
#define FSIO_TCP_STREAM			19

#define FSIO_NUM_STREAM_TYPES		20

/*
 * Two arrays are used to map between local and remote types.  This has
 * to happen when shipping Fs_FileIDs between clients and servers.
 */
extern int fsio_LclToRmtType[];
extern int fsio_RmtToLclType[];
/*
 * Fsio_MapLclToRmtType(type) - Maps from a local to a remote stream type.
 *	This returns -1 if the input type is out-of-range.
 */
#define Fsio_MapLclToRmtType(localType) \
    ( ((localType) < 0 || (localType) >= FSIO_NUM_STREAM_TYPES) ? -1 : \
	fsio_LclToRmtType[localType] )
/*
 * Fsio_MapRmtToLclType(type) - Maps from a remote to a local stream type.
 *	This returns -1 if the input type is out-of-range.
 */
#define Fsio_MapRmtToLclType(remoteType) \
    ( ((remoteType) < 0 || (remoteType) >= FSIO_NUM_STREAM_TYPES) ? -1 : \
	fsio_RmtToLclType[remoteType] )



/*
 * OPEN SWITCH
 * The srvOpen procedure is used on the server when opening streams or
 * setting up an I/O fileID for a file or device.  It is keyed on
 * disk file descriptor types( i.e. FS_FILE, FS_DIRECTORY, FS_DEVICE,
 * FS_PSEUDO_DEVICE).  The server open procedure returns an ioFileID
 * used for I/O on the file, plus other data needed for the client's
 * stream.  The streamIDPtr is NIL during set/get attributes, which
 * indicates that the extra stream information isn't needed.
 */

typedef struct Fsio_OpenOps {
    int		type;			/* One of the file descriptor types */
    /*
     * The calling sequence for the server-open routine is:
     *	FooSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
     *			sizePtr, dataPtr)
     *		Fsio_FileIOHandle	*handlePtr;
     *		int			clientID;
     *		int			useFlags;	(From the stream)
     *		Fs_FileID		ioFileIDPtr;	(Returned to client)
     *	   (The following arguments are ignored during set/get attrs)
     *	   (This case is indicated by a NIL streamIDPtr)
     *		Fs_FileID		streamIDPtr;	(Returned to client,)
     *		int			*sizePtr;	(Return size of data)
     *		ClientData		*dataPtr;	(Extra return data)
     */
    ReturnStatus (*srvOpen)();
} Fsio_OpenOps;

extern Fsio_OpenOps fsio_OpenOpTable[];



/*
 * STREAM SWITCH
 *	These procedures are called by top-level procedures (i.e. Fs_Read,
 *	or Fs_Select) to do stream-type specific processing.
 */

typedef struct Fsio_StreamTypeOps {
    int		type;			/* Stream types defined in fs.h */
    /*
     **************** Setup operation for clients. *************************
     *	This routine sets up an I/O handle for a stream.  It uses streamData
     *  that was genereated by the srvOpen routine on the file server.  As
     *  a side effect it fills in the nameInfoPtr->fileID for use later
     *  when getting/setting attributes.
     *
     *	FooCltOpen(fileIDPtr, flagsPtr, clientID, data, name, hdrPtrPtr)
     *		Fs_FileID	*fileIDPtr;	(indicates file)
     *		int		*flagsPtr;	(from the stream)
     *		int		clientID;	(who's opening it)
     *		ClientData	data;		(stream data from srvOpen)
     *		char 		*name;		(name for error messages)
     *		Fs_HandleHeader	**hdrPtrPtr;	(Returned I/O handle)
     */
    ReturnStatus (*cltOpen)();
    /*
     **************** Regular I/O operations. ******************************
     *  These are the standard read/write routines.  Note:  they are passed
     *  a stream pointer to support streams shared accross the network.
     *  A shared stream is indicated by FS_RMT_SHARED in the flags.  In this
     *  case the streamID should be passed to the I/O server who will use
     *  its own copy of the stream read/write offset.
     *  If not shared, the read/write routines are only guaranteed that the
     *  ioHandlePtr field of the stream is defined.  (The stream is partially
     *  defined this way on the server when a client is reading/writing its
     *  cache and there is no stream.)
     *
     *	FooRead(streamPtr, ioPtr, waitPtr, replyPtr)
     *	FooWrite(streamPtr, ioPtr, waitPtr, replyPtr)
     *		Fs_Stream	*streamPtr;	( !Only use ioHandlePtr field! )
     *		Fs_IOParam	*ioPtr;		(Standard parameter block)
     *		Sync_RemoteWaiter *waitPtr;	(For remote waiting)
     *		Fs_IOReply	*reply;		(For return length and signal)
     */
    ReturnStatus (*read)();
    ReturnStatus (*write)();
    /*
     ***************** I/O Control. ****************************************
     *  Stream-specific I/O controls.  The main procedure Fs_IOControl
     *	handles some generic I/O controls, and then passes the I/O control
     *	down to the stream-specific handler.
     *
     *	FooIOControl(hdrPtr, command, byteOrder, inBufSize, inBuf, outBufSize, outBuf)
     *		Fs_HandleHeader *hdrPtr;			(File handle)
     *		int		command;		(Special operation)
     *		int		byteOrder;		(client's byte order)
     *		int		inBufSize;		(Size of inBuf)
     *		Address		inBuf;			(Input data)
     *		int		outBufSize;		(Size of outBuf)
     *		Address		outBuf;			(Return data)
     */
    ReturnStatus (*ioControl)();
    /*
     ***************** Select. *********************************************
     *  The select interface includes three In/Out bit words.  There is
     *  at most one bit set in each, and the selectability can be easily
     *  turned off by clearing the word.  Never do anything to these
     *  words other than leave them alone or clear them.
     *
     *	FooSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
     *		Fs_HandleHeader		*hdrPtr;	(File handle)
     *		Sync_RemoteWaiter	*waitPtr;	(Remote waiting info)
     *		int			*readPtr;	(In/Out read bit)
     *		int			*writePtr;	(In/Out write bit)
     *		int			*exceptPtr;	(In/Out except bit)
     */
    ReturnStatus (*select)();
    /*
     **************** Attribute operations on open streams. *****************
     *	These just operate on the few attributes cached at the I/O server.
     *  The name server is first contacted to set/initialize the attributes,
     *  (either via the DOMAIN switch with pathnames,
     *  or via the ATTRIBUTE switch with open streams)
     *  and then these routines are called to complete the attributes from
     *  ones cached on the I/O server, or to update the ones cached there.
     *
     *	FooGetIOAttr(fileIDPtr, clientID, attrPtr)
     *	FooSetIOAttr(fileIDPtr, attrPtr, flags)
     *		Fs_FileID		*fileIDPtr;	(Identfies file)
     *		int			clientID;	(Client getting attrs)
     *		Fs_Attributes		*attrPtr;	(Attrs to set/update)
     *		int			flags;		(which attrs to set)
     */
    ReturnStatus (*getIOAttr)();
    ReturnStatus (*setIOAttr)();
    /*
     **************** Server verification of remote handle. *****************
     *	This returns the server's file handle given a client's fileID.
     *  There are two parts to this task.  The first is to map from the
     *  client's fileID.type (i.e. FSIO_RMT_DEVICE_STREAM) to the server's
     *  (i.e. FSIO_LCL_DEVICE_STREAM).  The second step is to check that the
     *  client is recorded in the clientList of the I/O handle.  The domain
     *	type is returned for use in naming operations.
     *
     *	Fs_HandleHeader *
     *	FooClientVerify(fileIDPtr, clientID, domainTypePtr)
     *		Fs_FileID	*fileIDPtr;		(Client's handle)
     *		int		clientID;		(The client hostID)
     *		int		*domainTypePtr;		(may be NIL)
     */
    Fs_HandleHeader *(*clientVerify)();
    /*
     *************** Migration calls. **************************************
     *
     *  The 'release' is called on the source client of migration via an RPC
     *		from the I/O server.  Its job is to release any referneces
     *		on the I/O handle that were due to a stream which has
     *		now migrated away from the source client.
     *  The 'migrate' is called from Fsio_DeencapStream to update client
     *		book-keeping to reflect the migration.  The version on
     *		remote clients just does an RPC to the I/O server to
     *		invoke the appropriate migrate routine there.
     *		Important:  If the FS_RMT_SHARED flag is present it means
     *		that there is still a stream on the original client and its
     *		references should not be removed from the I/O handle.
     *		Also, if the FS_NEW_STREAM flag is present it means that
     *		the stream is newly migrated to the client so references
     *		should be added for the dstClient.
     *  The 'migEnd' is called from Fsio_DeencapStream after the call to the
     *		migrate procedure, but only the first time the stream
     *		is migrated to the host.  (After that it suffices to
     *		add references to the existing stream.)
     *
     *	FooRelease(hdrPtr, flags)
     *		Fs_HandleHeader	*hdrPtr;		(File handle)
     *		int		flags;			(From the stream)
     *	FooMigEnd(migInfoPtr, size, data, hdrPtrPtr)
     *		FsMigInfo	migInfoPtr;		(Migration state)
     *		int		size;			(size of data)
     *		ClientData	data;			(data from migrate)
     *		Fs_HandleHeader	**hdrPtrPtr;		(Returned handle)
     *	FooSrvMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr sizePtr, dataPtr)
     *    	FsMigInfo	*migInfoPtr;		(Migration state)
     *		int		dstClientID;		(ID of target client)
     *		int		*flagsPtr;		(In/Out Stream flags)
     *		int		*offsetPtr;		(Return - new offset)
     *		int		*sizePtr;		(Return - size of data)
     *		Address		*dataPtr;		(Return data)
     */
    ReturnStatus (*release)();
    ReturnStatus (*migEnd)();
    ReturnStatus (*migrate)();
    /*
     *************** Recovery calls. ****************************************
     *  This (should be) two routines, one called on client host's top
     *	reopen at the server, and one called on the server in response
     *  to a client's reopen request.
     *
     *	FooReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
     *		(The hdrPtr is only valid on clients.)
     *		Fs_HandleHeader *hdrPtr;		(Handle to re-open)
     *		(The following are only valid on servers.)
     *		int		clientID;	(client doing the re-open)
     *		ClientData	inData;		(state from the client)
     *		int		*outSizePtr;	(sizeof outData)
     *		ClientData	*outDataPtr;	(state returned to client)
     */
    ReturnStatus (*reopen)();
    /*
     *************** Clean up operations. **********************************
     * 'scavenge' is called periodically to clean up unneeded handles.
     *		Important: the scavenge procedure must either remove or
     *		unlock the handle it is still in use.
     * 'clientKill' is called to clean up after dead clients.  This is
     *		called before the scavenge routine if a periodic check
     *		on the client fails, or called independently if some other
     *		communication failure occurs.
     * 'close' is called when the last reference to a stream is closed.
     *		This routine should clean up any use/reference counts
     *		due to the stream as indicated by the flags argument.
     *
     *	FooScavenge(hdrPtr)
     *		Fs_HandleHeader	*hdrPtr;		(File handle)
     *	FooClientKill(hdrPtr, clientID)
     *		Fs_HandleHeader	*hdrPtr;		(File handle)
     *		int		clientID;		(Client presumed down)
     *	FooClose(hdrPtr, clientID, procID, flags, size, data)
     *		Fs_HandleHeader	*hdrPtr;		(File handle)
     *		int		clientID;		(Host ID of closer)
     *		Proc_Pid	procID;			(ProcessID of closer)
     *		int		flags;			(From the stream)
     *		int		size;			(Size of data)
     *		ClientData	data;			(Extra close data)
     */
    Boolean	 (*scavenge)();
    void	 (*clientKill)();
    ReturnStatus (*close)();
} Fsio_StreamTypeOps;

extern Fsio_StreamTypeOps fsio_StreamOpTable[];

typedef struct FsioStreamClient {
    List_Links links;		/* This hangs off the stream */
    int		clientID;	/* The sprite ID of the client */
} FsioStreamClient;

/*
 * Structure that is transfered when a process is migrated.
 * SHOULD CHANGE NAME TO Fsio_MigInfo
 */

typedef struct FsMigInfo {
    Fs_FileID	streamID;	/* Stream identifier. */
    Fs_FileID    ioFileID;     	/* I/O handle for the stream. */
    Fs_FileID	nameID;		/* ID of name of the file.  Used for attrs. */
    Fs_FileID	rootID;		/* ID of the root of the file's domain. */
    int		srcClientID;	/* Client transfering from. */
    int         offset;     	/* File access position. */
    int         flags;      	/* Usage flags from the stream. */
} FsMigInfo;


/*
 * The following structures are subfields of the various I/O handles.
 * First we define a use count structure to handle common book keeping needs.
 * SHOULD CHANGE NAME TO Fsio_UseCounts
 */

typedef struct Fsutil_UseCounts {
    int		ref;		/* The number of referneces to handle */
    int		write;		/* The number of writers on handle */
    int		exec;		/* The number of executors of handle */
} Fsutil_UseCounts;


/*
 * Initialization
 */
extern void Fsio_InstallStreamOps();
extern void Fsio_InstallSrvOpenOp();

extern ReturnStatus Fsio_CreatePipe();

extern void Fsio_Bin();
extern void Fsio_InitializeOps();

/*
 * Stream client list functions.
 */
extern Boolean		Fsio_StreamClientOpen();
extern Boolean		Fsio_StreamClientClose();
extern Boolean		Fsio_StreamClientFind();

/*
 * Stream manipulation routines.
 */
extern Fs_Stream	*Fsio_StreamCreate();
extern Fs_Stream	*Fsio_StreamAddClient();
extern void		Fsio_StreamMigClient();
extern Fs_Stream	*Fsio_StreamClientVerify();
extern void		Fsio_StreamCreateID();
extern void		Fsio_StreamCopy();
extern void		Fsio_StreamDestroy();
extern Boolean		Fsio_StreamScavenge();
extern ReturnStatus	Fsio_StreamReopen();

/*
 * flock() support
 */
extern void		Fsio_LockInit();
extern ReturnStatus	Fsio_IocLock();
extern ReturnStatus	Fsio_Lock();
extern ReturnStatus	Fsio_Unlock();
extern void		Fsio_LockClose();
extern void		Fsio_LockClientKill();

/*
 * ftrunc() support
 */
extern ReturnStatus	Fsio_FileTrunc();


/*
 * Device support
 */
extern void Fsio_DevNotifyException();
extern void Fsio_DevNotifyReader();
extern void Fsio_DevNotifyWriter();

extern ReturnStatus Fsio_VanillaDevReopen();

/*
 * Migration support
 */
extern ReturnStatus	Fsio_EncapStream();
extern ReturnStatus	Fsio_DeencapStream();
extern ReturnStatus	Fsio_MigrateUseCounts();
extern void		Fsio_MigrateClient();

/*
 * Null procs for switch tables.
 */
extern void Fsio_NullClientKill();
extern ReturnStatus Fsio_NoProc();
extern ReturnStatus Fsio_NullProc();
extern Fs_HandleHeader *Fsio_NoHandle();


#endif
