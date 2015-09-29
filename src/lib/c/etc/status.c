/*
 * status.c --
 *
 * 	Returns the message associated with a status value in 
 *	the status.h file.
 *
 * Copyright 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/status.c,v 1.10 91/10/23 16:46:56 mgbaker Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <stdio.h>

typedef struct {
    char *name;
    char **array;
    int   size;
} StatusMsgs;

/*---------------------------------------------*/

/* Many of the gen messages are from string/strerror.c */
/*	gen.stat	*/
static char *genStatusMsgs[] = {
"the operation was successful",
"there was an error",
"the system call was aborted by a signal",
"the user did not have the required permission",
"this operation or feature isn't implemented",
"an argument to a call was invalid",
"the operation did not complete within the timeout period.",
"not owner",				/* EPERM */
"no such file or directory",		/* ENOENT */
"interrupted system call",		/* EINTR */
"argument list too long",		/* E2BIG */
"no more processes",			/* EAGAIN */
"permission denied",			/* EACCESS */
"bad address in system call argument",	/* EFAULT */
"file already exists",			/* EEXIST */
"invalid argument",			/* EINVAL */
"file too large",			/* EFBIG */
"no space left in file system domain",	/* ENOSPC */
"math result unrepresentable",		/* ERANGE */
"identifier removed",			/* EIDRM */
};

/*	proc.stat	*/
static char *procStatusMsgs[] = {
"the file name to exec was not null terminated",
"the a.out format of the file to be exec'd was incorrect",
"the virtual memory system ran out of segments",
"you are the child of a fork",
"proc_Wait did not find any exited or detached processes of interest",
"the process ID is not in the proper range or the process doesn't exist",
"the specified process's user ID does not match the current process's uid",
"the process doesn't have any children to wake it up from Proc_Wait",
"the family ID is not in the proper range or the family doesn't exist",
"the variable trying to be retrieved from the environment has not been set",
"the environment variable name or value was not null terminated",
"the environment has reached its maximum size and all entries are in use",
"the node identifier for process migration is not a valid node specifier",
"the node is refusing to accept processes for migration",
"a string passed to a system call was not null terminated",
"no kernel stacks are available (too many processes)",
"the peer process of a migrated process does not exist",
};

/*	sys.stat	*/
static char *sysStatusMsgs[] = {
"address given by the user for a system call was bad",
"the argument to a system call was invalid",
"the constant specified for a system call was invalid",
};

/*	rpc.stat	*/
static char *rpcStatusMsgs[] = {
"a consistency check revealed an invalid parameter passed to an RPC routine",
"no channels were available to conduct a Remote Procedure Call",
"no Reply to an RPC request within a threshold time limit",
"received a large number of acknowledgments but no answer to an RPC request",
"a consistency check revealed an internal error in the RPC system",
"the server received a request with an invalid procedure id",
"the client received a null error code from the server",
"the parameter area for the RPC was more that 1K",
"the data area for the RPC was more that 16K",
"there is no reply for this RPC",
"the host is up (booting), but its RPC system is off",
"a client with a `ramp down' policy received a negative ack from a server",
"the requested prefix does not exist",
};

/*	fs.stat	*/
static char *fsStatusMsgs[] = {
"the permissions on the file prohibit the requested access",
"bad argument to a filesystem routine",
"the operation on the remote file is not supported",
"the operation on the local file is not supported",
"the operation on the device file is not supported",
"the new stream ID requested is too large",
"the master process of a pseudo-device has closed its connection (INTERNAL)",
"the reader of the pipe died",
"no disk space available for write",
"lookup re-direct from a file server (INTERNAL)",
"there is no file handle for a prefix (INTERNAL)",
"new information has been added to the prefix table (INTERNAL)",
"the file does not exist",
"the read or write call could not complete immediately",
"the buffer is too large to be written",
"an invalid operation was attempted on a directory",
"a non-directory file was specified where a directory was needed",
"the caller did not own the file",
"the file handle is out of date",
"the file already exists",
"an attempt was made to delete a non-empty directory",
"too many links were expanded during name lookup",
"a Rename or HardLink was attempted between domains",
"the operation did not complete within the specified time",
"no shared lock is held on the file",
"no exclusive lock is held on the file",
"the file's type doesn't match what is required by Fs_Open",
"the file has been removed by the server (INTERNAL)",
"the file can't be used or is busy already",
"the stream does not support reposition of the read write access position",
"the domain is not attached",
"version mis-match on reopen (INTERNAL)",
"the file isn't cacheable on a remote client (INTERNAL)",
};

