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

#include "sys.h"
#include "sync.h"
#include "user/fs.h"


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
 * FsFileID - Uniquely identify a filesystem object.  A type is the first
 *	field, the hostID of the server is next, and the remaining fields 
 *	are interpreted by the implementation of that type of filesystem object
 *	(ie. files, devices, pipes, pseudo-devices, etc.)
 *	A global hash table of filesystem objects, (called "handles")
 *	is maintained with this FsFileID as the hash key.
 */
typedef struct FsFileID {
    int		type;		/* Defined below. Used in I/O switch, and
				 * implicitly indicates what kind of structure
				 * follows the FsHandleHeader in the Handle. */
    int		serverID;	/* Host that controls the object.  (This would
				 * have to be a multi-cast ID for objects
				 * that support replication.) */
    int		major;		/* First type specific identifier. */
    int		minor;		/* Second type sepcific identifier. */
} FsFileID;			/* 16 BYTES */

/*
 * All kinds of things are referenced from the object hash table.  The generic
 * term for each structure is "handle".  The following structure defines a 
 * common structure needed in the beginning of each handle.
 */

typedef struct FsHandleHeader {
    FsFileID		fileID;		/* Used as the hash key. */
    int			flags;		/* Defined in fsHandle.c. */
    Sync_Condition	unlocked;	/* Notified when handle is unlocked. */
    int			refCount;	/* Used for garbage collection. */
} FsHandleHeader;			/* 28 BYTES */

/*
 * The following name-related information is referenced by each stream.
 * This identifies the name of the file, which will be different than
 * the file itself in the case of devices.  This is used to get to the
 * name server during set/get attributes operations.  Also, this name
 * fileID is used as the starting point for relative lookups.
 */

typedef struct FsNameInfo {
    FsFileID		fileID;		/* Identifies file and name server. */
    FsFileID		rootID;		/* ID of file system root.  Passed
					 * to name server to prevent ascending
					 * past the root of a domain with ".."*/
    int			domainType;	/* Name domain type */
    struct FsPrefix	*prefixPtr;	/* Back pointer to prefix table entry.
					 * This is kept for efficient handling
					 * of lookup redirects. */
    char		*name;		/* For console error messages. */
} FsNameInfo;

/*
 * Fs_Stream - A clients handle on an open file is defined by the Fs_Stream
 *      structure.  A stream has a read-write offset index, state flags
 *	that determine how the stream is being used (ie, for reading
 *	or writing, etc.), a stream type, a name state used in name
 *	related actions, and an I/O handle used for I/O operations.
 *	There are many different types of streams; they correspond
 *	to local files, remote files, local devices, remote devices,
 *	local pipes, remote pipes, locally cached named pipes,
 *	remotely cached named pipes, control streams for pseudo devices,
 *	psuedo streams, their corresponding server streams, etc. etc.
 *	The different stream types are defined in fsInt.h
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
} Fs_Stream;				/* 52 BYTES */

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
 *	FS_READ - Open for reading
 *	FS_WRITE - Open for writing
 *	FS_EXECUTE - Open for execution
 *	FS_APPEND - Open for append mode
 *	FS_NON_BLOCKING - Open a stream with non-blocking I/O operations. If
 *		the operation would normally block, FS_WOULD_BLOCK is returned.
 *
 *	(These open-time bit values are defined below)
 *      FS_FOLLOW - follow symbolic links during look up.
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
 *	FS_WRITE_THRU_ASAP - Write blocks for this file to disk as soon as
 *		possible after the block is put into the cache.
 *	FS_LAST_DIRTY_BLOCK - Set on remote writes when this is the
 *		last dirty block of the file to be written back.
 *	FS_RMT_SHARED - Set on streams that are shared among clients on
 *		separate machines.  For regular files this means that the
 *		stream offset is being maintained on the server.
 *	FS_NEW_STREAM - Migration related.  This tells the I/O server that
 *		the destination of a migration is getting a stream for
 *		the first time.   It needs to know this in order to do
 *		I/O client book-keeping correctly.
 */
#define FS_KERNEL_FLAGS		0xfffff000
#define FS_FOLLOW		0x00001000
#define FS_SWAP			0x00004000
#define FS_USER			0x00008000
#define FS_OWNERSHIP		0x00010000
#define FS_DELETE		0x00020000
#define FS_LINK			0x00040000
#define FS_RENAME		0x00080000
#define FS_CLIENT_CACHE_WRITE	0x00100000
#define FS_CONSUME		0x00200000
#define FS_TRACE_FLAG		0x00400000
#define	FS_WRITE_THRU_ASAP	0x01000000
#define	FS_LAST_DIRTY_BLOCK	0x02000000
#define FS_RMT_SHARED		0x04000000
#define FS_NEW_STREAM		0x08000000

/*
 * Flags to Fs_WaitForHost
 *	FS_NAME_SERVER		Wait on the name server for the stream
 *	FS_IO_SERVER		Wait on the I/O server for the stream
 *	FS_NON_BLOCKING		Don't wait, just start recovery in background,
 *				(this is defined in user/fs.h)
 */
#define FS_NAME_SERVER		0x1
#define FS_IO_SERVER		0x2

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
 * Fs_MakeNameAccessible uses Proc_MakeNameAccessible to make a string
 * accessible if it isn't already in the kernel's address space.
 */

#define Fs_MakeNameAccessible(pathPtrPtr, numBytesPtr) \
		Proc_MakeStringAccessible(FS_MAX_PATH_NAME_LENGTH, \
		(pathPtrPtr), numBytesPtr, (int *) NIL)
#ifdef comment
    char **pathPtrPtr;
#endif comment

/*
 * Values for  page type for Fs_PageRead.
 */
typedef enum {
    FS_CODE_PAGE,
    FS_HEAP_PAGE,
    FS_SWAP_PAGE,
} Fs_PageType;
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
extern	void		Fs_HandleScavengeStub();
extern	void		Fs_PrintTrace();
extern  void		Fs_BlocksToDiskAddr();

/*
 * Routines to support process migration: encapsulate and deencapsulate
 * streams and other file state, and clear file state after migration.
 */
extern	ReturnStatus	Fs_EncapStream();
extern	ReturnStatus	Fs_DeencapStream();
extern	int		Fs_GetEncapSize();
extern	ReturnStatus	Fs_StreamCopy();
extern  ReturnStatus    Fs_EncapFileState();
extern  ReturnStatus    Fs_DeencapFileState();
extern  void            Fs_ClearFileState();

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

#endif _FS
