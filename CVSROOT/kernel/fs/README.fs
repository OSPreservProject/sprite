This file describes various indirection tables used in the file system.
I hope that this will be useful when trying to follow the
control flow through function tables.  The things here are ripped out
of various source files.
--Ken Shirriff 7/90

******************************************************************************
But before we get to that, here is a map of what happens on an Fs_Open call.
This traces the path through the different switch tables.   --Mary Baker 9/91

				Fs_Open
1) Fsprefix_LookupOperation(name, operation, ..., &openResults, ...)
2) streamPtr = Fsio_StreamAddClient(&openResults.streamID, ...)
3) fsio_StreamOpTable[type].ioOpen(openResults and streamData)

1) Fsprefix_LookupOperation(operation)
    domainType = GetPrefix(fileName)
    fs_DomainLookup[domainType][operation]
		//					\\
    op=FS_DOMAIN_OPEN, domainType=FS_LOCAL_DOMAIN	REMOTE
	//						  \\
FslclOpen						FsrmtOpen (see below)
    FslclLookup()
    fsio_OpenOpTable[descPtr->fileType].nameOpen
	    //				\\				
	type=FS_FILE			type=FS_DEVICE
	//				  \\
Fsio_FileNameOpen			Fsio_DeviceNameOpen
    Does some "ioOpen"			    Sets up ioServerID and whether
    stuff to avoid doing		    device is remote or local to
    extra rpc for files:		    client.  This is for later
    cache consistency.			    ioOpen operation.



FsrmtOpen (continued)
    RPC to Fsrmt_RpcOpen
	||
        \/
Fsrmt_RpcOpen
    prefixHandlePtr =
	    fsio_StreamOpTable[prefixID.type].clientVerify(..., &domainType, ..)
    fs_DomainLookup[domainType][operation]
	    //
	op=FS_DOMAIN_OPEN, domainType=FS_LOCAL_DOMAIN
	 //
FslclOpen (as above)



3) fsio_StreamOpTable[type].ioOpen
	    //				||			\\
    type=FSIO_LCL_FILE_STREAM	FSIO_RMT_FILE_STREAM	FSIO_RMT_DEVICE_STREAM
	 //				||			  \\
Fsio_FileIoOpen			FsrmtFileIoOpen		FsrmtDeviceIoOpen
    Sets up stream for local	    Set up a stream	    Sets type to LCL.
    disk file.  Consistency	    for a remote file.	    Fsrmt_DeviceOpen
    and stuff was done in	    This means setting		(see below)
    name open, above.		    up the recovery
				    use counts.

Fsrmt_DeviceOpen (continued)
    RPC to Fsrmt_RpcDevOpen
	||
	\/
Fsrmt_RpcDevOpen
    Maps rmt to lcl type.
    fsio_StreamOpTable[type].ioOpen
	    //
	type=FSIO_LCL_DEVICE_STREAM
	 //
Fsio_DeviceIoOpen
    devFsOpTable[devHandlePtr->device.type].open to device-specific open proc.
    consistency

******************************************************************************
And now back to our regularly-scheduled programming:

/*
 * OPEN SWITCH
 * The nameOpen procedure is used on the file server when opening streams or
 * setting up an I/O fileID for a file or device.  It is keyed on
 * disk file descriptor types( i.e. FS_FILE, FS_DIRECTORY, FS_DEVICE,
 * FS_PSEUDO_DEVICE).  The nameOpen procedure returns an ioFileID
 * used for I/O on the file, plus other data needed for the client's
 * stream.  The streamIDPtr is NIL during set/get attributes, which
 * indicates that the extra stream information isn't needed.
 */

