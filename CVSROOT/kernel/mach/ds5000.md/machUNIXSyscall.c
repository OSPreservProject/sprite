/* 
 * machUNIXSyscall.c --
 *
 *      UNIX system call handlers.  I'm sorry about the lack of header
 *	comments but I didn't feel like putting 100 headers that only
 *	said "MachUNIXFooBar -- emulates the FooBar system call".  Of course
 *	this should be done to match the Sprite coding conventions.
 *
 *	The routines in this file and the other associated files in this
 *	directory (socket.c, ioctl.c, etc.) provide full binary compatible
 *	with the following exceptions:
 *
 *		1) System call handlers are called with a Sprite context
 *		   rather than a UNIX context.  This could be fixed by
 *		   setting a compatibility bit in the machine state struct
 *		   when the first UNIX system call happens and then emulating
 *		   UNIX signal handler calling conventions after that for
 *		   the process.
 *
 *		2) Sprite doesn't implement ptys so we can't run commands
 *		   like xterm directly.
 *
 *		3) Reads that are interrupted do not restart like they
 *		   do in Sprite.  The problem is that if we restart them
 *		   in here then the user never has the oppurtunity to 
 *		   handle the signal.  A possible solution is to return to
 *		   user mode and then when the signal handler returns restart
 *		   the system call.  This will be complicated because the
 *		   arguments have to be kept around somehow.
 *
 *	IMPLEMENTATION DETAILS
 *
 *	The only real interesting thing about this implementation is that
 *	it's a little tricky calling kernel system call stubs from inside
 *	the kernel.  The problem is that the stubs assume that all pointers
 *	point into user space.  However, when we are emulating a system
 *	call we may call a system call stub (e.g. Fs_OpenStub) and want to
 *	pass a pointer to a kernel structure.  An example of this is 
 *	MachUNIXOpen (emulation of open) which calls Fs_OpenStub.  Fs_OpenStub
 *	returns the streamid in an out parameter in the user's address space
 *	but open doesn't provide any place to put it. The solution that I used
 *	was to push things onto the users stack and then copy the result
 *	back into the kernel's address space.  What, you don't understand
 *	what the hell I'm talking about?  Well look at MachUNIXOpen for
 *	an example.
 *	
 *	If something in here looks bizarre and I forgot to comment it it's
 *	probably because the UNIX system call stub does something weird.
 *	Extract it from the UNIX C library and disassemble it and things
 *	will probably make more sense.
 *	
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "status.h"
#include "machConst.h"
#include "machInt.h"
#include "mach.h"
#include "proc.h"
#include "vm.h"
#include "user/sys/types.h"
#include "user/sys/file.h"
#include "user/fs.h"
#include "user/sys/wait.h"
#include "user/sys/time.h"
#include "user/sys/resource.h"
#include "user/sys/uio.h"
#include "stat.h"
#include "ultrixSignal.h"
#include "sys.h"
#include "fs.h"
#include "fsdm.h"
#include "fslcl.h"	/* Directory format */
#include "sys/types.h"

/*
 * System call entry structure.  Note that this is different than the one
 * in the sys directory.  This should be converted over to that format
 * with the addition of the name field.
 */
typedef struct {
    char		*name;
    int			numArgs;
    ReturnStatus	(*func)();
} SyscallInfo;

extern int machNewUnixCompat;

/*
 * Values for system call tracing:
 *
 *	0: 	Use the fast assembly language interface and don't trace 
 *		anything.
 *	1: 	Use the slower C interface but no trace printing.
 *	> 1: 	Use the slower C interface and trace system calls.
 */

/*
 * The code to disable migration for processes using the compatibility
 * library is in the slower C interface...
 */
#ifdef CANT_MIGRATE_COMPAT
int machUNIXSyscallTrace = 0;
#else /* CANT_MIGRATE_COMPAT */
int machUNIXSyscallTrace = 0;
#endif /* CANT_MIGRATE_COMPAT */

/*
 * Zillions of forward declarations.
 */
extern int MachUNIXIoctl _ARGS_((int fd, int request, char *buf));
extern int MachUNIXFcntl _ARGS_((int fd, int cmd, int arg));
extern Boolean MachUNIXSyscall _ARGS_((void));
extern ReturnStatus MachUNIXExit _ARGS_((int status));
extern ReturnStatus MachUNIXFork _ARGS_((void));
extern ReturnStatus MachUNIXVFork _ARGS_((void));
extern ReturnStatus MachUNIXRead _ARGS_((int streamID, char *buffer, int numBytes));
extern ReturnStatus MachUNIXWrite _ARGS_((int streamID, char *buffer, int numBytes));
extern ReturnStatus MachUNIXOpen _ARGS_((char *pathName, int unixFlags, int permissions));
extern ReturnStatus MachUNIXClose _ARGS_((int streamID));
extern ReturnStatus MachUNIXCreat _ARGS_((char *pathName, int permissions));
extern ReturnStatus MachUNIXLink _ARGS_((char *name1, char *name2));
extern ReturnStatus MachUNIXUnlink _ARGS_((char *pathName));
extern ReturnStatus MachUNIXExecve _ARGS_((char *name, char *argv[], char *envp[]));
extern ReturnStatus MachUNIXExecv _ARGS_((char *name, char **argv));
extern ReturnStatus MachUNIXChdir _ARGS_((char *pathName));
extern ReturnStatus MachUNIXChmod _ARGS_((char *path, int mode));
extern ReturnStatus MachUNIXFChmod _ARGS_((int fd, int mode));
extern ReturnStatus MachUNIXChown _ARGS_((char *path, int owner, int group));
extern ReturnStatus MachUNIXFChown _ARGS_((int fd, int owner, int group));
extern ReturnStatus MachUNIXSbrk _ARGS_((Address addr));
extern ReturnStatus MachUNIXLseek _ARGS_((int streamID, long offset, int whence));
extern ReturnStatus MachUNIXGetPID _ARGS_((void));
extern ReturnStatus MachUNIXGetuid _ARGS_((void));
extern ReturnStatus MachUNIXAccess _ARGS_((char *pathName, int mode));
extern ReturnStatus MachUNIXKill _ARGS_((int pid, int sig));
extern ReturnStatus MachUNIXStat _ARGS_((char *pathName, struct stat *attsBufPtr));
extern ReturnStatus MachUNIXLstat _ARGS_((char *pathName, struct stat *attsBufPtr));
extern ReturnStatus MachUNIXFstat _ARGS_((int fd, struct stat *attsBufPtr));
extern ReturnStatus MachUNIXDup _ARGS_((int oldStreamID));
extern ReturnStatus MachUNIXDup2 _ARGS_((int oldStreamID, int newStreamID));
extern ReturnStatus MachUNIXPipe _ARGS_((int filedes[2]));
extern ReturnStatus MachUNIXGetGID _ARGS_((void));
extern ReturnStatus MachUNIXSymlink _ARGS_((char *target, char *link));
extern ReturnStatus MachUNIXReadLink _ARGS_((char *link, char *buffer, int numBytes));
extern ReturnStatus MachUNIXUmask _ARGS_((int newMask));
extern ReturnStatus MachUNIXGetPageSize _ARGS_((void));
extern ReturnStatus MachUNIXGetGroups _ARGS_((int gidsetlen, int *gidset));
extern ReturnStatus MachUNIXSetGroups _ARGS_((int ngroups, int *gidset));
extern ReturnStatus MachUNIXGetPGrp _ARGS_((int pid));
extern ReturnStatus MachUNIXSetPGrp _ARGS_((int pid, int pgrp));
extern ReturnStatus MachUNIXWait _ARGS_((union wait *statusPtr, int options, struct rusage *unixRusagePtr));
extern ReturnStatus MachUNIXGetDTableSize _ARGS_((void));
extern ReturnStatus MachUNIXSelect _ARGS_((int width, int *readfds, int *writefds, int *exceptfds, Time *timeout));
extern ReturnStatus MachUNIXFSync _ARGS_((int fd));
extern ReturnStatus MachUNIXSetPriority _ARGS_((int which, int who, int prio));
extern ReturnStatus MachUNIXGetPriority _ARGS_((int which, int who, int prio));
extern ReturnStatus MachUNIXGetTimeOfDay _ARGS_((struct timeval *tp, struct timezone *tzpPtr));
extern ReturnStatus MachUNIXSetTimeOfDay _ARGS_((struct timeval *tp, struct timezone *tzpPtr));
extern ReturnStatus MachUNIXGetDirEntries _ARGS_((int fd, char *buf, int nbytes, long *basep));
extern ReturnStatus MachUNIXGetRUsage _ARGS_((int who, struct rusage *rusage));
extern ReturnStatus MachUNIXReadv _ARGS_((int stream, register struct iovec *iov, int iovcnt));
extern ReturnStatus MachUNIXWritev _ARGS_((int stream, register struct iovec *iov, int iovcnt));
extern ReturnStatus MachUNIXSetREUID _ARGS_((int ruid, int euid));
extern ReturnStatus MachUNIXSetREGID _ARGS_((int rgid, int egid));
extern ReturnStatus MachUNIXRename _ARGS_((char *from, char *to));
extern ReturnStatus MachUNIXTruncate _ARGS_((char *path, unsigned long length));
extern ReturnStatus MachUNIXFTruncate _ARGS_((int fd, unsigned long length));
extern ReturnStatus MachUNIXLongJumpReturn _ARGS_((struct sigcontext *sigContextPtr));
extern ReturnStatus MachUNIXFLock _ARGS_((int descriptor, int operation));
extern ReturnStatus MachUNIXMkDir _ARGS_((char *pathName, int permissions));
extern ReturnStatus MachUNIXRMDir _ARGS_((char *pathName));
extern ReturnStatus MachUNIXUTimes _ARGS_((char *path, struct timeval tvp[2]));
extern ReturnStatus MachUNIXKillPG _ARGS_((int pgrp, int sig));
extern ReturnStatus MachUNIXGetRLimit _ARGS_((void));
extern ReturnStatus MachUNIXSetRLimit _ARGS_((void));
extern ReturnStatus MachUNIXGetHostName _ARGS_((char *name, int namelen));
extern ReturnStatus MachUNIXSetHostName _ARGS_((char *name, int namelen));
extern ReturnStatus MachUNIXGetHostID _ARGS_((void));
extern ReturnStatus MachUNIXSetHostID _ARGS_((int hostid));
extern ReturnStatus MachUNIXGetDomainName _ARGS_((char *name, int namelen));
extern ReturnStatus MachUNIXSetDomainName _ARGS_((char *name, int namelen));
extern ReturnStatus MachUNIXGetItimer _ARGS_((int which, struct itimerval *value));
extern ReturnStatus MachUNIXSetITimer _ARGS_((int which, struct itimerval *value, struct itimerval *ovalue));
extern ReturnStatus MachUNIXGetSysInfo _ARGS_((unsigned op, char *buffer, unsigned nbytes, int *start, char *arg));
extern ReturnStatus MachUNIXSemctl _ARGS_((int semid, int semnum, int cmd, unionsemun arg));
extern ReturnStatus MachUNIXSemop _ARGS_((int semid, struct sembuf *sops, int nsops));
extern ReturnStatus MachUNIXSemget _ARGS_((key_t key, int nsems, int semflg));
extern ReturnStatus MachUNIXMmap _ARGS_((caddr_t addr, int len, int prot, int share, int fd, off_t pos));
extern ReturnStatus MachUNIXMunmap _ARGS_((caddr_t addr, int len));
extern ReturnStatus MachUNIXMincore _ARGS_((caddr_t addr, int len, char vec[]));
extern ReturnStatus MachUNIXSocketPair _ARGS_((int d, int type, int protocol, int sv[2]));
extern ReturnStatus MachUNIXReboot _ARGS_((int howto));
extern ReturnStatus MachUNIXSync _ARGS_((void));
extern ReturnStatus MachUNIXPtrace _ARGS_((int request, int pid, int *addr, int data));
extern ReturnStatus MachUNIXGetDOpt _ARGS_((void));
extern ReturnStatus MachUNIXSetDOpt _ARGS_((void));
extern ReturnStatus MachUNIXAdjTime _ARGS_((struct timeval *delta, struct timeval *olddelta));
extern ReturnStatus MachUNIXError _ARGS_((void));
extern ReturnStatus MachUNIXProfil _ARGS_((char *buffer, int bufsize, int offset, int scale));
extern ReturnStatus MachUNIXSigvec _ARGS_((int sig, struct sigvec *newVectorPtr, struct sigvec *oldVectorPtr));
extern ReturnStatus MachUNIXBlock _ARGS_((int mask));
extern ReturnStatus MachUNIXSigSetmask _ARGS_((int mask));
extern ReturnStatus MachUNIXSigPause _ARGS_((int mask));
extern ReturnStatus MachUNIXSigStack _ARGS_((struct sigstack *ss, struct sigstack *oss));
extern ReturnStatus MachUNIXAccept _ARGS_((int socketID, struct sockaddr_in *addrPtr, int *addrLenPtr));
extern ReturnStatus MachUNIXBind _ARGS_((int socketID, struct sockaddr *namePtr, int nameLen));
extern ReturnStatus MachUNIXConnect _ARGS_((int socketID, struct sockaddr *namePtr, int nameLen));
extern ReturnStatus MachUNIXGetPeerName _ARGS_((int socketID, struct sockaddr *namePtr, int *nameLenPtr));
extern ReturnStatus MachUNIXGetSockName _ARGS_((int socketID, struct sockaddr *namePtr, int *nameLenPtr));
extern ReturnStatus MachUNIXGetSockOpt _ARGS_((int socketID, int level, int optName, char *optVal, int *optLenPtr));
extern ReturnStatus MachUNIXSetSockOpt _ARGS_((int socketID, int level, int optName, char *optVal, int optLen));
extern ReturnStatus MachUNIXListen _ARGS_((int socketID, int backlog));
extern ReturnStatus MachUNIXRecv _ARGS_((int socketID, char *bufPtr, int bufSize, int flags));
extern ReturnStatus MachUNIXRecvFrom _ARGS_((int socketID, char *bufPtr, int bufSize, int flags, struct sockaddr *senderPtr, int *senderLenPtr));
extern ReturnStatus MachUNIXRecvMsg _ARGS_((int socketID, struct msghdr *msgPtr, int flags));
extern ReturnStatus MachUNIXSend _ARGS_((int socketID, char *bufPtr, int bufSize, int flags));
extern ReturnStatus MachUNIXSendTo _ARGS_((int socketID, char *bufPtr, int bufSize, int flags, struct sockaddr *destPtr, int destLen));
extern ReturnStatus MachUNIXSendMsg _ARGS_((int socketID, struct msghdr *msgPtr, int flags));
extern ReturnStatus MachUNIXSocket _ARGS_((int domain, int type, int protocol));
extern ReturnStatus MachUNIXShutdown _ARGS_((int socketID, int action));


