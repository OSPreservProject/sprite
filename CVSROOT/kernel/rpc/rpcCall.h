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
 *	RPC_SEND	Send bytes to the server.  The request parameter
 *			area is empty.  The request data area has the bytes to
 *			send.  The reply is empty.
 *	RPC_RECEIVE	Unimplemented.
 *	RPC_FS_OPEN	Open a file.  Could create the file, always
 *				follows symbolic links. The request parameter
 *				area contains a FsOpenParams, and the request
 *				data area contains a file name.  The return
 *				parameter area contains an FsOpenReturn
 *				structure.
 *	RPC_FS_READ	Read from a file.  The request parameter area contains
 *			an FsSpriteReadParams structure, and the request data
 *			area is empty.  The return data area contains the
 *			bytes read.
 *	RPC_FS_WRITE	Write to a file.  Same request parameter format as Read.
 *			The request data area contains the bytes to write.  The
 *			return parameter area contains the offset at which the
 *			write started, and the amount actually written.
 *	RPC_FS_CLOSE	Close a file.  The request parameter area has an
 *			FsRtoken.  The reply is empty.
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
 *	RPC_FS_LINK	Link two files together.  The format of the request
 *			is the same as for FS_RENAME.  The reply is empty.
 *	RPC_GETTIME	Get the time of day.  The request is empty.  The reply
 *			parameter area contains the current time (Time),
 *			the number of minutes west of Greenwich (int), and the
 *			daylights savings flag (int).
 *	RPC_FS_PREFIX	Broadcast a pathname to a server and
 *				get a file handle for its prefix.
 *	RPC_FS_GET_ATTR	Get the file header information about a file.  The
 *			request parameter area contains an FsFileID.  The
 *			reply parameter area contains Fs_Attributes.
 *	RPC_FS_SET_ATTR	Set the file header information about a file.  The
 *			request parameter area contains an FsFileID and a
 *			Fs_Attributes struct.  There is a NULL reply.
 *	RPC_FS_GET_ATTR_PATH	Get attributes given a path name.
 *	RPC_FS_SET_ATTR_PATH	Set attributes given a pathname.
 *	RPC_FS_GET_IO_ATTR	Get attributes cached at the I/O server.
 *	RPC_FS_SET_IO_ATTR	Set attributes cached at the I/O server.
 *	RPC_PROC_MIG_COMMAND Transfer information about a process that is being
 *			migrated.
 *	RPC_PROC_REMOTE_CALL Perform a system call for a migrated process.
 *	RPC_FS_MIGRATE 		Start the migration of a file handle.
 *	RPC_FS_CONSIST	Call from the server to the client to initiate
 *				cache consistency operations.
 *	RPC_FS_CONSIST_REPLY	Reply from the client indicating that
 *				it has completed the cache consistency action.
 *	RPC_FS_DEV_OPEN	Open a connection to the I/O server for a device.
 *	RPC_REMOTE_WAKEUP 	Notify a remote process.
 *	RPC_PROC_REMOTE_WAIT	Perform a Proc_Wait for a migrated process.
 *	RPC_FS_SELECT	See if a file is readable or writable.
 *	RPC_FS_IO_CONTROL	Perform an iocontrol.
 *	RPC_FS_MKDEV		Make a special device file
 *	RPC_FS_REOPEN		Reopen a file.
 *	RPC_FS_DOMAIN_INFO	Return information about the given domain.
 *	RPC_FS_DEV_REOPEN	Reopen a device at the I/O server.
 *	RPC_FS_RECOVERY		Recovery control message used by clients to
 *				tell server's they want to re-establish
 *				its open file handles, and when they are done.
 *	RPC_PROC_GETPCB		Retrieve a process control block from another
 *				host.
 *
 * These procedure numbers and the service switch should be generated
 * from another file...
 */

