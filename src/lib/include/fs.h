/*
 * fs.h --
 *
 *	Definitions and types used in the user's interface to
 *	the filesystem.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/fs.h,v 1.21 91/12/04 14:27:03 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _FS_H
#define _FS_H

#include <spriteTime.h>
#include <kernel/procTypes.h>

/*
 * The macros major and minor are defined in sys/types.h.  They are also
 * the names of fields in a structure defined in fs.h.  Only gcc and ANSI
 * C are clever enough to handle this.  (The field name isn't followed
 * by an open paren...)  So, if you include this file and <sys/types.h>
 * then you can use the major() and minor() macros.  However, <sys/types.h>
 * also defines unix_major() and unix_minor, so you can use those.
 */
#ifndef __STDC__ 
#ifdef major
#undef	major
#endif
#ifdef minor 
#undef	minor
#endif 
#endif /* ! __STDC__ */

/*
 * Global constants.
 * FS_BLOCK_SIZE - the size of filesystem blocks
 * FS_MAX_PATH_NAME_LENGTH - the maximum length of a complete pathname.
 * FS_MAX_NAME_LENGTH - is the maximum length of one component of a name.
 */
#define	FS_BLOCK_SIZE		4096
#define FS_MAX_PATH_NAME_LENGTH	1024
#define FS_MAX_NAME_LENGTH	255



/*
 * Open stream flags that are passed to Fs_Open from user programs.  These
 *	flags are kept in a kernel data structure along with some other
 *	flags used by the operating system internally.  This fact is only
 *	important to pseudo-device and pseudo-file-system servers which
 *	may see the other flags, which are defined in <kernel/fs.h>.
 *
 *	FS_READ		- open the file for read access.
 *	FS_WRITE	- open the file for write access, can be combined
 *			  with FS_READ
 *	FS_EXECUTE	- open the file for execute access.  This mode is used
 *			  by the kernel when opening a.out files.  It can
 *			  be used to limit the open to executables files.
 *	FS_APPEND	- open for append mode. All writes get appended to the
 *			  end of the file regardless of the file pointer.
 *	FS_CLOSE_ON_EXEC - close the stream when the process execs.
 *	FS_NON_BLOCKING - I/O operations don't block (if applicable) but
 *			  instead return FS_WOULD_BLOCK.
 *	FS_CREATE	- create the file if it doesn't exist.
 *	FS_TRUNC	- truncate the file to zero length.
 *	FS_EXCLUSIVE	- If specified with FS_CREATE the open/create will
 *			  fail if the file already exists.
 *	FS_NAMED_PIPE_OPEN - Open as a named pipe. (NOT IMPLEMENTED)
 *	FS_PDEV_MASTER 	- Caller wants to be master of the pseudo-device.
 *	FS_PFS_MASTER 	- Caller wants to be server of the pseudo-filesystem.
 */
#define FS_USER_FLAGS	  		0xfff
#define FS_READ		  		0x001
#define FS_WRITE	  		0x002
#define FS_EXECUTE	  		0x004
#define FS_APPEND	  		0x008
#define FS_CLOSE_ON_EXEC		0x010
#define FS_PDEV_MASTER			0x020
#define FS_NAMED_PIPE_OPEN		0x040
#define	FS_PFS_MASTER			0x080
#define FS_NON_BLOCKING			0x100
#define FS_CREATE	  		0x200
#define FS_TRUNC	  		0x400
#define FS_EXCLUSIVE	  		0x800
/*			More high order bits are defined in <kernel/fs.h> !! */

/*
 * Flags for Fs_Select:
 *
 *	FS_READABLE	- Does the stream have data that can be read?
 *	FS_WRITABLE	- Can data be written to the stream?
 *	FS_EXCEPTION	- Are there any exception conditions that have
 *			  raised for the stream? (e.g. out-of-band data).
 */

#define FS_READABLE	FS_READ
#define FS_WRITABLE	FS_WRITE
#define FS_EXCEPTION	FS_EXECUTE
#define FS_EXCEPTABLE	FS_EXCEPTION



