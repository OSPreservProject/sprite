/*
 *	rpcCall.h --
 *
 *      Definitions of remote procedure numbers.  Each different RPC has a
 *      unique number that identifies it to the server machine.
 *
 *	This file must be consistent with lust:~brent/src/sun/sys/h/rfs.h
 *	so that sprite machines can do rpc with lust as server.
 *
 *	Copyright (C) 1985 Regents of the University of California
 *	All rights reserved.
 *
 *	$Header$ SPRITE (Berkeley)
 */

#ifndef	_RPCCALL
#define	_RPCCALL

/*
 *	The remote procedures.  For each procedure the format the request
 *	and reply message is summarized.  See the Rpc_Storage type for
 *	more explaination.  The stubs often rely on the sizes of these area
 *	being returned by the RPC system so that the size of variable length
 *	objects does not have to be in the message explicitly.
 *
 *	RPC_ECHO_1	Special echo that is handled completely by the
 *			server dispatcher.  The server process is not involved.
 *	RPC_ECHO_2	Echo bytes off the server. The request/reply parameter 
 *			area is empty.  The request/reply data area contains
 *			the bytes that are echoed.
 *	RPC_FS_SPRITE_OPEN	Open a file.  Could create the file, always 
 *				follows symbolic links. The request parameter 
 *				area contains a FsOpenParams, and the request 
 *				data area contains a file name.  The return 
 *				parameter area contains an FsOpenReturn 
 *				structure.
 *	RPC_FS_NAME	Open a file.  Won't create the file.  The request
 *			parameter is an FsUserInfo, a file mode (int), and
 *			a follow flag (int) which indicates that symbolic
 *			links should or should not be followed.  The return
 *			parameter area contains an FsOpenReturn struct.
 *	RPC_FS_LOCATE	UNUSED.  A Broadcasted empty request that results
 *			in an empty reply from any listening server.
 *	RPC_FS_READ	Read from a file.  The request parameter area contains
 *			an FsRtoken followed by a UnixUio struct (FIX ME!)
 *			which specifies length and offset of the read.  The
 *			return data area contains the bytes read.
 *	RPC_FS_WRITE	Write to a file.  Same request parameter format as Read.
 *			The request data area contains the bytes to write.  The
 *			return parameter area contains the offset at which the
 *			write started, and the amount actually written.
 *	RPC_FS_CLOSE	Close a file.  The request parameter area has an
 *			FsRtoken.  The reply is empty.
 *	RPC_FS_TRUNC	Truncate a file.  The request parameter area has an
 *			FsRtoken followed by a truncation length (int).  The
 *			reply is empty.
 *	RPC_FS_APPEND	Append mode write to a file.  Same request and reply
 *			format as RPC_FS_WRITE.
 *	RPC_FS_STAT	Get the file header information about a file.  The
 *			request parameter area contains an FsRtoken.  The
 *			reply parameter area contains FsUnixAttributes.
 *	RPC_FS_UNLINK	Remove a file.  The request parameter area contains an
 *			FsUserInfo.  The request data area has the file name.
 *	RPC_FS_RENAME	Change the name of a file.  The request parameters
 *			area contains an FsUserInfo, the length of the original
 *			pathname (int), the length of the new pathname (int),
 *			then the original pathname.  The data area
 *			contains the new pathname.
 *	RPC_FS_MKDIR	Make a directory.  The request parameter area contains
 *			an FsUserInfo and a creation mode (int).  The request
 *			data area contains the pathname.  The reply is empty.
 *	RPC_FS_RMDIR	Remove a directory.  The request parameter area contains
 *			an FsUserInfo.  The request data area contains the
 *			pathname.  The reply is empty.
 *	RPC_FS_CHMOD	Change the mode of a file.  The request parameter area
 *			contains an FsRtoken, and a new mode (int).  The
 *			reply is empty.
 *	RPC_FS_CHOWN	Change ownership of a file.  The request parameter area
 *			contains an FsRotken, a new user ID (int), and a new
 *			group ID (int).  The reply is empty.
 *	RPC_FS_LINK	Link two files together.  The format of the request
 *			is the same as for FS_RENAME.  The reply is empty.
 *	RPC_FS_UNIX_PREFIX	Broadcast a pathname and get an rtoken for its 
 *				prefix.  The request message is the same as 
 *				for FS_NAME.  FIX THIS to include a minimum 
 *				prefix length.  The return is an Rtoken for 
 *				the prefix of the file.
 *	RPC_FS_PULLIN	UNUSED.  Return all the prefixes exported by a server
 *			to a client.  The request message is empty.  The
 *			reply parameter are contains a list of prefixes.  These
 *			are a bunch on null-terminated strings.
 *	RPC_FS_UPDAT	Update time stamps on a file.  The request parameter
 *			area contains an FsRtoken, then the access time (Time),
 *			and the modify time (Time).  The reply is empty.
 *	RPC_GETTIME	Get the time of day.  The request is empty.  The reply
 *			parameter area contains the current time (Time),
 *			the number of minutes west of Greenwich (int), and the
 *			daylights savings flag (int).
 *	RPC_FS_UNIX_OPEN	Open a file.  A super-set of FS_OPEN which 
 *				allows the caller to restrict the type of the 
 *				file that can be opened, as well as the 
 *				permissions that are needed.  The request 
 *				parameter area contains an augmented 
 *				FsOpenParams which has a type field inserted 
 *				between the mode and mask fields.  The request 
 *				data area contains the file name.  The reply 
 *				parameter area 	contains an FsOpenReturn.
 *	RPC_SEND	Send bytes to the server.  The request parameter
 *			area is empty.  The request data area has the bytes to
 *			send.  The reply is empty.
 *	RPC_FS_SPRITE_PREFIX	Broadcast a pathname to a Sprite server and 
 *				get a file handle for its prefix.
 *	RPC_FS_GET_ATTR	Get the file header information about a file.  The
 *			request parameter area contains an FsFileID.  The
 *			reply parameter area contains Fs_Attributes.
 *	RPC_FS_SET_ATTR	Set the file header information about a file.  The
 *			request parameter area contains an FsFileID and a
 *			Fs_Attributes struct.  There is a NULL reply.
 *	RPC_PROC_MIG_INIT Initiate process migration by confirming that
 *			foreign processes will be accepted.
 *	RPC_PROC_MIG_INFO Transfer information about a process that is being
 *			migrated.
 *	RPC_PROC_REMOTE_CALL Perform a system call for a migrated process.
 *	RPC_FS_START_MIGATION 	Start the migration of a file handle.
 *	RPC_FS_REMOTE_WAKEUP	Wakeup a process on a remote machine that
 *				is waiting for a server.
 *	RPC_FS_CONSIST	Call from the server to the client to initiate
 *				cache consistency operations.
 *	RPC_FS_CONSIST_REPLY	Reply from the client indicating that
 *				it has completed the cache consistency action.
 *	RPC_FS_DEV_OPEN	Open a connection to the I/O server for a device.
 *	RPC_SIG_MIG_SEND	Send a signal to a migrated process.
 *	RPC_PROC_REMOTE_WAIT	Perform a Proc_Wait for a migrated process.
 *	RPC_FS_SELECT	See if a file is readable or writable.
 *	RPC_FS_FINISH_MIGATION 	Finish the migration of a file handle.
 *	RPC_FS_IO_CONTROL	Perform an iocontrol.
 *	RPC_FS_MKDEV		Make a special device file
 *	RPC_FS_REOPEN		Reopen a file.
 *	RPC_FS_DOMAIN_INFO	Return information about the given domain.
 *
 * These procedure numbers and the service switch should be generated
 * from another file...
 */