typedef struct Fsio_OpenOps {
    int		type;			/* One of the file descriptor types */
    /*
     * The calling sequence for the nameOpen routine is:
     *	FooNameOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
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
    ReturnStatus (*nameOpen)();
} Fsio_OpenOps;
/*
 * The OpenOps are called to do preliminary open-time processing.  They
 * are called after pathname resolution has obtained a local file I/O handle
 * that represents a name of some object.  The NameOpen routine maps
 * the attributes kept with the local file into an objectID (Fs_FileID)
 * and any other information needed to create an I/O stream to the object.
 */
This goes into fsio_OpenOpTable
static Fsio_OpenOps ioOpenOps[] = { 
    /*
     * FILE through SYMBOLIC_LINK are all the same.
     */
    { FS_FILE, Fsio_FileNameOpen },
    { FS_DIRECTORY, Fsio_FileNameOpen },
    { FS_SYMBOLIC_LINK, Fsio_FileNameOpen },
    /*
     * Remote devices are opened like ordinary devices, so the old
     * remote-device type is unused.
     */
    { FS_DEVICE, Fsio_DeviceNameOpen },
    /*
     * Local pipes.
     */
    { FS_LOCAL_PIPE, Fsio_NoProc},
    /*
     * Named pipes.  Unimplemented, treat like a regular file.
     */
    { FS_NAMED_PIPE, Fsio_FileNameOpen },
    /*
     * Special file type for testing new kinds of files.
     */
    { FS_XTRA_FILE, Fsio_FileNameOpen},
};
    /*
     * Pseudo devices.
     */
static Fsio_OpenOps pdevOpenOps[] = { 
    /*
     * Pseudo devices.
     */
    {FS_PSEUDO_DEV, FspdevNameOpen},
    { FS_PSEUDO_FS, Fsio_NoProc},
    /*
     * A remote link can either be treated like a regular file,
     * or opened by a pseudo-filesystem server.
     */
    { FS_REMOTE_LINK, FspdevRmtLinkNameOpen },
};

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
    ReturnStatus (*ioOpen)();
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
    ReturnStatus (*read)();
    ReturnStatus (*write)();
    /*
     **************** VM I/O operations. ******************************
     *  These are the read/write routines used by VM during paging.
     *	The interface is the same as the regular read and write routines,
     *	so those routines can be re-used, if appropriate.
     *
     *	FooPageRead(streamPtr, ioPtr, waitPtr, replyPtr)
     *	FoPageoWrite(streamPtr, ioPtr, waitPtr, replyPtr)
     *		Fs_Stream	*streamPtr;	(See above about valid fields)
     *		Fs_IOParam	*ioPtr;		(Standard parameter block)
     *		Sync_RemoteWaiter *waitPtr;	(For remote waiting)
     *		Fs_IOReply	*reply;		(For return length and signal)
     */
    ReturnStatus (*pageRead)();
    ReturnStatus (*pageWrite)();
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
/*
 * Streams for Local files, device, and pipes
 */

This goes into fsio_StreamOpTable:
static Fsio_StreamTypeOps ioStreamOps[] = {
    /*
     * Top level stream.  This is created by Fs_Open and returned to clients.
     * This in turn references a I/O handle of a different stream type.  The
     * main reason this exists as a handle is so that it can be found
     * during various cases of migration, although the reopen and scavenge
     * entry points in this table are used.
     */
    { FSIO_STREAM, Fsio_NoProc, Fsio_NoProc, Fsio_NoProc,/* open, read, write */
		Fsio_NoProc, Fsio_NoProc,	/* pageRead, pageWrite */
		Fsio_NoProc, Fsio_NoProc,	/* iocontrol, select */
		Fsio_NoProc, Fsio_NoProc,	/* getIOAttr, setIOAttr */
		Fsio_NoHandle,			/* clientVerify */
		Fsio_NoProc, Fsio_NoProc,	/* release, migEnd */
		Fsio_NoProc,			/* migrate */
		Fsio_StreamReopen,
		(Boolean (*)())NIL,		/* scavenge */
		Fsio_NullClientKill,
		Fsio_NoProc},			/* close */
    /*
     * Local file stream.  The file is on a local disk and blocks are
     * cached in the block cache.  The GetIOAttr(SetIOAttr) routine
     * is Fsio_NullProc because all the work of getting(setting) cached attributes
     * is already done by FslclGetAttr(FslclSetAttr).
     */
    { FSIO_LCL_FILE_STREAM, Fsio_FileIoOpen, Fsio_FileRead, Fsio_FileWrite,
		Fsio_FileRead, Fsio_FileWrite,		/* Paging routines */
		Fsio_FileIOControl, Fsio_FileSelect,
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		Fsio_NoHandle, Fsio_FileMigClose, Fsio_FileMigOpen, Fsio_FileMigrate,
		Fsio_FileReopen,
		Fsio_FileScavenge, Fsio_FileClientKill, Fsio_FileClose},
    /*
     * Local device stream.  These routines branch to the device driver
     * for the particular device type.
     */
    { FSIO_LCL_DEVICE_STREAM, Fsio_DeviceIoOpen, Fsio_DeviceRead, Fsio_DeviceWrite,
		Fsio_NoProc, Fsio_NoProc,		/* Paging routines */
		Fsio_DeviceIOControl, Fsio_DeviceSelect,
		Fsio_DeviceGetIOAttr, Fsio_DeviceSetIOAttr,
		Fsio_NoHandle,				/* clientVerify */
		Fsio_DeviceMigClose, Fsio_DeviceMigOpen, Fsio_DeviceMigrate,
		Fsrmt_DeviceReopen,
		Fsio_DeviceScavenge, Fsio_DeviceClientKill, Fsio_DeviceClose},
    /*
     * Local anonymous pipe stream.  
     */
    { FSIO_LCL_PIPE_STREAM, Fsio_NoProc, Fsio_PipeRead, Fsio_PipeWrite,
		Fsio_NoProc, Fsio_NoProc,		/* Paging routines */
		Fsio_PipeIOControl, Fsio_PipeSelect,
		Fsio_PipeGetIOAttr, Fsio_PipeSetIOAttr,
		Fsio_NoHandle,				/* clientVerify */
		Fsio_PipeMigClose, Fsio_PipeMigOpen, Fsio_PipeMigrate,
		Fsio_PipeReopen,
		Fsio_PipeScavenge, Fsio_PipeClientKill, Fsio_PipeClose},

};

/*
 * File stream type ops for FSIO_RMT_FILE_STREAM, FSIO_RMT_DEVICE_STREAM,
 * and FSIO_RMT_PIPE_STREAM;
 */

static Fsio_StreamTypeOps rmtFileStreamOps[] = {
/*
 * Remote file stream.  The file is at a remote server but blocks might 
 * be cached in the block cache.
 */
    { FSIO_RMT_FILE_STREAM, FsrmtFileIoOpen, FsrmtFileRead, FsrmtFileWrite,
		FsrmtFilePageRead, FsrmtFilePageWrite,
		FsrmtFileIOControl, Fsio_FileSelect,
		FsrmtFileGetIOAttr, FsrmtFileSetIOAttr,
		FsrmtFileVerify, FsrmtFileMigClose, FsrmtFileMigOpen,
		FsrmtFileMigrate, FsrmtFileReopen,
		FsrmtFileScavenge,
		Fsio_NullClientKill, FsrmtFileClose},
/*
 * Remote device stream.  Forward the operations to the remote I/O server.
 */
    { FSIO_RMT_DEVICE_STREAM, FsrmtDeviceIoOpen, Fsrmt_Read, Fsrmt_Write,
		Fsio_NoProc, Fsio_NoProc,	/* Paging routines */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FsrmtDeviceVerify, Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FsrmtDeviceMigrate, FsrmtDeviceReopen,
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill, Fsrmt_IOClose},
 /*
  * Remote anonymous pipe stream.  These arise because of migration.
  */
    { FSIO_RMT_PIPE_STREAM, Fsio_NoProc, Fsrmt_Read, Fsrmt_Write,
		Fsio_NoProc, Fsio_NoProc,	/* Paging routines */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FsrmtPipeVerify, Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FsrmtPipeMigrate, FsrmtPipeReopen,
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill, Fsrmt_IOClose},
};
/*
 * File stream type ops for FSIO_CONTROL_STREAM, FSIO_SERVER_STREAM,
 * and FSIO_LCL_PSEUDO_STREAM FSIO_RMT_PSEUDO_STREAM FSIO_PFS_CONTROL_STREAM
 * FSIO_PFS_NAMING_STREAM FSIO_LCL_PFS_STREAM FSIO_RMT_CONTROL_STREAM 
 * FSIO_PASSING_STREAM
 */

static Fsio_StreamTypeOps pdevFileStreamOps[] = {
    /*
     * A control stream is what a pdev server process gets when it opens a
     * pseudo device file.  This stream is used to notify the server
     * of new clients; the ID of the server stream set up for each
     * new client is passed over this control stream.
     */
    { FSIO_CONTROL_STREAM, FspdevControlIoOpen,
		FspdevControlRead, Fsio_NoProc,
		Fsio_NoProc, Fsio_NoProc,		/* Paging routines */
		FspdevControlIOControl, FspdevControlSelect,
		FspdevControlGetIOAttr, FspdevControlSetIOAttr,
		FspdevControlVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, FspdevControlReopen,	/* migrate, reopen */
		FspdevControlScavenge, FspdevControlClientKill,
		FspdevControlClose },
    /*
     * A server stream gets set up for the server whenever a client opens
     * a pseudo device.  The server reads the stream the learn about new
     * requests from the client.  IOControls on the stream are used
     * to control the connection to the client.
     */
    { FSIO_SERVER_STREAM, Fsio_NoProc,
		FspdevServerStreamRead, Fsio_NoProc,
		Fsio_NoProc, Fsio_NoProc,		/* Paging I/O */
		FspdevServerStreamIOControl, FspdevServerStreamSelect,
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		Fsio_NoHandle,				/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		Fsio_NullClientKill, FspdevServerStreamClose },
    /*
     * A pseudo stream with the server process running locally.  
     */
    { FSIO_LCL_PSEUDO_STREAM, FspdevPseudoStreamIoOpen,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite, /* Paging */
		FspdevPseudoStreamIOControl,
		FspdevPseudoStreamSelect,
		FspdevPseudoStreamGetIOAttr, FspdevPseudoStreamSetIOAttr,
		Fsio_NoHandle,				/* verify */
		FspdevPseudoStreamMigClose, FspdevPseudoStreamMigOpen,
		FspdevPseudoStreamMigrate,
		Fsio_NoProc,				/* reopen */
		(Boolean (*)())NIL,			/* scavenge */
		Fsio_NullClientKill, FspdevPseudoStreamClose },
    /*
     * A pseudo stream with a remote server.  
     */
    { FSIO_RMT_PSEUDO_STREAM, FspdevRmtPseudoStreamIoOpen,
		Fsrmt_Read, Fsrmt_Write,
		Fsrmt_Read, Fsrmt_Write,		/* Paging I/O */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FspdevRmtPseudoStreamVerify,
		Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FspdevRmtPseudoStreamMigrate,
		Fsio_NoProc,				/* reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		Fsrmt_IOClose },
    /*
     * A control stream used to mark the existence of a pseudo-filesystem.
     * The server doesn't do I/O to this stream; it is only used at
     * open and close time.
     */
    { FSIO_PFS_CONTROL_STREAM, FspdevPfsIoOpen,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging I/O */
		Fsio_NoProc, Fsio_NoProc,		/* IOControl, select */
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		FspdevControlVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, FspdevControlReopen,	/* migrate, reopen */
		FspdevControlScavenge, FspdevControlClientKill,
		FspdevControlClose },
    /*
     * The 'naming stream' for a pseudo-filesystem is used to forward lookup
     * operations from the kernel up to the user-level server.  It is like
     * a regular pdev request-response stream, except that extra connections
     * to it are established via the prefix table when remote Sprite hosts
     * import a pseudo-filesystem.  The routines here are used to set this up.
     * This stream is only accessed locally.  To remote hosts the domain is
     * a regular remote sprite domain, and the RPC stubs on the server then
     * switch out to either local-domain or pseudo-domain routines.
     */
    { FSIO_PFS_NAMING_STREAM, FspdevPfsNamingIoOpen,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging I/O */
		Fsio_NoProc, Fsio_NoProc,		/* IOControl, select */
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		FspdevRmtPseudoStreamVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		Fsrmt_IOClose },
    /*
     * A pseudo stream to a pseudo-filesystem server.  This is just like
     * a pseudo stream to a pseudo-device server, except for the CltOpen
     * routine because setup is different.  
     */
    { FSIO_LCL_PFS_STREAM, FspdevPfsStreamIoOpen,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite, /* Paging */
		FspdevPseudoStreamIOControl,
		FspdevPseudoStreamSelect,
		FspdevPseudoStreamGetIOAttr, FspdevPseudoStreamSetIOAttr,
		Fsio_NoHandle,					/* verify */
		FspdevPseudoStreamMigClose, FspdevPseudoStreamMigOpen,
		FspdevPseudoStreamMigrate,
		Fsio_NoProc,					/* reopen */
		(Boolean (*)())NIL,				/* scavenge */
		Fsio_NullClientKill, FspdevPseudoStreamClose },
    /*
     * A pseudo stream to a remote pseudo-filesystem server.  This is
     * like the remote pseudo-device stream, except for setup because the
     * pseudo-device connection is already set up by the time the
     * CltOpen routine is called.
     */
    { FSIO_RMT_PFS_STREAM, FspdevRmtPfsStreamIoOpen,
		Fsrmt_Read, Fsrmt_Write,
		Fsrmt_Read, Fsrmt_Write,			/* Paging */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FspdevRmtPseudoStreamVerify,
		Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FspdevRmtPseudoStreamMigrate,
		Fsio_NoProc,					/* reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		Fsrmt_IOClose },
    /*
     * This stream type is only used during get/set I/O attributes when
     * the pseudo-device server is remote.  No handles of this type are
     * actually created, only fileIDs that map to FSIO_CONTROL_STREAM.  
     */
    { FSIO_RMT_CONTROL_STREAM, Fsio_NoProc,		/* ioOpen */
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging */
		Fsio_NoProc, Fsio_NoProc,		/* ioctl, select */
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		(Fs_HandleHeader *(*)())Fsio_NoProc,	/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* release, migend */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		(void (*)())Fsio_NoProc, Fsio_NoProc },	/* kill, close */
    /*
     * Stream used to pass streams from a pseudo-device server to
     * a client in response to an open request.
     */
    { FSIO_PASSING_STREAM, FspdevPassStream,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging */
		Fsio_NoProc, Fsio_NoProc,		/* ioctl, select */
		Fsio_NoProc, Fsio_NoProc,		/* get/set attr */
		(Fs_HandleHeader *(*)())Fsio_NoProc,	/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* release, migend */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		(void (*)())Fsio_NoProc, Fsio_NoProc },	/* kill, close */


};


/*
 * File stream type ops for FSIO_CONTROL_STREAM, FSIO_SERVER_STREAM,
 * and FSIO_LCL_PSEUDO_STREAM FSIO_RMT_PSEUDO_STREAM FSIO_PFS_CONTROL_STREAM
 * FSIO_PFS_NAMING_STREAM FSIO_LCL_PFS_STREAM FSIO_RMT_CONTROL_STREAM 
 * FSIO_PASSING_STREAM
 */

static Fsio_StreamTypeOps pdevFileStreamOps[] = {
    /*
     * A control stream is what a pdev server process gets when it opens a
     * pseudo device file.  This stream is used to notify the server
     * of new clients; the ID of the server stream set up for each
     * new client is passed over this control stream.
     */
    { FSIO_CONTROL_STREAM, FspdevControlIoOpen,
		FspdevControlRead, Fsio_NoProc,
		Fsio_NoProc, Fsio_NoProc,		/* Paging routines */
		FspdevControlIOControl, FspdevControlSelect,
		FspdevControlGetIOAttr, FspdevControlSetIOAttr,
		FspdevControlVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, FspdevControlReopen,	/* migrate, reopen */
		FspdevControlScavenge, FspdevControlClientKill,
		FspdevControlClose },
    /*
     * A server stream gets set up for the server whenever a client opens
     * a pseudo device.  The server reads the stream the learn about new
     * requests from the client.  IOControls on the stream are used
     * to control the connection to the client.
     */
    { FSIO_SERVER_STREAM, Fsio_NoProc,
		FspdevServerStreamRead, Fsio_NoProc,
		Fsio_NoProc, Fsio_NoProc,		/* Paging I/O */
		FspdevServerStreamIOControl, FspdevServerStreamSelect,
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		Fsio_NoHandle,				/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		Fsio_NullClientKill, FspdevServerStreamClose },
    /*
     * A pseudo stream with the server process running locally.  
     */
    { FSIO_LCL_PSEUDO_STREAM, FspdevPseudoStreamIoOpen,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite, /* Paging */
		FspdevPseudoStreamIOControl,
		FspdevPseudoStreamSelect,
		FspdevPseudoStreamGetIOAttr, FspdevPseudoStreamSetIOAttr,
		Fsio_NoHandle,				/* verify */
		FspdevPseudoStreamMigClose, FspdevPseudoStreamMigOpen,
		FspdevPseudoStreamMigrate,
		Fsio_NoProc,				/* reopen */
		(Boolean (*)())NIL,			/* scavenge */
		Fsio_NullClientKill, FspdevPseudoStreamClose },
    /*
     * A pseudo stream with a remote server.  
     */
    { FSIO_RMT_PSEUDO_STREAM, FspdevRmtPseudoStreamIoOpen,
		Fsrmt_Read, Fsrmt_Write,
		Fsrmt_Read, Fsrmt_Write,		/* Paging I/O */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FspdevRmtPseudoStreamVerify,
		Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FspdevRmtPseudoStreamMigrate,
		Fsio_NoProc,				/* reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		Fsrmt_IOClose },
    /*
     * A control stream used to mark the existence of a pseudo-filesystem.
     * The server doesn't do I/O to this stream; it is only used at
     * open and close time.
     */
    { FSIO_PFS_CONTROL_STREAM, FspdevPfsIoOpen,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging I/O */
		Fsio_NoProc, Fsio_NoProc,		/* IOControl, select */
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		FspdevControlVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, FspdevControlReopen,	/* migrate, reopen */
		FspdevControlScavenge, FspdevControlClientKill,
		FspdevControlClose },
    /*
     * The 'naming stream' for a pseudo-filesystem is used to forward lookup
     * operations from the kernel up to the user-level server.  It is like
     * a regular pdev request-response stream, except that extra connections
     * to it are established via the prefix table when remote Sprite hosts
     * import a pseudo-filesystem.  The routines here are used to set this up.
     * This stream is only accessed locally.  To remote hosts the domain is
     * a regular remote sprite domain, and the RPC stubs on the server then
     * switch out to either local-domain or pseudo-domain routines.
     */
    { FSIO_PFS_NAMING_STREAM, FspdevPfsNamingIoOpen,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging I/O */
		Fsio_NoProc, Fsio_NoProc,		/* IOControl, select */
		Fsio_NullProc, Fsio_NullProc,		/* Get/Set IO Attr */
		FspdevRmtPseudoStreamVerify,
		Fsio_NoProc, Fsio_NoProc,		/* migStart, migEnd */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		Fsrmt_IOClose },
    /*
     * A pseudo stream to a pseudo-filesystem server.  This is just like
     * a pseudo stream to a pseudo-device server, except for the CltOpen
     * routine because setup is different.  
     */
    { FSIO_LCL_PFS_STREAM, FspdevPfsStreamIoOpen,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite,
		FspdevPseudoStreamRead,	FspdevPseudoStreamWrite, /* Paging */
		FspdevPseudoStreamIOControl,
		FspdevPseudoStreamSelect,
		FspdevPseudoStreamGetIOAttr, FspdevPseudoStreamSetIOAttr,
		Fsio_NoHandle,					/* verify */
		FspdevPseudoStreamMigClose, FspdevPseudoStreamMigOpen,
		FspdevPseudoStreamMigrate,
		Fsio_NoProc,					/* reopen */
		(Boolean (*)())NIL,				/* scavenge */
		Fsio_NullClientKill, FspdevPseudoStreamClose },
    /*
     * A pseudo stream to a remote pseudo-filesystem server.  This is
     * like the remote pseudo-device stream, except for setup because the
     * pseudo-device connection is already set up by the time the
     * CltOpen routine is called.
     */
    { FSIO_RMT_PFS_STREAM, FspdevRmtPfsStreamIoOpen,
		Fsrmt_Read, Fsrmt_Write,
		Fsrmt_Read, Fsrmt_Write,			/* Paging */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FspdevRmtPseudoStreamVerify,
		Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FspdevRmtPseudoStreamMigrate,
		Fsio_NoProc,					/* reopen */
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill,
		Fsrmt_IOClose },
    /*
     * This stream type is only used during get/set I/O attributes when
     * the pseudo-device server is remote.  No handles of this type are
     * actually created, only fileIDs that map to FSIO_CONTROL_STREAM.  
     */
    { FSIO_RMT_CONTROL_STREAM, Fsio_NoProc,		/* ioOpen */
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging */
		Fsio_NoProc, Fsio_NoProc,		/* ioctl, select */
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		(Fs_HandleHeader *(*)())Fsio_NoProc,	/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* release, migend */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		(void (*)())Fsio_NoProc, Fsio_NoProc },	/* kill, close */
    /*
     * Stream used to pass streams from a pseudo-device server to
     * a client in response to an open request.
     */
    { FSIO_PASSING_STREAM, FspdevPassStream,
		Fsio_NoProc, Fsio_NoProc,		/* read, write */
		Fsio_NoProc, Fsio_NoProc,		/* Paging */
		Fsio_NoProc, Fsio_NoProc,		/* ioctl, select */
		Fsio_NoProc, Fsio_NoProc,		/* get/set attr */
		(Fs_HandleHeader *(*)())Fsio_NoProc,	/* verify */
		Fsio_NoProc, Fsio_NoProc,		/* release, migend */
		Fsio_NoProc, Fsio_NoProc,		/* migrate, reopen */
		(Boolean (*)())NIL,			/* scavenge */
		(void (*)())Fsio_NoProc, Fsio_NoProc },	/* kill, close */


};

/*
 * File stream type ops for FSIO_RMT_FILE_STREAM, FSIO_RMT_DEVICE_STREAM,
 * and FSIO_RMT_PIPE_STREAM;
 */

static Fsio_StreamTypeOps rmtFileStreamOps[] = {
/*
 * Remote file stream.  The file is at a remote server but blocks might 
 * be cached in the block cache.
 */
    { FSIO_RMT_FILE_STREAM, FsrmtFileIoOpen, FsrmtFileRead, FsrmtFileWrite,
		FsrmtFilePageRead, FsrmtFilePageWrite,
		FsrmtFileIOControl, Fsio_FileSelect,
		FsrmtFileGetIOAttr, FsrmtFileSetIOAttr,
		FsrmtFileVerify, FsrmtFileMigClose, FsrmtFileMigOpen,
		FsrmtFileMigrate, FsrmtFileReopen,
		FsrmtFileScavenge,
		Fsio_NullClientKill, FsrmtFileClose},
/*
 * Remote device stream.  Forward the operations to the remote I/O server.
 */
    { FSIO_RMT_DEVICE_STREAM, FsrmtDeviceIoOpen, Fsrmt_Read, Fsrmt_Write,
		Fsio_NoProc, Fsio_NoProc,	/* Paging routines */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FsrmtDeviceVerify, Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FsrmtDeviceMigrate, FsrmtDeviceReopen,
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill, Fsrmt_IOClose},
 /*
  * Remote anonymous pipe stream.  These arise because of migration.
  */
    { FSIO_RMT_PIPE_STREAM, Fsio_NoProc, Fsrmt_Read, Fsrmt_Write,
		Fsio_NoProc, Fsio_NoProc,	/* Paging routines */
		Fsrmt_IOControl, Fsrmt_Select,
		Fsrmt_GetIOAttr, Fsrmt_SetIOAttr,
		FsrmtPipeVerify, Fsrmt_IOMigClose, Fsrmt_IOMigOpen,
		FsrmtPipeMigrate, FsrmtPipeReopen,
		Fsutil_RemoteHandleScavenge, Fsio_NullClientKill, Fsrmt_IOClose},
};

/*
 * fs_DomainLookup:
 * Domain specific routine table for lookup operations.
 * The following operate on a single pathname.  They are called via
 * Fsprefix_LookupOperation with arguments described in fsOpTable.h
 *	DomainImport
 *	DomainExport
 *	DomainOpen
 *	DomainGetAttrPath
 *	DomainSetAttrPath
 *	DomainMakeDevice
 *	DomainMakeDir
 *	DomainRemove
 *	DomainRemoveDir
 * The following operate on two pathnames.
 *	DomainRename
 *	DomainHardLink
 */

/*
 * THIS ARRAY INDEXED BY DOMAIN TYPE.  Do not arbitrarily insert entries.
 */
ReturnStatus (*fs_DomainLookup[FS_NUM_DOMAINS][FS_NUM_NAME_OPS])() = {
/* FS_LOCAL_DOMAIN */
/* FS_REMOTE_SPRITE_DOMAIN */
/* FS_PSEUDO_DOMAIN */
/* FS_NFS_DOMAIN */
};

static ReturnStatus (*lclDomainLookup[FS_NUM_NAME_OPS])() = {
     Fsio_NoProc, FslclExport, FslclOpen, FslclGetAttrPath,
     FslclSetAttrPath, FslclMakeDevice, FslclMakeDir,
     FslclRemove, FslclRemoveDir, FslclRename, FslclHardLink,
};
static ReturnStatus (*rmtDomainLookup[FS_NUM_NAME_OPS])() = {
     FsrmtImport, Fsio_NoProc, FsrmtOpen, FsrmtGetAttrPath,
     FsrmtSetAttrPath, FsrmtMakeDevice, FsrmtMakeDir, 
     FsrmtRemove, FsrmtRemoveDir, FsrmtRename, FsrmtHardLink
};
static ReturnStatus (*pdevDomainLookup[FS_NUM_NAME_OPS])() = {
     Fsio_NoProc, FspdevPfsExport, FspdevPfsOpen, FspdevPfsGetAttrPath, FspdevPfsSetAttrPath,
     FspdevPfsMakeDevice, FspdevPfsMakeDir, FspdevPfsRemove, FspdevPfsRemoveDir,
     FspdevPfsRename, FspdevPfsHardLink
};

/*
 * Domain specific get/set attributes table.  These routines are used
 * to get/set attributes on the name server given a fileID (not a pathname).
 */
Fs_AttrOps fs_AttrOpTable[FS_NUM_DOMAINS] = {
/* FS_LOCAL_DOMAIN */
/* FS_REMOTE_SPRITE_DOMAIN */
/* FS_PSEUDO_DOMAIN */
/* FS_NFS_DOMAIN */
};
/*
 * Domain specific get/set attributes table.  These routines are used
 * to get/set attributes on the name server given a fileID (not a pathname).
 */
static Fs_AttrOps rmtAttrOpTable =   { FsrmtGetAttr, FsrmtSetAttr };
static Fs_AttrOps lclAttrOpTable =   { FslclGetAttr, FslclSetAttr };

static Fs_AttrOps pdevAttrOpTable = { FspdevPseudoGetAttr, FspdevPseudoSetAttr };
static Fs_AttrOps rmtAttrOpTable =   { FsrmtGetAttr, FsrmtSetAttr };


Cache backend routines:
typedef struct Fscache_BackendRoutines {
    /*  
     *  FooAllocate(hdrPtr, offset, bytes, flags, blockAddrPtr, newBlockPtr)
     *          Fs_HandleHeader *hdrPtr;                        (File handle)
     *          int             offset;                 (Byte offset)
     *          int             bytes;                  (Bytes to allocate)
     *          int             flags;                  (FSCACHE_DONT_BLOCK)
     *          int             *blockAddrPtr;          (Returned block number)
     *          Boolean         *newBlockPtr;           (TRUE if new block)
     *  FooTruncate(hdrPtr, size, delete)
     *          Fs_HandleHeader *hdrPtr;                (File handle)
     *          int             size;                   (New size)
     *          Boolean         delete;                 (TRUE if file being
     *                                                   removed)
     *  FooBlockRead(hdrPtr, blockPtr,remoteWaitPtr)
     *          Fs_HandleHeader *hdrPtr;                (File handle)
     *          Fscache_Block   *blockPtr;              (Cache block to read)
     *          Sync_RemoteWaiter *remoteWaitPtr;       (For remote waiting)
     *  FooBlockWrite(hdrPtr, blockPtr, lastDirtyBlock)
     *          Boolean         lastDirtyBlock;         (Indicates last block)
     *  FooReallocBlock(data, callInfoPtr)
     *          ClientData      data =  blockPtr;  (Cache block to realloc)
     *          Proc_CallInfo   *callInfoPtr;
     *  FooStartWriteBack(backendPtr)
     *        Fscache_Backend *backendPtr;      (Backend to start writeback.)
     */
    ReturnStatus (*allocate) _ARGS_((Fs_HandleHeader *hdrPtr, int offset,
                                    int numBytes, int flags, int *blockAddrPtr,
                                    Boolean *newBlockPtr));
    ReturnStatus (*truncate) _ARGS_((Fs_HandleHeader *hdrPtr, int size,
                                     Boolean delete));
    ReturnStatus (*blockRead) _ARGS_((Fs_HandleHeader *hdrPtr,
                                      Fscache_Block *blockPtr,
                                      Sync_RemoteWaiter *remoteWaitPtr));
    ReturnStatus (*blockWrite) _ARGS_((Fs_HandleHeader *hdrPtr,
                                       Fscache_Block *blockPtr, int flags));
    void         (*reallocBlock) _ARGS_((ClientData data,
                                         Proc_CallInfo *callInfoPtr));

    ReturnStatus (*startWriteBack) _ARGS_((struct Fscache_Backend *backendPtr));
} Fscache_BackendRoutines;

static Fscache_BackendRoutines  ofsBackendRoutines = {
            Fsdm_BlockAllocate,
            Fsdm_FileTrunc,
            Fsdm_FileBlockRead,
            Fsdm_FileBlockWrite,
            Ofs_ReallocBlock,
            Ofs_StartWriteBack,
}

static Fscache_BackendRoutines  fsrmtBackendRoutines = {
            FsrmtFileBlockAllocate,
            FsrmtFileTrunc,
            FsrmtFileBlockRead,
            FsrmtFileBlockWrite,
            FsrmtReallocBlock,
            FsrmtStartWriteBack,
};

static Fscache_BackendRoutines  lfsBackendRoutines = {
            Fsdm_BlockAllocate,
            Fsdm_FileTrunc,
            Fsdm_FileBlockRead,
            Fsdm_FileBlockWrite,
            Lfs_ReallocBlock,
            Lfs_StartWriteBack,

};
