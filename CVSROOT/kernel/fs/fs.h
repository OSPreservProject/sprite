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
#include "sys.h"
#include "sync.h"
#include "proc.h"
#include "user/fs.h"
#else
#include <kernel/sys.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <fs.h>
#endif


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
 * operations on handles.  One exception is that the refCount on FS_STREAM
 * handles is manipulated by the stream routines.  The handle must be
 * locked when examining the refCount, and it should only be changed
 * under the handle monitor lock by the FsHandle* routines.
 */

typedef struct FsHandleHeader {
    Fs_FileID		fileID;		/* Used as the hash key. */
    int			flags;		/* Defined in fsHandle.c. */
    Sync_Condition	unlocked;	/* Notified when handle is unlocked. */
    int			refCount;	/* Used for garbage collection. */
    char		*name;		/* Used for error messages */
    List_Links		lruLinks;	/* For LRU list of handles */
#ifndef CLEAN
    int			lockProcID;	/* Process ID of locker */
#endif
} FsHandleHeader;

#define LRU_LINKS_TO_HANDLE(listPtr) \
	( (FsHandleHeader *)((int)(listPtr) - sizeof(Fs_FileID) \
		- 2 * sizeof(int) - sizeof(char *) - sizeof(Sync_Condition)) )

/*
 * The following name-related information is referenced by each stream.
 * This identifies the name of the file, which will be different than
 * the file itself in the case of devices.  This is used to get to the
 * name server during set/get attributes operations.  Also, this name
 * fileID is used as the starting point for relative lookups.
 */

typedef struct FsNameInfo {
    Fs_FileID		fileID;		/* Identifies file and name server. */
    Fs_FileID		rootID;		/* ID of file system root.  Passed
					 * to name server to prevent ascending
					 * past the root of a domain with ".."*/
    int			domainType;	/* Name domain type */
    struct FsPrefix	*prefixPtr;	/* Back pointer to prefix table entry.
					 * This is kept for efficient handling
					 * of lookup redirects. */
} FsNameInfo;

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
    FsHandleHeader	hdr;		/* Global stream identifier.  This
					 * includes a reference count which
					 * is incremented on fork/dup */
    int			offset;		/* File access position */
    int			flags;		/* Flags defined below */
    FsHandleHeader	*ioHandlePtr;	/* Stream specific data used for I/O.
					 * This really references a somewhat
					 * larger object, see fsInt.h */
    FsNameInfo	 	*nameInfoPtr;	/* Used to contact the name server */
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
 *	FS_LINK - This is used by FsLocalLookup to make hard links.  Instead
 *		of creating a new file, FsLocalLookup makes another directory
 *		reference (hard link) to an existing file.
 *	FS_RENAME - This goes with FS_LINK if the link is being made in
 *		preparation for a rename operation.  This allows the link,
 *		which in this case is temporary, to be made to a directory.
 *
 *	(These I/O related bit values are defined below)
 *      FS_USER - the file is a user file.  The buffer space is
 *              in a user's address space.
 *	FS_CLIENT_CACHE_WRITE -	This write is coming from a client's cache.
 *              This means the modify time should not be updated
 *              since the client has the correct modify time.
 *	FS_CONSUME - This is a consuming read from a named pipe.  This is
 *		usually the case for named pipes, although when the ioServer
 *		is writing back blocks from its cache to the file server
 *		this flag is not set.
 *	FS_TRACE_FLAG - This is used to enable the taking of trace records
 *		by low level routines.  This means that the tracing can
 *		be confined to particular operations, like open, while
 *		other operations, like remove, don't pollute the trace.
 *	FS_SERVER_WRITE_THRU - Set on writes that are supposed to be written
 *			       through to the server.
 *	FS_LAST_DIRTY_BLOCK - Set on remote writes when this is the
 *		last dirty block of the file to be written back.
 *	FS_RMT_SHARED - Set on streams that are shared among clients on
 *		separate machines.  For regular files this means that the
 *		stream offset is being maintained on the server.
 *	FS_NEW_STREAM - Migration related.  This tells the I/O server that
 *		the destination of a migration is getting a stream for
 *		the first time.   It needs to know this in order to do
 *		I/O client book-keeping correctly.
 *	FS_WB_ON_LDB - Write this file back to disk if this is the last dirty
 *		       block.
 */