/*
 * The system call table.
 */
SyscallInfo machUNIXSysCallTable[] = {
#if 1
	"indir", 		-1, 	MachUNIXError,
	"exit",			-1, 	MachUNIXExit,
	"fork",			-1, 	MachUNIXFork,
	"read",			-1, 	MachUNIXRead,
	"write", 		-1, 	MachUNIXWrite,
	"open",			-1, 	MachUNIXOpen,
	"close", 		-1, 	MachUNIXClose,
	"old wait", 		-1, 	MachUNIXError,
	"creat", 		-1, 	MachUNIXCreat,
	"link",			-1, 	MachUNIXLink,	
	"unlink", 		-1, 	MachUNIXUnlink,
	"execv", 		-1, 	MachUNIXExecv,
	"chdir", 		-1, 	MachUNIXChdir,
	"old time", 		-1, 	MachUNIXError,
	"mknod", 		-1, 	MachUNIXError,
	"chmod", 		-1, 	MachUNIXChmod,
	"chown", 		-1, 	MachUNIXChown, 
	"sbrk", 		-1, 	MachUNIXSbrk,
	"old stat", 		-1,	MachUNIXError,
	"lseek", 		-1, 	MachUNIXLseek,
	"getpid", 		-1, 	MachUNIXGetPID,
	"mount", 		-1, 	MachUNIXError,
	"umount", 		-1, 	MachUNIXError,
	"old setuid", 		-1, 	MachUNIXError,
	"getuid", 		-1, 	MachUNIXGetuid,
	"old stime", 		-1, 	MachUNIXError,
	"ptrace", 		-1, 	MachUNIXPtrace,
	"old alarm", 		-1, 	MachUNIXError,
	"old fstat", 		-1, 	MachUNIXError,
	"old pause", 		-1, 	MachUNIXError,
	"old utime", 		-1, 	MachUNIXError,
	"old stty", 		-1, 	MachUNIXError,
	"old gtty", 		-1, 	MachUNIXError,
	"access", 		-1, 	MachUNIXAccess,
	"old nice", 		-1, 	MachUNIXError,
	"old ftime", 		-1, 	MachUNIXError,
	"sync",			-1, 	MachUNIXSync,
	"kill",			-1, 	MachUNIXKill,
	"stat",			-1, 	MachUNIXStat,
	"old setpgrp", 		-1, 	MachUNIXError,
	"lstat",		-1, 	MachUNIXLstat,
	"dup", 			-1, 	MachUNIXDup,
	"pipe",			-1, 	MachUNIXPipe,
	"old times",		-1, 	MachUNIXError,
	"profil", 		-1, 	MachUNIXProfil,
	"nosys", 		-1, 	MachUNIXError,
	"old setgid", 		-1, 	MachUNIXError,
	"getgid", 		-1, 	MachUNIXGetGID,
	"old sig", 		-1, 	MachUNIXError,
	"USG 1", 		-1, 	MachUNIXError,
	"USG 2", 		-1, 	MachUNIXError,
	"acct", 		-1, 	MachUNIXError,
	"old set phys addr", 	-1, 	MachUNIXError,
	"old syslock in core",	-1, 	MachUNIXError,
	"ioctl", 		-1, 	MachUNIXIoctl,
	"reboot", 		-1, 	MachUNIXReboot,
	"old mpxchan", 		-1, 	MachUNIXError,
	"symlink", 		-1, 	MachUNIXSymlink,
	"readlink", 		-1, 	MachUNIXReadLink,
	"execve", 		-1, 	MachUNIXExecve,
	"umask", 		-1, 	MachUNIXUmask,
	"chroot", 		-1, 	MachUNIXError,
	"fstat", 		-1, 	MachUNIXFstat,
	"used internally", 	-1, 	MachUNIXError,
	"getpagesize", 		-1, 	MachUNIXGetPageSize,
	"mremap", 		-1, 	MachUNIXError,
	"vfork", 		-1, 	MachUNIXVFork,
	"old vread", 		-1, 	MachUNIXError,
	"old vwrite", 		-1, 	MachUNIXError,
	"new sbrk",		-1, 	MachUNIXError,
	"sstk",			-1, 	MachUNIXError,
	"mmap", 		-1, 	MachUNIXMmap,
	"old vadvise", 		-1, 	MachUNIXError,
	"munmap", 		-1, 	MachUNIXMunmap,
	"mprotect", 		-1, 	MachUNIXError,
	"madvise", 		-1, 	MachUNIXError,
	"vhangup", 		-1, 	MachUNIXError,
	"old vlimit", 		-1, 	MachUNIXError,
	"mincore", 		-1, 	MachUNIXMincore,
	"getgroups", 		-1, 	MachUNIXGetGroups,
	"setgroups", 		-1, 	MachUNIXSetGroups,
	"getpgrp", 		-1, 	MachUNIXGetPGrp,
	"setpgrp", 		-1, 	MachUNIXSetPGrp,
	"setitimer", 		-1, 	MachUNIXSetITimer,
	"wait",			-1, 	MachUNIXWait,
	"swapon", 		-1, 	MachUNIXError,
	"getitimer", 		-1,	MachUNIXGetItimer,
	"gethostname", 		-1, 	MachUNIXGetHostName,
	"sethostname", 		-1, 	MachUNIXSetHostName,
	"getdtablesize", 	-1,	MachUNIXGetDTableSize,
	"dup2",			-1, 	MachUNIXDup2,
	"getdopt", 		-1, 	MachUNIXGetDOpt,
	"fcntl", 		-1, 	MachUNIXFcntl,
	"select", 		-1, 	MachUNIXSelect,
	"setdopt", 		-1, 	MachUNIXSetDOpt,
	"fsync", 		-1, 	MachUNIXFSync,
	"setpriority", 		-1, 	MachUNIXSetPriority,
	"socket", 		-1, 	MachUNIXSocket,
	"connect", 		-1, 	MachUNIXConnect,
	"accept", 		-1, 	MachUNIXAccept,
	"getpriority", 		-1,	MachUNIXGetPriority,
	"send",			-1,	MachUNIXSend,
	"recv",			-1, 	MachUNIXRecv,
	"sigreturn", 		-1,	MachUNIXLongJumpReturn,
	"bind",			-1,	MachUNIXBind,
	"setsockopt", 		-1, 	MachUNIXSetSockOpt,
	"listen", 		-1, 	MachUNIXListen,
	"old vtimes", 		-1,	MachUNIXError,
	"sigvec", 		-1, 	MachUNIXSigvec,
	"sigblock", 		-1, 	MachUNIXBlock,
	"sigsetmask", 		-1, 	MachUNIXSigSetmask,
	"sigpause", 		-1, 	MachUNIXSigPause,
	"sigstack", 		-1, 	MachUNIXSigStack,
	"recvmsg", 		-1, 	MachUNIXRecvMsg,
	"sendmsg", 		-1, 	MachUNIXSendMsg,
	"#115",			-1,	MachUNIXError,
	"gettimeofday",		-1, 	MachUNIXGetTimeOfDay,
	"getrusage", 		-1, 	MachUNIXGetRUsage,
	"getsockopt", 		-1, 	MachUNIXGetSockOpt,
	"#119", 		-1,	MachUNIXError,
	"readv", 		-1, 	MachUNIXReadv,
	"writev", 		-1, 	MachUNIXWritev,
	"settimeofday",		-1, 	MachUNIXSetTimeOfDay,
	"fchown", 		-1, 	MachUNIXFChown,
	"fchmod", 		-1, 	MachUNIXFChmod,
	"recvfrom", 		-1, 	MachUNIXRecvFrom,
	"setreuid", 		-1, 	MachUNIXSetREUID,
	"setregid", 		-1, 	MachUNIXSetREGID,
	"rename", 		-1, 	MachUNIXRename,
	"truncate", 		-1, 	MachUNIXTruncate,
	"ftruncate", 		-1, 	MachUNIXFTruncate,
	"flock", 		-1, 	MachUNIXFLock,
	"#132", 		-1,	MachUNIXError,
	"sendto",		-1, 	MachUNIXSendTo,
	"shutdown",		-1,	MachUNIXShutdown,
	"socketpair",		-1,	MachUNIXSocketPair,
	"mkdir",		-1,	MachUNIXMkDir,
	"rmdir",		-1,	MachUNIXRMDir,
	"utimes",		-1,	MachUNIXUTimes,
	"sigreturn(ljmp)", 	-1,	MachUNIXLongJumpReturn,
	"adjtime", 		-1, 	MachUNIXAdjTime,
	"getpeername",		-1, 	MachUNIXGetPeerName,
	"gethostid",		-1, 	MachUNIXGetHostID,
	"sethostid",		-1, 	MachUNIXSetHostID,
	"getrlimit",		-1, 	MachUNIXGetRLimit,
	"setrlimit",		-1, 	MachUNIXSetRLimit,
	"killpg",		-1, 	MachUNIXKillPG,
	"#147",			-1, 	MachUNIXError,
	"setquota", 		-1, 	MachUNIXError,
	"quota",		-1, 	MachUNIXError,
	"getsockname",  	-1, 	MachUNIXGetSockName,	
	"sysmips", 		-1, 	MachUNIXError,
	"cacheflush",		-1, 	MachUNIXError,
	"cachectl",		-1, 	MachUNIXError,
	"debug",		-1, 	MachUNIXError,
	"#155",			-1, 	MachUNIXError,
	"#156",			-1, 	MachUNIXError,
	"#157",			-1, 	MachUNIXError,
	"nfs_svc", 		-1, 	MachUNIXError,
	"getdirentries",	-1, 	MachUNIXGetDirEntries,
	"#160",			-1, 	MachUNIXError,
	"#161",			-1, 	MachUNIXError,
	"#162",			-1, 	MachUNIXError,
	"nfs_biod",		-1, 	MachUNIXError,
	"nfs_getfh",		-1, 	MachUNIXError,
	"getdomainname", 	-1, 	MachUNIXGetDomainName,
	"setdomainname", 	-1, 	MachUNIXSetDomainName,
	"#167",			-1, 	MachUNIXError,
	"#168",			-1, 	MachUNIXError,
	"exportfs",		-1, 	MachUNIXError,
	"#170",			-1, 	MachUNIXError,
	"#171",			-1, 	MachUNIXError,
	"msgctl",		-1, 	MachUNIXError,
	"msgget",		-1, 	MachUNIXError,
	"msgrcv",		-1, 	MachUNIXError,
	"msgsnd",		-1, 	MachUNIXError,
	"semctl",		-1, 	MachUNIXSemctl,
	"semget",		-1, 	MachUNIXSemget,
	"semop",		-1, 	MachUNIXSemop,
	"uname",		-1,	MachUNIXError,
	"shmsys",		-1, 	MachUNIXError,
	"plock",		-1, 	MachUNIXError,
	"lockf",		-1, 	MachUNIXError,
	"ustat",		-1, 	MachUNIXError,
	"getmnt",		-1, 	MachUNIXError,
	"mount",		-1, 	MachUNIXError,
	"umount",		-1, 	MachUNIXError,
	"sigpending",		-1, 	MachUNIXError,
	"#188",			-1, 	MachUNIXError,
	"#189",			-1, 	MachUNIXError,
	"#190",			-1, 	MachUNIXError,
	"#191",			-1, 	MachUNIXError,
	"#192",			-1, 	MachUNIXError,
	"#193",			-1, 	MachUNIXError,
	"#194",			-1, 	MachUNIXError,
	"#195",			-1, 	MachUNIXError,
	"#196",			-1, 	MachUNIXError,
	"#197",			-1, 	MachUNIXError,
	"#198",			-1, 	MachUNIXError,
	"#199",			-1, 	MachUNIXError,
	"#200",			-1, 	MachUNIXError,
	"#201",			-1, 	MachUNIXError,
	"#202",			-1, 	MachUNIXError,
	"#203",			-1, 	MachUNIXError,
	"#204",			-1, 	MachUNIXError,
	"#205",			-1, 	MachUNIXError,
	"#206",			-1, 	MachUNIXError,
	"#207",			-1, 	MachUNIXError,
	"#208",			-1, 	MachUNIXError,
	"#209",			-1, 	MachUNIXError,
	"#210",			-1, 	MachUNIXError,
	"#211",			-1, 	MachUNIXError,
	"#212",			-1, 	MachUNIXError,
	"#213",			-1, 	MachUNIXError,
	"#214",			-1, 	MachUNIXError,
	"#215",			-1, 	MachUNIXError,
	"#216",			-1, 	MachUNIXError,
	"#217",			-1, 	MachUNIXError,
	"#218",			-1, 	MachUNIXError,
	"#219",			-1, 	MachUNIXError,
	"#220",			-1, 	MachUNIXError,
	"#221",			-1, 	MachUNIXError,
	"#222",			-1, 	MachUNIXError,
	"#223",			-1, 	MachUNIXError,
	"#224",			-1, 	MachUNIXError,
	"#225",			-1, 	MachUNIXError,
	"#226",			-1, 	MachUNIXError,
	"#227",			-1, 	MachUNIXError,
	"#228",			-1, 	MachUNIXError,
	"#229",			-1, 	MachUNIXError,
#else
	"indir", 		0, 	MachUNIXError,
	"exit",			1, 	MachUNIXExit,
	"fork",			0, 	MachUNIXFork,
	"read",			3, 	MachUNIXRead,
	"write", 		3, 	MachUNIXWrite,
	"open",			3, 	MachUNIXOpen,
	"close", 		1, 	MachUNIXClose,
	"old wait", 		0, 	MachUNIXError,
	"creat", 		2, 	MachUNIXCreat,
	"link",			2, 	MachUNIXLink,	
	"unlink", 		1, 	MachUNIXUnlink,
	"execv", 		2, 	MachUNIXExecv,
	"chdir", 		1, 	MachUNIXChdir,
	"old time", 		0, 	MachUNIXError,
	"mknod", 		3, 	MachUNIXError,
	"chmod", 		2, 	MachUNIXChmod,
	"chown", 		3, 	MachUNIXChown, 
	"sbrk", 		1, 	MachUNIXSbrk,
	"old stat", 		2,	MachUNIXError,
	"lseek", 		3, 	MachUNIXLseek,
	"getpid", 		0, 	MachUNIXGetPID,
	"mount", 		5, 	MachUNIXError,
	"umount", 		1, 	MachUNIXError,
	"old setuid", 		1, 	MachUNIXError,
	"getuid", 		0, 	MachUNIXGetuid,
	"old stime", 		1, 	MachUNIXError,
	"ptrace", 		4, 	MachUNIXPtrace,
	"old alarm", 		1, 	MachUNIXError,
	"old fstat", 		2, 	MachUNIXError,
	"old pause", 		0, 	MachUNIXError,
	"old utime", 		2, 	MachUNIXError,
	"old stty", 		0, 	MachUNIXError,
	"old gtty", 		0, 	MachUNIXError,
	"access", 		2, 	MachUNIXAccess,
	"old nice", 		1, 	MachUNIXError,
	"old ftime", 		1, 	MachUNIXError,
	"sync",			0, 	MachUNIXSync,
	"kill",			2, 	MachUNIXKill,
	"stat",			2, 	MachUNIXStat,
	"old setpgrp", 		2, 	MachUNIXError,
	"lstat",		2, 	MachUNIXLstat,
	"dup", 			1, 	MachUNIXDup,
	"pipe",			1, 	MachUNIXPipe,
	"old times",		1, 	MachUNIXError,
	"profil", 		4, 	MachUNIXProfil,
	"nosys", 		0, 	MachUNIXError,
	"old setgid", 		1, 	MachUNIXError,
	"getgid", 		0, 	MachUNIXGetGID,
	"old sig", 		2, 	MachUNIXError,
	"USG 1", 		0, 	MachUNIXError,
	"USG 2", 		0, 	MachUNIXError,
	"acct", 		1, 	MachUNIXError,
	"old set phys addr", 	0, 	MachUNIXError,
	"old syslock in core",	0, 	MachUNIXError,
	"ioctl", 		3, 	MachUNIXIoctl,
	"reboot", 		1, 	MachUNIXReboot,
	"old mpxchan", 		0, 	MachUNIXError,
	"symlink", 		2, 	MachUNIXSymlink,
	"readlink", 		3, 	MachUNIXReadLink,
	"execve", 		3, 	MachUNIXExecve,
	"umask", 		1, 	MachUNIXUmask,
	"chroot", 		1, 	MachUNIXError,
	"fstat", 		2, 	MachUNIXFstat,
	"used internally", 	0, 	MachUNIXError,
	"getpagesize", 		0, 	MachUNIXGetPageSize,
	"mremap", 		5, 	MachUNIXError,
	"vfork", 		0, 	MachUNIXVFork,
	"old vread", 		0, 	MachUNIXError,
	"old vwrite", 		0, 	MachUNIXError,
	"new sbrk",		1, 	MachUNIXError,
	"sstk",			1, 	MachUNIXError,
	"mmap", 		6, 	MachUNIXMmap,
	"old vadvise", 		1, 	MachUNIXError,
	"munmap", 		2, 	MachUNIXMunmap,
	"mprotect", 		3, 	MachUNIXError,
	"madvise", 		3, 	MachUNIXError,
	"vhangup", 		1, 	MachUNIXError,
	"old vlimit", 		2, 	MachUNIXError,
	"mincore", 		3, 	MachUNIXMincore,
	"getgroups", 		2, 	MachUNIXGetGroups,
	"setgroups", 		2, 	MachUNIXSetGroups,
	"getpgrp", 		1, 	MachUNIXGetPGrp,
	"setpgrp", 		2, 	MachUNIXSetPGrp,
	"setitimer", 		3, 	MachUNIXSetITimer,
	"wait",			3, 	MachUNIXWait,
	"swapon", 		1, 	MachUNIXError,
	"getitimer", 		2,	MachUNIXGetItimer,
	"gethostname", 		2, 	MachUNIXGetHostName,
	"sethostname", 		2, 	MachUNIXSetHostName,
	"getdtablesize", 	0,	MachUNIXGetDTableSize,
	"dup2",			2, 	MachUNIXDup2,
	"getdopt", 		2, 	MachUNIXGetDOpt,
	"fcntl", 		3, 	MachUNIXFcntl,
	"select", 		5, 	MachUNIXSelect,
	"setdopt", 		2, 	MachUNIXSetDOpt,
	"fsync", 		1, 	MachUNIXFSync,
	"setpriority", 		3, 	MachUNIXSetPriority,
	"socket", 		3, 	MachUNIXSocket,
	"connect", 		3, 	MachUNIXConnect,
	"accept", 		3, 	MachUNIXAccept,
	"getpriority", 		2,	MachUNIXGetPriority,
	"send",			4,	MachUNIXSend,
	"recv",			4, 	MachUNIXRecv,
	"sigreturn", 		1,	MachUNIXLongJumpReturn,
	"bind",			3,	MachUNIXBind,
	"setsockopt", 		5, 	MachUNIXSetSockOpt,
	"listen", 		2, 	MachUNIXListen,
	"old vtimes", 		2,	MachUNIXError,
	"sigvec", 		4, 	MachUNIXSigvec,
	"sigblock", 		1, 	MachUNIXBlock,
	"sigsetmask", 		1, 	MachUNIXSigSetmask,
	"sigpause", 		1, 	MachUNIXSigPause,
	"sigstack", 		2, 	MachUNIXSigStack,
	"recvmsg", 		3, 	MachUNIXRecvMsg,
	"sendmsg", 		3, 	MachUNIXSendMsg,
	"#115",			0,	MachUNIXError,
	"gettimeofday",		2, 	MachUNIXGetTimeOfDay,
	"getrusage", 		2, 	MachUNIXGetRUsage,
	"getsockopt", 		5, 	MachUNIXGetSockOpt,
	"#119", 		0,	MachUNIXError,
	"readv", 		3, 	MachUNIXReadv,
	"writev", 		3, 	MachUNIXWritev,
	"settimeofday",		2, 	MachUNIXSetTimeOfDay,
	"fchown", 		3, 	MachUNIXFChown,
	"fchmod", 		2, 	MachUNIXFChmod,
	"recvfrom", 		6, 	MachUNIXRecvFrom,
	"setreuid", 		2, 	MachUNIXSetREUID,
	"setregid", 		2, 	MachUNIXSetREGID,
	"rename", 		2, 	MachUNIXRename,
	"truncate", 		2, 	MachUNIXTruncate,
	"ftruncate", 		2, 	MachUNIXFTruncate,
	"flock", 		2, 	MachUNIXFLock,
	"#132", 			0,	MachUNIXError,
	"sendto",		6, 	MachUNIXSendTo,
	"shutdown",		2,	MachUNIXShutdown,
	"socketpair",		5,	MachUNIXSocketPair,
	"mkdir",		2,	MachUNIXMkDir,
	"rmdir",		1,	MachUNIXRMDir,
	"utimes",		2,	MachUNIXUTimes,
	"sigreturn(ljmp)", 	1,	MachUNIXLongJumpReturn,
	"adjtime", 		2, 	MachUNIXAdjTime,
	"getpeername",		3, 	MachUNIXGetPeerName,
	"gethostid",		2, 	MachUNIXGetHostID,
	"sethostid",		2, 	MachUNIXSetHostID,
	"getrlimit",		2, 	MachUNIXGetRLimit,
	"setrlimit",		2, 	MachUNIXSetRLimit,
	"killpg",		2, 	MachUNIXKillPG,
	"#147",			0, 	MachUNIXError,
	"setquota", 		0, 	MachUNIXError,
	"quota",		0, 	MachUNIXError,
	"getsockname",  	3, 	MachUNIXGetSockName,	
	"sysmips", 		0, 	MachUNIXError,
	"cacheflush",		0, 	MachUNIXError,
	"cachectl",		0, 	MachUNIXError,
	"debug",		0, 	MachUNIXError,
	"#155",			0, 	MachUNIXError,
	"#156",			0, 	MachUNIXError,
	"#157",			0, 	MachUNIXError,
	"nfs_svc", 		0, 	MachUNIXError,
	"getdirentries",	4, 	MachUNIXGetDirEntries,
	"#160",			0, 	MachUNIXError,
	"#161",			0, 	MachUNIXError,
	"#162",			0, 	MachUNIXError,
	"nfs_biod",		0, 	MachUNIXError,
	"nfs_getfh",		0, 	MachUNIXError,
	"getdomainname", 	2, 	MachUNIXGetDomainName,
	"setdomainname", 	2, 	MachUNIXSetDomainName,
	"#167",			0, 	MachUNIXError,
	"#168",			0, 	MachUNIXError,
	"exportfs",		0, 	MachUNIXError,
	"#170",			0, 	MachUNIXError,
	"#171",			0, 	MachUNIXError,
	"msgctl",		0, 	MachUNIXError,
	"msgget",		0, 	MachUNIXError,
	"msgrcv",		0, 	MachUNIXError,
	"msgsnd",		0, 	MachUNIXError,
	"semctl",		4, 	MachUNIXSemctl,
	"semget",		3, 	MachUNIXSemget,
	"semop",		3, 	MachUNIXSemop,
	"uname",		0,	MachUNIXError,
	"shmsys",		0, 	MachUNIXError,
	"plock",		0, 	MachUNIXError,
	"lockf",		0, 	MachUNIXError,
	"ustat",		0, 	MachUNIXError,
	"getmnt",		0, 	MachUNIXError,
	"mount",		0, 	MachUNIXError,
	"umount",		0, 	MachUNIXError,
	"sigpending",		0, 	MachUNIXError,
	"#188",			0, 	MachUNIXError,
	"#189",			0, 	MachUNIXError,
	"#190",			0, 	MachUNIXError,
	"#191",			0, 	MachUNIXError,
	"#192",			0, 	MachUNIXError,
	"#193",			0, 	MachUNIXError,
	"#194",			0, 	MachUNIXError,
	"#195",			0, 	MachUNIXError,
	"#196",			0, 	MachUNIXError,
	"#197",			0, 	MachUNIXError,
	"#198",			0, 	MachUNIXError,
	"#199",			0, 	MachUNIXError,
	"#200",			0, 	MachUNIXError,
	"#201",			0, 	MachUNIXError,
	"#202",			0, 	MachUNIXError,
	"#203",			0, 	MachUNIXError,
	"#204",			0, 	MachUNIXError,
	"#205",			0, 	MachUNIXError,
	"#206",			0, 	MachUNIXError,
	"#207",			0, 	MachUNIXError,
	"#208",			0, 	MachUNIXError,
	"#209",			0, 	MachUNIXError,
	"#210",			0, 	MachUNIXError,
	"#211",			0, 	MachUNIXError,
	"#212",			0, 	MachUNIXError,
	"#213",			0, 	MachUNIXError,
	"#214",			0, 	MachUNIXError,
	"#215",			0, 	MachUNIXError,
	"#216",			0, 	MachUNIXError,
	"#217",			0, 	MachUNIXError,
	"#218",			0, 	MachUNIXError,
	"#219",			0, 	MachUNIXError,
	"#220",			0, 	MachUNIXError,
	"#221",			0, 	MachUNIXError,
	"#222",			0, 	MachUNIXError,
	"#223",			0, 	MachUNIXError,
	"#224",			0, 	MachUNIXError,
	"#225",			0, 	MachUNIXError,
	"#226",			0, 	MachUNIXError,
	"#227",			0, 	MachUNIXError,
	"#228",			0, 	MachUNIXError,
	"#229",			0, 	MachUNIXError,
#endif
	"#230",			0, 	MachUNIXError,
	"#231",			0, 	MachUNIXError,
	"#232",			0, 	MachUNIXError,
	"#233",			0, 	MachUNIXError,
	"#234",			0, 	MachUNIXError,
	"#235",			0, 	MachUNIXError,
	"#236",			0, 	MachUNIXError,
	"#237",			0, 	MachUNIXError,
	"#238",			0, 	MachUNIXError,
	"#239",			0, 	MachUNIXError,
	"#240",			0, 	MachUNIXError,
	"#241",			0, 	MachUNIXError,
	"#242",			0, 	MachUNIXError,
	"#243",			0, 	MachUNIXError,
	"#244",			0, 	MachUNIXError,
	"#245",			0, 	MachUNIXError,
	"#246",			0, 	MachUNIXError,
	"#247",			0, 	MachUNIXError,
	"#248",			0, 	MachUNIXError,
	"#249",			0, 	MachUNIXError,
	"#250",			0, 	MachUNIXError,
	"#251",			0, 	MachUNIXError,
	"#252",			0, 	MachUNIXError,
	"#253",			0, 	MachUNIXError,
	"#254",			0, 	MachUNIXError,
	"#255",			0, 	MachUNIXError,
	"getsysinfo",		0, 	MachUNIXGetSysInfo,
	"setsysinfo",		0, 	MachUNIXError,
};