#define	RPC_BAD_COMMAND		0
#define	RPC_ECHO_1		1
#define	RPC_ECHO_2		2
#define	RPC_FS_SPRITE_OPEN	3
#define	RPC_FS_NAME		4
#define	RPC_FS_LOCATE		5
#define	RPC_FS_READ		6
#define	RPC_FS_WRITE		7
#define	RPC_FS_CLOSE		8
#define	RPC_FS_TRUNC		9
#define	RPC_FS_APPEND		10
#define	RPC_FS_STAT		11
#define	RPC_FS_UNLINK		12
#define	RPC_FS_RENAME		13
#define	RPC_FS_MKDIR		14
#define	RPC_FS_RMDIR		15
#define	RPC_FS_CHMOD		16
#define	RPC_FS_CHOWN		17
#define	RPC_FS_LINK		18
#define	RPC_FS_UNIX_PREFIX	19
#define	RPC_FS_PULLIN		20
#define	RPC_FS_UPDAT		21
#define	RPC_GETTIME		22
#define	RPC_FS_UNIX_OPEN	23
#define	RPC_SEND		24
#define	RPC_FS_SPRITE_PREFIX	25
#define	RPC_FS_GET_ATTR		26
#define	RPC_FS_SET_ATTR		27
#define	RPC_PROC_MIG_INIT 	28
#define	RPC_PROC_MIG_INFO 	29
#define	RPC_PROC_REMOTE_CALL 	30
#define	RPC_FS_START_MIGRATION 	31
#define	RPC_FS_REMOTE_WAKEUP 	32
#define	RPC_FS_CONSIST 		33
#define	RPC_FS_DEV_OPEN 	34
#define	RPC_SIG_MIG_SEND 	35
#define	RPC_REMOTE_WAKEUP 	36
#define	RPC_FS_LOCK	 	37
#define	RPC_PROC_REMOTE_WAIT	38
#define	RPC_FS_SELECT		39
#define	RPC_FS_FINISH_MIGRATION 40
#define	RPC_FS_IO_CONTROL	41
#define	RPC_FS_CONSIST_REPLY	42
#define	RPC_FS_COPY_BLOCK	43
#define RPC_FS_MKDEV		44
#define RPC_FS_GET_ATTR_PATH	45
#define	RPC_SIG_SEND		46
#define	RPC_FS_REOPEN		47
#define	RPC_FS_DOMAIN_INFO	48
#define	RPC_LAST_COMMAND RPC_FS_DOMAIN_INFO

