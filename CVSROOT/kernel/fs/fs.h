/*
 * fs.h --
 *
 *	The external interface to the filesystem module is defined here.
 *	The main object at the interface level is Fs_Stream.  A standard set
 *	of operations apply to all streams.  A stream is created by
 *	Fs_Open on a pathname, with flags used to differentiate between
 *	the different kinds of streams.  After that, the main operations
 *	are Fs_Read, Fs_Write, Fs_IOControl, Fs_Select, and Fs_Close.
 *
 *	Parts of the Fs to Dev interface are also described here, including
 *	Fs_Device, which describes a device, and Fs_IOParam, Fs_IOCParam,
 *	and Fs_IOReply, which are standard parameter blocks.
 *
 * Copyright (C) 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FS
#define _FS

#ifdef KERNEL
#include <sys.h>
#include <syncTypes.h>
#include <procTypes.h>
#include <user/fs.h>
#include <fmt.h>
#include <bstring.h>
#else
#include <kernel/sys.h>
#include <kernel/syncTypes.h>
#include <kernel/procTypes.h>
#include <fs.h>
#include <fmt.h>
#endif

/*
 * Fragment stuff that is dependent on the filesystem block size
 * defined in user/fs.h.  Actually, the fragmenting code is specific
 * to a 4K block size and a 1K fragment size.
 */

#define	FS_BLOCK_OFFSET_MASK	(FS_BLOCK_SIZE - 1)
#define	FS_FRAGMENT_SIZE	1024
#define	FS_FRAGMENTS_PER_BLOCK	4


/*
 * The following structure is referenced by the process table entry for
 * a process.  It contains all the filesystem related state for the process.
 */
typedef struct Fs_ProcessState {
    struct Fs_Stream	*cwdPtr;	/* The current working directory. */
    unsigned int   	filePermissions;/* The bits in this mask correspond
					 * to the permissions mask of a file.
					 * If one of these bits is set it
					 * TURNS OFF the corresponding
					 * permission when a file is created. */
    int		   	numStreams;	/* Size of streamList array. */
    struct Fs_Stream   **streamList;	/* Array of pointers to open files.
					 * This list is indexed by an integer
					 * known as a streamID. */
    char		*streamFlags;	/* Array of flags, one for element
					 * for each open stream.  Used to
					 * keep the close-on-exec property */
    int			numGroupIDs;	/* The length of the groupIDs array */
    int			*groupIDs;	/* An array of group IDs.  Group IDs
					 * are used similarly to the User ID. */
} Fs_ProcessState;


/*
 * The following low-level file system types have to be exported because
 * the type of Fs_Stream is already exported.  This could be fixed by
 * only exporting an opaque Fs_StreamPtr type, or by changing the definition
 * of Fs_Stream from a struct to a pointer to the struct.
 */

/*
 * All kinds of things are referenced from the object hash table.  The generic
 * term for each structure is "handle".  The following structure defines a 
 * common structure needed in the beginning of each handle.  Note, most of
 * these fields are private to the FsHandle* routines that do generic
 * operations on handles.  One exception is that the refCount on FSIO_STREAM
 * handles is manipulated by the stream routines.  The handle must be
 * locked when examining the refCount, and it should only be changed
 * under the handle monitor lock by the FsHandle* routines.
 */

typedef struct Fs_HandleHeader {
    Fs_FileID		fileID;		/* Used as the hash key. */
    int			flags;		/* Defined in fsHandle.c. */
    Sync_Condition	unlocked;	/* Notified when handle is unlocked. */
    int			refCount;	/* Used for garbage collection. */
    char		*name;		/* Used for error messages */
    List_Links		lruLinks;	/* For LRU list of handles */
#ifndef CLEAN
    Proc_ControlBlock	*lockProcPtr;	/* pcb of process that has the 
					 * lock */
#endif
} Fs_HandleHeader;