int machNumUNIXSyscalls = sizeof(machUNIXSysCallTable) / sizeof(SyscallInfo);

extern Mach_State	*machCurStatePtr;

#define UNIX_SIG_RETURN		103
#define UNIX_LONG_JUMP_RETURN	139


/*
 * ----------------------------------------------------------------------------
 *
 * MachUNIXSyscall --
 *
 *	Try and handle the given UNIX system call.
 *	Note that this routine is called only if (machUNIXSyscallTrace > 0)
 *	or under special conditions.
 *
 * Results:
 *	TRUE if this was really a UNIX system call.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
MachUNIXSyscall()
{
    unsigned		*regs;
    int			args[6];
    ReturnStatus	status;
    int			type;
    int			numArgs;
#ifdef CANT_MIGRATE_COMPAT
    Proc_ControlBlock	*procPtr;
#endif /* CANT_MIGRATE_COMPAT */

    if (machNewUnixCompat) {
	return MachUNIXSyscallNew();
    }

    /*
     * See if we got a UNIX system call.  Unix passes the system call type
     * in v0.
     */
    type = machCurStatePtr->userState.regState.regs[V0];
    if (type < 0 || type >= machNumUNIXSyscalls) {
	printf("MachUNIXSyscall failed with type %d\n", type);
	return(FALSE);
    }