/*	vm.stat	*/
static char *vmStatusMsgs[] = {
"the address given to allocate virtual memory was not in a heap or code segment",
"there is insufficient virtual memory in the segment to expand it",
"a read from the file server on a page fault returned less than a page",
"a write to the file server wrote less than a page",
"the swap file for the process could not either be opened, read, or written",
"the virtual memory system ran out of segments",
};

/*	sig.stat	*/
static char *sigStatusMsgs[] = {
"the signal number is not in the range 0 to 31",
"the action is not one of the valid actions",
};

/*	dev.stat	*/
static char *devStatusMsgs[] = {
"a bus error occurred during DMA because something wasn't mapped",
"the device unit number doesn't correspond to an existing device",
"the device did not respond after a timeout period",
"the device is off-line",
"the handshaking protocol with the device failed",
"too many retriable errors occurred",
"no such device exists",
"an invalid argument was passed to a device driver",
"a device encountered a hard, uncorrectable, error",
"the end of tape was encountered during a command",
"no tape reel or cartridge is loaded in the drive",
"a command to a device completed prematurely",
"there is no current sense information",
"an attempt was made to rewrite write-once data or to read unwritten data",
"the device is busy",
"the device has been reset",
};

/*	net.stat	*/
static char *netStatusMsgs[] = {
"the remote network cannot be reached from this host.",
"the remote host is cannot be reached from this host.",
"the remote host refused to accept the connection.",
"the connection was reset by the remote host.",
"no connection requests are pending.",
"the socket is already connected to a remote peer.",
"the socket is not connected to a remote peer.",
"the socket address is already in use.",
"the socket address is not available on this host.",
"an unknown protocol was specified.",
"an invalid operation was attempted on the socket.",
"an invalid option was specified.",
};

static StatusMsgs statusMsgs[] = {
	{"Gen", 	 genStatusMsgs       , 	 20},
	{"Proc", 	 procStatusMsgs      , 	 17},
	{"Sys", 	 sysStatusMsgs       , 	 3},
	{"Rpc", 	 rpcStatusMsgs       , 	 11},
	{"Fs", 	 fsStatusMsgs        , 	 33},
	{"Vm", 	 vmStatusMsgs        , 	 6},
	{"Sig", 	 sigStatusMsgs       , 	 2},
	{"Dev", 	 devStatusMsgs       , 	 16},
	{"Net", 	 netStatusMsgs       , 	 12},
};
static int maxNumModules = 9;

/*---------------------------------------------*/


/*
 *-----------------------------------------------------------------------
 *
 * Stat_GetMsg --
 *
 *	Given a ReturnStatus, return a string indicating the meaning
 *	of the status.
 *
 * Results:
 *	A pointer to a statically-allocated string. If the status is
 *	invalid, a customized error message is returned.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */

char *
Stat_GetMsg(status)
    ReturnStatus  status;   	/* Status for which message is desired */
{
    int	    	  module = STAT_MODULE(status);
    int	    	  msg = STAT_MSGNUM(status);
    static  char  errMsg[100];	    /* static space for erroneous stati */

    if (module >= maxNumModules) {
	(void) sprintf(errMsg, "Warning: invalid module # in status %x",
		       status);
	return(errMsg);
    } else if (msg >= statusMsgs[module].size) {
	(void) sprintf(errMsg,
		      "Warning: invalid message # for %s module in status %x", 
		       statusMsgs[module].name, status);
	return(errMsg);
    } else {
	return(statusMsgs[module].array[msg]);
    }
}