#define LRU_LINKS_TO_HANDLE(listPtr) \
	( (Fs_HandleHeader *)((int)(listPtr) - sizeof(Fs_FileID) \
		- 2 * sizeof(int) - sizeof(char *) - sizeof(Sync_Condition)) )



/*
 * The following name-related information is referenced by each stream.
 * This identifies the name of the file, which will be different than
 * the file itself in the case of devices.  This is used to get to the
 * name server during set/get attributes operations.  Also, this name
 * fileID is used as the starting point for relative lookups.
 */

typedef struct Fs_NameInfo {
    Fs_FileID		fileID;		/* Identifies file and name server. */
    Fs_FileID		rootID;		/* ID of file system root.  Passed
					 * to name server to prevent ascending
					 * past the root of a domain with ".."*/
    int			domainType;	/* Name domain type */
    struct Fsprefix	*prefixPtr;	/* Back pointer to prefix table entry.
					 * This is kept for efficient handling
					 * of lookup redirects. */
} Fs_NameInfo;

/*
 * Fs_Stream - A clients handle on an open file is defined by the Fs_Stream
 *      structure.  A stream has a read-write offset index, state flags
 *	that determine how the stream is being used (ie, for reading
 *	or writing, etc.), name state used for get/set attributes,
 *	and an I/O handle used for I/O operations.
 *	There are many different types of I/O handles; they correspond
 *	to local files, remote files, local devices, remote devices,
 *	local pipes, remote pipes, locally cached named pipes,
 *	remotely cached named pipes, control streams for pseudo devices,
 *	psuedo streams, their corresponding server streams, etc. etc.
 *	The different I/O handles types are defined in fsInt.h
 *
 *	This top level Fs_Stream structure is also given a handle type
 *	and kept in the handle table because of process migration and
 *	the shadow streams that file servers have to keep - on both
 *	the client and the server there will be a Fs_Stream that
 *	references an I/O handle.  The I/O handles will be different
 *	types on the client (i.e. remote file) and the server (local file).
 *	The Fs_Stream object will be the same, however, with the same
 *	usage flags and internal ID.
 *
 */

typedef struct Fs_Stream {
    Fs_HandleHeader	hdr;		/* Global stream identifier.  This
					 * includes a reference count which
					 * is incremented on fork/dup */
    int			offset;		/* File access position */
    int			flags;		/* Flags defined below */
    Fs_HandleHeader	*ioHandlePtr;	/* Stream specific data used for I/O.
					 * This really references a somewhat
					 * larger object, see fsInt.h */
    Fs_NameInfo	 	*nameInfoPtr;	/* Used to contact the name server */
    List_Links		clientList;	/* Needed for recovery and sharing
					 * detection */
} Fs_Stream;

