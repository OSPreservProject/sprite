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

#include <fs.h>

/*
 * Structure that is transfered when a stream is migrated with a process
 */

typedef struct Fsio_MigInfo {
    Fs_FileID	streamID;	/* Stream identifier. */
    Fs_FileID    ioFileID;     	/* I/O handle for the stream. */
    Fs_FileID	nameID;		/* ID of name of the file.  Used for attrs. */
    Fs_FileID	rootID;		/* ID of the root of the file's domain. */
    int		srcClientID;	/* Client transfering from. */
    int         offset;     	/* File access position. */
    int         flags;      	/* Usage flags from the stream. */
} Fsio_MigInfo;

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
 *		Internet Protocols (Not implemented in the kernel (yet?))
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
 * STREAM SWITCH
 *	These procedures are called by top-level procedures (i.e. Fs_Read,
 *	or Fs_Select) to do stream-type specific processing.
 */

typedef struct Fsio_StreamTypeOps {
    int		type;			/* Stream types defined in fs.h */
    /*
     **************** Setup operation for clients. *************************
     *	This routine sets up an I/O handle for a stream.  It uses streamData
     *  that was genereated by the nameOpen routine on the file server.  As
     *  a side effect it fills in the nameInfoPtr->fileID for use later
     *  when getting/setting attributes.
     *
     *	FooIoOpen(ioFileIDPtr, flagsPtr, clientID, data, name, hdrPtrPtr)
     *		Fs_FileID	*ioFileIDPtr;	(indicates object)
     *		int		*flagsPtr;	(from the stream)
     *		int		clientID;	(who's opening it)
     *		ClientData	data;		(stream data from nameOpen)
     *		char 		*name;		(name for error messages)
     *		Fs_HandleHeader	**hdrPtrPtr;	(Returned I/O handle)
     */
    ReturnStatus (*ioOpen) _ARGS_((Fs_FileID *ioFileIDPtr, int *flagsPtr, 
				int clientID, ClientData streamData, 
				char *name, Fs_HandleHeader **ioHandlePtrPtr));

    /*
     **************** Regular I/O operations. ******************************
     *  These are the standard read/write routines.  Note:  they are passed
     *  a stream pointer to support streams shared accross the network.
     *  A shared stream is indicated by FS_RMT_SHARED in the ioPtr->flags.
     *	Note that only the fileID and the ioHandlePtr of the streamPtr
     *	is guaranteed to be valid.  The stream-specific routine should ignore
     *	the flags and offset kept (or not kept) in the stream structure.
     *
     *	FooRead(streamPtr, ioPtr, waitPtr, replyPtr)
     *	FooWrite(streamPtr, ioPtr, waitPtr, replyPtr)
     *		Fs_Stream	*streamPtr;	(See above about valid fields )
     *		Fs_IOParam	*ioPtr;		(Standard parameter block)
     *		Sync_RemoteWaiter *waitPtr;	(For remote waiting)
     *		Fs_IOReply	*reply;		(For return length and signal)
     */
    ReturnStatus (*read) _ARGS_((Fs_Stream *streamPtr, Fs_IOParam *readPtr, 
				Sync_RemoteWaiter *remoteWaitPtr, 
				Fs_IOReply *replyPtr));
    ReturnStatus (*write) _ARGS_((Fs_Stream *streamPtr, Fs_IOParam *writePtr, 
                                  Sync_RemoteWaiter *remoteWaitPtr, 
				  Fs_IOReply *replyPtr));
    /*
     **************** VM I/O operations. ******************************
     *  These are the read/write routines used by VM during paging.
     *	The interface is the same as the regular read and write routines,
     *	so those routines can be re-used, if appropriate.
     *
     *	FooPageRead(streamPtr, ioPtr, waitPtr, replyPtr)
     *	FooPageWrite(streamPtr, ioPtr, waitPtr, replyPtr)
     *		Fs_Stream	*streamPtr;	(See above about valid fields)
     *		Fs_IOParam	*ioPtr;		(Standard parameter block)
     *		Sync_RemoteWaiter *waitPtr;	(For remote waiting)
     *		Fs_IOReply	*reply;		(For return length and signal)
     * 
     *	FooBlockCopy(srcHdrPtr, destHdrPtr, blockNum)
     * 		Fs_HandleHeader *srcHdrPtr;  ( Handle for source file. )
     * 		Fs_HandleHeader *destHdrPtr; ( Handle for dest file. )
     * 		int		*blockNum;   ( Block number to copy. )
     */
    ReturnStatus (*pageRead) _ARGS_((Fs_Stream *streamPtr, Fs_IOParam *readPtr, 
				Sync_RemoteWaiter *remoteWaitPtr, 
				Fs_IOReply *replyPtr));
    ReturnStatus (*pageWrite) _ARGS_((Fs_Stream *streamPtr,
				      Fs_IOParam *writePtr, 
                                      Sync_RemoteWaiter *remoteWaitPtr, 
				      Fs_IOReply *replyPtr));
    ReturnStatus (*blockCopy) _ARGS_((Fs_HandleHeader *srcHdrPtr, 
					Fs_HandleHeader *dstHdrPtr, 
					int blockNum));
    /*
     ***************** I/O Control. ****************************************
     *  Stream-specific I/O controls.  The main procedure Fs_IOControl
     *	handles some generic I/O controls, and then passes the I/O control
     *	down to the stream-specific handler.
     *
     */
    ReturnStatus (*ioControl) _ARGS_((Fs_Stream *streamPtr, 
				      Fs_IOCParam *ioctlPtr, 
				      Fs_IOReply *replyPtr));
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
    ReturnStatus (*select)  _ARGS_((Fs_HandleHeader *hdrPtr, 
				    Sync_RemoteWaiter *waitPtr, int *readPtr, 
				    int *writePtr, int *exceptPtr));
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
    ReturnStatus (*getIOAttr) _ARGS_((Fs_FileID *fileIDPtr, int clientID, 
				      Fs_Attributes *attrPtr));
    ReturnStatus (*setIOAttr) _ARGS_((Fs_FileID *fileIDPtr, 
				      Fs_Attributes *attrPtr, int flags));
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
    Fs_HandleHeader *(*clientVerify) _ARGS_((Fs_FileID *fileIDPtr, 
					     int clientID, int *domainTypePtr));
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
     *		Fsio_MigInfo	migInfoPtr;		(Migration state)
     *		int		size;			(size of data)
     *		ClientData	data;			(data from migrate)
     *		Fs_HandleHeader	**hdrPtrPtr;		(Returned handle)
     *	FooSrvMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr sizePtr, dataPtr)
     *    	Fsio_MigInfo	*migInfoPtr;		(Migration state)
     *		int		dstClientID;		(ID of target client)
     *		int		*flagsPtr;		(In/Out Stream flags)
     *		int		*offsetPtr;		(Return - new offset)
     *		int		*sizePtr;		(Return - size of data)
     *		Address		*dataPtr;		(Return data)
     */
    ReturnStatus (*release) _ARGS_((Fs_HandleHeader *hdrPtr, int flags));
    ReturnStatus (*migEnd) _ARGS_((Fsio_MigInfo *migInfoPtr, int size, 
				  ClientData data, 
				  Fs_HandleHeader **hdrPtrPtr));
    ReturnStatus (*migrate) _ARGS_((Fsio_MigInfo *migInfoPtr, int dstClientID, 
				    int *flagsPtr, int *offsetPtr, 
				    int *sizePtr, Address *dataPtr));
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
    ReturnStatus (*reopen)  _ARGS_((Fs_HandleHeader *hdrPtr, int clientID, 
				    ClientData inData, int *outSizePtr,
				    ClientData *outDataPtr));
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
    Boolean	 (*scavenge) _ARGS_((Fs_HandleHeader *hdrPtr));
    void	 (*clientKill) _ARGS_((Fs_HandleHeader *hdrPtr, int clientID));