#ifdef CANT_MIGRATE_COMPAT
    /*
     * We don't want to migrate processes that are making unix-compatible calls.
     */
    procPtr = Proc_GetCurrentProc();
    if (!(procPtr->genFlags & PROC_DONT_MIGRATE)) {
	Proc_NeverMigrate(procPtr);
    }
#endif /* CANT_MIGRATE_COMPAT */
    
    numArgs = machUNIXSysCallTable[type].numArgs;
    regs = machCurStatePtr->userState.regState.regs;
    if (numArgs > 4) {
	if (Vm_CopyIn(4 * (numArgs - 4), regs[SP] + 16, args) != SUCCESS) {
	    regs[V0] = Compat_MapCode(SYS_ARG_NOACCESS);
	    regs[A3] = 1;
	    return(TRUE);
	}
    }
    machCurStatePtr->userState.unixRetVal = 0;
    switch (numArgs) {
	case 0:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s() => ", 
			machUNIXSysCallTable[type].name);
	    }
	    status = machUNIXSysCallTable[type].func();
	    break;
	case 1:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0]);
	    break;
	case 2:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0], regs[A1]);
	    break;
	case 3:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2]);
	    break;
	case 4:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3]);
	    break;
	case 5: 
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0]);
	    break; 
	case 6:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x, %x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0],
					     args[1]);
	    break; 
	case 7:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x, %x, %x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1], args[2]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0],
					     args[1], args[2]);
	    break;
	case 8:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscall: %s(%x, %x, %x, %x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1], args[2], args[3]);
	    }
	    status = machUNIXSysCallTable[type].func(regs[A0], regs[A1], 
					     regs[A2], regs[A3], args[0],
					     args[1], args[2], args[3]);
	default:
	    return MachUNIXSyscallNew();
	    /*
	    panic("Too many args to UNIX system call\n");
	    */
	    break;
    }
    /* 
     * The long jump and signal return system calls do things differently on
     * a return.  Everyone else does the standard thing.
     */
    if (type != MACH_UNIX_LONG_JUMP_RETURN && type != MACH_UNIX_SIG_RETURN) {
	/*
	 * The UNIX stubs looks at a3 to decide what to do.  If a3 == 1 then
	 * v0 has the UNIX error code and the stub will stuff the error
	 * into errno.  If a3 == 0 the V0 has the return value.
	 */
	if (status == GEN_ABORTED_BY_SIGNAL) {
	    printf("System call interrupted by signal\n");
	}
	if (status == SUCCESS) {
	    regs[V0] = machCurStatePtr->userState.unixRetVal;
	    regs[A3] = 0;
	} else {
	    regs[V0] = Compat_MapCode(status);
	    regs[A3] = 1;
	}
	if (machUNIXSyscallTrace > 1) {
	    printf("V0 = %x A3 = %x\n", regs[V0], regs[A3]);
	}
    }
    machCurStatePtr->userState.regState.pc += 4;

    return(TRUE);
}



/*
 * ----------------------------------------------------------------------------
 *
 * MachUNIXSyscallNew --
 *
 *	Try and handle the new given UNIX system call.
 *	Note that this routine is called only if (machUNIXSyscallTrace > 0)
 *	or under special conditions.
 *
 * Results:
 *	TRUE if this was really a UNIX system call.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
MachUNIXSyscallNew()
{
    unsigned		*regs;
    int			args[6];
    ReturnStatus	status;
    int			type;
    int			numArgs;
#ifdef CANT_MIGRATE_COMPAT
    Proc_ControlBlock	*procPtr;
#endif /* CANT_MIGRATE_COMPAT */

    /*
     * See if we got a UNIX system call.  Unix passes the system call type
     * in v0.
     */
    type = machCurStatePtr->userState.regState.regs[V0];
    machCurStatePtr->userState.savedV0 = type;
    machCurStatePtr->userState.savedA3 =
	    machCurStatePtr->userState.regState.regs[A3];
    if (type < 0 || type >= MACH_MAX_UNIX_SYSCALL) {
	printf("MachUNIXSyscallNew failed with type %d\n", type);
	return(FALSE);
    }

#ifdef CANT_MIGRATE_COMPAT
    /*
     * We don't want to migrate processes that are making unix-compatible calls.
     */
    procPtr = Proc_GetCurrentProc();
    if (!(procPtr->genFlags & PROC_DONT_MIGRATE)) {
	Proc_NeverMigrate(procPtr);
    }