/*
 * Flags in Fs_Stream that are only set/used by the kernel.
 * (Flags passed in from callers of Fs_Open are defined in user/fs.h
 *  The low order 12 bits of the flags word are reserved for those flags.)
 * There are two groups of flags, the first is used mainly at lookup time
 *	and the rest apply to I/O operations.
 *
 *	(These open related bit values are defined in user/fs.h)
 *	FS_CREATE - Create if it doesn't exist.
 *	FS_TRUNC  - Truncate to zero length on opening
 *	FS_EXCULSIVE - With FS_CREATE causes open to fail if the file exists.
 *	FS_MASTER - Open pseudo-device as the server process
 *	FS_NAMED_PIPE_OPEN - Open a named pipe
 *	FS_CLOSE_ON_EXEC - Cause stream to be closed when the opening
 *		process executes a different program.
 *
 *	(These I/O related bit values are defined in user/fs.h)
 *	FS_READ - Open for reading	(also FS_READABLE)
 *	FS_WRITE - Open for writing	(also FS_WRITABLE)
 *	FS_EXECUTE - Open for execution	(also FS_EXCEPTION)
 *	FS_APPEND - Open for append mode
 *	FS_NON_BLOCKING - Open a stream with non-blocking I/O operations. If
 *		the operation would normally block, FS_WOULD_BLOCK is returned.
 *
 *	(These open-time bit values are defined below)
 *      FS_FOLLOW - follow symbolic links during look up.
 *	FS_PREFIX - Set when opening prefixes for export.
 *	FS_SWAP - Set when opening swap files.  They are not cached on
 *		remote clients and this flag is used to set caching state.
 *	FS_OWNERSHIP - ownership permission.  The calling process has to
 *		own the file in order for a lookup to succeed.  This is
 *		used for setting attributes.
 *	FS_DELETE - Open a file for deletion.  Write permission is needed
 *		in the parent directory for this to succeed.
 *	FS_LINK - This is used by FslclLookup to make hard links.  Instead
 *		of creating a new file, FslclLookup makes another directory
 *		reference (hard link) to an existing file.
 *	FS_RENAME - This goes with FS_LINK if the link is being made in
 *		preparation for a rename operation.  This allows the link,
 *		which in this case is temporary, to be made to a directory.
 *
 *	(These I/O related bit values are defined below)
 *      FS_USER - the file is a user file.  The buffer space is
 *              in a user's address space.
 *	FS_USER_IN - For I/O Control, the input buffer is in user space
 *	FS_USER_OUT - For I/O Control, the output buffer is in user space
 *	FS_CLIENT_CACHE_WRITE -	This write is coming from a client's cache.
 *              This means the modify time should not be updated
 *              since the client has the correct modify time.
 *	FS_SERVER_WRITE_THRU - Set on writes that are supposed to be written
 *			       through to the server.
 *	FS_LAST_DIRTY_BLOCK - Set on remote writes when this is the
 *		last dirty block of the file to be written back.
    Now not used -- was in sosp changes and must find something else.
 *	FS_HEAP - This is a heap page for special treatment in the cache.
 *	FS_RMT_SHARED - Set on streams that are shared among clients on
 *		separate machines.  For regular files this means that the
 *		stream offset is being maintained on the server.
 *	FS_NEW_STREAM - Migration related.  This tells the I/O server that
 *		the destination of a migration is getting a stream for
 *		the first time.   It needs to know this in order to do
 *		I/O client book-keeping correctly.
 *	FS_WRITE_TO_DISK - Write this block through to disk.
 *	FS_MAP - File is being mapped into virtual memory.
 *	FS_MIGRATED_FILE - Migration related.  This says a file has been
 *		involved in a migration, so (for example) if the file
 *		gets flushed to the server then the host that's flushing
 *		it knows to attribute the flush to migration.  Used
 *		for statistics.
 *	FS_MIGRATING - Migration related.  This says a file is in the
 *		process of migrating.  Also used for statistics.
 *		MIGRATED_FILE stays set for any file that has ever been
 *		migrated (it is reset if the file is not open anywhere), while
 *		MIGRATING is set as a temporary flag during cache consistency
 *		operations, for a single reference to the file.
 */
#define FS_KERNEL_FLAGS		0xfffff000
#define FS_FOLLOW		0x00001000
#define FS_PREFIX		0x00002000
#define FS_SWAP			0x00004000
#define FS_USER			0x00008000
#define FS_USER_IN		FS_USER
#define FS_OWNERSHIP		0x00010000
#define FS_DELETE		0x00020000
#define FS_LINK			0x00040000
#define FS_RENAME		0x00080000
#define FS_CLIENT_CACHE_WRITE	0x00100000
#define FS_USER_OUT		0x00800000
#define	FS_SERVER_WRITE_THRU	0x01000000
#define	FS_LAST_DIRTY_BLOCK	0x02000000
/*
 * This was used in the sosp changes.  I've got to figure something else.
#define	FS_HEAP			0x02000000
 */ 
#define FS_RMT_SHARED		0x04000000
#define FS_NEW_STREAM		0x08000000
#define	FS_WRITE_TO_DISK	0x10000000
#define	FS_MAP			0x20000000
#define	FS_MIGRATED_FILE	0x40000000
#define	FS_MIGRATING		0x80000000