/*
 * The Fs_Attributes type is the information returned about a file
 * from the Fs_GetAttributes and Fs_GetAttributesID system calls.
 * This struct is also the input parameter for the Fs_SetAttributes
 * and Fs_SetAttributesID system calls.
 */
typedef struct Fs_Attributes {
    int	serverID;		/* Host ID of file server */
    int domain;			/* Server-relative domain number of the file */
    int fileNumber;		/* Domain-relative file number */
    int type;			/* File types defined below */
    int size;			/* Number of bytes in the file */
    int numLinks;		/* Number of directory references to the file */
    unsigned int permissions;	/* Permission bits defined below */
    int uid;			/* User ID of file's owner */
    int gid;			/* ID of file's owning group */
    int devServerID;		/* ID of device server */
    int devType;		/* Type of the device */
    int devUnit;		/* Interpreted by the device driver */
    Time createTime;		/* Time of the files creation */
    Time accessTime;		/* Time of last access to the file */
    Time descModifyTime;	/* Time the file descriptor was last modified */
    Time dataModifyTime;	/* Time the file's data was last modified */
    int  blocks;		/* The number of blocks taken by the file */
    int  blockSize;		/* The size of each block */
    int	version;		/* This is incremented when file is written */
    int userType;		/* User defined file type */
    int pad[4];			/* Reserved */
} Fs_Attributes;

/*
 * The following are values for the fileOrLink argument to Fs_Set/GetAttributes.
 *	FS_ATTRIB_LINK	Get the attributes of the named link, not of the
 *		file the link refers to.
 *	FS_ATTRIB_FILE	Get the attributes of the name file.  If the last
 *		component of the file name is a link then use the file
 *		to which the link refers.
 */
#define FS_ATTRIB_LINK			1
#define FS_ATTRIB_FILE			2

/*
 * The following are values for the flags passed to Fs_SetAttr and Fs_SetAttrID
 *	FS_SET_ALL_ATTRS - Attempt to set all settable attributes (see below)
 *	FS_SET_TIMES	- Set data modify and file access times.
 *	FS_SET_MODE	- Set the permission mode bits of the file.
 *	FS_SET_OWNER	- Set the owner and group owner of a file.
 *	FS_SET_FILE_TYPE - Set the user-defined file type of a file.
 *	FS_SET_DEVICE	- Set device attributes - server, type, unit
 */
#define FS_SET_ALL_ATTRS	0x1F
#define FS_SET_TIMES		0x01
#define FS_SET_MODE		0x02
#define FS_SET_OWNER		0x04
#define FS_SET_FILE_TYPE	0x08
#define FS_SET_DEVICE		0x10

/*
 * FS_LOCALHOST_ID is used as the device server ID for generic devices,
 * those expected to exist on all hosts.  It is also used in the kernel
 * when the "ioServerID" is the local host.
 */
#define FS_LOCALHOST_ID		-1

/*
 * File types kept in FsFileDescriptors on disk:
 *	FS_FILE			ordinary disk file
 *	FS_DIRECTORY		file used to implement the directory stucture
 *	FS_SYMBOLIC_LINK	regular file used to implement links
 *	FS_REMOTE_LINK		symbolic link used to mark the top of a domain
 *	FS_DEVICE		Placeholder for peripheral device
 *	FS_REMOTE_DEVICE	not used
 *	FS_LOCAL_PIPE		Temporary half-duplex pipe
 *	FS_NAMED_PIPE		Persistent half-duplex pipe (not implemented)
 *	FS_PSEUDO_DEV		Full duplex communication to a user process
 *	FS_PSEUDO_FS		Marks a domain controlled by a user process
 *	FS_XTRA_FILE		Extra file type used to stage the
 *				(re)implementation of standard file types
 */

#define	FS_FILE				0
#define	FS_DIRECTORY			1
#define	FS_SYMBOLIC_LINK		2
#define	FS_REMOTE_LINK			3
#define	FS_DEVICE			4
#define	FS_REMOTE_DEVICE		5
#define	FS_LOCAL_PIPE			6
#define	FS_NAMED_PIPE			7
#define	FS_PSEUDO_DEV			8
#define FS_PSEUDO_FS			9
#define FS_XTRA_FILE			10