#define FS_KERNEL_FLAGS		0xfffff000
#define FS_FOLLOW		0x00001000
#define FS_PREFIX		0x00002000
#define FS_SWAP			0x00004000
#define FS_USER			0x00008000
#define FS_OWNERSHIP		0x00010000
#define FS_DELETE		0x00020000
#define FS_LINK			0x00040000
#define FS_RENAME		0x00080000
#define FS_CLIENT_CACHE_WRITE	0x00100000
#define FS_CONSUME		0x00200000
#define FS_TRACE_FLAG		0x00400000
#define	FS_SERVER_WRITE_THRU	0x01000000
#define	FS_LAST_DIRTY_BLOCK	0x02000000
#define FS_RMT_SHARED		0x04000000
#define FS_NEW_STREAM		0x08000000
#define	FS_WB_ON_LDB		0x10000000


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
    int		gid;			/* Effective group ID */
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
	if (procPtr->fsPtr->numGroupIDs > 0) { \
	    (ioPtr)->gid = procPtr->fsPtr->groupIDs[0]; \
	} else { \
	    (ioPtr)->gid = -1; \
	} \
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
    int		byteOrder;	/* Defines client's byte ordering */
    Proc_PID	procID;		/* ID of invoking process */
    Proc_PID	familyID;	/* Family of invoking process */
    int		uid;		/* Effective user ID */
    int		gid;		/* Effective group ID */
} Fs_IOCParam;

/*
 * Fragment stuff that is dependent on the filesystem block size
 * defined in user/fs.h.  Actually, the fragmenting code is specific
 * to a 4K block size and a 1K fragment size.
 */

#define	FS_BLOCK_OFFSET_MASK	(FS_BLOCK_SIZE - 1)
#define	FS_FRAGMENT_SIZE	1024
#define	FS_FRAGMENTS_PER_BLOCK	4

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
    FS_SWAP_PAGE
} Fs_PageType;

/*
 * Buffer type that includes size, location, and kernel space flag.
 * This is passed into Fs_IOControl to specify the input/output buffers.
 */
typedef struct Fs_Buffer {
    Address addr;
    int size;
    int flags;		/* 0 or FS_USER */
} Fs_Buffer;

/*
 * Device drivers use Fs_NotifyReader and Fs_NotifyWriter to indicate
 * that a device is ready.  They pass a Fs_NotifyToken as an argument
 * that represents to the file system the object that is ready.
 */
typedef Address Fs_NotifyToken;

/*
 * Filesystem initialization calls.
 */
extern	void	Fs_Init();
extern	void	Fs_ProcInit();
extern	void	Fs_SetupStream();
extern	void	Fs_InheritState();
extern	void	Fs_CloseState();

/*
 * Prefix Table routines.
 */
extern	void	Fs_PrefixLoad();
extern	void	Fs_PrefixExport();

/*
 * Filesystem processes.
 */

extern	void	Fs_SyncProc();
extern	void	Fs_Sync();
extern	void	Fs_BlockCleaner();
extern	void	Fs_ConsistProc();
/*
 * Filesystem system calls.
 */
extern	ReturnStatus	Fs_AttachDiskStub();
extern	ReturnStatus	Fs_ChangeDirStub();
extern	ReturnStatus	Fs_CommandStub();
extern	ReturnStatus	Fs_CreatePipeStub();
extern	ReturnStatus	Fs_GetAttributesIDStub();
extern	ReturnStatus	Fs_GetAttributesStub();
extern	ReturnStatus	Fs_GetNewIDStub();
extern	ReturnStatus	Fs_HardLinkStub();
extern	ReturnStatus	Fs_IOControlStub();
extern	ReturnStatus	Fs_LockStub();
extern	ReturnStatus	Fs_MakeDeviceStub();
extern	ReturnStatus	Fs_MakeDirStub();
extern	ReturnStatus	Fs_OpenStub();
extern	ReturnStatus	Fs_ReadLinkStub();
extern	ReturnStatus	Fs_ReadStub();
extern	ReturnStatus	Fs_ReadVectorStub();
extern	ReturnStatus	Fs_RemoveDirStub();
extern	ReturnStatus	Fs_RemoveStub();
extern	ReturnStatus	Fs_RenameStub();
extern	ReturnStatus	Fs_SelectStub();
extern	ReturnStatus	Fs_SetAttributesIDStub();
extern	ReturnStatus	Fs_SetAttributesStub();
extern	ReturnStatus	Fs_SetAttrIDStub();
extern	ReturnStatus	Fs_SetAttrStub();
extern	ReturnStatus	Fs_SetDefPermStub();
extern	ReturnStatus	Fs_SymLinkStub();
extern	ReturnStatus	Fs_TruncateIDStub();
extern	ReturnStatus	Fs_TruncateStub();
extern	ReturnStatus	Fs_WriteStub();
extern	ReturnStatus	Fs_WriteVectorStub();
extern	ReturnStatus	Fs_FileWriteBackStub();