/*
 * Basic I/O parameters.  Note that the stream is identified by context,
 * either the pseudo-device connection or additional RPC parameters.
 * This structure is initialized in Fs_Read/Write and passed down to
 * the stream-specific I/O routines.  This record should be considered
 * read-only to avoid conflicts with other layers.
 * Fs_IOReply is used to return information about the transfer.
 */

typedef struct Fs_IOParam {
    Address	buffer;			/* Buffer for data */
    int		length;			/* Byte amount to transfer */
    int		offset;			/* Byte offset at which to transfer */
    int		flags;			/* Operation specific flags */
    Proc_PID	procID;			/* Process ID and Family ID of this */
    Proc_PID	familyID;		/* process */
    int		uid;			/* Effective user ID */
    int		reserved;		/* Not used */
} Fs_IOParam;

/*
 * FsSetIOParam - macro to initialize Fs_IOParam record.
 */
#define FsSetIOParam(ioPtr, zbuffer, zlength, zoffset, zflags) \
    { \
	register Proc_ControlBlock *procPtr = Proc_GetEffectiveProc(); \
	(ioPtr)->buffer = zbuffer; \
	(ioPtr)->length = zlength; \
	(ioPtr)->offset = zoffset; \
	(ioPtr)->flags = zflags; \
	(ioPtr)->procID = procPtr->processID; \
	(ioPtr)->familyID = procPtr->familyID; \
	(ioPtr)->uid = procPtr->userID; \
	(ioPtr)->reserved = 0; \
    }

/*
 * Fs_IOReply is used to return info from stream-specific I/O routines
 * back up to Fs_Read/Write.  The length has to be updated to reflect how
 * much data was transferred.  The routines can also cause a signal to
 * be generated by setting signal to non-zero.  The signal and code
 * are initialized to zero by top-level routines, so these fields can
 * be ignored if no signal is to be generated.  Finally, the flags field
 * contains the three select bits that indicate the state of the object
 * after the operation.
 */
typedef struct Fs_IOReply {
    int		length;			/* Amount transferred */
    int		flags;			/* Stream flags used, noted below. */
    int		signal;			/* Signal to generate, or zero */
    int		code;			/* Code to modify signal */
} Fs_IOReply;

/*
 * Flag bits for Fs_IOReply:
 *	FS_READABLE		Object is currently readable
 *	FS_WRITABLE		Object is currently writable
 *	FS_EXCEPTION		Object has an outstanding exception
 *
 * These bits are defined above because they come from the stream flags.
 */

/*
 * Parameters for I/O Control.  There is also a returned Fs_IOReply
 * that specifies the amount of valid data in the outBuffer,
 * and a signal to return, if any.
 */

typedef struct Fs_IOCParam {
    int		command;	/* I/O Control to perform. */
    Address	inBuffer;	/* Input buffer */
    int		inBufSize;	/* Size of input params to iocontrol. */
    Address	outBuffer;	/* Output buffer */
    int		outBufSize;	/* Size of results from iocontrol. */
    Fmt_Format	format;		/* Defines client's byte order/alignment
				 * format. */
    Proc_PID	procID;		/* ID of invoking process */
    Proc_PID	familyID;	/* Family of invoking process */
    int		uid;		/* Effective user ID */
    int		flags;		/* FS_USER_IN and FS_USER_OUT indicate if
				 * input and output buffers are in user space,
				 * respectively */
} Fs_IOCParam;

/*
 * FS_MAX_LINKS	- the limit on the number of symbolic links that can be
 *	expanded within a single domain.  This is also the limit on the
 *	number of re-directs between domains that can occur during lookup.
 */
#define FS_MAX_LINKS	10

/*
 * Values for  page type for Fs_PageRead.
 */
typedef enum {
    FS_CODE_PAGE,
    FS_HEAP_PAGE,
    FS_SWAP_PAGE,
    FS_SHARED_PAGE
} Fs_PageType;