/*
 * User-defined file types.  A number of types are standardized, but others
 * may be defined by the user.
 *
 *	 FS_USER_TYPE_UNDEFINED		- no type set
 *	 FS_USER_TYPE_TMP      		- temporary file
 *	 FS_USER_TYPE_SWAP     		- swap file
 *	 FS_USER_TYPE_OBJECT   		- ".o" file
 *	 FS_USER_TYPE_BINARY   		- executable
 *	 FS_USER_TYPE_OTHER   		- file that doesn't correspond to any
 *					  specific type.  This is distinct from
 *					  undefined, which says the type is
 *					  uninitialized and may be inferred by
 *					  parent directory or file name.
 */
#define FS_USER_TYPE_UNDEFINED  0
#define FS_USER_TYPE_TMP        1
#define FS_USER_TYPE_SWAP	2
#define FS_USER_TYPE_OBJECT	3
#define FS_USER_TYPE_BINARY	4
#define FS_USER_TYPE_OTHER	5


/*
 * The Fs_FileID and Fs_UserIDs types are exported to user-level so that
 * pseudo-filesystem servers can understand the arguments to lookup operations
 * that are defined in fsNameOps.h
 *
 * Fs_FileID - Uniquely identify a filesystem object.  A type is the first
 *	field, the hostID of the server is next, and the remaining fields 
 *	are interpreted by the implementation of that type of filesystem object
 *	(ie. files, devices, pipes, pseudo-devices, etc.)
 *	A global hash table of filesystem objects, (called "handles")
 *	is maintained with this Fs_FileID as the hash key.
 */
typedef struct Fs_FileID {
    int		type;		/* Defined in kernel fsio.h (stream types).
				 * Used in I/O switch, and implicitly
				 * indicates what kind of structure follows
				 * the FsHandleHeader in the Handle. */
    int		serverID;	/* Host that controls the object.  (This would
				 * have to be a multi-cast ID for objects
				 * that support replication.) */
    int		major;		/* First type specific identifier. */
    int		minor;		/* Second type sepcific identifier. */
} Fs_FileID;			/* 16 BYTES */

/*
 *	The FS_NUM_GROUPS constant limits the number of group IDs that
 *	are used even though the proc table supports a variable number.
 */
#define FS_NUM_GROUPS	8

typedef struct Fs_UserIDs {
    int user;			/* Indicates effective user ID */
    int numGroupIDs;		/* Number of valid entries in groupIDs */
    int group[FS_NUM_GROUPS];	/* The set of groups the user is in */
} Fs_UserIDs;			/* 40 BYTES */




/*
 * Generic IO Control operations.
 *	IOC_REPOSITION		Reposition the current offset into the file.
 *	IOC_GET_FLAGS		Return the flags associated with the stream.
 *	IOC_SET_FLAGS		Set all the flags for the stream.
 *	IOC_SET_BITS		Set some of the flags for the stream.
 *	IOC_CLEAR_BITS		Clear some of the flags for the stream.
 *	IOC_TRUNCATE		Truncate the stream to a given length.
 *	IOC_LOCK		Lock the stream or underlying file.
 *	IOC_UNLOCK		Unlock the stream.
 *	IOC_NUM_READABLE	Return the number of bytes available.
 *	IOC_GET_OWNER		Return the process or family that gets signals.
 *	IOC_SET_OWNER		Set the process or family that gets signals.
 *	IOC_MAP			Map the stream into the processes VM
 *	IOC_PREFIX		Get the prefix under which the stream was
 *				opened.  This is useful if a server is exporting
 *				a domain under more than one name.  The getwd()
 *				library call uses this feature.
 *	IOC_WRITE_BACK		Write back any cached stream data.
 *	IOC_MMAP_INFO		Provide server with information on
 *				VM memory mapping.
 *
 *	IOC_GENERIC_LIMIT	This is the maximum IOC number that can be
 *				used for the generic I/O controls supported
 *				by the kernel.  Device drivers define I/O
 *				control numbers above this limit.  This limit
 *				is used in the kernel to optimize handling
 *				generic vs. non-generic I/O controls.
 */

