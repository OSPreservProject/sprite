/* 
 * fsStubs.c --
 *
 *	Stubs for Unix compatible system calls.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */

#define MACH_UNIX_COMPAT

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <status.h>
#include <errno.h>
#include <assert.h>
#include <fsprefix.h>
#include <user/sys/types.h>
#include <user/sys/wait.h>
#include <user/sys/time.h>
#include <user/sys/resource.h>
#include <user/sys/file.h>
#include <user/sys/stat.h>
#include <user/sys/uio.h>
#include <user/sys/ioctl.h>
#include <user/sys/termio.h>
#include <user/sys/termios.h>
#include <user/sys/socket.h>
#include <user/sys/dir.h>
#include <user/sys/dirent.h>
#include <user/dev/tty.h>
#include <user/dev/net.h>
#include <user/dev/graphics.h>
#include <user/net/if.h>
#include <user/sys/termios.h>
#include <user/fcntl.h>
#include <mach.h>
#include <proc.h>
#include <vm.h>
#include <fs.h>
#include <fsutil.h>
#include <fsutilTrace.h>
#include <fsio.h>
#include <fslcl.h>

char *errs[] = {"ENOERR", "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO",
	"ENXIO", "E2BIG", "ENOEXEC", "EBADF", "ECHILD", "EAGAIN", "ENOMEM",
	"EACCES", "EFAULT", "ENOTBLK", "EBUSY", "EEXIST", "EXDEV", "ENODEV",
	"ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", "ENOTTY",
	"ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE", "EROFS", "EMLINK", "EPIPE",
	"EDOM", "ERANGE", "EWOULDBLOCK", "EINPROGRESS", "EALREADY", "ENOTSOCK",
	"EDESTADDRREQ", "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT",
	"EPROTONOSUPPORT", "ESOCKTNOSUPPORT", "EOPNOTSUPP", "EPFNOSUPPORT",
	"EAFNOSUPPORT", "EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN",
	"ENETUNREACH", "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS",
	"EISCONN", "ENOTCONN", "ESHUTDOWN", "ETIMEDOUT", "ECONNREFUSED",
	"ELOOP", "ENAMETOOLONG", "EHOSTDOWN", "EHOSTUNREACH", "ENOTEMPTY",
	"EPROCLIM", "EUSERS", "EDQUOT", "ESTALE", "EREMOTE"};

#undef Mach_SetErrno
#define Mach_SetErrno(err) printf("Error %d (%s) at %d in %s\n", err,\
	err<sizeof(errs)/sizeof(char *)?errs[err]:"",\
	__LINE__, __FILE__); Proc_GetActualProc()->unixErrno = (err)

/*
 * The following defines are flags that are defined differently in
 * termios.h (T_) and ioctl.h (I_).
 * We define them uniquely here, so we don't get messed up.
 */
#define T_NL1	0x00000100
#define I_NL1	0x00000100
#define I_NL2	0x00000200
#define I_CR0	0x00000000
#define T_CR1	0x00000200
#define I_CR1	0x00001000
#define T_CR2	0x00000400
#define I_CR2	0x00002000
#define T_CR3	0x00000600
#define I_CR3	0x00003000
#define T_TAB1	0x00000800
#define I_TAB1	0x00000400
#define T_TAB2	0x00001000
#define I_TAB2	0x00000800
#define T_NOFLSH	0x00000080
#define I_NOFLSH	0x80000000
#define T_TOSTOP	0x00000100
#define I_TOSTOP	0x00400000
#define T_FLUSHO	0x00002000
#define I_FLUSHO	0x00800000

#ifndef Mach_SetErrno
#define Mach_SetErrno(err)
#endif

int debugFsStubs;

/*
 *----------------------------------------------------------------------
 *
 * CvtSpriteToUnixType --
 *
 *	Convert from Sprite file types to the IFMT mode bits of a unix file
 *
 * Results:
 *	Unix file type bits.
 *
 * Side effects:
 *	 none.
 *
 *----------------------------------------------------------------------
 */
static int
CvtSpriteToUnixType(spriteFileType)
    register	int spriteFileType;
{
    register unixModeBits;

    switch (spriteFileType) {
	default:
	case FS_FILE:
	    unixModeBits = S_IFREG;
	    break;
	case FS_DIRECTORY:
	    unixModeBits = S_IFDIR;
	    break;
	case FS_SYMBOLIC_LINK:
	    unixModeBits = S_IFLNK;
	    break;
	case FS_DEVICE:
	case FS_REMOTE_DEVICE:		/* not used */
	    unixModeBits = S_IFCHR;
	    break;
	case FS_LOCAL_PIPE:		/* not used */
	case FS_NAMED_PIPE:
	    unixModeBits = S_IFIFO;
	    break;
	case FS_REMOTE_LINK:
	    unixModeBits = S_IFRLNK;
	    break;
	case FS_PSEUDO_DEV:
	    unixModeBits = S_IFPDEV;
	    break;
    }
    return(unixModeBits);
}

/*
 *----------------------------------------------------------------------
 *
 * CvtSpriteToUnixAtts --
 *
 *	Procedure to convert the Sprite file system attributes 
 *	structure to the Unix format.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	 none.
 *
 *----------------------------------------------------------------------
 */