/*
 * Device drivers use Fs_NotifyReader and Fs_NotifyWriter to indicate
 * that a device is ready.  They pass a Fs_NotifyToken as an argument
 * that represents to the file system the object that is ready.
 */
typedef Address Fs_NotifyToken;

/*
 * Flags returned from device driver open procedures.
 *
 * FS_DEV_DONT_LOCK	- Do not lock the device's handle during IO operations.
 *			  The default is to only allow one call to a device's
 *			  read or write procedures active at any time. Device's
 *			  that set this flag are responsible for doing their
 *			  own synchronization.
 * FS_DEV_DONT_COPY	- Do not copy user resident IO buffers in and out of
 *			  the kernel for this device. The default is to 
 *			  malloc() a kernel resident buffer for device IO
 *			  operation.  Device's that set this flag are
 *			  responsible for doing IO operations directly to
 *			  user's address spaces.
 */

#define	FS_DEV_DONT_LOCK	0x1
#define	FS_DEV_DONT_COPY	0x2

/*
 * TRUE once the file system has been initialized, so we
 * know we can sync the disks safely.
 */
extern  Boolean fsutil_Initialized;	

/*
 * These record the maximum transfer size supported by the RPC system.
 */
extern int fsMaxRpcDataSize;
extern int fsMaxRpcParamSize;

/*
 * Filesystem initialization calls.
 */
extern void Fs_Init _ARGS_((void));
extern void Fs_InitData _ARGS_((void));
extern void Fs_InitNameSpace _ARGS_((void));
extern void Fs_Bin _ARGS_((void));
extern void Fs_ProcInit _ARGS_((void));
extern void Fs_InheritState _ARGS_((Proc_ControlBlock *parentProcPtr,
				Proc_ControlBlock *newProcPtr));
extern void Fs_CloseState _ARGS_((Proc_ControlBlock *procPtr, int phase));


/*
 * Filesystem system calls.
 */
extern ReturnStatus Fs_AttachDiskStub _ARGS_((char *userDeviceName, 
			char *userLocalName, int flags));
extern ReturnStatus Fs_ChangeDirStub _ARGS_((char *pathName));
extern ReturnStatus Fs_RemoveStub _ARGS_((char *pathName));
extern ReturnStatus Fs_CommandStub _ARGS_((int command, int bufSize, 
			Address buffer));
extern ReturnStatus Fs_CreatePipeStub _ARGS_((int *inStreamIDPtr, 
			int *outStreamIDPtr));
extern ReturnStatus Fs_GetAttributesIDStub _ARGS_((int streamID, 
			Fs_Attributes *attrPtr));
extern ReturnStatus Fs_GetAttributesStub _ARGS_((char *pathName, 
			int fileOrLink, Fs_Attributes *attrPtr));
extern ReturnStatus Fs_GetNewIDStub _ARGS_((int streamID, int *newStreamIDPtr));
extern ReturnStatus Fs_HardLinkStub _ARGS_((char *fileName, char *linkName));
extern ReturnStatus Fs_IOControlStub _ARGS_((int streamID, int command, 
			int inBufSize, Address inBuffer, int outBufSize, 
			Address outBuffer));
extern ReturnStatus Fs_MakeDeviceStub _ARGS_((char *pathName, 
			Fs_Device *devicePtr, int permissions));
extern ReturnStatus Fs_MakeDirStub _ARGS_((char *pathName, int permissions));
extern ReturnStatus Fs_OpenStub _ARGS_((char *pathName, int usageFlags, 
			int permissions, int *streamIDPtr));
extern ReturnStatus Fs_ReadLinkStub _ARGS_((char *linkName, int bufSize, 
			char *buffer, int *linkSizePtr));
extern ReturnStatus Fs_ReadStub _ARGS_((int streamID, int amountRead, 
			Address buffer, int *amountReadPtr));
extern ReturnStatus Fs_ReadVectorStub _ARGS_((int streamID, int numVectors, 
			Fs_IOVector userVectorArray[], int *amountReadPtr));