/*
 * Filesystem system calls given accessible arguments.
 */
extern	ReturnStatus	Fs_UserClose();
extern	ReturnStatus	Fs_UserRead();
extern	ReturnStatus	Fs_UserReadVector();
extern	ReturnStatus	Fs_UserWrite();
extern	ReturnStatus	Fs_UserWriteVector();

/*
 * Kernel equivalents of the filesystem system calls.
 */
extern	ReturnStatus	Fs_ChangeDir();
extern	ReturnStatus	Fs_Close();
extern	ReturnStatus	Fs_Command();
extern	ReturnStatus	Fs_CreatePipe();
extern	ReturnStatus	Fs_CheckAccess();
extern	ReturnStatus	Fs_GetAttributes();
extern	ReturnStatus	Fs_GetAttributesID();
extern	ReturnStatus	Fs_GetNewID();
extern	ReturnStatus	Fs_HardLink();
extern	ReturnStatus	Fs_IOControl();
extern	ReturnStatus	Fs_Lock();
extern	ReturnStatus	Fs_MakeDevice();
extern	ReturnStatus	Fs_MakeDir();
extern	ReturnStatus	Fs_Open();
extern	ReturnStatus	Fs_Read();
extern	ReturnStatus	Fs_ReadLink();
extern	ReturnStatus	Fs_Remove();
extern	ReturnStatus	Fs_RemoveDir();
extern	ReturnStatus	Fs_Rename();
extern	ReturnStatus	Fs_SetAttributes();
extern	ReturnStatus	Fs_SetAttributesID();
extern	ReturnStatus	Fs_SetDefPerm();
extern	ReturnStatus	Fs_SymLink();
extern	ReturnStatus	Fs_Trunc();
extern	ReturnStatus	Fs_TruncFile();
extern	ReturnStatus	Fs_TruncID();
extern	ReturnStatus	Fs_Write();

/*
 * Filesystem utility routines.
 */
extern	Boolean		Fs_SameFile();
extern	int		Fs_Cat();
extern	int		Fs_Copy();
extern	void		Fs_HandleScavenge();
extern	void		Fs_PrintTrace();
extern  void		Fs_BlocksToDiskAddr();
extern  void		Fs_CheckSetID();
extern  void		Fs_CloseOnExec();
extern	char *		Fs_GetFileName();

/*
 * Routines called via the L1 key bindings.
 */
extern	void		Fs_HandleScavengeStub();
extern	void		Fs_PdevPrintTrace();
extern	void		Fs_DumpCacheStats();
extern	void		Fs_NameHashStats();

/*
 * Routines to support process migration: encapsulate and deencapsulate
 * streams and other file state, and clear file state after migration.
 */
extern	ReturnStatus	Fs_EncapStream();
extern	ReturnStatus	Fs_DeencapStream();
extern	int		Fs_GetEncapSize();
extern	ReturnStatus	Fs_InitiateMigration();
extern	void		Fs_StreamCopy();
extern  ReturnStatus    Fs_EncapFileState();
extern  ReturnStatus    Fs_DeencapFileState();

/*
 * Routines to wakeup readers and writers.
 */
extern	void		Fs_NotifyReader();
extern	void		Fs_NotifyWriter();
extern	void		Fs_WakeupProc();

/*
 * Routines for virtual memory.
 */
extern	ReturnStatus	Fs_PageRead();
extern	ReturnStatus	Fs_PageWrite();
extern	ReturnStatus	Fs_PageCopy();
extern	void		Fs_CacheBlocksUnneeded();
extern	void		Fs_GetPageFromFS();
extern	ClientData	Fs_GetFileHandle();
extern struct Vm_Segment **Fs_GetSegPtr();

#endif /* _FS */