#ifdef SOSP91
    ReturnStatus (*close) _ARGS_((Fs_Stream *streamPtr, int clientID, 
				  Proc_PID procID, int flags, int dataSize, 
				  ClientData closeData, int *offsetPtr,
				  int *rwFlagsPtr));
#else
    ReturnStatus (*close) _ARGS_((Fs_Stream *streamPtr, int clientID, 
				  Proc_PID procID, int flags, int dataSize, 
				  ClientData closeData));
#endif
} Fsio_StreamTypeOps;

extern Fsio_StreamTypeOps fsio_StreamOpTable[];

typedef struct FsioStreamClient {
    List_Links links;		/* This hangs off the stream */
    int		clientID;	/* The sprite ID of the client */
} FsioStreamClient;



/*
 * The following structures are subfields of the various I/O handles.
 * First we define a use count structure to handle common book keeping needs.
 */

typedef struct Fsio_UseCounts {
    int		ref;		/* The number of referneces to handle */
    int		write;		/* The number of writers on handle */
    int		exec;		/* The number of executors of handle */
} Fsio_UseCounts;

/*
 * Exported type for async I/O requests.
 */

typedef struct Fsio_Request *Fsio_RequestToken;

extern Fsio_RequestToken Fsio_DeviceBlockIOAsync();
extern Fsio_DeviceBlockIOPoll();