#define	IOC_REPOSITION			1
#define	IOC_GET_FLAGS			2
#define IOC_SET_FLAGS			3
#define IOC_SET_BITS			4
#define IOC_CLEAR_BITS			5
#define IOC_TRUNCATE			6
#define IOC_LOCK			7
#define IOC_UNLOCK			8
#define IOC_NUM_READABLE		9
#define IOC_GET_OWNER			10
#define IOC_SET_OWNER			11
#define IOC_MAP				12
#define IOC_PREFIX			13
#define IOC_WRITE_BACK			14
#define	IOC_MMAP_INFO			15
#define IOC_GENERIC_LIMIT		((1<<16)-1)

/*
 * Maximum number of bytes that be copied in on an iocontrol.
 */

#define	IOC_MAX_BYTES	4096


/*
 * IOC_REPOSITION - reposition the file access position.
 */

typedef struct Ioc_RepositionArgs {
    int base;	/* Base at which to start reposition: defines below */
    int offset;	/* Offset from base */
} Ioc_RepositionArgs;

/*
 * Base argument definitions:
 *	IOC_BASE_ZERO		base is the beginning of the file.
 *	IOC_BASE_CURRENT	base is the current position in the file.
 *	IOC_BASE_EOF		base is the end of the file.
 */

#define IOC_BASE_ZERO		0
#define IOC_BASE_CURRENT	1
#define IOC_BASE_EOF		2



/*
 *	IOC_GET_FLAGS		Return the flags associated with the stream.
 *	IOC_SET_FLAGS		Set all the flags for the stream.
 *	IOC_SET_BITS		Set some of the flags for the stream.
 *	IOC_CLEAR_BITS		Clear some of the flags for the stream.
 *
 *	A few of the low order bits in the flags field are reserved
 *	for use by the kernel.  The rest bits are left for interpretation
 *	by the (pseudo) device driver.
 *		IOC_APPEND	Do append mode writes to the stream
 *		IOC_NON_BLOCKING Do not block if I/O is not ready
 *		IOC_ASYNCHRONOUS Dispatch I/O and signal when complete
 *				 This is not implemented yet, 6/87
 *		IOC_CLOSE_ON_EXEC This forces the stream to be closed when
 *				the process execs another program.
 *		IOC_READ	Stream is open for reading.
 *		IOC_WRITE	Stream is open for writing.
 */

#define IOC_GENERIC_FLAGS	0xFF
#define	IOC_APPEND		0x01
#define IOC_NON_BLOCKING	0x02
#define IOC_ASYNCHRONOUS	0x04
#define IOC_CLOSE_ON_EXEC	0x08
#define IOC_READ		0x10
#define IOC_WRITE		0x20


/*
 *	IOC_LOCK		Lock the stream or underlying file.
 *	IOC_UNLOCK		Unlock the stream.
 */
typedef struct Ioc_LockArgs {
    int		flags;		/* IOC_LOCK_EXCLUSIVE, no other locks allowed
				 * IOC_LOCK_SHARED, can have many of these,
				 *	but no exclusive locks 
				 * IOC_LOCK_NO_BLOCK, don't block if the lock
				 *	can't be obtained, return FS_WOUD_BLOCK
				 */
    /*
     * The following fields are set by the kernel and used by
     * lower levels to notify when the lock is obtainable.  Pseudo-device
     * masters use IOC_PDEV_LOCK_READY IOControl to do this notify.
     */
    int		hostID;		/* Set by the kernel */
    Proc_PID	pid;		/* Set by the kernel */
    int		token;		/* Set by the kernel */
} Ioc_LockArgs;

#define IOC_LOCK_SHARED			0x1
#define IOC_LOCK_EXCLUSIVE		0x2
#define IOC_LOCK_NO_BLOCK		0x8

/*
 *	IOC_GET_OWNER		Return the process or family that gets signals.
 *	IOC_SET_OWNER		Set the process or family that gets signals.
 */
typedef struct Ioc_Owner {
    Proc_PID	id;		/* Process or Family ID */
    int		procOrFamily;	/* IOC_OWNER_FAMILY or IOC_OWNER_PROC */
} Ioc_Owner;