#endif /* CANT_MIGRATE_COMPAT */
    
    numArgs = sysUnixSysCallTable[type].numArgs;
    regs = machCurStatePtr->userState.regState.regs;
    if (numArgs > 4) {
	if (Vm_CopyIn(4 * (numArgs - 4), regs[SP] + 16, args) != SUCCESS) {
	    regs[V0] = Compat_MapCode(SYS_ARG_NOACCESS);
	    regs[A3] = 1;
	    return(TRUE);
	}
    }
    switch (numArgs) {
	case 0:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s() => ", 
			machUNIXSysCallTable[type].name);
	    }
	    status = sysUnixSysCallTable[type].func();
	    break;
	case 1:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0]);
	    break;
	case 2:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1]);
	    break;
	case 3:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2]);
	    break;
	case 4:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3]);
	    break;
	case 5: 
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0]);
	    break; 
	case 6:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x, %x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0],
					     args[1]);
	    break; 
	case 7:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x, %x, %x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1], args[2]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0],
					     args[1], args[2]);
	    break;
	case 8:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: %s(%x, %x, %x, %x, %x, %x, %x, %x) => ", 
			machUNIXSysCallTable[type].name,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1], args[2], args[3]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1], 
					     regs[A2], regs[A3], args[0],
					     args[1], args[2], args[3]);
	default:
	    panic("Too many args to UNIX system call\n");
	    break;
    }
    /*
     * The UNIX stubs looks at a3 to decide what to do.  If a3 == 1 then
     * v0 has the UNIX error code and the stub will stuff the error
     * into errno.  If a3 == 0 the V0 has the return value.
     */
    if (type != MACH_UNIX_LONG_JUMP_RETURN && type != MACH_UNIX_SIG_RETURN) {
	if (status >= 0) {
	    regs[V0] = status;
	    regs[A3] = 0;
	} else {
	    regs[V0] = Proc_GetActualProc()->unixErrno;
	    regs[A3] = 1;
	}
    }
    if (machUNIXSyscallTrace > 1) {
	printf("V0 = %x A3 = %x\n", regs[V0], regs[A3]);
    }

    machCurStatePtr->userState.regState.pc += 4;

    return(TRUE);
}

ReturnStatus
MachUNIXExit(status)
    int	status;
{
    Proc_Exit(status);
    return(FAILURE);
}

ReturnStatus
MachUNIXFork()
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);

    /*
     * Put the right values in V1 and A3 for the child because the process
     * jumps directly to user space after it is created.  A 1 in v1 
     * is what the system call stub in the user process expects for the
     * child.
     */
    machCurStatePtr->userState.regState.regs[V1] = 1;
    machCurStatePtr->userState.regState.regs[A3] = 0;

    status = Proc_Fork(FALSE, (int *)usp);
    if (status == PROC_CHILD_PROC) {
	printf("MachUNIXFork: Child came alive here?\n");
	return(SUCCESS);
    }
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
		(Address)&machCurStatePtr->userState.unixRetVal);
    }
    /* 
     * The user fork stub expects a 0 for the parent.
     */
    machCurStatePtr->userState.regState.regs[V1] = 0;
    return(status);
}


ReturnStatus
MachUNIXVFork()
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);

    /*
     * Put the right values in V1 and A3 for the child because the process
     * jumps directly to user space after it is created.  A 1 in v1 
     * is what the system call stub in the user process expects for the
     * child.
     */
    machCurStatePtr->userState.regState.regs[V1] = 1;
    machCurStatePtr->userState.regState.regs[A3] = 0;

    status = Proc_Fork(TRUE, (int *)usp);
    if (status == PROC_CHILD_PROC) {
	printf("MachUNIXFork: Child came alive here?\n");
	return(SUCCESS);
    }
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
    } 
    /* 
     * The user fork stub expects a 0 for the parent.
     */
    machCurStatePtr->userState.regState.regs[V1] = 0;
    return(status);
}

ReturnStatus
MachUNIXRead(streamID, buffer, numBytes)
    int 	streamID;	/* descriptor for stream to read */
    char	*buffer;	/* pointer to buffer area */
    int		numBytes;	/* number of bytes to read */
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Fs_ReadStub(streamID, numBytes, buffer, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    if (status == GEN_ABORTED_BY_SIGNAL) {
	printf("Read blown away by signal\n");
    }
    return(status);
}


ReturnStatus
MachUNIXWrite(streamID, buffer, numBytes)
    int 	streamID;	/* descriptor for stream to read */
    char	*buffer;	/* pointer to buffer area */
    int		numBytes;	/* number of bytes to read */
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Fs_WriteStub(streamID, numBytes, buffer, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    if (status == GEN_ABORTED_BY_SIGNAL) {
	printf("Write blown away by signal\n");
    }
    return(status);
}

ReturnStatus
MachUNIXOpen(pathName, unixFlags, permissions)
    char	*pathName;	/* The name of the file to open */
    int		unixFlags;	/* O_RDONLY O_WRONLY O_RDWR O_NDELAY
				 * O_APPEND O_CREAT O_TRUNC O_EXCL */
    int		permissions;	/* Permission mask to use on creation */
{
    ReturnStatus status;	/* result returned by Fs_Open */
    register int useFlags = 0;	/* Sprite version of flags */
    Address		usp;

    /*
     * Convert unixFlags to FS_READ, etc.
     */
     
    if (unixFlags & FASYNC) {
	printf("open - FASYNC not supported\n");
	return(SYS_INVALID_ARG);
    }
    if (unixFlags & O_RDWR) {
	useFlags |= FS_READ|FS_WRITE;
    } else if (unixFlags & O_WRONLY) {
	useFlags |= FS_WRITE;
    } else {
	useFlags |= FS_READ;
    }
    if (unixFlags & FNDELAY) {
	useFlags |= FS_NON_BLOCKING;
    }
    if (unixFlags & FAPPEND) {
	useFlags |= FS_APPEND;
    }
    if (unixFlags & FTRUNC) {
	useFlags |= FS_TRUNC;
    }
    if (unixFlags & FEXCL) {
	useFlags |= FS_EXCLUSIVE;
    }
    if (unixFlags & O_MASTER) {
	useFlags |= FS_PDEV_MASTER;
    }
    if (unixFlags & O_PFS_MASTER) {
	useFlags |= FS_PFS_MASTER;
    }
    if (unixFlags & FCREAT) {
	useFlags |= FS_CREATE;
    }

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);

    status = Fs_OpenStub(pathName, useFlags, permissions, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp,
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXClose(streamID)
    int streamID;
{
    return(Fs_UserClose(streamID));
}

ReturnStatus
MachUNIXCreat(pathName, permissions)
    char *pathName;		/* The name of the file to create */
    int permissions;		/* Permission mask to use on creation */
{
    return(MachUNIXOpen(pathName,  FS_CREATE|FS_TRUNC|FS_WRITE,
			permissions));
}

ReturnStatus
MachUNIXLink(name1, name2)
    char *name1;
    char *name2;
{
    return(Fs_HardLinkStub(name1, name2));
}

ReturnStatus
MachUNIXUnlink(pathName)
    char *pathName;
{
    return(Fs_RemoveStub(pathName));
}

ReturnStatus
MachUNIXExecve(name, argv, envp)
    char *name;			/* name of file to exec */
    char *argv[];		/* array of arguments */
    char *envp[];		/* array of environment pointers */
{
    return(Proc_ExecEnv(name, argv, envp, FALSE));
}

ReturnStatus
MachUNIXExecv(name, argv)
    char *name;			/* Name of file containing program to exec. */
    char **argv;		/* Array of arguments to pass to program. */
{
    return(Proc_Exec(name, argv, FALSE));
}

ReturnStatus
MachUNIXChdir(pathName)
    char	*pathName;
{
    return(Fs_ChangeDirStub(pathName));
}

ReturnStatus
MachUNIXChmod(path, mode)
    char	*path;
    int		mode;
{
    Fs_Attributes	attributes;
    Address		usp;
    ReturnStatus	status;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 
		     sizeof(Fs_Attributes));

    attributes.permissions = mode;
    status = Vm_CopyOut(sizeof(Fs_Attributes), (Address)&attributes, usp);
    if (status == SUCCESS) {
	status = Fs_SetAttrStub(path,  FS_ATTRIB_FILE, (Fs_Attributes *)usp, 
			        FS_SET_MODE);
    }
    return(status);
}

ReturnStatus
MachUNIXFChmod(fd, mode)
    int		fd;
    int		mode;
{
    Fs_Attributes	attributes;
    Address		usp;
    ReturnStatus	status;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 
		     sizeof(Fs_Attributes));

    attributes.permissions = mode;
    status = Vm_CopyOut(sizeof(Fs_Attributes), (Address)&attributes, usp);
    if (status == SUCCESS) {
	status = Fs_SetAttrIDStub(fd, (Fs_Attributes *)usp, FS_SET_MODE);
    }
    return(status);
}

ReturnStatus
MachUNIXChown(path, owner, group)
    char *path;
    int owner;
    int group;
{
    ReturnStatus status;	/* result returned by Sprite system calls */
    Fs_Attributes attributes;	/* struct containing all file attributes.
				 * only ownership is looked at. */
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 
		     sizeof(Fs_Attributes));

    attributes.uid = owner;
    attributes.gid = group;
    status = Vm_CopyOut(sizeof(Fs_Attributes), (Address)&attributes, usp);
    if (status == SUCCESS) {
	status = Fs_SetAttrStub(path,  FS_ATTRIB_LINK, (Fs_Attributes *)usp,
			        FS_SET_OWNER);
    }
    return(status);
}

ReturnStatus
MachUNIXFChown(fd, owner, group)
    int fd;
    int owner;
    int group;
{
    ReturnStatus status;	/* result returned by Sprite system calls */
    Fs_Attributes attributes;	/* struct containing all file attributes.
				 * only ownership is looked at. */
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 
		     sizeof(Fs_Attributes));

    attributes.uid = owner;
    attributes.gid = group;
    status = Vm_CopyOut(sizeof(Fs_Attributes), (Address)&attributes, usp);
    if (status == SUCCESS) {
	status = Fs_SetAttrIDStub(fd,  (Fs_Attributes *)usp, FS_SET_OWNER);
    }
    return(status);
}

ReturnStatus
MachUNIXSbrk(addr)
    Address	addr;
{
    Vm_Segment	*segPtr;
    Address	lastAddr;
    Proc_ControlBlock	*procPtr;

    /*
     * The UNIX brk and sbrk call stubs figure where the end of the
     * heap is and they always call us with the new end of data segment.
     */
    procPtr = Proc_GetCurrentProc();
    segPtr = procPtr->vmPtr->segPtrArray[VM_HEAP];
    if (segPtr != (Vm_Segment *)NIL) {
	lastAddr =
		(Address) ((segPtr->offset + segPtr->numPages) * vm_PageSize);
	return(Vm_CreateVA(lastAddr, addr - lastAddr));
    }
    return(FAILURE);
}

ReturnStatus
MachUNIXLseek(streamID, offset, whence)
    int streamID;			/* array of stream identifiers */
    long offset;
    int whence;
{
    ReturnStatus	status;
    Ioc_RepositionArgs args;
    Address		usp;

    switch(whence) {
	case L_SET:
	    args.base = IOC_BASE_ZERO;
	    break;
	case L_INCR:
	    args.base = IOC_BASE_CURRENT;
	    break;
	case L_XTND:
	    args.base = IOC_BASE_EOF;
	    break;
	default:
	    return(SYS_INVALID_ARG);
    }
    args.offset = offset;
    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Fs_IOControlStub(streamID, IOC_REPOSITION, 
			      sizeof(Ioc_RepositionArgs),
			      (Address)&args, sizeof(int), usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp,
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXGetPID()
{
#ifdef notdef
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Proc_GetIDs((int *)usp, (int *) NULL, (int *) NULL, (int *) NULL);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp,
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
#endif
    register	Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetEffectiveProc();
    machCurStatePtr->userState.unixRetVal = procPtr->processID;
    return(SUCCESS);
}

ReturnStatus
MachUNIXGetuid()
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Proc_GetIDs((int *) NULL, (int *) NULL, (int *)usp, (int *) NULL);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp,
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXAccess(pathName, mode)
    char *pathName;		/* The name of the file to open */
    int	 mode;			/* access mode to test for */
{
    int spriteMode;

    if (mode == F_OK) {
	spriteMode = FS_EXISTS;
    } else {
	spriteMode = ((mode&R_OK)?FS_READ:0) | ((mode&W_OK)?FS_WRITE:0) |
	    ((mode&X_OK)?FS_EXECUTE:0);
    }
    return(Fs_CheckAccess(pathName, spriteMode, TRUE));
}

ReturnStatus MachUNIXKill(pid, sig)
    int pid;
    int sig;
{
    ReturnStatus status;
    int		 spriteSignal;

    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || (spriteSignal == NULL && sig != 0)) {
	return(SYS_INVALID_ARG);
    }
    if (pid == 0) {
	pid = PROC_MY_PID;
    }
    return(Sig_UserSend(spriteSignal, pid, FALSE));
}

ReturnStatus MachUNIXStat(pathName, attsBufPtr)
    char *pathName;		/* The name of the file to stat */
    struct stat *attsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAtts;	/* buffer for attributes using
					   Sprite format. */
    Address		usp;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 
		    sizeof(Fs_Attributes));

    status = Fs_GetAttributesStub(pathName, FS_ATTRIB_FILE,
				  (Fs_Attributes *)usp);
    if (status != SUCCESS) {
	return(status);
    }
    (void)Vm_CopyIn(sizeof(Fs_Attributes), (Address)usp, (Address)&spriteAtts);
    return(CvtSpriteToUnixAtts(&spriteAtts, attsBufPtr));
}


