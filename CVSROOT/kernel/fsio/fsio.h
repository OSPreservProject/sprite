/*
 * fsOpTable.h --
 *
 *	There are four operation switch tables defined here.  (See fsOpTable.c
 *	for their initialization.)
 *	1. The DOMAIN table used for naming operations like OPEN or REMOVE_DIR.
 *		These operations take file names as arguments and have to
 *		be pre-processed by the prefix table module in order to
 *		chose the correct domain type and server.
 *	2. The OPEN table is used on the server when opening files.  This
 *		is keyed on disk file descriptor type, i.e. file, device,
 *		pseudo-device.  The server has to set up state that the
 *		client will use when setting up its own I/O stream.
 *	3. The ATTR table is used when getting/setting attributes when
 *		starting with an open stream (not with a file name).  This
 *		is keyed on the type of the nameFileID in the stream.
 *	4. The STREAM table is used for all other operations on open streams
 *		and I/O handles.  It is keyed on the type of the ioFileID,
 *		i.e. local file, remote device, local pipe, etc.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSOPTABLE
#define _FSOPTABLE

/*
 * Name Domain Types:
 *
 *	FS_LOCAL_DOMAIN		The file is stored locally.
 *	FS_REMOTE_SPRITE_DOMAIN	The file is stored on a Sprite server.
 *	FS_PSEUDO_DOMAIN	The file system is implemented by
 *				a user-level server process
 *	FS_NFS_DOMAIN		The file is stored on an NFS server.
 *
 */

#define FS_LOCAL_DOMAIN			0
#define FS_REMOTE_SPRITE_DOMAIN		1
#define FS_PSEUDO_DOMAIN		2
#define FS_NFS_DOMAIN			3

#define FS_NUM_DOMAINS			4

/*
 * DOMAIN SWITCH
 * Domain specific operations that operate on file names for lookup.
 * Naming operations are done through FsLookupOperation, which uses
 * the prefix table to choose the domain type and the server for the name.
 * It then branches through the fsDomainLookup table to complete the operation.
 * The arguments to these operations are documented in fsNameOps.h
 * because they are collected into structs (declared in fsNameOps.h)
 * and passed through FsLookupOperation() to domain-specific routines.
 */

#define	FS_DOMAIN_IMPORT		0
#define	FS_DOMAIN_EXPORT		1
#define	FS_DOMAIN_OPEN			2
#define	FS_DOMAIN_GET_ATTR		3
#define	FS_DOMAIN_SET_ATTR		4
#define	FS_DOMAIN_MAKE_DEVICE		5
#define	FS_DOMAIN_MAKE_DIR		6
#define	FS_DOMAIN_REMOVE		7
#define	FS_DOMAIN_REMOVE_DIR		8
#define	FS_DOMAIN_RENAME		9
#define	FS_DOMAIN_HARD_LINK		10

#define	FS_NUM_NAME_OPS			11

extern	ReturnStatus (*fsDomainLookup[FS_NUM_DOMAINS][FS_NUM_NAME_OPS])();


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