#define IOC_OWNER_FAMILY	0x1
#define IOC_OWNER_PROC		0x2

/*
 *	IOC_MAP			Map the stream into the processes VM
 */
typedef struct Ioc_MapArgs {
    int		numBytes;
    Address	address;
} Ioc_MapArgs;

/*
 *	IOC_PREFIX		Return prefix under which stream was opened.
 */
typedef struct Ioc_PrefixArgs {
    char	prefix[FS_MAX_PATH_NAME_LENGTH];  /* Set by kernel */
} Ioc_PrefixArgs;

/*
 *	IOC_WRITE_BACK		Write back the cached data of a file.
 *				Although the arguments are in terms
 *				of bytes, the cache will block align
 *				the write-back so the bytes are fully
 *				included in the blocks written back.
 */
typedef struct Ioc_WriteBackArgs {
    int		firstByte;	/* Index of first byte to write back */
    int		lastByte;	/* Index of last byte to write back */
    Boolean	shouldBlock;	/* If TRUE, call blocks until write back done */
} Ioc_WriteBackArgs;

/*
 *	IOC_MMAP_INFO		Give information to the server that a
 *				client is mapping a stream into memory.
 */
typedef struct Ioc_MmapInfoArgs {
    int		isMapped;	/* 1 if mapping, 0 if unmapping. */
    int		clientID;	/* ID of the requesting client. */
} Ioc_MmapInfoArgs;

/*
 * A mask of 9 permission bits is used to define the permissions on a file.
 * A mask like this occurs in the FileDescriptor for a file.  A mask like
 * this is also part of the state of each process.  It defines the maximal
 * set of permissions that a newly created file can have. The following
 * define the various permission bits.
 *	FS_OWNER_{READ|WRITE|EXEC}	A process with a UID that matches the
 *			file's UID has {READ|WRITE|EXEC} permission on the file.
 *	FS_GROUP_{READ|WRITE|EXEC}	A process with one of its group IDS
 *			that matches the file's GID has permission...
 *	FS_WORLD_{READ|WRITE|EXEC}	Any process has permission if WORLD
 *			permission bits are set.
 */
#define FS_OWNER_READ			00400
#define FS_OWNER_WRITE			00200
#define FS_OWNER_EXEC			00100
#define FS_GROUP_READ			00040
#define FS_GROUP_WRITE			00020
#define FS_GROUP_EXEC			00010
#define FS_WORLD_READ			00004
#define FS_WORLD_WRITE			00002
#define FS_WORLD_EXEC			00001

/*
 * Other permission bits:
 *	FS_SET_UID	This bit set on a program image or shell script
 *		causes the execed process to take on the user id of
 *		the file.  (Thanks to Dennis Ritchie for this great idea.)
 *	FS_SET_GID	As above, but for the group id.
 */
#define FS_SET_UID			04000
#define FS_SET_GID			02000



/*
 * Values of the mode argument to the Fs_CheckAccess system call.
 *	FS_EXISTS	does the file exists (can the caller see it)
 *	FS_READ		does the caller have read access
 *	FS_WRITE	does the caller have write access
 *	FS_EXECUTE	does the caller have execution access
 */
#define FS_EXISTS		0x0

/*
 * Flag to Fs_GetNewID call that says choose any new stream ID.
 */
#define FS_ANYID			-1

/*
 * The Fs_AttachDisk system call takes flags that affect just what is
 * done with the disk partition and the associated prefix.
 *	FS_ATTACH_READ_ONLY	Set the disk up to be read only.
 *	FS_DETACH		The disk becomes inaccessible.  Any modified
 *				filesystem data is flushed first.
 *	FS_ATTACH_LOCAL		The disk is attached locally and not exported
 *	FS_DEFAULT_DOMAIN	The domain is being attached by the kernel
 *				during boot as the default.
 *
 */
#define FS_ATTACH_READ_ONLY		0x1
#define FS_DETACH			0x2
#define FS_ATTACH_LOCAL			0x4
#define FS_DEFAULT_DOMAIN		0x8