ReturnStatus MachUNIXLstat(pathName, attsBufPtr)
    char *pathName;		/* The name of the file to stat */
    struct stat *attsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAtts;	/* buffer for attributes using
					   Sprite format. */
    Address		usp;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 
		    sizeof(Fs_Attributes));

    status = Fs_GetAttributesStub(pathName, FS_ATTRIB_LINK,
				  (Fs_Attributes *)usp);
    if (status != SUCCESS) {
	return(status);
    }
    (void)Vm_CopyIn(sizeof(Fs_Attributes), (Address)usp, (Address)&spriteAtts);
    /*
     * See if we just got a remote link.  If so turn around and do a normal
     * stat because in compatibility mode we want to follow remote links.
     */
    if (spriteAtts.type == FS_REMOTE_LINK) {
	status = Fs_GetAttributesStub(pathName, FS_ATTRIB_FILE,
				      (Fs_Attributes *)usp);
	if (status != SUCCESS) {
	    return(status);
	}
	(void)Vm_CopyIn(sizeof(Fs_Attributes), (Address)usp, 
			(Address)&spriteAtts);
    }

    return(CvtSpriteToUnixAtts(&spriteAtts, attsBufPtr));
}


ReturnStatus MachUNIXFstat(fd, attsBufPtr)
    int	fd;		/* The name of the file to stat */
    struct stat *attsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAtts;	/* buffer for attributes using
					   Sprite format. */
    Address		usp;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 
		    sizeof(Fs_Attributes));

    status = Fs_GetAttributesIDStub(fd, (Fs_Attributes *)usp);
    if (status != SUCCESS) {
	return(status);
    }
    (void)Vm_CopyIn(sizeof(Fs_Attributes), (Address)usp, (Address)&spriteAtts);
    return(CvtSpriteToUnixAtts(&spriteAtts, attsBufPtr));
}

ReturnStatus
MachUNIXDup(oldStreamID)
    int oldStreamID;
{
    ReturnStatus	status;
    Address		usp;
    int			dummy;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    dummy = FS_ANYID;
    status = Vm_CopyOut(sizeof(int), (Address)&dummy, usp);
    if (status != SUCCESS) {
	return(status);
    }
    status = Fs_GetNewIDStub(oldStreamID, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXDup2(oldStreamID, newStreamID)
    int oldStreamID;		/* original stream identifier */
    int newStreamID;		/* new stream identifier */
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Vm_CopyOut(sizeof(int), (Address)&newStreamID, usp);
    if (status != SUCCESS) {
	return(status);
    }
    status = Fs_GetNewIDStub(oldStreamID, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXPipe(filedes)
    int	filedes[2];
{
    ReturnStatus	status;

    status = Fs_CreatePipeStub(&(filedes[0]), &(filedes[1]));
    if (status == SUCCESS) {
	/*
	 * When pipe returns, the stub expects v0 to contain the first
	 * file id and v1 to contain the second one.  Our caller will
	 * put unixRetVal into v0 for us, but we have to deal with v1.
	 */
	(void)Vm_CopyIn(sizeof(int), (Address)&(filedes[0]),
			(Address)&machCurStatePtr->userState.unixRetVal);
	(void)Vm_CopyIn(sizeof(int), (Address)&(filedes[1]),
			(Address)&machCurStatePtr->userState.regState.regs[V1]);
    }
}

ReturnStatus
MachUNIXGetGID()
{
    /*
     * The Sprite group id for Sprite at Berkeley.  Should do a better job
     * of this.
     */
    machCurStatePtr->userState.unixRetVal = 155;

    return(SUCCESS);
}

ReturnStatus
MachUNIXSymlink(target, link) 
    char *target;
    char *link;
{
    return(Fs_SymLinkStub(target, link, FALSE));
}

ReturnStatus
MachUNIXReadLink(link, buffer, numBytes)
    char *link;			/* name of link file to read */
    char *buffer;		/* pointer to buffer area */
    int numBytes;		/* number of bytes to read */
{
    ReturnStatus status;	/* result returned by Fs_Read */
    Address	 usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);

    status = Fs_ReadLinkStub(link, numBytes, buffer, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
	/*
	 * Sprite's Fs_ReadLink includes the terminating null character
	 * in the character count return  while Unix doesn't. 
	 */
	machCurStatePtr->userState.unixRetVal -= 1;
    }
    return(status);
}

#define PERMISSION_MASK 0777

ReturnStatus
MachUNIXUmask(newMask)
    int newMask;
{
    ReturnStatus status;	/* result returned by Fs_Read */
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);

    status = Fs_SetDefPermStub((~newMask) & PERMISSION_MASK, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
	machCurStatePtr->userState.unixRetVal = 
		(~(machCurStatePtr->userState.unixRetVal)) & PERMISSION_MASK;
    }
    return(status);
}

ReturnStatus
MachUNIXGetPageSize()
{
    machCurStatePtr->userState.unixRetVal = vm_PageSize;
    return(SUCCESS);
}

ReturnStatus
MachUNIXGetGroups(gidsetlen, gidset)
    int gidsetlen;
    int *gidset;
{
    ReturnStatus status;	/* result returned by Proc_GetGroupIDs */
    Address usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);

    status = Proc_GetGroupIDs(gidsetlen, gidset, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
	if (machCurStatePtr->userState.unixRetVal > gidsetlen) {
	    machCurStatePtr->userState.unixRetVal = gidsetlen;
	}
    }
    return(status);
}

ReturnStatus
MachUNIXSetGroups(ngroups, gidset)
    int ngroups;
    int *gidset;
{
    return(Proc_SetGroupIDs(ngroups, gidset));
}