static void
CvtSpriteToUnixAtts(spriteAttsPtr, unixAttsPtr)
    register	struct stat	*unixAttsPtr;
    register	Fs_Attributes	*spriteAttsPtr;
{
    unixAttsPtr->st_dev		= spriteAttsPtr->domain;
    unixAttsPtr->st_ino		= spriteAttsPtr->fileNumber;
    unixAttsPtr->st_mode	= (spriteAttsPtr->permissions & 0xfff) |
				    CvtSpriteToUnixType(spriteAttsPtr->type);
    unixAttsPtr->st_nlink	= spriteAttsPtr->numLinks;
    unixAttsPtr->st_uid		= spriteAttsPtr->uid;
    unixAttsPtr->st_gid		= spriteAttsPtr->gid;
    unixAttsPtr->st_rdev	= (spriteAttsPtr->devType << 8) |
				  (spriteAttsPtr->devUnit & 0xff);
    unixAttsPtr->st_size	= spriteAttsPtr->size;
    unixAttsPtr->st_blksize	= spriteAttsPtr->blockSize;
    unixAttsPtr->st_blocks	= spriteAttsPtr->blocks * 2;
    unixAttsPtr->st_atime	= spriteAttsPtr->accessTime.seconds;
    unixAttsPtr->st_spare1	= 0;
    unixAttsPtr->st_mtime	= spriteAttsPtr->dataModifyTime.seconds;
    unixAttsPtr->st_spare2	= 0;
    unixAttsPtr->st_ctime	= spriteAttsPtr->descModifyTime.seconds;
    unixAttsPtr->st_spare3	= 0;
    /*
     * The Unix stat structure is 8 bytes shorter than we declare it in
     * sys/stat.h.  The Unix stat structure has 2 unused words on the
     * end.
     */
    unixAttsPtr->st_serverID	= 0;
    unixAttsPtr->st_version	= 0;
    /*
    unixAttsPtr->st_serverID	= spriteAttsPtr->serverID;
    unixAttsPtr->st_version	= spriteAttsPtr->version;
    unixAttsPtr->st_userType	= spriteAttsPtr->userType;
    unixAttsPtr->st_devServerID = spriteAttsPtr->devServerID;
    */
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * CvtTermiosToSgttyb --
 *
 *	Convert from termios structure to sgttyb structure.
 *	Note: ttyb and lmode must be initialized on entry.
 *	This routine is carefully set up to match the behavior
 *	of SunOS for all input combinations.  As a result, this
 *	routine is not intuitive.  It's probably best not to
 *	change this routine unless you have a good reason.
 *	Then again, there's a good chance I got something wrong.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes ttyb, lmode, tc, ltc.
 *
 *----------------------------------------------------------------------
 */
static void
CvtTermiosToSgttyb(termiosFlag, ts, ttyb, lmode, tc, ltc)
    int termiosFlag;		/* 1 if termios, 0 if termio. */
    struct termios *ts;		/* In */
    struct sgttyb *ttyb;	/* In, out */
    unsigned int *lmode;	/* In, out */
    struct tchars *tc;		/* In, out */
    struct ltchars *ltc;	/* In, out */
{
    unsigned int flag, iflag, oflag, lflag, cflag;

    iflag = ts->c_iflag;
    oflag = ts->c_oflag;
    lflag = ts->c_lflag;
    cflag = ts->c_cflag;

    flag = EVENP|ODDP|CBREAK;

    if (!(cflag&PARENB)&&(cflag&CSIZE)==CS8) {
	flag &= ~(ODDP|EVENP|PASS8);
	flag |= ttyb->sg_flags & (ODDP|EVENP);
	if (*lmode&(PASS8>>16)) flag |= PASS8;
	if (!(oflag&OPOST)) flag |= LITOUT;
	if (!(iflag&ISTRIP)&&(oflag&OPOST)) flag |= PASS8;
    } else {
	if (cflag&PARODD) flag &= ~EVENP;
	if ((iflag&INPCK)&&!(cflag&PARODD)) flag &= ~ODDP;
    }

    if (!(iflag&IXANY)) flag |= DECCTQ;
    if (iflag&IXOFF) flag |= TANDEM;


    if (oflag&OLCUC) flag |= LCASE;
    if (oflag&ONLCR) {
	flag |= CRMOD;
	if ((oflag&CRDLY)==T_CR2) {
	    flag |= I_CR1;
	} else if ((oflag&CRDLY)==T_CR3) {
	    flag |= I_CR2;
	}
    }
    if (oflag&ONLRET) {
	if ((oflag&NLDLY)==T_NL1) flag |= I_NL2;
	if (!(oflag&ONLCR) && (oflag&T_CR1)) {
	    flag |= I_NL1;
	}
    }
    if (oflag&T_TAB1) flag |= I_TAB1;
    if (oflag&T_TAB2) flag |= I_TAB2;
    if (oflag&BSDLY) flag |= BSDELAY;
    if (oflag&FFDLY) flag |= VTDELAY;

    if (cflag&CLOCAL) flag |= NOHANG;

    if (lflag&ICANON) flag &= ~CBREAK;
    if (lflag&ECHOE) flag |= CRTERA|CRTBS;
    if (lflag&T_NOFLSH) flag |= I_NOFLSH;
    if (lflag&T_TOSTOP) flag |= I_TOSTOP;
    if (lflag&ECHOCTL) flag |= CTLECH;
    if (lflag&ECHOPRT) flag |= PRTERA;
    if (lflag&ECHOKE) flag |= CRTKIL;
    if (lflag&ECHO) flag |= ECHO;
    if (lflag&T_FLUSHO) flag |= I_FLUSHO;

    ttyb->sg_flags = flag&0xffff;
    *lmode = flag>>16;
    ttyb->sg_ispeed = cflag&CBAUD;
    ttyb->sg_ospeed = cflag&CBAUD;
    tc->t_intrc = ts->c_cc[VINTR];
    tc->t_quitc = ts->c_cc[VQUIT];
    ttyb->sg_erase = ts->c_cc[VERASE];
    ttyb->sg_kill = ts->c_cc[VKILL];
    tc->t_eofc = ts->c_cc[VEOF];
    if (termiosFlag) {
	tc->t_startc = ts->c_cc[VSTART];
	tc->t_stopc = ts->c_cc[VSTOP];
	ltc->t_suspc = ts->c_cc[VSUSP];
	ltc->t_rprntc = ts->c_cc[VREPRINT];
	ltc->t_flushc = ts->c_cc[VDISCARD];
	ltc->t_werasc = ts->c_cc[VWERASE];
	ltc->t_lnextc = ts->c_cc[VLNEXT];
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CvtSgttybToTermios --
 *
 *	Convert from termios structure to sgttyb structure.
 *	Note: ttyb and lmode must be initialized on entry.
 *
 * Results:
 *	Changes ts.
 *
 * Side effects:
 *	 
 *
 *----------------------------------------------------------------------
 */
static void
CvtSgttybToTermios(ttyb, lmode, tc, ltc, ts)
    struct sgttyb *ttyb;
    unsigned int *lmode;
    struct tchars *tc;
    struct ltchars *ltc;
    struct termios *ts;
{
    unsigned int flag;
    unsigned int iflag, oflag, cflag, lflag;

    flag = ttyb->sg_flags | (*lmode<<16);
    iflag = IMAXBEL|IXON|ISTRIP|IGNPAR|BRKINT|IXANY;
    oflag = ONLRET|OPOST;
    cflag = CREAD|CS8|B9600;
    lflag = ICANON|ISIG|IEXTEN|ECHOK;
    if (flag&CBREAK) lflag &= ~ICANON;
    if (flag&TANDEM) iflag |= IXOFF;
    if (flag&LCASE) {
	iflag |= IUCLC;
	oflag |= OLCUC;
	lflag |= XCASE;
    }
    if (flag&ECHO) lflag |= ECHO;
    if (flag&EVENP) {
	iflag |= INPCK;
	cflag |= PARENB;
	cflag &= ~CS6;
	if (flag&ODDP) {
	    iflag &= ~INPCK;
	}
    } else if (flag&ODDP) {
	iflag |= INPCK;
	cflag |= PARODD|PARENB;
	cflag &= ~CS6;
    }
    if ((flag&NLDELAY)==I_NL2) {
	oflag |= T_NL1;
    }
    if (flag&CRMOD) {
	iflag |= ICRNL;
	oflag |= ONLCR;
	oflag &= ~CRDLY;
    } else {
	oflag &= ~CRDLY;
    }
    if (flag&I_TAB1) oflag |= T_TAB1;
    if (flag&I_TAB2) oflag |= T_TAB2;
    if ((flag&CRDELAY)==I_CR0 || (flag&CRDELAY)==I_CR3) {
	if (!(flag&CRMOD) && (flag&NLDELAY)==I_NL1) {
	    oflag |= T_CR1;
	}
    } else if ((flag&CRDELAY)==I_CR1) {
	if (flag&CRMOD) {
	    oflag |= T_CR2;
	} else if ((flag&NLDELAY)==I_NL1) {
	    oflag |= T_CR1;
	}
    } else if ((flag&CRDELAY)==I_CR2) {
	if (flag&CRMOD) {
	    oflag |= T_CR3;
	} else if ((flag&NLDELAY)==I_NL1) {
	    oflag |= T_CR1;
	}
    }
    if (flag&VTDELAY) oflag |= FFDLY;
    if (flag&BSDELAY) oflag |= BSDLY;
    if (flag&PRTERA) lflag |= ECHOPRT;
    if (flag&CRTERA) lflag |= ECHOE;
    if (flag&I_TOSTOP) lflag |= TOSTOP;
    if (flag&I_FLUSHO) lflag |= FLUSHO;
    if (flag&NOHANG) cflag |= CLOCAL;
    if (flag&CRTKIL) lflag |= ECHOKE;
    if (flag&CTLECH) lflag |= ECHOCTL;
    if (flag&DECCTQ) iflag &= ~IXANY;
    if (flag&I_NOFLSH) lflag |= T_NOFLSH;
    if (flag&LITOUT) {
	iflag &= ~(ISTRIP|INPCK);
	oflag &= ~OPOST;
	cflag &= CLOCAL;
	cflag |= CREAD|CS8|B9600;
    }
    if (flag&PASS8) {
	iflag &= ~(ISTRIP|INPCK);
	cflag &= CLOCAL;
	cflag |= CREAD|CS8|B9600;
    }
    if (flag&RAW) {
	iflag &= IXOFF|IXANY;
	oflag &= ~OPOST;
	oflag |= ONLRET;
	lflag &= ~(IEXTEN|ISIG|ICANON|ECHOCTL|XCASE);
	cflag &= ~(PARODD|PARENB);
	cflag |= CREAD|CS8|B9600;
	if ((flag&BSDELAY) && ((flag&CRDELAY==I_CR1))) {
	    oflag |= T_CR2;
	}
    }
    cflag |= ttyb->sg_ospeed&CBAUD;
    ts->c_iflag = iflag;
    ts->c_oflag = oflag;
    ts->c_cflag = cflag;
    ts->c_lflag = lflag;
    ts->c_line = 0;
    ts->c_cc[VINTR] = tc->t_intrc;
    ts->c_cc[VQUIT] = tc->t_quitc;
    ts->c_cc[VERASE] = ttyb->sg_erase;
    ts->c_cc[VKILL] = ttyb->sg_kill;
    ts->c_cc[VEOF] = tc->t_eofc;
    ts->c_cc[VEOL] = 0;
    ts->c_cc[VEOL2] = 0;
    ts->c_cc[VSWTCH] = 0;
    ts->c_cc[VSTART] = tc->t_startc;
    ts->c_cc[VSTOP] = tc->t_stopc;
    ts->c_cc[VSUSP] = ltc->t_suspc;
    ts->c_cc[VREPRINT] = ltc->t_rprntc;
    ts->c_cc[VDISCARD] = ltc->t_flushc;
    ts->c_cc[VWERASE] = ltc->t_werasc;
    ts->c_cc[VLNEXT] = ltc->t_lnextc;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckIfPresent --
 *
 *	Check if a prefix is in the prefix table.  This is so we don't
 *	have to do pointless broadcasts for getwd's.
 *
 * Results:
 *	SUCCESS or FAILURE
 *
 * Side effects:
 *	 none.
 *
 *----------------------------------------------------------------------
 */
extern int fsprefix_FileNameTrace;
static ReturnStatus
CheckIfPresent(name)
    char *name;
{
    ReturnStatus	status;
    Fs_HandleHeader	*hdrPtr;
    Fs_FileID		rootID;
    char		*lookupName;
    int			serverID=123;
    int			domainType;
    Fsprefix		*prefixPtr;
    Fs_RedirectInfo	*oldInfoPtr = (Fs_RedirectInfo *)NIL;
    Fs_RedirectInfo	*redirectInfoPtr = (Fs_RedirectInfo *)NIL;
    Fs_OpenArgs		openArgs;
    Fs_GetAttrResults	getAttrResults;
    Proc_ControlBlock	*procPtr = Proc_GetEffectiveProc();
    Fs_FileID		ioFileID;
    Fs_Attributes	attrs;
    int			numRedirects=0;

    /*
     * This code if from Fs_GetAttributes);
     */
    openArgs.useFlags = FS_FOLLOW;
    openArgs.permissions = 0;
    openArgs.type = FS_FILE;
    openArgs.clientID = rpc_SpriteID;
    Fs_SetIDs(procPtr, &openArgs.id);
    if (procPtr->genFlags & PROC_FOREIGN) {
	openArgs.migClientID = procPtr->peerHostID;
    } else {
	openArgs.migClientID = rpc_SpriteID;
    }
    getAttrResults.attrPtr = &attrs;
    getAttrResults.fileIDPtr = &ioFileID;

    /*
     * This loop is out of Fsprefix_LookupOperation
     */
    while (1) {
#if 1
	/*
	 * The next loop is from GetPrefix.
	 */
	do {
	    status = Fsprefix_Lookup(name, FSPREFIX_IMPORTED, FS_LOCALHOST_ID,
		    &hdrPtr, &rootID, &lookupName, &serverID, &domainType,
		    &prefixPtr);
	    if (status == FS_NO_HANDLE) {
		return FAILURE;
	    }
	} while (status == FS_NEW_PREFIX);
#else
	status = GetPrefix(name, FS_FOLLOW, &hdrPtr, &rootID, &lookupName,
		&domainType, &prefixPtr);
#endif
	{
	    register Fs_LookupArgs *lookupArgsPtr = (Fs_LookupArgs *)&openArgs;
	    lookupArgsPtr->prefixID = hdrPtr->fileID;
	    lookupArgsPtr->rootID = rootID;
	}
	status = (*fs_DomainLookup[domainType][FS_DOMAIN_GET_ATTR])
	    (hdrPtr, lookupName, &openArgs, &getAttrResults, &redirectInfoPtr);
	if (status==SUCCESS) {
	    return SUCCESS;
	} else if (status == FS_LOOKUP_REDIRECT) {
	    numRedirects++;
	    if (numRedirects > FS_MAX_LINKS) {
		printf("Loop\n");
		return FAILURE;
	    }
	    status = FsprefixLookupRedirect(redirectInfoPtr, prefixPtr,
		    &name);
	    if (oldInfoPtr != (Fs_RedirectInfo *)NIL) {
		free((Address)oldInfoPtr);
	    }
	    oldInfoPtr = redirectInfoPtr;
	    redirectInfoPtr = (Fs_RedirectInfo *)NIL;
	} else {
	    return FAILURE;
	}
    }

    return FAILURE;
}

/*
 *----------------------------------------------------------------------
 *
 * copyin --
 *
 *	Copy a string from user space to a static buffer.
 *
 * Results:
 *	The string.
 *
 * Side effects:
 *	 none.
 *
 *----------------------------------------------------------------------
 */
static char *
copyin(string)
    char *string;
{
    static char buf[FS_MAX_PATH_NAME_LENGTH + 1];
    int x;

    assert(debugFsStubs);
    Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, string, buf, &x);
    return buf;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_NewReadStub --
 *
 *	The stub for the "read" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_NewReadStub(streamID, buffer, numBytes)
    int 	streamID;	/* descriptor for stream to read */
    char	*buffer;	/* pointer to buffer area */
    int		numBytes;	/* number of bytes to read */
{
    ReturnStatus	status;
    Proc_ControlBlock	*procPtr;
    Fs_Stream		*streamPtr;
    int                 amountRead;
    int                 totalAmountRead;

    if (debugFsStubs) {
	printf("Fs_NewReadStub(%d, %d)\n", streamID, numBytes);
    }
    totalAmountRead = 0;
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    for (;;) {
	amountRead = numBytes - totalAmountRead;
	printf("Fs_Read(%x, %x, %x, %x(%d)\n", streamPtr, buffer,
		streamPtr->offset, &amountRead, amountRead);
	status = Fs_Read(streamPtr, buffer, streamPtr->offset, &amountRead);
	totalAmountRead += amountRead;
	if (status != SUCCESS) {
#if 0	    
	    if (status == GEN_ABORTED_BY_SIGNAL) {
		if (debugFsStubs) {
		    printf("Fs_Read aborted by signal\n");
		}
		assert(proc_RunningProcesses[0]->specialHandling);
		assert(procPtr->specialHandling);
		MachRecoverFromSignal();
		buffer += amountRead;
		continue;
	    }
#endif	    
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
	return totalAmountRead;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_NewWriteStub --
 *
 *	The stub for the "write" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_NewWriteStub(streamID, buffer, numBytes)
    int 	streamID;	/* descriptor for stream to read */
    char	*buffer;	/* pointer to buffer area */
    int		numBytes;	/* number of bytes to read */
{
    ReturnStatus	status;
    Fs_Stream		*streamPtr;
    int                 writeLength;
    int                 totalAmountWritten;

#if 0
    if (debugFsStubs) {
	printf("Fs_NewWriteStub(%d, %d)\n", streamID, numBytes);
    }
#endif
    totalAmountWritten = 0;
    status = Fs_GetStreamPtr(Proc_GetEffectiveProc(), streamID, &streamPtr);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    for (;;) {
	writeLength = numBytes - totalAmountWritten;
	status = Fs_Write(streamPtr, buffer, streamPtr->offset, &writeLength);
	totalAmountWritten += writeLength;
	if (status != SUCCESS) {
#if 0	    
	    if (status == GEN_ABORTED_BY_SIGNAL) {
		if (debugFsStubs) {
		    printf("write aborted by signal\n");
		}
		assert(proc_RunningProcesses[0]->specialHandling);
		MachRecoverFromSignal();
		buffer += writeLength;
		continue;
	    }
#endif	    
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
	return totalAmountWritten;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_NewOpenStub --
 *
 *	The stub for the "open" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_NewOpenStub(pathName, unixFlags, permissions)
    char	*pathName;	/* The name of the file to open */
    int		unixFlags;	/* O_RDONLY O_WRONLY O_RDWR O_NDELAY
				 * O_APPEND O_CREAT O_TRUNC O_EXCL */
    int		permissions;	/* Permission mask to use on creation */
{
    ReturnStatus status;	/* result returned by Fs_Open */
    register int useFlags;	/* Sprite version of flags */
    int			pathNameLength;
    int streamID;
    Fs_Stream	 	*streamPtr;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Convert unixFlags to FS_READ, etc.
     */
    if (debugFsStubs) {
	printf("Fs_NewOpenStub(%s, 0x%x, 0x%x)\n",
	    copyin(pathName), unixFlags, permissions);
    }

    if (unixFlags & FASYNC) {
	printf("open - FASYNC not supported\n");
	Mach_SetErrno(EINVAL);
	return -1;
    }
    if (unixFlags & O_RDWR) {
	useFlags = FS_READ|FS_WRITE;
    } else if (unixFlags & O_WRONLY) {
	useFlags = FS_WRITE;
    } else {
	useFlags = FS_READ;
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

    /*
     * Copy the name in from user space to the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	Mach_SetErrno(EACCES);
	printf("Errno: %x = %d\n", &Proc_GetActualProc()->unixErrno,
		Proc_GetActualProc()->unixErrno);
	return -1;
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	Mach_SetErrno(ENAMETOOLONG);
	return -1;
    }

    /*
     * Open the file and get a stream descriptor for it.
     */
    useFlags &= ~FS_KERNEL_FLAGS;
    useFlags |= (FS_USER | FS_FOLLOW);

    printf("Fs_NewOpen: Fs_Open(%s, %x, %x, %x, %x\n", newName, useFlags,
	    FS_FILE, permissions&0777, &streamPtr);
    status = Fs_Open(newName, useFlags, FS_FILE,
	permissions & 0777, &streamPtr);

    if (status != SUCCESS) {
	printf("Fs_Open: error %x\n", status);
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }

    /*
     * Save a pointer to the descriptor in a list kept in the proc table.
     * Its index in the list is the "Stream ID".
     */
    status = Fs_GetStreamID(streamPtr, &streamID);
    if (status != SUCCESS) {
	printf("open (2) status = %08lx\n", status);
	(void) Fs_Close(streamPtr);
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return streamID;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_NewCloseStub --
 *
 *	The stub for the "close" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_NewCloseStub(streamID)
    int streamID;
{
    ReturnStatus status;

    if (debugFsStubs) {
	printf("Fs_CloseStub(%d)\n", streamID);
    }
    status = Fs_UserClose(streamID);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_CreatStub --
 *
 *	The stub for the "creat" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_CreatStub(pathName, permissions)
    char *pathName;		/* The name of the file to create */
    int permissions;		/* Permission mask to use on creation */
{

    if (debugFsStubs) {
	printf("Fs_CreatStub(%s, 0x%x)\n", copyin(pathName), permissions);
    }
    return (Fs_NewOpenStub(pathName, FS_CREATE|FS_TRUNC|FS_WRITE, permissions));
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ReadvStub --
 *
 *	The stub for the "readv" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_ReadvStub(streamID, iov, iovcnt)
    int streamID;		/* descriptor for stream to read. */
    register struct iovec *iov;	/* pointer to array of iovecs. */
    int iovcnt;			/* number of  iovecs in iov. */
{
    int amountRead;		/* place to hold number of bytes read */
    int totalRead = 0;  	/* place to hold total # of bytes read */
    int i;

    if (debugFsStubs) {
	printf("Fs_ReadvStub\n");
    }
    for (i=0; i < iovcnt; i++, iov++) {
	amountRead = Fs_NewReadStub(streamID, iov->iov_base, iov->iov_len);
	if (amountRead == -1) {
	    return -1;
	}
	totalRead += amountRead;
    }
    return totalRead;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_WritevStub --
 *
 *	The stub for the "writev" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_WritevStub(streamID, iov, iovcnt)

    int streamID;		/* descriptor for stream to read. */
    register struct iovec *iov;	/* pointer to array of iovecs. */
    int iovcnt;			/* number of  iovecs in iov. */
{
    int amountWritten;		/* place to hold number of bytes read */
    int totalWritten = 0;	/* place to hold total # of bytes written */
    int i;

#if 0
    if (debugFsStubs) {
	printf("Fs_WritevStub\n");
    }
#endif
    for (i=0; i < iovcnt; i++, iov++) {
	amountWritten = Fs_NewWriteStub(streamID, iov->iov_base, iov->iov_len);
	if (amountWritten == -1) {
	    return -1;
	}
	totalWritten += amountWritten;
    }
    return totalWritten;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_LinkStub --
 *
 *	The stub for the "link" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_LinkStub(fileName, linkName)
    char *fileName;
    char *linkName;
{
    ReturnStatus status;
    int			fileNameLength;
    int			linkNameLength;
    char		newFileName[FS_MAX_PATH_NAME_LENGTH];
    char		*newLinkName;

    if (debugFsStubs) {
	printf("Fs_LinkStub(%s, %s)\n", copyin(fileName), copyin(linkName));
    }

    /*
     * Copy the filename in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, fileName, newFileName,
		       &fileNameLength) != SUCCESS) {
	Mach_SetErrno(EACCES);
	return -1;
    }
    if (fileNameLength == FS_MAX_PATH_NAME_LENGTH) {
	Mach_SetErrno(EINVAL);
	return -1;
    }

    /*
     * Maybe I could put the link name onto the stack as well but I am 
     * allocing it here because I'm not sure if the stack is big enough.
     * Should just make the stack bigger I suppose ...
     */
    newLinkName = (char  *) malloc(FS_MAX_PATH_NAME_LENGTH);
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, linkName, newLinkName,
		       &linkNameLength) == SUCCESS) {
	if (linkNameLength == FS_MAX_PATH_NAME_LENGTH) {
	    status = FS_INVALID_ARG;
	} else {
	    status = Fs_HardLink(newFileName, newLinkName);
	}
    } else {
	status = SYS_ARG_NOACCESS;
    }
    free((Address) newLinkName);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_UnlinkStub --
 *
 *	The stub for the "unlink" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_UnlinkStub(pathName)
    char *pathName;
{
    ReturnStatus status;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_UnlinkStub(%s)\n", copyin(pathName));
    }
    /*
     * Copy the name in from user space to the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	status = FS_INVALID_ARG;
    } else {
	status = Fs_Remove(newName);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ChdirStub --
 *
 *	The stub for the "chdir" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_ChdirStub(pathName)
    char	*pathName;
{
    ReturnStatus status;
    int		pathNameLength;
    char	newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_ChdirStub(%s)\n", copyin(pathName));
    }

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	status = FS_INVALID_ARG;
    } else {
	status = Fs_ChangeDir(newName);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ChmodStub --
 *
 *	The stub for the "chmod" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_ChmodStub(path, mode)
    char	*path;
    int		mode;
{
    Fs_Attributes	attr;
    ReturnStatus	status;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_ChmodStub(%s, %x)\n", copyin(path), mode);
    }
    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, path, newName,
	                   &pathNameLength) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	status = FS_INVALID_ARG;
    } else {
	attr.permissions = mode;
	status = Fs_SetAttributes(newName, FS_ATTRIB_FILE, &attr, FS_SET_MODE);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_FchmodStub --
 *
 *	The stub for the "fchmod" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_FchmodStub(fd, mode)
    int		fd;
    int		mode;
{
    ReturnStatus	status;
    Fs_Attributes	attr;
    Fs_Stream		*streamPtr;
    Fs_UserIDs		ids;
    Proc_ControlBlock	*procPtr;

    if (debugFsStubs) {
	printf("Fs_FchmodStub(%d, %x)\n", fd, mode);
    }

    /*
     * Map from stream ID to file pointer and get the attributes.
     */
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, fd, &streamPtr);
    if (status == SUCCESS) {
	Fs_SetIDs(procPtr, &ids);
	attr.permissions = mode;
	status = Fs_SetAttrStream(streamPtr, &attr, &ids, FS_SET_MODE);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ChownStub --
 *
 *	The stub for the "chown" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_ChownStub(path, owner, group)
    char *path;
    int owner;
    int group;
{
    ReturnStatus    status;	/* result returned by Sprite system calls */
    Fs_Attributes   attr; 	/* struct containing all file attributes.
				 * only ownership is looked at. */
    int	            pathNameLength;
    char	    newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_ChownStub(%s, %d, %d)\n", copyin(path), owner, group);
    }

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, path, newName,
		       &pathNameLength) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	status = FS_INVALID_ARG;
    } else {
	attr.uid = owner;
	attr.gid = group;
	status = Fs_SetAttributes(newName, FS_ATTRIB_LINK, &attr, FS_SET_OWNER);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_FchownStub --
 *
 *	The stub for the "fchown" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_FchownStub(fd, owner, group)
    int fd;
    int owner;
    int group;
{
    ReturnStatus status;	/* result returned by Sprite system calls */
    Fs_Attributes attr; 	/* struct containing all file attributes.
				 * only ownership is looked at. */
    Fs_Stream			*streamPtr;
    Fs_UserIDs			ids;
    Proc_ControlBlock		*procPtr;

    if (debugFsStubs) {
	printf("Fs_FchownStub(%d, %d, %d)\n", fd,  owner, group);
    }
    /*
     * Map from stream ID to file pointer and get the attributes.
     */
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, fd, &streamPtr);
    if (status == SUCCESS) {
	Fs_SetIDs(procPtr, &ids);
	attr.uid = owner;
	attr.gid = group;
	status = Fs_SetAttrStream(streamPtr, &attr, &ids, FS_SET_OWNER);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_LseekStub --
 *
 *	The stub for the "lseek" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_LseekStub(streamID, offset, whence)
    int streamID;			/* array of stream identifiers */
    long offset;
    int whence;
{
    ReturnStatus	status;
    Ioc_RepositionArgs  inArgs;
    long                retVal;
    Fs_IOCParam         ioctl;
    Fs_IOReply          reply;
    Proc_ControlBlock	*procPtr;
    Fs_Stream		*streamPtr;

    if (debugFsStubs) {
	printf("Fs_LseekStub(%d, 0x%x, %d)\n", streamID, offset, whence);
    }
    switch(whence) {

	case L_SET:
	    inArgs.base = IOC_BASE_ZERO;
	    break;

	case L_INCR:
	    inArgs.base = IOC_BASE_CURRENT;
	    break;

	case L_XTND:
	    inArgs.base = IOC_BASE_EOF;
	    break;

	default:
	    Mach_SetErrno(EINVAL);
	    return -1;
    }
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	inArgs.offset = offset;
	ioctl.command = IOC_REPOSITION;
	ioctl.format = mach_Format;
	ioctl.procID = procPtr->processID;
	ioctl.familyID = procPtr->familyID;
	ioctl.uid = procPtr->effectiveUserID;
	ioctl.flags = 0;
	ioctl.inBuffer = (Address) &inArgs;
	ioctl.inBufSize = sizeof(Ioc_RepositionArgs);
	ioctl.outBuffer = (Address) &retVal;
	ioctl.outBufSize = sizeof(retVal);
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return retVal;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_AccessStub --
 *
 *	The stub for the "access" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_AccessStub(pathName, mode)
    char *pathName;		/* The name of the file to open */
    int	 mode;			/* access mode to test for */
{
    ReturnStatus status;
    int spriteMode;

    if (debugFsStubs) {
	printf("Fs_AccessStub(%s, %d)\n", copyin(pathName), mode);
    }
    if (mode == F_OK) {
	spriteMode = FS_EXISTS;
    } else {
	spriteMode = ((mode&R_OK)?FS_READ:0) | ((mode&W_OK)?FS_WRITE:0) |
	    ((mode&X_OK)?FS_EXECUTE:0);
    }
    status = Fs_CheckAccess(pathName, spriteMode, TRUE);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_FsyncStub --
 *
 *	The stub for the "fsync" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_FsyncStub(fd)
    int fd;
{
    ReturnStatus status;

    if (debugFsStubs) {
	printf("Fs_FsyncStub\n");
    }
    status = Fs_FileWriteBackStub(fd, -1, -1, 1);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}




/*
 *----------------------------------------------------------------------
 *
 * Fs_StatStub --
 *
 *	The stub for the "stat" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_StatStub(pathName, attrsBufPtr)
    char *pathName;		/* The name of the file to stat */
    struct stat *attrsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAttrs;	/* buffer for attributes using
					   Sprite format. */
    struct stat		tmpAttrsBuf;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_StatStub(%s)\n", copyin(pathName));
    }
    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
	printf("Fs_StatStub: arg no access\n");
    } else if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	status = FS_INVALID_ARG;
	printf("Fs_StatStub: len\n");
    } else {
	if (CheckIfPresent(newName) != SUCCESS) {
	    Mach_SetErrno(ENOENT);
	    return -1;
	}
	status = Fs_GetAttributes(newName, FS_ATTRIB_FILE, &spriteAttrs);
	if (status == SUCCESS) {
	    CvtSpriteToUnixAtts(&spriteAttrs, &tmpAttrsBuf);
	    /*
	     * The Unix stat structure is 8 bytes shorter than in stat.h
	     */
	    status = Vm_CopyOut(sizeof(struct stat)-8, (Address) &tmpAttrsBuf,
		    (Address) attrsBufPtr);
	}
    }
    if (status != SUCCESS) {
	printf("Fs_StatStub: failure\n");
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    printf("Fs_StatStub: success\n");
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_LstatStub --
 *
 *	The stub for the "lstat" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_LstatStub(pathName, attrsBufPtr)
    char *pathName;		/* The name of the file to stat */
    struct stat *attrsBufPtr;	/* ptr to buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAttrs;	/* buffer for attributes using
					   Sprite format. */
    struct stat		*tmpAttrsBuf;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_LstatStub(%s)\n", copyin(pathName));
    }
    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	status = FS_INVALID_ARG;
    } else {
	if (CheckIfPresent(newName) != SUCCESS) {
	    Mach_SetErrno(ENOENT);
	    return -1;
	}
	status = Fs_GetAttributes(newName, FS_ATTRIB_LINK, &spriteAttrs);
	/*
	 * See if we just got a remote link.  If so turn around and do a normal
	 * stat because in compatibility mode we want to follow remote links.
	 */
	if (spriteAttrs.type == FS_REMOTE_LINK) {
	   status = Fs_GetAttributes(newName,FS_ATTRIB_FILE,&spriteAttrs);
	}
	if (status == SUCCESS) {
	    CvtSpriteToUnixAtts(&spriteAttrs, &tmpAttrsBuf);
	    /*
	     * The Unix stat structure is 8 bytes shorter than in stat.h
	     */
	    status = Vm_CopyOut(sizeof(struct stat)-8, (Address) &tmpAttrsBuf,
		    (Address) attrsBufPtr);
	}
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_FstatStub --
 *
 *	The stub for the "fstat" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_FstatStub(streamID, attrsBufPtr)
    int	streamID;		/* The name of the file to stat */
    struct stat *attrsBufPtr;	/* ptr to user buffer to hold attributes in 
				   Unix format */
{
    ReturnStatus 	status;	/* status returned by Fs_GetAttributes */
    Fs_Attributes	spriteAttrs;	/* buffer for attributes using
					   Sprite format. */
    struct stat		tmpAttrsBuf;
    Fs_Stream		*streamPtr;

    if (debugFsStubs) {
	printf("Fs_FstatStub(%d)\n", streamID);
    }
    status = Fs_GetStreamPtr(Proc_GetEffectiveProc(), streamID, &streamPtr);
    if (status == SUCCESS) {
	status = Fs_GetAttrStream(streamPtr, &spriteAttrs);
	if (status == SUCCESS) {
	    CvtSpriteToUnixAtts(&spriteAttrs, &tmpAttrsBuf);
	    /*
	     * The Unix stat structure is 8 bytes shorter than in stat.h
	     */
	    status = Vm_CopyOut(sizeof(struct stat)-8, (Address) &tmpAttrsBuf,
		    (Address) attrsBufPtr);
	}
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_DupStub --
 *
 *	The stub for the "dup" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_DupStub(oldStreamID)
    int oldStreamID;
{
    ReturnStatus	status;
    int 		newStreamID;

    if (debugFsStubs) {
	printf("Fs_DupStub(%d)\n", oldStreamID);
    }
    newStreamID = FS_ANYID;
    status = Fs_GetNewID(oldStreamID, &newStreamID);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return newStreamID;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_Dup2Stub --
 *
 *	The stub for the "dup2" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_Dup2Stub(oldStreamID, newStreamID)
    int oldStreamID;		/* original stream identifier */
    int newStreamID;		/* new stream identifier */
{
    ReturnStatus	status;

    if (debugFsStubs) {
	printf("Fs_Dup2Stub(%d, %d)\n", oldStreamID, newStreamID);
    }
    status = Fs_GetNewID(oldStreamID, &newStreamID);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return newStreamID;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PipeStub --
 *
 *	The stub for the "pipe" Unix system call.
 *	This code is ripped off from Fs_CreatePipeStub.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Creates a pipe.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_PipeStub()
{
    ReturnStatus	status;
    Fs_Stream		*inStreamPtr, *outStreamPtr;
    int			inStreamID, outStreamID;

    if (debugFsStubs) {
	printf("Fs_PipeStub\n");
    }
    status = Fsio_CreatePipe(&inStreamPtr, &outStreamPtr);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    inStreamPtr->flags |= FS_USER;
    outStreamPtr->flags |= FS_USER;

    /*
     * Get stream ids for the two streams.
     */
    status = Fs_GetStreamID(inStreamPtr, &inStreamID);
    if (status != SUCCESS) {
        (void) Fs_Close(inStreamPtr);
        (void) Fs_Close(outStreamPtr);
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    status = Fs_GetStreamID(outStreamPtr, &outStreamID);
    if (status != SUCCESS) {
        Fs_ClearStreamID(inStreamID, (Proc_ControlBlock *)NIL);
        (void) Fs_Close(inStreamPtr);
        (void) Fs_Close(outStreamPtr);
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    printf("Fs_PipeStub: return %d, %d\n", inStreamID, outStreamID);
    Mach_Return2(outStreamID);
    return inStreamID;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SymlinkStub --
 *
 *	The stub for the "symlink" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SymlinkStub(target, link) 
    char *target;
    char *link;
{
    ReturnStatus	status;

    if (debugFsStubs) {
	printf("Fs_SymlinkStub\n");
    }
    status = Fs_SymLinkStub(target, link, FALSE);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ReadlinkStub --
 *
 *	The stub for the "readlink" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_ReadlinkStub(linkName, buffer, bufSize)
    char *linkName;		/* name of link file to read */
    char *buffer;		/* pointer to buffer area */
    int   bufSize;		/* number of bytes to read */
{
    ReturnStatus    status;
    Fs_Stream	    *streamPtr;
    int		    linkNameLength;
    char	    newLinkName[FS_MAX_PATH_NAME_LENGTH];
    int             linkSize;

    if (debugFsStubs) {
	printf("Fs_ReadlinkStub\n");
    }
    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, linkName, newLinkName,
		       &linkNameLength) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else if (linkNameLength == FS_MAX_PATH_NAME_LENGTH) {
	status = FS_INVALID_ARG;
    } else {
	status = Fs_Open(newLinkName, FS_READ | FS_USER,
	                 FS_SYMBOLIC_LINK|FS_REMOTE_LINK, 0, &streamPtr);
	if (status == SUCCESS) {
	    status = Fs_Read(streamPtr, buffer, 0, &bufSize);
	    if (status == SUCCESS) {
		/*
		 * Sprite's link count includes the terminating null character
		 * in the character count return while Unix doesn't.  Make our
		 * count backward-compatible with Unix.
		 */
		if (buffer[bufSize-1] == '\0') {
		    bufSize--;
		}
		status = Proc_ByteCopy(FALSE, sizeof(int), (Address)&bufSize, 
		                       (Address) &linkSize);
	    }
	    (void)Fs_Close(streamPtr);
	}
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return bufSize;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_IoctlStub --
 *
 *	The stub for the "ioctl" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_IoctlStub(streamID, request, buf)
    int  streamID;
    int  request;
    char *buf;
{
    Proc_ControlBlock   *procPtr;
    Fs_Stream 	        *streamPtr;
    ReturnStatus        status = SUCCESS;
    Fs_IOCParam         ioctl;
    Fs_IOReply          reply;
    Mach_State          *machStatePtr;
    int                 flags;

    if (debugFsStubs) {
	printf("Fs_IoctlStub(%d, 0x%x)\n", streamID, request);
    }

    /*
     * Get a stream pointer.
     */
    procPtr = Proc_GetEffectiveProc();
    machStatePtr = procPtr->machStatePtr;
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    if (!Fsutil_HandleValid(streamPtr->ioHandlePtr)) {
	Mach_SetErrno(EBADF);
	return -1;
    }
    ioctl.format = mach_Format;
    ioctl.procID = procPtr->processID;
    ioctl.familyID = procPtr->familyID;
    ioctl.uid = procPtr->effectiveUserID;
    ioctl.inBuffer = NULL;
    ioctl.inBufSize = 0;
    ioctl.outBuffer = NULL;
    ioctl.outBufSize = 0;
    ioctl.flags = 0;
    switch (request) {

    case FIOCLEX:
	if (debugFsStubs) {
                printf("ioctl: FIOCLEX\n");
                }
	ioctl.command = IOC_SET_BITS;
	ioctl.inBufSize = sizeof(int);
	ioctl.inBuffer = (Address) &flags;
	flags = IOC_CLOSE_ON_EXEC;
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	procPtr->fsPtr->streamFlags[streamID] |= FS_CLOSE_ON_EXEC;
	break;

    case FIONCLEX:
	if (debugFsStubs) {
                printf("ioctl: FIONCLEX\n");
                }
	ioctl.command = IOC_CLEAR_BITS;
	ioctl.inBufSize = sizeof(int);
	ioctl.inBuffer = (Address) &flags;
	flags = IOC_CLOSE_ON_EXEC;
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	procPtr->fsPtr->streamFlags[streamID] |= FS_CLOSE_ON_EXEC;
	break;


    case FIONREAD:
	if (debugFsStubs) {
                printf("ioctl: FIONREAD\n");
                }
	ioctl.command = IOC_NUM_READABLE;
	ioctl.outBuffer = (Address) &flags;
	ioctl.outBufSize = sizeof(flags);
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	break;


    case FIONBIO:
	if (debugFsStubs) {
                printf("ioctl: FIONBIO\n");
                }
	if (Vm_CopyIn(4, buf, (Address) &flags) != SUCCESS) {
	    Mach_SetErrno(EINVAL);
	    return -1;
	}
	if (flags) {
	    ioctl.command = IOC_SET_BITS;
	} else {
	    ioctl.command = IOC_CLEAR_BITS;
	}
	ioctl.inBufSize = sizeof(int);
	ioctl.inBuffer = (Address) &flags;
	flags = IOC_NON_BLOCKING;
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	break;

    case FIOASYNC:
	if (debugFsStubs) {
                printf("ioctl: FIOASYNC\n");
                }
	if (Vm_CopyIn(4, buf, (Address) &flags) != SUCCESS) {
	    Mach_SetErrno(EINVAL);
	    return -1;
	}
	if (flags)  {
	    ioctl.command = IOC_SET_BITS;
	} else {
	    ioctl.command = IOC_CLEAR_BITS;
	}
	ioctl.inBufSize = sizeof(int);
	ioctl.inBuffer = (Address) &flags;
	flags = IOC_ASYNCHRONOUS;
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	break;

    case FIOGETOWN:
    case SIOCGPGRP:
    case TIOCGPGRP: {
	    Ioc_Owner owner;

	    if (debugFsStubs) {
		printf("ioctl: FIOGETOWN\n");
	    }
	    ioctl.command = IOC_GET_OWNER;
	    ioctl.outBuffer = (Address) &owner;
	    ioctl.outBufSize = sizeof(owner);
	    status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    if (status != SUCCESS) {
		Mach_SetErrno(Compat_MapCode(status));
		return -1;
	    }
	    status = Vm_CopyOut(sizeof(int), (Address) &owner.id, buf);
	    if (status != SUCCESS) {
		Mach_SetErrno(EINVAL);
		return -1;
	    }
	    }
	    break;

    case FIOSETOWN:
    case SIOCSPGRP:
    case TIOCSPGRP: {
	    Ioc_Owner owner;

	    if (debugFsStubs) {
                printf("ioctl: FIOSETOWN\n");
	    }
	    status = Vm_CopyIn(sizeof(int), buf, (Address) &owner.id);
	    if (status != SUCCESS) {
		Mach_SetErrno(EINVAL);
		return -1;
	    }
	    owner.procOrFamily = IOC_OWNER_FAMILY;
	    ioctl.command = IOC_SET_OWNER;
	    ioctl.inBuffer = (Address) &owner;
	    ioctl.inBufSize = sizeof(owner);
	    status = Fs_IOControl(streamPtr, &ioctl, &reply);
        }
	break;

	/* 
	 * Tty-related calls:
	 */

     case TIOCGETP: {
	    struct sgttyb temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCGETP\n");
	    }
	    ioctl.command = IOC_TTY_GET_PARAMS;
	    ioctl.outBufSize = sizeof(struct sgttyb);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(struct sgttyb),
			                (Address) &temp, buf);
		}
	    }
         }
	 break;

     case TIOCSETP: {
	    struct sgttyb temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCSETP\n");
	    }
	    ioctl.command = IOC_TTY_SET_PARAMS;
	    ioctl.inBufSize = sizeof(struct sgttyb);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(struct sgttyb), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

     case TIOCSETN: {
	    struct sgttyb temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCSETN\n");
	    }
	    ioctl.command = IOC_TTY_SETN;
	    ioctl.inBufSize = sizeof(struct sgttyb);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(struct sgttyb), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

     case TIOCEXCL:
	if (debugFsStubs) {
	    printf("ioctl: TIOCEXCL\n");
	}
	ioctl.command = IOC_TTY_EXCL;
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	break;

     case TIOCNXCL:
	 if (debugFsStubs) {
	     printf("ioctl: TIOCNXCL\n");
	 }
	 ioctl.command = IOC_TTY_NXCL;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case TIOCHPCL:
	 if (debugFsStubs) {
	     printf("ioctl: TIOCHPCL\n");
	 }
	 ioctl.command = IOC_TTY_HUP_ON_CLOSE;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case TIOCFLUSH: {
	 int temp;

	 if (debugFsStubs) {
             printf("ioctl: TIOCFLUSH\n");
         }
	 ioctl.command = IOC_TTY_NXCL;
	 ioctl.inBufSize = sizeof(int);
	 if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM) {
	     ioctl.inBuffer = buf;
	     ioctl.flags = FS_USER_IN|FS_USER_OUT;
	     status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 } else {
	     ioctl.inBuffer = (Address) &temp;
	     status = Vm_CopyIn(sizeof(int), buf, (Address) &temp);
	     if (status == SUCCESS) {
		 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	     }
	 }
	 break;
         }

     case TIOCSTI: {
	 int temp;

	 if (debugFsStubs) {
	     printf("ioctl: TIOCSTI\n");
	 }
	 ioctl.command = IOC_TTY_INSERT_CHAR;
	 ioctl.inBufSize = sizeof(int);
	 if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM) {
	     ioctl.inBuffer = buf;
	     ioctl.flags = FS_USER_IN|FS_USER_OUT;
	     status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 } else {
	     ioctl.inBuffer = (Address) &temp;
	     status = Vm_CopyIn(sizeof(int), buf, (Address) &temp);
	     if (status == SUCCESS) {
		 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	     }
	 }
     }
     break;

     case TIOCSBRK:
         if (debugFsStubs) {
              printf("ioctl: TIOCSBRK\n");
         }
	 ioctl.command = IOC_TTY_SET_BREAK;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case TIOCCBRK:
	 if (debugFsStubs) {
             printf("ioctl: TIOCCBRK\n");
         }
	 ioctl.command = IOC_TTY_CLEAR_BREAK;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case TIOCSDTR:
	 if (debugFsStubs) {
             printf("ioctl: TIOCSDTR\n");
         }
	 ioctl.command = IOC_TTY_SET_DTR;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case TIOCCDTR:
	 if (debugFsStubs) {
             printf("ioctl: TIOCCDTR\n");
         }
	 ioctl.command = IOC_TTY_CLEAR_DTR;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case TIOCGETC: {
	    struct tchars temp;

	    if (debugFsStubs) {
		printf("ioctl: TIOCGETC\n");
	    }
	    ioctl.command = IOC_TTY_GET_TCHARS;
	    ioctl.outBufSize = sizeof(struct tchars);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(struct tchars),
			(Address) &temp, buf);
		}
	    }
         }
	 break;

     case TIOCSETC: {
	    struct tchars temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCSETC\n");
            }
	    ioctl.command = IOC_TTY_SET_TCHARS;
	    ioctl.inBufSize = sizeof(struct tchars);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(struct tchars), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

     case TIOCGLTC: {
	    struct ltchars temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCGLTC\n");
            }
	    ioctl.command = IOC_TTY_GET_LTCHARS;
	    ioctl.outBufSize = sizeof(struct ltchars);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(struct ltchars),
			(Address) &temp, buf);
		}
	    }
         }
	 break;

     case TIOCSLTC: {
	    struct ltchars temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCSLTC\n");
            }
	    ioctl.command = IOC_TTY_SET_LTCHARS;
	    ioctl.inBufSize = sizeof(struct ltchars);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(struct ltchars), buf, (Address)&temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
        }
	break;

     case TIOCLBIS: {
	    int temp;

	    ioctl.command = IOC_TTY_BIS_LM;
	    ioctl.inBufSize = sizeof(int);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(int), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

     case TIOCLBIC: {
	    int temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCLBIC\n");
            }
	    ioctl.command = IOC_TTY_BIC_LM;
	    ioctl.inBufSize = sizeof(int);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(int), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
        }
	break;

     case TIOCLSET: {
	    int temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCLSET\n");
            }
	    ioctl.command = IOC_TTY_SET_LM;
	    ioctl.inBufSize = sizeof(int);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(int), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

     case TIOCLGET: {
	    int temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCLGET\n");
            }
	    ioctl.command = IOC_TTY_GET_LM;
	    ioctl.outBufSize = sizeof(int);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(int), (Address) &temp, buf);
		}
	    }
         }
	 break;

     case TIOCGETD: {
	    int temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCGETD\n");
            }
	    ioctl.command = IOC_TTY_GET_DISCIPLINE;
	    ioctl.outBufSize = sizeof(int);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(struct tchars),
			(Address) &temp, buf);
		}
	    }
         }
	 break;

     case TIOCSETD: {
	    int temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCSETD\n");
            }
	    ioctl.command = IOC_TTY_SET_DISCIPLINE;
	    ioctl.inBufSize = sizeof(int);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(int), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
        }
	break;

     case SIOCATMARK: {
	    int temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCATMARK\n");
            }
	    ioctl.command = IOC_NET_IS_OOB_DATA_NEXT;
	    ioctl.outBufSize = sizeof(int);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(int), (Address) &temp, buf);
		}
	    }
        }
	break;

     case TCGETA:
     case TCGETS: {
	    struct sgttyb ttyb;
	    unsigned int lmode;
	    struct tchars tc;
	    struct ltchars ltc;
	    struct termios ts;
	    struct termio tio;
#if 1

	    if (debugFsStubs) {
                printf("ioctl: TCGETS\n");
            }

	    ioctl.command = IOC_TTY_GET_PARAMS;
	    ioctl.outBufSize = sizeof(struct sgttyb);
	    ioctl.outBuffer = (Address) &ttyb;
	    status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    if (status == SUCCESS) {
		ioctl.command = IOC_TTY_GET_LM;
		ioctl.outBufSize = sizeof(int);
		ioctl.outBuffer = (Address) &lmode;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    }
	    if (status == SUCCESS) {
		ioctl.command = IOC_TTY_GET_TCHARS;
		ioctl.outBufSize = sizeof(struct tchars);
		ioctl.outBuffer = (Address) &tc;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    }
	    if (status == SUCCESS) {
		ioctl.command = IOC_TTY_GET_LTCHARS;
		ioctl.outBufSize = sizeof(struct ltchars);
		ioctl.outBuffer = (Address) &ltc;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    }
	    if (status == SUCCESS) {
		CvtSgttybToTermios(&ttyb, &lmode, &tc, &ltc, &ts);
		if (request==TCGETS) {
		    status = Vm_CopyOut(sizeof(struct termios),
			(Address) &ts, buf);
		} else {
		    tio.c_iflag = ts.c_iflag;
		    tio.c_oflag = ts.c_oflag;
		    tio.c_cflag = ts.c_cflag;
		    tio.c_lflag = ts.c_lflag;
		    tio.c_line = ts.c_line;
		    bcopy((Address) ts.c_cc, (Address) tio.c_cc, NCC);
		    status = Vm_CopyOut(sizeof(struct termio),
			(Address) &tio, buf);
		}
	    }
#else
	printf("ioctl: fake TCGETS\n");
	return -1;
#endif
        }
	break;

     case TCSETA:
     case TCSETS: {
	    struct sgttyb ttyb;
	    unsigned int lmode;
	    struct tchars tc;
	    struct ltchars ltc;
	    struct termio tio;
	    struct termios ts;

	    if (debugFsStubs) {
                printf("ioctl: TCSETS\n");
            }
	    if (request==TCSETS) {
	        status = Vm_CopyIn(sizeof(struct winsize),
		    buf, (Address) &ts);
	    } else {
	        status = Vm_CopyIn(sizeof(struct winsize),
		    buf, (Address) &tio);
		ts.c_iflag = tio.c_iflag;
		ts.c_oflag = tio.c_oflag;
		ts.c_cflag = tio.c_cflag;
		ts.c_lflag = tio.c_lflag;
		ts.c_line = tio.c_line;
		bzero((Address) ts.c_cc, NCCS);
		bcopy((Address) tio.c_cc, (Address) ts.c_cc, NCC);
	    }
	    if (status == SUCCESS) {
		CvtTermiosToSgttyb((request==TCSETS)?1:0, &ts, &ttyb,
			&lmode, &tc, &ltc);
		ioctl.command = IOC_TTY_SET_PARAMS;
		ioctl.outBufSize = sizeof(struct sgttyb);
		ioctl.outBuffer = (Address) &ttyb;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    }
	    if (status == SUCCESS) {
		ioctl.command = IOC_TTY_SET_LM;
		ioctl.inBufSize = sizeof(int);
		ioctl.inBuffer = (Address) &lmode;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    }
	    if (status == SUCCESS) {
		ioctl.command = IOC_TTY_SET_TCHARS;
		ioctl.inBufSize = sizeof(struct tchars);
		ioctl.inBuffer = (Address) &tc;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    }
	    if (status == SUCCESS) {
		ioctl.command = IOC_TTY_GET_LTCHARS;
		ioctl.outBufSize = sizeof(struct ltchars);
		ioctl.outBuffer = (Address) &ltc;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    }
        }
	break;

     case TIOCGWINSZ: {
	    struct winsize temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCGWINSZ\n");
	    }
	    ioctl.command = IOC_TTY_GET_WINDOW_SIZE;
	    ioctl.outBuffer = (Address) &temp;
	    ioctl.outBufSize = sizeof(struct winsize);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(struct winsize),
			(Address) &temp, buf);
		}
	    }
        }
	break;

     case TIOCSWINSZ: {
	    struct winsize temp;

	    if (debugFsStubs) {
                printf("ioctl: TIOCSWINSZ\n");
	    }
	    ioctl.command = IOC_TTY_SET_WINDOW_SIZE;
	    ioctl.inBufSize = sizeof(struct winsize);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
	        status = Vm_CopyIn(sizeof(struct winsize),
		    buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

	/*
	 * Graphics requests.
	 */

     case QIOCGINFO: {
	    DevScreenInfo *temp;

	    if (debugFsStubs) {
                printf("ioctl: QIOCGINFO\n");
	    }
	    ioctl.command = IOC_GRAPHICS_GET_INFO;
	    ioctl.outBufSize = sizeof(DevScreenInfo *);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(DevScreenInfo *),
			(Address) &temp, buf);
		}
	    }
        }
	break;

     case QIOCPMSTATE: {
	    DevCursor temp;

	    if (debugFsStubs) {
                printf("ioctl: QIOCPMSTATE\n");
	    }
	    ioctl.command = IOC_GRAPHICS_MOUSE_POS;
	    ioctl.outBufSize = sizeof(DevCursor);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(DevCursor),
			(Address) &temp, buf);
		}
	    }
        }
	break;

     case QIOWCURSORCOLOR: {
	    unsigned int temp[6];

	    if (debugFsStubs) {
                printf("ioctl: QIOWCURSORCOLOR\n");
	    }
	    ioctl.command = IOC_GRAPHICS_CURSOR_COLOR;
	    ioctl.inBufSize = sizeof(unsigned int [6]);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) temp;
		status = Vm_CopyIn(sizeof(unsigned int [6]),
		    buf, (Address) temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

     case QIOCINIT:
	 if (debugFsStubs) {
	     printf("ioctl: QIOCINIT\n");
	 }
	 ioctl.command = IOC_GRAPHICS_INIT_SCREEN;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case QIOCKPCMD: {
	     DevKpCmd temp;

	     if (debugFsStubs) {
                printf("ioctl: QIOCKPCMD\n");
             }
	     ioctl.command = IOC_GRAPHICS_KBD_CMD;
	     ioctl.inBufSize = sizeof(DevKpCmd);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(DevKpCmd), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
         }
	 break;

     case QIOCADDR: {
	    DevScreenInfo *temp;

	    if (debugFsStubs) {
                printf("ioctl: QIOCADDR\n");
            }
	    ioctl.command = IOC_GRAPHICS_GET_INFO_ADDR;
	    ioctl.outBufSize = sizeof(DevScreenInfo *);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.outBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.outBuffer = (Address) &temp;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
		if (status == SUCCESS) {
		    status = Vm_CopyOut(sizeof(DevScreenInfo *),
			(Address) &temp, buf);
		}
	    }
        }
	break;

     case QIOWCURSOR: {
	    short temp[32];

	    if (debugFsStubs) {
                printf("ioctl: QIOWCURSOR\n");
	    }
	    ioctl.command = IOC_GRAPHICS_CURSOR_BIT_MAP;
	    ioctl.inBufSize = sizeof(short[32]);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) temp;
		status = Vm_CopyIn(sizeof(temp), buf, (Address) temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
        }
	break;

     case QIOKERNLOOP:
	 if (debugFsStubs) {
                printf("ioctl: QIOKERNLOOP\n");
	 }
	 ioctl.command = IOC_GRAPHICS_KERN_LOOP;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case QIOKERNUNLOOP:
	 if (debugFsStubs) {
                printf("ioctl: QIOKERNUNLOOP\n");
	 }
	 ioctl.command = IOC_GRAPHICS_KERN_UNLOOP;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case QIOVIDEOON:
	 if (debugFsStubs) {
                printf("ioctl: QIOVIDEOON\n");
	 }
	 ioctl.command = IOC_GRAPHICS_VIDEO_ON;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case QIOVIDEOOFF:
	 if (debugFsStubs) {
                printf("ioctl: QIOVIDEOOFF\n");
	 }
	 ioctl.command = IOC_GRAPHICS_VIDEO_OFF;
	 status = Fs_IOControl(streamPtr, &ioctl, &reply);
	 break;

     case QIOSETCMAP: {
	    DevColorMap temp;

	    if (debugFsStubs) {
                printf("ioctl: QIOSETCMAP\n");
	    }
	    ioctl.command = IOC_GRAPHICS_COLOR_MAP;
	    ioctl.inBufSize = sizeof(DevColorMap);
	    if (streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM){
		ioctl.inBuffer = buf;
		ioctl.flags = FS_USER_IN|FS_USER_OUT;
		status = Fs_IOControl(streamPtr, &ioctl, &reply);
	    } else {
		ioctl.inBuffer = (Address) &temp;
		status = Vm_CopyIn(sizeof(temp), buf, (Address) &temp);
		if (status == SUCCESS) {
		    status = Fs_IOControl(streamPtr, &ioctl, &reply);
		}
	    }
        }
	break;

      case SIOCGIFCONF: {
	  /*
	   * Fake the ifconfig ioctl.  This is a hack done for X.  
	   * A more general implementation is probably needed.
	   */
	  struct ifconf ifc;
	  struct ifreq   ifreq;
	  extern int	  sysHostID;
	  int		  *intPtr;

	  if (debugFsStubs) {
                printf("ioctl: SIOCGIFCONF\n");
	  }
	  status = Vm_CopyIn(sizeof(struct ifconf), buf, (Address)&ifc);
	  if (status != SUCCESS) {
	      return(status);
	  }

	  if (ifc.ifc_len < 32) {
	      status = SYS_INVALID_ARG;
	      break;
	  }
	  /*
	   * We give a length of 32 and put in the request buffer 
	   * the name ("se0"), followed by the family (AF_INET), 
	   * and finally our internet address.
	   */
	  ifc.ifc_len = 32;
	  strcpy(ifreq.ifr_name, "se0");
	  intPtr = (int *)&ifreq.ifr_ifru;
	  *intPtr = AF_INET;
	  *(intPtr + 1) = sysHostID;

	  status = Vm_CopyOut(sizeof(struct ifconf), (Address)&ifc, buf);
	  status = Vm_CopyOut(32, (Address)&ifreq,
	      (Address)ifc.ifc_ifcu.ifcu_req);
      }
      break;

      case SIOCRPHYSADDR: {
	    /* Get the ethernet address. */

	    struct ifdevea *p;
	    Net_EtherAddress etherAddress;

	    if (debugFsStubs) {
		printf("ioctl: SIOCRPHYSADDR\n");
	    }
	    Mach_GetEtherAddress(&etherAddress);
	    p = (struct ifdevea *) buf;
	      p->default_pa[0] = p->current_pa[0] = etherAddress.byte1;
	      p->default_pa[1] = p->current_pa[1] = etherAddress.byte2;
	      p->default_pa[2] = p->current_pa[2] = etherAddress.byte3;
	      p->default_pa[3] = p->current_pa[3] = etherAddress.byte4;
	      p->default_pa[4] = p->current_pa[4] = etherAddress.byte5;
	      p->default_pa[5] = p->current_pa[5] = etherAddress.byte6;
	      status = SUCCESS;
         }
	 break;

     default:
	if (debugFsStubs) {
	    printf("Bad Ioctl: 0x%08x\n",request);
	}
	Mach_SetErrno(EINVAL);
	return -1;
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }	    
    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Fs_NewRenameStub --
 *
 *	The stub for the "newrename" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_NewRenameStub(pathName, newName)
    char *pathName;
    char *newName;
{
    ReturnStatus	status;
    int			pathNameLength;
    int			newNameLength;
    char		newPathName[FS_MAX_PATH_NAME_LENGTH];
    char		*newNewName;

    if (debugFsStubs) {
	printf("Fs_RenameStub\n");
    }
    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newPathName,
		       &pathNameLength) != SUCCESS) {
	Mach_SetErrno(EACCES);
	return -1;
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	Mach_SetErrno(EINVAL);
	return -1;
    }

    /*
     * Maybe I could put the new name onto the stack as well but I am 
     * allocing it here because I'm not sure if the stack is big enough.
     * Should just make the stack bigger I suppose ...
     */
    newNewName = (char  *) malloc(FS_MAX_PATH_NAME_LENGTH);
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, newName, newNewName,
		       &newNameLength) == SUCCESS) {
	if (newNameLength == FS_MAX_PATH_NAME_LENGTH) {
	    status = FS_INVALID_ARG;
	} else {
	    status = Fs_Rename(newPathName, newNewName);
	}
    } else {
	status = SYS_ARG_NOACCESS;
    }
    free((Address) newNewName);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_FtruncateStub --
 *
 *	The stub for the "ftruncate" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_FtruncateStub(streamID, length)
    int streamID;
    unsigned long length;
{
    ReturnStatus        status;
    Proc_ControlBlock   *procPtr;
    Fs_Stream 	        *streamPtr;
    Fs_IOCParam         ioctl;
    Fs_IOReply          reply;

    if (debugFsStubs) {
	printf("Fs_FtruncateStub\n");
    }
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    ioctl.command = IOC_TRUNCATE;
    ioctl.format = mach_Format;
    ioctl.procID = procPtr->processID;
    ioctl.familyID = procPtr->familyID;
    ioctl.uid = procPtr->effectiveUserID;
    ioctl.flags = 0;
    ioctl.outBuffer = 0;
    ioctl.outBufSize = 0;
    ioctl.inBuffer = (Address) &length;
    ioctl.inBufSize = sizeof(length);
    status = Fs_IOControl(streamPtr, &ioctl, &reply);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_TruncateStub --
 *
 *	The stub for the "truncate" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_TruncateStub(path, length)
    char *path;
    unsigned long length;
{
    int	streamID;
    int status;

    if (debugFsStubs) {
	printf("Fs_TruncateStub\n");
    }
    streamID = Fs_NewOpenStub(path, FS_WRITE, 0);
    if (streamID == -1) {
	return -1;
    }
    status = Fs_FtruncateStub(streamID, length);
    (void) Fs_UserClose(streamID);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_FlockStub --
 *
 *	The stub for the "flock" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_FlockStub(streamID, operation)
    int	streamID;
    int operation;
{
    ReturnStatus status;
    Ioc_LockArgs args;
    Proc_ControlBlock   *procPtr;
    Fs_Stream 	        *streamPtr;
    Fs_IOCParam         ioctl;
    Fs_IOReply          reply;

    if (debugFsStubs) {
	printf("Fs_FlockStub\n");
    }
    args.flags = 0;
    if (operation & LOCK_EX) {
	args.flags |= IOC_LOCK_EXCLUSIVE;
    } else if (operation & LOCK_SH) {
	args.flags |= IOC_LOCK_SHARED;
    }
    if (operation & LOCK_NB) {
	args.flags |= IOC_LOCK_NO_BLOCK;
    }
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (operation & LOCK_UN) {
	ioctl.command = IOC_UNLOCK;
    } else {
	ioctl.command = IOC_LOCK;
    }
    ioctl.format = mach_Format;
    ioctl.procID = procPtr->processID;
    ioctl.familyID = procPtr->familyID;
    ioctl.uid = procPtr->effectiveUserID;
    ioctl.flags = 0;
    ioctl.outBuffer = 0;
    ioctl.outBufSize = 0;
    ioctl.inBuffer = (Address) &args;
    ioctl.inBufSize = sizeof(Ioc_LockArgs),
    status = Fs_IOControl(streamPtr, &ioctl, &reply);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_MkdirStub --
 *
 *	The stub for the "mkdir" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_MkdirStub(pathName, permissions)
    char *pathName;		/* The name of the directory to create */
    int permissions;		/* Permission mask to use on creation */
{
    ReturnStatus        status;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_MkdirStub\n");
    }
    /*
     * Copy the name in from user space to the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	Mach_SetErrno(EACCES);
	return -1;
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Fs_MakeDir(newName, permissions);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RmdirStub --
 *
 *	The stub for the "rmdir" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_RmdirStub(pathName)
    char *pathName;		/* The name of the directory to create */
{
    ReturnStatus    status;
    int		    pathNameLength;
    char	    newName[FS_MAX_PATH_NAME_LENGTH];

    if (debugFsStubs) {
	printf("Fs_RmdirStub\n");
    }
    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	Mach_SetErrno(EACCES);
	return -1;
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Fs_RemoveDir(newName);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_UtimesStub --
 *
 *	The stub for the "utimes" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_UtimesStub(pathName, tvp)
    char	*pathName;
    struct timeval *tvp;
{
    ReturnStatus    status;
    Fs_Attributes   attributes;
    int		    pathNameLength;
    char	    newName[FS_MAX_PATH_NAME_LENGTH];
    struct timeval  tv[2];

    if (debugFsStubs) {
	printf("Fs_Utimes\n");
    }

    if (Vm_CopyIn(sizeof(tv), (Address) tvp, (Address) tv) != SUCCESS) {
	Mach_SetErrno(EFAULT);
	return -1;
    }
    attributes.accessTime.seconds = tv[0].tv_sec;
    attributes.accessTime.microseconds = tv[0].tv_usec;
    attributes.dataModifyTime.seconds = tv[1].tv_sec;
    attributes.dataModifyTime.microseconds = tv[1].tv_usec;

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
        Mach_SetErrno(EACCES);
	return -1;
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Fs_SetAttributes(newName,
	                      FS_ATTRIB_FILE, &attributes, FS_SET_TIMES);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SyncStub --
 *
 *	The stub for the "sync" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SyncStub()
{

    printf("Fs_SyncStub is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetdtablesizeStub --
 *
 *	The stub for the "getdtablesize" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int 
Fs_GetdtablesizeStub()
{

    if (debugFsStubs) {
	printf("Fs_GetdtablesizeStub\n");
    }
    return 100;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetdoptStub --
 *
 *	The stub for the "getdopt" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_GetdoptStub()

{

    printf("getdopt is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetdoptStub --
 *
 *	The stub for the "setdopt" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SetdoptStub()
{

    printf("setdopt is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_FcntlStub --
 *
 *	The stub for the "fcntl" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_FcntlStub(fd, cmd, arg)
int fd, cmd, arg;
{

    Proc_ControlBlock *procPtr=Proc_GetEffectiveProc();
    Fs_Stream *streamPtr;
    ReturnStatus status;

    status = Fs_GetStreamPtr(procPtr, fd, &streamPtr);
    if (status != SUCCESS) {
	Mach_SetErrno(EBADF);
	return -1;
    }

    switch (cmd) {
	case F_SETFD:
	    /*
	     * SunOS uses this function.  To prevent excess error messages,
	     * we'll just silently ignore it.
	     */
	    break;
	case F_DUPFD:
	case F_GETFD:
	case F_GETFL:
	case F_SETFL:
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
	case F_GETOWN:
	case F_SETOWN:
	case F_RSETLK:
	case F_RSETLKW:
	case F_RGETLK:
	default:
	    printf("Unimplemented fcntl function %d\n", cmd);
	    Mach_SetErrno(EINVAL);
	    return -1;
	    break;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SocketStub --
 *
 *	The stub for the "socket" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SocketStub()
{

    printf("socket is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ConnectStub --
 *
 *	The stub for the "connect" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_ConnectStub()
{

    printf("connect is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_AcceptStub --
 *
 *	The stub for the "accept" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_AcceptStub()
{
    printf("accept is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SendStub --
 *
 *	The stub for the "send" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SendStub()
{
    printf("send is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RecvStub --
 *
 *	The stub for the "recv" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_RecvStub()
{
    printf("recv is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SendmsgStub --
 *
 *	The stub for the "sendmsg" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SendmsgStub()
{
    printf("sendmsg is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RecvmsgStub --
 *
 *	The stub for the "recvmsg" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_RecvmsgStub()
{
    printf("recvmsg is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_BindStub --
 *
 *	The stub for the "bind" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_BindStub()
{
    printf("Fs_Bind is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetsockoptStub --
 *
 *	The stub for the "setsockopt" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SetsockoptStub()
{
    printf("Fs_SetSockOpt is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ListenStub --
 *
 *	The stub for the "listen" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_ListenStub()
{
    printf("Fs_Listen is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*ARGSUSED*/
/*
 *----------------------------------------------------------------------
 *
 * Fs_SocketpairStub --
 *
 *	The stub for the "socketpair" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SocketpairStub(d, type, protocol, sv)
    int d, type, protocol;
    int sv[2];
{
    printf("socketpair is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetsockoptStub --
 *
 *	The stub for the "getsockopt" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_GetsockoptStub()
{
    printf("getsockopt is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SendtoStub --
 *
 *	The stub for the "sendto" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_SendtoStub()
{
    printf("sendto is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RecvfromStub --
 *
 *	The stub for the "recvform" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_RecvfromStub()
{
    printf("recvfrom is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetsocknameStub --
 *
 *	The stub for the "getsockname" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_GetsocknameStub()
{
    printf("getsockname is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

#define SWAP2(val) ((char *)&tmp)[0] = ((char *)&val)[1];\
	((char *)&tmp)[1] = ((char *)&val)[0]; val = tmp
#define SWAP4(val) ((char *)&tmp)[0] = ((char *)&val)[3]; \
	((char *)&tmp)[1] = ((char *)&val)[2]; \
	((char *)&tmp)[2] = ((char *)&val)[1]; \
	((char *)&tmp)[3] = ((char *)&val)[0]; val = tmp
/*
 *----------------------------------------------------------------------
 *
 * Fs_GetdirInt --
 *
 *	This is an internal routine to get a directory entry from a file.
 *
 * Results:
 *	Returns 0 if successful.
 *	Returns EISDIR if we reach end of directory.
 *	Returns other Unix errors for other error conditions.
 *
 * Side effects:
 *	Reads from the file.
 *	Puts return value in variables.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
Fs_GetdirInt(streamPtr, inDir, oldOff, newOff, kernDir)
    Fs_Stream		*streamPtr;
    struct direct *inDir;	/* This buffer must be kernel accessible.  */
    int *oldOff;
    int *newOff;
    struct direct	*kernDir;
{
    ReturnStatus	status;
    int                 amountRead;
    int			amountToRead;
    int			tmp;

    /*
     * If Fslcl_DirEntry and struct direct are not the same, we'll
     * have to do yet another layer of translation.
     */
    assert(sizeof(Fslcl_DirEntry)==sizeof(struct direct));

#define HDR_SIZE sizeof(u_long)+2*sizeof(u_short)
    *oldOff = streamPtr->offset;
    while (1) {
	amountRead = HDR_SIZE;
	status = Fs_Read(streamPtr, (Address)inDir, streamPtr->offset,
		&amountRead);
	if (status != SUCCESS) {
	    printf("Fs_GetdirInt: Read failed: %x\n", status);
	    return Compat_MapCode(status);
	}
	if (amountRead < HDR_SIZE) {
	    return EISDIR;
	}
	if (Vm_CopyIn(HDR_SIZE, (Address)inDir, (Address)kernDir) != SUCCESS) {
	    printf("Copy in failure\n");
	    Mach_SetErrno(EINVAL);
	    return -1;
	}

	/*
	 * Check against big-endian/little-endian conflict.
	 * The max record length is 512, which is 02 when byteswapped.
	 * The min record length is 8, which is > 512 when byteswapped.
	 * All other values fall outside the range 8-512 when byteswapped.
	 */
	if (kernDir->d_reclen > FSLCL_DIR_BLOCK_SIZE ||
		kernDir->d_reclen < 2 * sizeof(int)) {
	    SWAP4(kernDir->d_ino);
	    SWAP2(kernDir->d_reclen);
	    SWAP2(kernDir->d_namlen);
	    if (Vm_CopyOut(HDR_SIZE, (Address)kernDir, (Address) inDir)
		    != SUCCESS) {
		printf("Copy out failure\n");
		Mach_SetErrno(EINVAL);
		return -1;
	    }
	}

		
	amountToRead = kernDir->d_reclen - amountRead;
	if (amountToRead<0 || kernDir->d_reclen < kernDir->d_namlen ||
		kernDir->d_namlen>255) {
	    Mach_SetErrno(EINVAL);
	    return -1;
	}
	amountRead = amountToRead;
	status = Fs_Read(streamPtr, ((Address)(inDir))+HDR_SIZE,
		streamPtr->offset, &amountRead);
	*newOff = streamPtr->offset;
	if (status != SUCCESS) {
	    return Compat_MapCode(status);
	}
	if (amountRead < amountToRead) {
	    /*
	     * Directory is truncated?
	     */
	    printf("Fs_GetdirInt: directory truncated\n");
	    return ENOTDIR;
	}
	if (kernDir->d_ino != 0) break;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetdentsStub --
 *
 *	The stub for the "getdents" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Fs_GetdentsStub(fd, buf, nbytes)
    int  fd;
    char *buf;
    int nbytes;
{
    Fs_Stream		*streamPtr;
    Proc_ControlBlock	*procPtr;
    struct dirent	tmpBuf;
    int			status;
    ReturnStatus	returnStatus;
    int			oldOff;	/* dummy */
    int			totalBytes = 0;
    Fs_Attributes	spriteAttrs;

    if (debugFsStubs) {
	printf("Fs_Getdents(%d, %x, %d)\n", fd, buf, nbytes);
    }
    assert(sizeof(struct dirent) == sizeof(struct direct)+4);

    if (nbytes<sizeof(struct dirent)) {
	Mach_SetErrno(EINVAL);
	return -1;
    }

    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, fd, &streamPtr);
    if (status == SUCCESS) {
	status = Fs_GetAttrStream(streamPtr, &spriteAttrs);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(EBADF);
	return -1;
    }

    if (spriteAttrs.type != FS_DIRECTORY) {
	Mach_SetErrno(ENOTDIR);
	return -1;
    }

    /*
     * We do something tricky here.  The dirent structure has an extra
     * byte at the beginning for the offset.  So we offset tmpBuf
     * appropriately.
     */
    while (nbytes-totalBytes>=sizeof(struct dirent)) {
	status = Fs_GetdirInt(streamPtr,
		(struct direct *)(buf+sizeof(off_t)),
		&oldOff, &tmpBuf.d_off, (struct direct *)(&tmpBuf.d_fileno));
	if (status == EISDIR) {
	    break;
	} else if (status != 0) {
	    printf("GetdirInt failure: %d\n", status);
	    Mach_SetErrno(status);
	    return -1;
	}
	tmpBuf.d_reclen += sizeof(off_t);

	returnStatus = Vm_CopyOut(HDR_SIZE+sizeof(off_t), (Address) &tmpBuf,
		buf);
	if (returnStatus != SUCCESS) {
	    Mach_SetErrno(EFAULT);
	    return -1;
	}
	buf += tmpBuf.d_reclen;
	totalBytes += tmpBuf.d_reclen;
    }
    return totalBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetdirentriesStub --
 *
 *	The stub for the "getdirentries" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int 
Fs_GetdirentriesStub(fd, buf, nbytes, basep)
    int  fd;
    char *buf;
    int nbytes;
    long *basep;
{
    Fs_Stream		*streamPtr;
    Proc_ControlBlock	*procPtr;
    struct direct	tmpBuf;
    int			status;
    Fs_Attributes	spriteAttrs;
    int			oldOff = -1;
    int			newOff;
    int			totalBytes = 0;

    if (debugFsStubs) {
	printf("Fs_GetdirentriesStub(%d, %x, %d, %x\n", fd, buf, nbytes,
		basep);
    }

    if (nbytes<sizeof(struct direct)) {
	printf("nbytes too small\n");
	Mach_SetErrno(EINVAL);
	return -1;
    }

    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, fd, &streamPtr);
    if (status == SUCCESS) {
	status = Fs_GetAttrStream(streamPtr, &spriteAttrs);
    }
    if (status != SUCCESS) {
	printf("Bad stream\n");
	Mach_SetErrno(EBADF);
	return -1;
    }

    if (spriteAttrs.type != FS_DIRECTORY) {
	printf("Not directory; type = %d\n", spriteAttrs.type);
	Mach_SetErrno(ENOTDIR);
	return -1;
    }

    *basep = streamPtr->offset;

    while (nbytes-totalBytes>=sizeof(struct direct)) {
	status = Fs_GetdirInt(streamPtr, (struct direct *)buf, &oldOff,
		&newOff, &tmpBuf);
	if (status == EISDIR) {
	    break;
	} else if (status != 0) {
	    printf("GetdirInt Error: %x\n", status);
	    Mach_SetErrno(status);
	    return -1;
	}

	buf += tmpBuf.d_reclen;
	totalBytes += tmpBuf.d_reclen;
    }
    return totalBytes;
}