typedef struct FsOpenOps {
    int		type;			/* One of the file descriptor types */
    /*
     * The calling sequence for the server-open routine is:
     *	FooSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
     *			sizePtr, dataPtr)
     *		FsLocalFileIOHandle	*handlePtr;
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
} FsOpenOps;

extern FsOpenOps fsOpenOpTable[];

/*
 * ATTRIBUTE SWITCH
 * A switch is used to get to the name server in set/get attributesID,
 * which take an open stream.  The stream refereneces a nameFileID, and
 * this switch is keyed on the nameFileID.type (i.e. local or remote file).
 */

typedef struct FsAttrOps {
    ReturnStatus	(*getAttr)();
    ReturnStatus	(*setAttr)();
} FsAttrOps;

extern FsAttrOps fsAttrOpTable[];


/*
 * STREAM SWITCH
 *	These procedures are called by top-level procedures (i.e. Fs_Read,
 *	or Fs_Select) to do stream-type specific processing.
 */

typedef struct FsStreamTypeOps {
    int		type;			/* Stream types defined in fs.h */
    /*
     **************** Setup operation for clients. *************************
     *	This routine sets up an I/O handle for a stream.  It uses streamData
     *  that was genereated by the srvOpen routine on the file server.  As
     *  a side effect it fills in the nameInfoPtr->fileID for use later
     *  when getting/setting attributes.
     *  BRENT - why can't nameFileID be set by the server?
     *
     *	FooCltOpen(fileIDPtr, flagsPtr, clientID, data, hdrPtrPtr)
     *		Fs_FileID	*fileIDPtr;	(indicates file)
     *		int		*flagsPtr;	(from the stream)
     *		int		clientID;	(who's opening it)
     *		ClientData	data;		(stream data from srvOpen)
     *		FsHandleHeader	**hdrPtrPtr;	(Returned I/O handle)
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
     *	FooRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
     *	FooWrite(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
     *		Fs_Stream	*streamPtr;	( !Only use ioHandlePtr! )
     *		int		flags;		(from the stream)
     *		Address		buffer;		(Target for data)
     *		int		*offsetPtr;	(Byte offset In/Out)
     *		int		*lenPtr;	(Byte count In/Out)
     *		Sync_RemoteWaiter *waitPtr;	(For remote waiting)
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
     *		FsHandleHeader *hdrPtr;			(File handle)
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
     *		FsHandleHeader		*hdrPtr;	(File handle)
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
     *  client's fileID.type (i.e. FS_RMT_DEVICE_STREAM) to the server's
     *  (i.e. FS_LCL_DEVICE_STREAM).  The second step is to check that the
     *  client is recorded in the clientList of the I/O handle.  The domain
     *	type is returned for use in naming operations.
     *
     *	FsHandleHeader *
     *	FooClientVerify(fileIDPtr, clientID, domainTypePtr)
     *		Fs_FileID	*fileIDPtr;		(Client's handle)
     *		int		clientID;		(The client hostID)
     *		int		*domainTypePtr;		(may be NIL)
     */
    FsHandleHeader *(*clientVerify)();
    /*
     *************** Migration calls. **************************************
     *
     *  The 'release' is called on the source client of migration via an RPC
     *		from the I/O server.  Its job is to release any referneces
     *		on the I/O handle that were due to a stream which has
     *		now migrated away from the source client.
     *  The 'migrate' is called from Fs_DeencapStream to update client
     *		book-keeping to reflect the migration.  The version on
     *		remote clients just does an RPC to the I/O server to
     *		invoke the appropriate migrate routine there.
     *		Important:  If the FS_RMT_SHARED flag is present it means
     *		that there is still a stream on the original client and its
     *		references should not be removed from the I/O handle.
     *		Also, if the FS_NEW_STREAM flag is present it means that
     *		the stream is newly migrated to the client so references
     *		should be added for the dstClient.
     *  The 'migEnd' is called from Fs_DeencapStream after the call to the
     *		migrate procedure, but only the first time the stream
     *		is migrated to the host.  (After that it suffices to
     *		add references to the existing stream.)
     *
     *	FooRelease(hdrPtr, flags)
     *		FsHandleHeader	*hdrPtr;		(File handle)
     *		int		flags;			(From the stream)
     *	FooMigEnd(migInfoPtr, size, data, hdrPtrPtr)
     *		FsMigInfo	migInfoPtr;		(Migration state)
     *		int		size;			(size of data)
     *		ClientData	data;			(data from migrate)
     *		FsHandleHeader	**hdrPtrPtr;		(Returned handle)
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
     *		FsHandleHeader *hdrPtr;		(Handle to re-open)
     *		(The following are only valid on servers.)
     *		int		clientID;	(client doing the re-open)
     *		ClientData	inData;		(state from the client)
     *		int		*outSizePtr;	(sizeof outData)
     *		ClientData	*outDataPtr;	(state returned to client)
     */
    ReturnStatus (*reopen)();
    /*
     *************** Cache Operations. **************************************
     *  These should be moved out to a new switch, or passed into the
     *  Fs_CacheInfoInit procedure, or something.
     *
     *	FooAllocate(hdrPtr, offset, bytes, blockAddrPtr, newBlockPtr)
     *		FsHandleHeader *hdrPtr;			(File handle)
     *		int		offset;			(Byte offset)
     *		int		bytes;			(Bytes to allocate)
     *		int		*blockAddrPtr;		(Returned block number)
     *		Boolean		*newBlockPtr;		(TRUE if new block)
     *	FooBlockRead(hdrPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
     *		FsHandleHeader *hdrPtr;			(File handle)
     *		int		flags;		(For compatibility with .read)
     *		Address		buffer;			(Target of read)
     *		int		*offsetPtr;		(Byte offset)
     *		int		*lenPtr;		(Byte count)
     *		Sync_RemoteWaiter *waitPtr;		(For remote waiting)
     *	FooBlockWrite(hdrPtr, blockNumber, numBytes, buffer, lastDirtyBlock)
     *		FsHandleHeader	*hdrPtr;		(File handle)
     *		int		blockNumber;		(Disk block number)
     *		int		numBytes;		(Byte count in block)
     *		Address		buffer;			(Source of data)
     *		Boolean		lastDirtyBlock;		(Indicates last block)
     *	FooBlockCopy(srcHdrPtr, dstHdrPtr, blockNumber)
     *		FsHandleHeader	*srcHdrPtr;		(Source file handle)
     *		FsHandleHeader	*dstHdrPtr;		(Destination handle)
     *		int		blockNumber;		(Block to copy)
     */
    ReturnStatus (*allocate)();
    ReturnStatus (*blockRead)();
    ReturnStatus (*blockWrite)();
    ReturnStatus (*blockCopy)();
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
     *		FsHandleHeader	*hdrPtr;		(File handle)
     *	FooClientKill(hdrPtr, clientID)
     *		FsHandleHeader	*hdrPtr;		(File handle)
     *		int		clientID;		(Client presumed down)
     *	FooClose(hdrPtr, clientID, procID, flags, size, data)
     *		FsHandleHeader	*hdrPtr;		(File handle)
     *		int		clientID;		(Host ID of closer)
     *		Proc_Pid	procID;			(ProcessID of closer)
     *		int		flags;			(From the stream)
     *		int		size;			(Size of data)
     *		ClientData	data;			(Extra close data)
     */
    Boolean	 (*scavenge)();
    void	 (*clientKill)();
    ReturnStatus (*close)();
} FsStreamTypeOps;

extern FsStreamTypeOps fsStreamOpTable[];

#endif _FSOPTABLE