typedef struct Fs_TwoPaths {
    int		pathLen1;	/* Length of the first path, including null */
    int		pathLen2;	/* Length of the second path, including null */
    char 	*path1;		/* First pathname */
    char 	*path2;		/* Second pathname */
} Fs_TwoPaths;

/*
 * Information about a file system domain (volume).
 */
typedef struct {
    int	maxKbytes;		/* Total Kbytes in the domain.  The allocation
				 * routine might reserve some (%10) of this */
    int	freeKbytes;		/* The number of available blocks.  This
				 * reflects any reservations made by the
				 * allocator.  If this is positive, blocks
				 * are available. */
    int	maxFileDesc;		/* The total number of files that can be
				 * created in the domain. */
    int	freeFileDesc;		/* The number of free file descriptors */
    int blockSize;		/* Bytes per block */
    int optSize;		/* Optimal transfer size, in bytes */
} Fs_DomainInfo;


/*
 * User visible prefix table entry.  This is used by the routine that
 * copies individual entries out to user programs.
 */
#define FS_USER_PREFIX_LENGTH	64
#define FS_NO_SERVER		0
typedef struct Fs_Prefix {
    int serverID;		/* From FsFileID of prefix, FS_NO_SERVER if 
				 * no handle */
    int domain;			/* ditto */
    int fileNumber;		/* ditto */
    int version;		/* ditto */
    int flags;			/* Defined below */
    char prefix[FS_USER_PREFIX_LENGTH];
    Fs_DomainInfo domainInfo;	/* Information about the domain. */
} Fs_Prefix;

#ifndef FS_EXPORTED_PREFIX
#define	FS_EXPORTED_PREFIX		0x1
#define	FS_IMPORTED_PREFIX		0x2
#define	FS_LOCAL_PREFIX			0x4
#endif


/*
 * The Fs_ReadVector and Fs_WriteVector system calls take an array of
 * I/O vectors. This allows data to be read to or written from non-contiguous
 * areas of memory.
 */
typedef struct {
    int		bufSize;	/* Size in bytes of the buffer */
    Address	buffer;		/* For Fs_WriteVector, data to be written.
				 * For Fs_ReadVector, place where read data 
				 * is stored. */
} Fs_IOVector;


/*
 * The structure below is use for creating devices with Fs_MakeDevice.
 * It's also used internally by the kernel to hold the information passed
 * to device specific routines so they can operate on their particular device.
 */
typedef struct Fs_Device {
    int		serverID;	/* The host ID of the server that controls
				 * the device. */
    int		type;		/* The type of device.  This field is used to
				 * index into an operation switch */
    int		unit;		/* Type dependent unit specification. The
				 * interpretation is up to the device driver */
    ClientData	data;		/* Device type dependent data. This can be set
				 * during the device open routine and should
				 * be cleaned up in the device close routine. */
} Fs_Device;

/*
 * Definitions for the FS dispatcher library.
 */
typedef ClientData Fs_TimeoutHandler;

extern void		    Fs_Dispatch();
extern void		    Fs_EventHandlerCreate();
extern void 		    Fs_EventHandlerDestroy();
extern ClientData 	    Fs_EventHandlerData();
extern ClientData 	    Fs_EventHandlerChangeData();
extern char		    *Fs_GetTempName();
extern int		    Fs_GetTempFile();
extern int                  Fs_IOControl();
extern Boolean		    Fs_IsATerm();
extern Fs_TimeoutHandler    Fs_TimeoutHandlerCreate();
extern void 		    Fs_TimeoutHandlerDestroy();

extern int                  Ioc_ClearBits();
extern int                  Ioc_GetFlags();
extern int                  Ioc_GetOwner();
extern int                  Ioc_Lock();
extern int                  Ioc_Map();
extern int                  Ioc_NumReadable();
extern int                  Ioc_SetBits();
extern int                  Ioc_Reposition();
extern int                  Ioc_SetFlags();
extern int                  Ioc_SetOwner();
extern int                  Ioc_Truncate();
extern int                  Ioc_Unlock();
extern int                  Ioc_WriteBack();

#endif /* _FS_H */