#define	RPC_BAD_COMMAND		0
#define	RPC_ECHO_1		1
#define	RPC_ECHO_2		2
#define	RPC_SEND		3
#define	RPC_RECEIVE		4
#define	RPC_FS_OPEN		5
#define	RPC_FS_READ		6
#define	RPC_FS_WRITE		7
#define	RPC_FS_CLOSE		8
#define	RPC_FS_UNLINK		9
#define	RPC_FS_RENAME		10
#define	RPC_FS_MKDIR		11
#define	RPC_FS_RMDIR		12
#define	RPC_FS_LINK		13
#define	RPC_GETTIME		14
#define	RPC_FS_PREFIX		15
#define	RPC_FS_GET_ATTR		16
#define	RPC_FS_SET_ATTR		17
#define RPC_FS_GET_ATTR_PATH	18
#define RPC_FS_SET_ATTR_PATH	19
#define RPC_FS_GET_IO_ATTR	20
#define RPC_FS_SET_IO_ATTR	21
#ifdef notdef
#define	RPC_PROC_MIG_INIT 	22
#endif
#define	RPC_PROC_MIG_COMMAND 	23
#define	RPC_PROC_REMOTE_CALL 	24
#define	RPC_FS_MIGRATE 		25
#define	RPC_FS_CONSIST 		26
#define	RPC_FS_DEV_OPEN 	27
#define	RPC_REMOTE_WAKEUP 	28
#define	RPC_PROC_REMOTE_WAIT	29
#define	RPC_FS_SELECT		30
#define	RPC_FS_IO_CONTROL	31
#define	RPC_FS_CONSIST_REPLY	32
#define	RPC_FS_COPY_BLOCK	33
#define RPC_FS_MKDEV		34
#define	RPC_SIG_SEND		35
#define	RPC_FS_REOPEN		36
#define	RPC_FS_DOMAIN_INFO	37
#define RPC_FS_DEV_REOPEN	38
#define RPC_FS_RECOVERY		39
#define	RPC_PROC_GETPCB		40
#define	RPC_LAST_COMMAND	RPC_PROC_GETPCB

/*
 * RPC_LAST_COMMAND is used to declare the rpc procedure switch
 * and arrays of counters for each rpc.
 */

/*
 * Procedures in the rpc service switch.
 */
extern ReturnStatus RpcNull();
extern ReturnStatus RpcEcho();
extern ReturnStatus Fs_RpcOpen();		/*  FS_OPEN */
extern ReturnStatus Fs_RpcRead();		/*  READ */
extern ReturnStatus Fs_RpcWrite();		/*  WRITE */
extern ReturnStatus Fs_RpcClose();		/*  CLOSE */
extern ReturnStatus Fs_RpcRemove();		/*  UNLINK, RMDIR */
extern ReturnStatus Fs_Rpc2Path();		/*  RENAME, LINK */
extern ReturnStatus Fs_RpcMakeDir();		/*  MKDIR */
extern ReturnStatus RpcGetTime();		/*  GETTIME */
extern ReturnStatus Fs_RpcPrefix();		/*  FS_PREFIX */
extern ReturnStatus Fs_RpcGetAttr();		/*  FS_GET_ATTR */
extern ReturnStatus Fs_RpcSetAttr();		/*  FS_SET_ATTR */
extern ReturnStatus Fs_RpcGetAttrPath();	/*  FS_GET_ATTR_PATH */
extern ReturnStatus Fs_RpcSetAttrPath();	/*  FS_SET_ATTR_PATH */
extern ReturnStatus Fs_RpcGetIOAttr();		/*  FS_GET_IO_ATTR */
extern ReturnStatus Fs_RpcSetIOAttr();		/*  FS_SET_IO_ATTR */
#ifdef notdef
extern ReturnStatus Proc_RpcMigInit();		/*  PROC_MIG_INIT */
#endif /* notdef */
extern ReturnStatus Proc_RpcMigCommand();	/*  PROC_MIG_COMMAND */
extern ReturnStatus Proc_RpcRemoteCall();	/*  PROC_REMOTE_CALL */
extern ReturnStatus Fs_RpcStartMigration();	/*  FS_MIGRATE */
extern ReturnStatus Fs_RpcConsist();		/*  FS_CONSIST */
extern ReturnStatus Fs_RpcDevOpen();		/*  FS_DEV_OPEN */
extern ReturnStatus Sync_RemoteNotifyStub();	/*  REMOTE_WAKEUP */
extern ReturnStatus Proc_RpcRemoteWait();	/*  PROC_REMOTE_WAIT */
extern ReturnStatus Fs_RpcSelectStub();		/*  FS_SELECT */
extern ReturnStatus Fs_RpcIOControl();		/*  FS_RPC_IO_CONTROL */
extern ReturnStatus Fs_RpcConsistReply();	/*  FS_CONSIST_REPLY */
extern ReturnStatus Fs_RpcBlockCopy();		/*  FS_COPY_BLOCK */
extern ReturnStatus Fs_RpcMakeDev();		/*  FS_MKDEV */
extern ReturnStatus Sig_RpcSend();		/*  SIG_SEND */
extern ReturnStatus Fs_RpcReopen();		/*  FS_REOPEN */
extern ReturnStatus Fs_RpcDomainInfo();		/*  FS_DOMAIN_INFO */
extern ReturnStatus Fs_RpcDevReopen();		/*  FS_DEV_REOPEN */
extern ReturnStatus Fs_RpcRecovery();		/*  FS_RECOVERY */
extern ReturnStatus Proc_RpcGetPCB();		/*  PROC_GETPCB  */

#endif /*	_RPCCALL */