ReturnStatus
MachUNIXGetPGrp(pid)
    int pid;
{
    Address usp;
    ReturnStatus status;	/* result returned by Proc_GetFamilyID */

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    if (pid == 0) {
	pid = PROC_MY_PID;
    }
    status = Proc_GetFamilyID(pid, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
		        (Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus MachUNIXSetPGrp(pid, pgrp)
    int pid;
{
    if (pid == 0) {
	pid = PROC_MY_PID;
    }
    return(Proc_SetFamilyID(pid, pgrp));
}

ReturnStatus MachUNIXWait(statusPtr, options, unixRusagePtr)
    union	wait	*statusPtr;
    int			options;
    struct	rusage	*unixRusagePtr;
{
    Proc_ResUsage spriteRusage;
    Proc_ResUsage *spriteRusagePtr;
    ReturnStatus status;	/* result returned by Proc_Wait */
    int pid;			/* process ID of child */
    int *pidPtr;
    int reason;			/* reason child exited */
    int *reasonPtr;
    int childStatus;		/* ReturnStatus of child */
    int *childStatusPtr;
    int	flags = 0;
    Address usp;
    union wait waitStatus;
    struct	rusage	unixRusage;

    if (!(options & WNOHANG)) {
	flags |= PROC_WAIT_BLOCK;
    }
    if (options & WUNTRACED) {
	flags |= PROC_WAIT_FOR_SUSPEND;
    }

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP]);
    pidPtr = (int *)(usp - 4);
    reasonPtr = (int *)(usp - 8);
    childStatusPtr = (int *)(usp - 12);
    spriteRusagePtr = (Proc_ResUsage *)(usp - 12 - sizeof(Proc_ResUsage));
    status = Proc_Wait(0, (int *) NULL, flags, pidPtr, reasonPtr, 
		       childStatusPtr, (int *)NULL, spriteRusagePtr);
    if (status == GEN_ABORTED_BY_SIGNAL) {
	printf("Wait blown away by signal\n");
    }
    if (status != SUCCESS) {
	return(status);
    }
    (void)Vm_CopyIn(sizeof(int), (Address)pidPtr, (Address)&pid);
    (void)Vm_CopyIn(sizeof(int), (Address)reasonPtr, (Address)&reason);
    (void)Vm_CopyIn(sizeof(int), (Address)childStatusPtr,
		   (Address)&childStatus);
    (void)Vm_CopyIn(sizeof(int), (Address)spriteRusagePtr,
		   (Address)&spriteRusage);
    if (statusPtr != NULL)  {
	int	unixSignal;
	waitStatus.w_status = 0;
	if (reason == PROC_TERM_SUSPENDED) {
	    (void)Compat_SpriteSignalToUnix(childStatus, &unixSignal);
	    waitStatus.w_stopval = WSTOPPED;
	    waitStatus.w_stopsig = unixSignal;
	} else if (reason == PROC_TERM_SIGNALED ||
		   reason == PROC_TERM_RESUMED) {
	    (void)Compat_SpriteSignalToUnix(childStatus, &unixSignal);
	    waitStatus.w_termsig = unixSignal;
	    /* NEED TO HANDLE coredump FIELD */
	} else {
	    waitStatus.w_retcode = childStatus;
	}
	status = Vm_CopyOut(sizeof(waitStatus), (Address)&waitStatus,
			    (Address)statusPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    if (unixRusagePtr != NULL) {
	/*
	 * Return the total time used by the process and all its children.
	 */
	Time totalKTime;
	Time totalUTime;

	bzero((char *) &unixRusage, sizeof(*unixRusagePtr));
	Time_Add(spriteRusage.userCpuUsage, spriteRusage.childUserCpuUsage,
					    &totalUTime);
	Time_Add(spriteRusage.kernelCpuUsage,
		 spriteRusage.childKernelCpuUsage, &totalKTime);
	unixRusage.ru_utime.tv_sec = totalUTime.seconds;
	unixRusage.ru_utime.tv_usec = totalUTime.microseconds;
	unixRusage.ru_stime.tv_sec = totalKTime.seconds;
	unixRusage.ru_stime.tv_usec = totalKTime.microseconds;
	unixRusage.ru_nvcsw = spriteRusage.numWaitEvents;
	unixRusage.ru_nivcsw = spriteRusage.numQuantumEnds;
	status = Vm_CopyOut(sizeof(unixRusage), (Address)&unixRusage,
			    (Address)unixRusagePtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    machCurStatePtr->userState.unixRetVal = pid;
    return(SUCCESS);
}

ReturnStatus
MachUNIXGetDTableSize()
{
    machCurStatePtr->userState.unixRetVal = 100;
    return(SUCCESS);
}

ReturnStatus
MachUNIXSelect(width, readfds, writefds, exceptfds, timeout)
    int width, *readfds, *writefds, *exceptfds;
    Time *timeout;
{
    Address		usp;
    ReturnStatus	status;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Fs_SelectStub(width, timeout, readfds, writefds,
			   exceptfds, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp, 
			(Address)&machCurStatePtr->userState.unixRetVal);
    } else if (status == FS_TIMEOUT) {
	int bytesInMask;
	int *zeroPtr;

	bytesInMask = ((width + 31) / 32) * sizeof(int);
	zeroPtr = (int *)malloc(bytesInMask);
	bzero((Address)zeroPtr, bytesInMask);

	if (readfds != NULL) {
	    (void)Vm_CopyOut(bytesInMask, (Address)zeroPtr, 
			     (Address)readfds);
	}
	if (writefds != NULL) {
	    (void)Vm_CopyOut(bytesInMask, (Address)zeroPtr, 
			     (Address)writefds);
	}
	if (exceptfds != NULL) {
	    (void)Vm_CopyOut(bytesInMask, (Address)zeroPtr, 
			     (Address)exceptfds);
	}
	free((Address)zeroPtr);
        status = SUCCESS;
    }
    return(status);
}

ReturnStatus
MachUNIXFSync(fd)
    int fd;
{
    return(Fs_FileWriteBackStub(fd, -1, -1, 1));
}

/*ARGSUSED*/
ReturnStatus
MachUNIXSetPriority(which, who, prio)
    int which, who, prio;
{
    return(SUCCESS);
}

/*ARGSUSED*/
ReturnStatus
MachUNIXGetPriority(which, who, prio)
    int which, who, prio;
{
    machCurStatePtr->userState.unixRetVal = 0;
    return(SUCCESS);
}

ReturnStatus
MachUNIXGetTimeOfDay(tp, tzpPtr)
    struct timeval 	*tp;
    struct timezone 	*tzpPtr;
{
    struct timezone	tzp;
    int			localOffset;
    Boolean		DST;	
    ReturnStatus	status;
    Time		curTime;

    Timer_GetRealTimeOfDay(&curTime, &localOffset, &DST);

    status = Vm_CopyOut(sizeof(Time), (Address)&curTime, (Address)tp);
    if (status != SUCCESS) {
	return(status);
    }

    if (tzpPtr != (struct timezone *) NULL) {
	tzp.tz_minuteswest 	= -localOffset;
	tzp.tz_dsttime 		= DST;
	return(Vm_CopyOut(sizeof(struct timezone), (Address)&tzp,
			  (Address)tzpPtr));
    } else {
	return(SUCCESS);
    }
}



ReturnStatus
MachUNIXSetTimeOfDay(tp, tzpPtr)
    struct timeval *tp;
    struct timezone *tzpPtr;
{
    ReturnStatus status;	/* result returned by Sys_SetTimeOfDay */
    Time curTime;
    struct timezone tzp;


    /*
     * Unix negates the local offset from UTC to make it positive
     * for locations west of the prime meridian. 
     */

    if (tzpPtr == NULL) {
	int localOffset;
	Boolean DST;

	Timer_GetRealTimeOfDay(&curTime, &localOffset, &DST);
	status = Vm_CopyIn(sizeof(Time), (Address)tp, (Address)&curTime);
	if (status != SUCCESS) {
	    return(status);
	}
	Timer_SetTimeOfDay(curTime, localOffset, DST);
    } else if (tp == NULL) {
	Timer_GetRealTimeOfDay(&curTime, (int *) NULL, (Boolean *) NULL);
	status = Vm_CopyIn(sizeof(tzp), (Address)tzpPtr, (Address)&tzp);
	if (status != SUCCESS) {
	    return(status);
	}
	Timer_SetTimeOfDay(curTime, -(tzp.tz_minuteswest), tzp.tz_dsttime);
    } else {
	status = Vm_CopyIn(sizeof(Time), (Address)tp, (Address)&curTime);
	if (status != SUCCESS) {
	    return(status);
	}
	status = Vm_CopyIn(sizeof(tzp), (Address)tzpPtr, (Address)&tzp);
	if (status != SUCCESS) {
	    return(status);
	}
	Timer_SetTimeOfDay(curTime, -(tzp.tz_minuteswest), tzp.tz_dsttime);
    }
    return(SUCCESS);
}

/*ARGSUSED*/
ReturnStatus
MachUNIXGetDirEntries(fd, buf, nbytes, basep)
    int  fd;
    char *buf;
    int nbytes;
    long *basep;
{
    ReturnStatus status;	/* result returned by Fs_Read */
    Address	usp;
    int		bytesAcc;
    Fslcl_DirEntry	*dirPtr;
    Address	addr;
    int		i;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Fs_ReadStub(fd, nbytes, buf, (int *)usp);
    if (status == SUCCESS) {
	(void)Vm_CopyIn(sizeof(int), usp,
		        (Address)&machCurStatePtr->userState.unixRetVal);
	if (machCurStatePtr->userState.unixRetVal == 0) {
	    return(SUCCESS);
	}
    } else {
	return(status);
    }
    Vm_MakeAccessible(VM_OVERWRITE_ACCESS, 
		machCurStatePtr->userState.unixRetVal, buf, &bytesAcc, &addr);
    if (bytesAcc != machCurStatePtr->userState.unixRetVal) {
	panic("User buffer not accessible, but we just wrote to it !!!\n");
    }
    /*
     * Check against big-endian/little-endian conflict.
     * The max record length is 512, which is 02 when byteswapped.
     * The min record length is 8, which is > 512 when byteswapped.
     * All other values fall outside the range 8-512 when byteswapped.
     */
    dirPtr = (Fslcl_DirEntry *)addr;
    if (dirPtr->recordLength > FSLCL_DIR_BLOCK_SIZE ||
	dirPtr->recordLength < 2 * sizeof(int)) {
	i = bytesAcc;
	while (i > 0) {
	    union {
		short	s;
		char    c[2];
	    } shortIn, shortOut;
	    union {
		int	i;
		char    c[4];
	    } intIn, intOut;

	    if (dirPtr->nameLength <= FS_MAX_NAME_LENGTH) {
		printf("MachUNIXGetDirEntries: Bad directory format\n");
	    }
	    intIn.i = dirPtr->fileNumber;
	    intOut.c[0] = intIn.c[3];
	    intOut.c[1] = intIn.c[2];
	    intOut.c[2] = intIn.c[1];
	    intOut.c[3] = intIn.c[0];
	    dirPtr->fileNumber = intOut.i;

	    shortIn.s = dirPtr->recordLength;
	    shortOut.c[0] = shortIn.c[1];
	    shortOut.c[1] = shortIn.c[0];
	    dirPtr->recordLength = shortOut.s;

	    shortIn.s = dirPtr->nameLength;
	    shortOut.c[0] = shortIn.c[1];
	    shortOut.c[1] = shortIn.c[0];
	    dirPtr->nameLength = shortOut.s;

	    i -= dirPtr->recordLength;
	    dirPtr = (Fslcl_DirEntry *) ((Address)dirPtr + dirPtr->recordLength);
	}
    }
    Vm_MakeUnaccessible(addr, bytesAcc);
    return(SUCCESS);
}

#define COPYTIME(TO,FROM) { \
	    (TO).tv_sec = (FROM).seconds; \
	    (TO).tv_usec = (FROM).microseconds; \
	  }

ReturnStatus
MachUNIXGetRUsage(who, rusage)
     int who;
     struct rusage *rusage;
{
    ReturnStatus status;	    /* result returned by Proc_GetResUsage */
    Proc_ResUsage spriteUsage; 	    /* sprite resource usage buffer */
    struct rusage unixUsage;

    Address	usp;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 
		    sizeof(Proc_ResUsage));
    status = Proc_GetResUsage(PROC_MY_PID, (Proc_ResUsage *)usp);
    if (status != SUCCESS) {
	return(status);
    } else {
	(void)Vm_CopyIn(sizeof(Proc_ResUsage), usp, (Address)&spriteUsage);
	if (who == RUSAGE_SELF) {
	    COPYTIME(unixUsage.ru_utime, spriteUsage.userCpuUsage);
	    COPYTIME(unixUsage.ru_stime, spriteUsage.kernelCpuUsage);
	} else {
	    COPYTIME(unixUsage.ru_utime, spriteUsage.childUserCpuUsage);
	    COPYTIME(unixUsage.ru_stime, spriteUsage.childKernelCpuUsage);
	}
	unixUsage.ru_maxrss = 0;
	unixUsage.ru_ixrss = 0;	/* integral shared memory size */
	unixUsage.ru_idrss = 0;	/* integral unshared data size */
	unixUsage.ru_isrss = 0;	/* integral unshared stack size */
	unixUsage.ru_minflt = 0;	/* page reclaims */
	unixUsage.ru_majflt = 0;	/* page faults */
	unixUsage.ru_nswap = 0;	/* swaps */
	unixUsage.ru_inblock = 0;	/* block input operations */
	unixUsage.ru_oublock = 0;	/* block output operations */
	unixUsage.ru_msgsnd = 0;	/* messages sent */
	unixUsage.ru_msgrcv = 0;	/* messages received */
	unixUsage.ru_nsignals = 0;	/* signals received */
	unixUsage.ru_nvcsw =
	        spriteUsage.numWaitEvents;  /* voluntary context switches */
	unixUsage.ru_nivcsw =
	        spriteUsage.numQuantumEnds;  /* involuntary context switches */

	return(Vm_CopyOut(sizeof(unixUsage), (Address)&unixUsage, 
			  (Address)rusage));
    }
}

ReturnStatus
MachUNIXReadv(stream, iov, iovcnt)
    int stream;			/* descriptor for stream to read. */
    register struct iovec *iov;	/* pointer to array of iovecs. */
    int iovcnt;			/* number of  iovecs in iov. */
{
    ReturnStatus status;	/* result returned by Fs_Read */
    int amountRead;		/* place to hold number of bytes read */
    int totalRead = 0;		/* place to hold total # of bytes read */
    int i;
    Address	usp;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 4);

    for (i=0; i < iovcnt; i++, iov++) {
	status = Fs_ReadStub(stream, iov->iov_len, iov->iov_base, (int *)usp);
	if (status != SUCCESS) {
	    break;
	} else {
	    (void)Vm_CopyIn(sizeof(int), usp, (Address)&amountRead);
	    totalRead += amountRead;
	}
    }

    /* 
     * If we get an error after reading in something, that still 
     * counts as success.
     */

    if (totalRead > 0) {
	status = SUCCESS;
    }
    if (status == SUCCESS) {
	machCurStatePtr->userState.unixRetVal = totalRead;
    }
    return(status);
}

ReturnStatus
MachUNIXWritev(stream, iov, iovcnt)
    int stream;			/* descriptor for stream to read. */
    register struct iovec *iov;	/* pointer to array of iovecs. */
    int iovcnt;			/* number of  iovecs in iov. */
{
    ReturnStatus status;	/* result returned by Fs_Read */
    int amountWritten;		/* place to hold number of bytes read */
    int totalWritten = 0;	/* place to hold total # of bytes written */
    int i;
    Address	usp;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 4);

    for (i=0; i < iovcnt; i++, iov++) {
	status = Fs_WriteStub(stream, iov->iov_len, iov->iov_base, (int *)usp);
	if (status != SUCCESS) {
	    break;
	} else {
	    (void)Vm_CopyIn(sizeof(int), usp, (Address)&amountWritten);
	    totalWritten += amountWritten;
	}
    }

    /* 
     * If we get an error after writing out something, that still 
     * counts as success.
     */

    if (totalWritten > 0) {
	status = SUCCESS;
    }
    if (status == SUCCESS) {
	machCurStatePtr->userState.unixRetVal = totalWritten;
    }
    return(status);
}

ReturnStatus
MachUNIXSetREUID(ruid, euid)
    int	ruid, euid;
{
    if (ruid == -1) {
	ruid = PROC_NO_ID;
    }
    if (euid == -1) {
	euid = PROC_NO_ID;
    }
    return(Proc_SetIDs(ruid, euid));
}

ReturnStatus
MachUNIXSetREGID(rgid, egid)
    int	rgid, egid;
{
    Address	usp;
    int		array[2];
    int		num;
    ReturnStatus	status;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 8);
    if (rgid != -1) {
	array[0] = rgid;
	num = 1;
	if (egid != rgid && egid != -1) {
	    array[1] = egid;
	    num++;
	}
    } else if (egid != -1) {
	array[0] = egid;
	num++;
    }
    if (num > 0) {
	status = Vm_CopyOut(2 * sizeof(int), (Address)array, usp);
	if (status != SUCCESS) {
	    return(status);
	}
	return(Proc_SetGroupIDs(num, (int *)usp));
    }
    return(SUCCESS);
}