extern ReturnStatus Fs_RemoveDirStub _ARGS_((char *pathName));
extern ReturnStatus Fs_RemoveDirStub _ARGS_((char *pathName));
extern ReturnStatus Fs_RenameStub _ARGS_((char *pathName, char *newName));
extern ReturnStatus Fs_SelectStub _ARGS_((int numStreams, Time *userTimeoutPtr,
			int *userReadMaskPtr, int *userWriteMaskPtr, 
			int *userExceptMaskPtr, int *numReadyPtr));
extern ReturnStatus Fs_SetAttributesIDStub _ARGS_((int streamID, 
			Fs_Attributes *attrPtr));
extern ReturnStatus Fs_SetAttributesStub _ARGS_((char *pathName, 
			int fileOrLink, Fs_Attributes *attrPtr));
extern ReturnStatus Fs_SetAttrIDStub _ARGS_((int streamID, 
			Fs_Attributes *attrPtr, int flags));
extern ReturnStatus Fs_SetAttrStub _ARGS_((char *pathName, int fileOrLink, 
			Fs_Attributes *attrPtr, int flags));
extern ReturnStatus Fs_SetDefPermStub _ARGS_((int permissions, int *oldPermPtr));
extern ReturnStatus Fs_SymLinkStub _ARGS_((char *targetName, 
			char *linkName, Boolean remoteFlag));
extern ReturnStatus Fs_WriteStub _ARGS_((int streamID, int writeLength, 
			Address buffer, int *writeLengthPtr));
extern ReturnStatus Fs_WriteVectorStub _ARGS_((int streamID, int numVectors,
			Fs_IOVector userVectorArray[], int *amountWrittenPtr));
extern ReturnStatus Fs_FileWriteBackStub _ARGS_((int streamID, int firstByte, 
			int lastByte, Boolean shouldBlock));

/*
 * Filesystem system calls given accessible arguments.
 */
extern ReturnStatus Fs_UserClose _ARGS_((int streamID));
extern ReturnStatus Fs_UserRead _ARGS_((int streamID, int amountRead,
			Address buffer, int *amountReadPtr));
extern ReturnStatus Fs_UserReadVector _ARGS_((int streamID, int numVectors, 
			Fs_IOVector *vectorPtr, int *amountReadPtr));
extern ReturnStatus Fs_UserWrite _ARGS_((int streamID, int writeLength, 
			Address buffer, int *writeLengthPtr));
extern ReturnStatus Fs_WriteVectorStub _ARGS_((int streamID, int numVectors, 
			Fs_IOVector userVectorArray[], int *amountWrittenPtr));
extern ReturnStatus Fs_UserWriteVector _ARGS_((int streamID, int numVectors, 
			Fs_IOVector *vectorPtr, int *amountWrittenPtr));

/*
 * Kernel equivalents of the filesystem system calls.
 */
extern ReturnStatus Fs_ChangeDir _ARGS_((char *pathName));
extern ReturnStatus Fs_Close _ARGS_((register Fs_Stream *streamPtr));
extern ReturnStatus Fs_Command _ARGS_((int command, int bufSize, 
			Address buffer));
extern ReturnStatus Fs_CheckAccess _ARGS_((char *pathName, int perm, 
			Boolean useRealID));
extern ReturnStatus Fs_GetAttributes _ARGS_((char *pathName, int fileOrLink,
			Fs_Attributes *attrPtr));