/*
 * Recovery testing switch table.
 */
typedef struct Fsio_RecovTestInfo {
    int		(*refFunc)();		/* number of references */
    int		(*numBlocksFunc)();	/* number of blocks in the cache */
    int		(*numDirtyBlocksFunc)();/* number of dirty blocks in cache */
} Fsio_RecovTestInfo;

extern	Fsio_RecovTestInfo	fsio_StreamRecovTestFuncs[];

/*
 * Recovery testing operations.
 */
extern	int	Fsio_FileRecovTestUseCount();
extern	int	Fsio_FileRecovTestNumCacheBlocks();
extern	int	Fsio_FileRecovTestNumDirtyCacheBlocks();
extern	int	Fsio_DeviceRecovTestUseCount();
extern	int	Fsio_PipeRecovTestUseCount();

/*
 * Initialization
 */
extern void Fsio_InstallStreamOps _ARGS_((int streamType,
			Fsio_StreamTypeOps *streamOpsPtr));

extern ReturnStatus Fsio_CreatePipe _ARGS_((Fs_Stream **inStreamPtrPtr, 
			Fs_Stream **outStreamPtrPtr));

extern void Fsio_Bin _ARGS_((void));
extern void Fsio_InitializeOps _ARGS_((void));

/*
 * Stream client list functions.
 */
extern Boolean Fsio_StreamClientOpen _ARGS_((List_Links *clientList,
			int clientID, int useFlags, Boolean *foundPtr));
extern Boolean Fsio_StreamClientClose _ARGS_((List_Links *clientList, 
			int clientID));
extern Boolean Fsio_StreamClientFind _ARGS_((List_Links *clientList, 
			int clientID));

/*
 * Stream manipulation routines.
 */
extern Fs_Stream *Fsio_StreamCreate _ARGS_((int serverID, int clientID,
		    Fs_HandleHeader *ioHandlePtr, int useFlags, char *name));
extern Fs_Stream *Fsio_StreamAddClient _ARGS_((Fs_FileID *streamIDPtr,
			int clientID, Fs_HandleHeader *ioHandlePtr, 
			int useFlags, char *name, Boolean *foundClientPtr, 
			Boolean *foundStreamPtr));
extern void Fsio_StreamMigClient _ARGS_((Fsio_MigInfo *migInfoPtr, 
			int dstClientID, Fs_HandleHeader *ioHandlePtr, 
			Boolean *closeSrcClientPtr));
extern Fs_Stream *Fsio_StreamClientVerify _ARGS_((Fs_FileID *streamIDPtr, 
			Fs_HandleHeader *ioHandlePtr, int clientID));
extern void Fsio_StreamCreateID _ARGS_((int serverID, Fs_FileID *streamIDPtr));
extern void Fsio_StreamCopy _ARGS_((Fs_Stream *oldStreamPtr,
			Fs_Stream **newStreamPtrPtr));
extern void Fsio_StreamDestroy _ARGS_((Fs_Stream *streamPtr));
extern ReturnStatus Fsio_StreamMigClose _ARGS_((Fs_Stream *streamPtr, 
			Boolean *inUsePtr));
extern ReturnStatus Fsio_StreamMigCloseNew _ARGS_((Fs_Stream *streamPtr, 
			Boolean *inUsePtr, int *offsetPtr));
extern ReturnStatus Fsio_StreamReopen _ARGS_((Fs_HandleHeader *hdrPtr, 
			int clientID, ClientData inData, int *outSizePtr, 
			ClientData *outDataPtr));



/*
 * Migration support
 */
extern ReturnStatus Fsio_EncapStream _ARGS_((Fs_Stream *streamPtr, 
			Address bufPtr));
extern ReturnStatus Fsio_DeencapStream _ARGS_((Address bufPtr,
			Fs_Stream **streamPtrPtr));
extern void Fsio_MigrateUseCounts _ARGS_((int flags, 
			Boolean closeSrcClient, Fsio_UseCounts *usePtr));
extern void Fsio_MigrateClient _ARGS_((List_Links *clientList, int srcClientID,
			int dstClientID, int flags, Boolean closeSrcClient));

/*
 * Null procs for switch tables.
 */
extern void Fsio_NullClientKill _ARGS_((Fs_HandleHeader *hdrPtr, int clientID));
extern ReturnStatus Fsio_NullProc ();
extern ReturnStatus Fsio_NoProc ();
extern Fs_HandleHeader *Fsio_NoHandle ();


#endif