/*
 * The above constant RPC_LAST_COMMAND is used to declare arrays that
 * keep information for each procedure.
 */

/*
 * Procedures in the rpc service switch.
 */
extern ReturnStatus RpcNull();
extern ReturnStatus RpcEcho();
extern ReturnStatus Fs_RpcRead();		/* 6 - READ */
extern ReturnStatus Fs_RpcWrite();		/* 7 - WRITE */
extern ReturnStatus Fs_RpcClose();		/* 8 - CLOSE */
extern ReturnStatus Fs_RpcTruncStub();		/* 9 - TRUNC */
extern ReturnStatus Fs_RpcRemove();		/* 12 - UNLINK, 15 - RMDIR */
extern ReturnStatus Fs_Rpc2Path();		/* 13 - RENAME, 18 - LINK */
extern ReturnStatus Fs_RpcMakeDir();		/* 14 - MKDIR */
extern ReturnStatus RpcFsUnixPrefix();		/* 19 - FS_UNIX_PREFIX */
extern ReturnStatus RpcGetTime();		/* 22 - GETTIME */
extern ReturnStatus Fs_RpcOpen();		/* 23 - FS_SPRITE_OPEN */
extern ReturnStatus Fs_RpcPrefix();		/* 25 - FS_SPRITE_PREFIX */
extern ReturnStatus Fs_RpcGetAttr();		/* 26 - FS_GET_ATTR */
extern ReturnStatus Fs_RpcSetAttr();		/* 27 - FS_SET_ATTR */
extern ReturnStatus RpcProcMigInit();		/* 28 - PROC_MIG_INIT */
extern ReturnStatus RpcProcMigInfo();		/* 29 - PROC_MIG_INFO */
extern ReturnStatus RpcProcRemoteCall();	/* 30 - PROC_REMOTE_CALL */
extern ReturnStatus Fs_RpcStartMigration();	/* 31 - FS_START_MIGRATION */
extern ReturnStatus Fs_RpcConsist();		/* 33 - FS_CONSIST */
extern ReturnStatus Fs_RpcDevOpen();		/* 34 - FS_DEV_OPEN */
extern ReturnStatus RpcSigMigSend();		/* 35 - SIG_MIG_SEND */
extern ReturnStatus Sync_RemoteNotifyStub();	/* 36 - REMOTE_NOTIFY */
extern ReturnStatus Fs_RpcLockStub();		/* 37 - FS_LOCK */
extern ReturnStatus Proc_RpcRemoteWait();	/* 38 - PROC_REMOTE_WAIT */
extern ReturnStatus Fs_RpcSelectStub();		/* 39 - FS_SELECT */
extern ReturnStatus Fs_RpcFinishMigration();	/* 40 - FS_START_MIGRATION */
extern ReturnStatus Fs_RpcIOControl();		/* 41 - FS_RPC_IO_CONTROL */
extern ReturnStatus Fs_RpcConsistReply();	/* 42 - FS_CONSIST_REPLY */
extern ReturnStatus Fs_RpcBlockCopy();		/* 43 - FS_COPY_BLOCK */
extern ReturnStatus Fs_RpcMakeDev();		/* 44 - FS_MKDEV */
extern ReturnStatus Fs_RpcGetAttrPath();	/* 45 - FS_GET_ATTR_PATH */
extern ReturnStatus Sig_RpcSend();		/* 46 - SIG_SEND */
extern ReturnStatus Fs_RpcReopen();		/* 47 - FS_RPC_REOPEN */
extern ReturnStatus Fs_RpcDomainInfo();		/* 48 - FS_RPC_DOMAIN_INFO. */

#endif	_RPCCALL