extern ReturnStatus Fs_GetNewID _ARGS_((int streamID, int *newStreamIDPtr));
extern ReturnStatus Fs_HardLink _ARGS_((char *pathName, char *linkName));
extern ReturnStatus Fs_IOControl _ARGS_((Fs_Stream *streamPtr,
			Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus Fs_MakeDevice _ARGS_((char *name, Fs_Device *devicePtr,
			int permissions));
extern ReturnStatus Fs_MakeDir _ARGS_((char *name, int permissions));
extern ReturnStatus Fs_Open _ARGS_((char *name, register int useFlags, 
			int type, int permissions, Fs_Stream **streamPtrPtr));
extern ReturnStatus Fs_Read _ARGS_((Fs_Stream *streamPtr, Address buffer, 
			int offset, int *lenPtr));
extern ReturnStatus Fs_Remove _ARGS_((char *name));
extern ReturnStatus Fs_RemoveDir _ARGS_((char *name));
extern ReturnStatus Fs_Rename _ARGS_((char *pathName, char *newName));
extern ReturnStatus Fs_SetAttributes _ARGS_((char *pathName, int fileOrLink, 
			Fs_Attributes *attrPtr, int flags));
extern ReturnStatus Fs_SymLink _ARGS_((char *targetName, char *linkName, 
			Boolean remoteFlag));
extern ReturnStatus Fs_Trunc _ARGS_((char *pathName, int length));
extern ReturnStatus Fs_TruncStream _ARGS_((Fs_Stream *streamPtr, int length));
extern ReturnStatus Fs_Write _ARGS_((Fs_Stream *streamPtr, Address buffer, 
			int offset, int *lenPtr));

/*
 * Filesystem utility routines.
 */
extern void Fs_CheckSetID _ARGS_((Fs_Stream *streamPtr, int *uidPtr, 
				int *gidPtr));
extern void Fs_CloseOnExec _ARGS_((Proc_ControlBlock *procPtr));


/*
 * Routines to support process migration: encapsulate and deencapsulate
 * streams and other file state, and clear file state after migration.
 */
extern ReturnStatus Fs_InitiateMigration _ARGS_((Proc_ControlBlock *procPtr, 
			int hostID, Proc_EncapInfo *infoPtr));
extern int Fs_GetEncapSize _ARGS_((void));
extern ReturnStatus Fs_EncapFileState _ARGS_((Proc_ControlBlock *procPtr, 
			int hostID, Proc_EncapInfo *infoPtr, 
			Address ptr));
extern ReturnStatus Fs_DeencapFileState _ARGS_((Proc_ControlBlock *procPtr, 
			 Proc_EncapInfo *infoPtr, Address buffer));

/*
 * Routines for virtual memory.
 */
extern ReturnStatus Fs_PageRead _ARGS_((Fs_Stream *streamPtr, Address pageAddr,
			int offset, int numBytes, Fs_PageType pageType));
extern ReturnStatus Fs_PageWrite _ARGS_((Fs_Stream *streamPtr,Address pageAddr,
			int offset, int numBytes, Boolean toDisk));
extern ReturnStatus Fs_PageCopy _ARGS_((Fs_Stream *srcStreamPtr, 
			Fs_Stream *destStreamPtr, int offset, int numBytes));
extern ReturnStatus Fs_FileBeingMapped _ARGS_((Fs_Stream *streamPtr, 
			int isMapped));

/*
 * Routines that map to/from user-level streamIDs.
 */
extern ReturnStatus Fs_GetStreamID _ARGS_((Fs_Stream *streamPtr, 
			int *streamIDPtr));
extern void Fs_ClearStreamID _ARGS_((int streamID, Proc_ControlBlock *procPtr));
extern ReturnStatus Fs_GetStreamPtr _ARGS_((Proc_ControlBlock *procPtr, 
			int streamID, Fs_Stream **streamPtrPtr));

extern ReturnStatus Fs_GetAttrStream _ARGS_((Fs_Stream *streamPtr, 
			Fs_Attributes *attrPtr));
extern ReturnStatus Fs_SetAttrStream _ARGS_((Fs_Stream *streamPtr,
			Fs_Attributes *attrPtr, Fs_UserIDs *idPtr, int flags));
extern void Fs_CheckSetID _ARGS_((Fs_Stream *streamPtr, int *uidPtr, 
			int *gidPtr));

extern ClientData Fs_GetFileHandle _ARGS_((Fs_Stream *streamPtr));
extern struct Vm_Segment **Fs_GetSegPtr _ARGS_((ClientData fileHandle));

#endif /* _FS */