ReturnStatus
MachUNIXRename(from, to)
    char *from;
    char *to;
{
    return(Fs_RenameStub(from, to));
}

ReturnStatus
MachUNIXTruncate(path, length)
    char *path;
    unsigned long length;
{
    Address	usp;
    int		*intPtr;
    int		streamID;
    ReturnStatus status;

    usp = (Address)machCurStatePtr->userState.regState.regs[SP];
    intPtr = (int *)(usp - 4);
    status = Fs_OpenStub(path, FS_WRITE, 0, intPtr);
    if (status != SUCCESS) {
	return(status);
    }
    (void)Vm_CopyIn(sizeof(streamID), (Address)intPtr, (Address)&streamID);
    (void)Vm_CopyOut(sizeof(int), (Address)&length, (Address)intPtr);
    status = Fs_IOControlStub(streamID, IOC_TRUNCATE, sizeof(int),
			      (Address)intPtr, 0, (Address)NULL);
    (void)Fs_UserClose(streamID);
    return(status);
}

ReturnStatus
MachUNIXFTruncate(fd, length)
    int fd;
    unsigned long length;
{
    Address	usp;
    ReturnStatus status;

    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Vm_CopyOut(sizeof(int), (Address)&length, usp);
    if (status != SUCCESS) {
	return(status);
    }
    return(Fs_IOControlStub(fd, IOC_TRUNCATE, sizeof(int),
			    usp, 0, (Address)NULL));
}

#ifndef lint
ReturnStatus
MachUNIXLongJumpReturn(sigContextPtr)
    struct sigcontext *sigContextPtr;
{
    struct sigcontext	sigContext;
    Mach_RegState	*regsPtr;
    int			dummy;
    ReturnStatus	status;

    status = Vm_CopyIn(sizeof(struct sigcontext), (Address)sigContextPtr,
		       (Address)&sigContext);
    if (status != SUCCESS) {
	return(status);
    }
    regsPtr = &machCurStatePtr->userState.regState;
    regsPtr->pc = (Address)(sigContext.sc_pc - 4);
    bcopy(sigContext.sc_regs, regsPtr->regs, sizeof(sigContext.sc_regs));
    regsPtr->mflo = sigContext.sc_mdlo;
    regsPtr->mfhi = sigContext.sc_mdhi;
    bcopy(sigContext.sc_fpregs, regsPtr->fpRegs, sizeof(sigContext.sc_fpregs));
    regsPtr->fpStatusReg = sigContext.sc_fpc_csr;
    (void) MachUNIXBlock(&dummy, sigContext.sc_mask);
    return(SUCCESS);
}
#endif

ReturnStatus
MachUNIXFLock(descriptor, operation)
    int	descriptor;
    int operation;
{
    ReturnStatus status;
    int spriteLockOp = 0;
    Ioc_LockArgs args;
    Address usp;

    if (operation & LOCK_EX) {
	spriteLockOp |= IOC_LOCK_EXCLUSIVE;
    } else if (operation & LOCK_SH) {
	spriteLockOp |= IOC_LOCK_SHARED;
    }
    if (operation & LOCK_NB) {
	spriteLockOp |= IOC_LOCK_NO_BLOCK;
    }
    args.flags = spriteLockOp;
    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 
		     sizeof(Ioc_LockArgs));
    status = Vm_CopyOut(sizeof(Ioc_LockArgs), (Address)&args, usp);
    if (status != SUCCESS) {
	return(status);
    }
    if (operation & LOCK_UN) {
	return(Fs_IOControlStub(descriptor, IOC_UNLOCK, sizeof(Ioc_LockArgs),
				usp, 0, (Address)NULL));
    } else {
	return(Fs_IOControlStub(descriptor, IOC_LOCK, sizeof(Ioc_LockArgs),
				usp, 0, (Address)NULL));
    }
}

ReturnStatus
MachUNIXMkDir(pathName, permissions)
    char *pathName;		/* The name of the directory to create */
    int permissions;		/* Permission mask to use on creation */
{
    return(Fs_MakeDirStub(pathName, permissions));
}


ReturnStatus
MachUNIXRMDir(pathName)
    char *pathName;		/* The name of the directory to create */
{
    return(Fs_RemoveDirStub(pathName));
}

ReturnStatus
MachUNIXUTimes(path, tvp)
    char	*path;
    struct timeval tvp[2];
{
    Address		usp;
    ReturnStatus	status;
    Fs_Attributes attributes;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 
		     sizeof(Fs_Attributes));
    attributes.accessTime.seconds = tvp[0].tv_sec;
    attributes.accessTime.microseconds = tvp[0].tv_usec;
    attributes.dataModifyTime.seconds = tvp[1].tv_sec;
    attributes.dataModifyTime.microseconds = tvp[1].tv_usec;
    status = Vm_CopyOut(sizeof(Fs_Attributes), (Address)&attributes, usp);
    if (status != SUCCESS) {
	return(status);
    }
    return(Fs_SetAttrStub(path, FS_ATTRIB_FILE, (Fs_Attributes *)usp,
			  FS_SET_TIMES));
}

ReturnStatus
MachUNIXKillPG(pgrp, sig)
    int pgrp;
    int sig;
{
    ReturnStatus status;
    int		 spriteSignal;

    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || (spriteSignal == NULL && sig != 0)) {
	return(SYS_INVALID_ARG);
    }
    return(Sig_UserSend(spriteSignal, pgrp, TRUE));
}

ReturnStatus
MachUNIXGetRLimit()
{
    return(SUCCESS);
}

ReturnStatus
MachUNIXSetRLimit()
{
    return(SUCCESS);
}

#define MAX_HOST_NAME_LEN 255

char machHostName[MAX_HOST_NAME_LEN + 1];
int machHostNameLen = 0;
char machDomainName[MAX_HOST_NAME_LEN + 1];
int machDomainNameLen = 0;
int machHostID;

ReturnStatus
MachUNIXGetHostName(name, namelen)
    char *name;
    int namelen;
{
    int copyLen;

    if (namelen < machHostNameLen + 1) {
	copyLen = namelen;
    } else {
	copyLen = machHostNameLen + 1;
    }
    return(Vm_CopyOut(copyLen, machHostName, name));
}

ReturnStatus
MachUNIXSetHostName(name, namelen)
    char *name;
    int namelen;
{
    ReturnStatus	status;

    if (namelen > MAX_HOST_NAME_LEN) {
	return(SYS_INVALID_ARG);
    }
    status = Vm_CopyIn(namelen, name, machHostName);
    if (status != SUCCESS) {
	machHostNameLen = 0;
	return(status);
    }
    machHostName[namelen] = 0;
    machHostNameLen = namelen;
    return(SUCCESS);
}

ReturnStatus
MachUNIXGetHostID()
{
    machCurStatePtr->userState.unixRetVal = machHostID;
    return(SUCCESS);
}

ReturnStatus
MachUNIXSetHostID(hostid)
    int hostid;
{
    machHostID = hostid;
    return(SUCCESS);
}

ReturnStatus
MachUNIXGetDomainName(name, namelen)
    char *name;
    int namelen;
{
    int copyLen;

    if (namelen < machDomainNameLen + 1) {
	copyLen = namelen;
    } else {
	copyLen = machDomainNameLen + 1;
    }
    return(Vm_CopyOut(copyLen, machDomainName, name));
}

ReturnStatus
MachUNIXSetDomainName(name, namelen)
    char *name;
    int namelen;
{
    ReturnStatus	status;

    if (namelen > MAX_HOST_NAME_LEN) {
	return(SYS_INVALID_ARG);
    }
    status = Vm_CopyIn(namelen, name, machDomainName);
    if (status != SUCCESS) {
	machDomainNameLen = 0;
	return(status);
    }
    machDomainName[namelen] = 0;
    machDomainNameLen = namelen;
    return(SUCCESS);
}

ReturnStatus
MachUNIXGetItimer(which, value)
    int which;
    struct itimerval *value;
{
    return(Proc_GetIntervalTimer(which, (Proc_TimerInterval *) value));
}

ReturnStatus
MachUNIXSetITimer(which, value, ovalue)
    int which;
    struct itimerval *value;
    struct itimerval *ovalue;
{
    return(Proc_SetIntervalTimer(which, (Proc_TimerInterval *) value,
				 (Proc_TimerInterval *) ovalue));
}

/*ARGSUSED*/
ReturnStatus
MachUNIXGetSysInfo(op, buffer, nbytes, start, arg)
    unsigned  op;
    char      *buffer;
    unsigned   nbytes;
    int       *start;
    char       *arg;
{
#ifdef actually_do_it
    switch (op) {
	case GSI_PROG_ENV:
	case GSI_MAX_UPROCS:
	case GSI_TTYP:
	case GSI_NETBLK:
	case GSI_BOOTDEV:
	case GSI_UACSYS:
	case GSI_UACPARNT:
	case GSI_UACPROC:
	default:
    }
#endif
    /*
     * Just return a 0.  This says that the requested information is
     * not available which is certainly true for the most part.
     */
    machCurStatePtr->userState.unixRetVal = 0;
    return(SUCCESS);
}

ReturnStatus
MachUNIXSemctl(semid, semnum, cmd, arg)
int semid, semnum, cmd;
union semun arg;
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Sync_SemctlStub(semid, semnum, cmd, arg, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXSemop(semid, sops, nsops)
int semid, nsops;
struct sembuf *sops;
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Sync_SemopStub(semid, sops, nsops, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXSemget(key, nsems, semflg)
key_t key;
int nsems, semflg;
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Sync_SemgetStub(key, nsems, semflg, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXMmap(addr, len, prot, share, fd, pos)
caddr_t	addr;
int	len, prot, share, fd;
off_t	pos;
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Vm_Mmap(addr, len, prot, share, fd, pos, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXMunmap(addr, len)
caddr_t	addr;
int	len;
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Vm_Munmap(addr, len, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

ReturnStatus
MachUNIXMincore(addr, len, vec)
caddr_t	addr;
int len;
char vec[];
{
    ReturnStatus	status;
    Address		usp;

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Vm_Mincore(addr, len, vec, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->userState.unixRetVal);
    }
    return(status);
}

/************************ Begin Unimplemented calls **********************/

/*ARGSUSED*/
ReturnStatus
MachUNIXSocketPair(d, type, protocol, sv)
    int d, type, protocol;
    int sv[2];
{
    printf("socketpair is not implemented\n");
    return(FAILURE);
}

/*ARGSUSED*/
ReturnStatus
MachUNIXReboot(howto)
    int howto;
{
    printf("reboot is not implemented\n");
    return(FAILURE);
}

ReturnStatus
MachUNIXSync()
{
    printf("sync is not implemented\n");
    return(FAILURE);
}

/*ARGSUSED*/
ReturnStatus
MachUNIXPtrace(request, pid, addr, data)
    int request, pid, *addr, data;
{
    printf("ptrace is not implemented\n");
    return(FAILURE);
}

ReturnStatus
MachUNIXGetDOpt()
{
    printf("getdopt is not implemented\n");
    return(FAILURE);
}

ReturnStatus
MachUNIXSetDOpt()
{
    printf("setdopt is not implemented\n");
    return(FAILURE);
}


/*ARGSUSED*/
ReturnStatus
MachUNIXAdjTime(delta, olddelta)
    struct timeval	*delta;
    struct timeval	*olddelta;
{
    printf("adjtime is not implemented\n");
    return(FAILURE);
}

ReturnStatus
MachUNIXError()
{
    printf("MachUNIXError: %s is not implemented\n", 
	    machUNIXSysCallTable[machCurStatePtr->userState.regState.regs[V0]].name);
    return(FAILURE);
}

ReturnStatus
MachUNIXProfil(buffer, bufsize, offset, scale)
    char *buffer;
    int bufsize;
    int offset;
    int scale;
{

    Prof_Profil(buffer, bufsize, offset, scale);
    return SUCCESS;
}

