/*
 * errno.h --
 *
 *	Declaration for the errno variable, the error constants, and
 *	the strings giving error messages for various errors.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/errno.h,v 1.6 90/03/29 12:44:34 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _ERRNO
#define _ERRNO

extern int	errno;		/* Last error condition returned from a
				 * kernel call or some other procedure. */
extern int	sys_nerr;	/* Number of error message in array below. */
extern char	*sys_errlist[];	/* Array containing string error messages
				 * for errors 0 up to sys_nerr-1. */

/*
 * Known values for errno:
 */

#define EPERM		1
#define ENOENT		2
#define ESRCH		3
#define EINTR		4
#define EIO		5
#define ENXIO		6
#define E2BIG		7
#define ENOEXEC		8
#define EBADF		9
#define ECHILD		10
#define EAGAIN		11
#define ENOMEM		12
#define EACCES		13
#define EFAULT		14
#define ENOTBLK		15
#define EBUSY		16
#define EEXIST		17
#define EXDEV		18
#define ENODEV		19
#define ENOTDIR		20
#define EISDIR		21
#define EINVAL		22
#define ENFILE		23
#define EMFILE		24
#define ENOTTY		25
#define ETXTBSY		26
#define EFBIG		27
#define ENOSPC		28
#define ESPIPE		29
#define EROFS		30
#define EMLINK		31
#define EPIPE		32
#define EDOM		33
#define ERANGE		34
#define EWOULDBLOCK	35
#define EINPROGRESS	36
#define EALREADY	37
#define ENOTSOCK	38
#define EDESTADDRREQ	39
#define EMSGSIZE	40
#define EPROTOTYPE	41
#define ENOPROTOOPT	42
#define EPROTONOSUPPORT 43
#define ESOCKTNOSUPPORT 44
#define EOPNOTSUPP	45
#define EPFNOSUPPORT	46
#define EAFNOSUPPORT	47
#define EADDRINUSE	48
#define EADDRNOTAVAIL	49
#define ENETDOWN	50
#define ENETUNREACH	51
#define ENETRESET	52
#define ECONNABORTED	53
#define ECONNRESET	54
#define ENOBUFS		55
#define EISCONN		56
#define ENOTCONN	57
#define ESHUTDOWN	58

#define ETIMEDOUT	60
#define ECONNREFUSED	61
#define ELOOP		62
#define ENAMETOOLONG	63
#define EHOSTDOWN	64
#define EHOSTUNREACH	65
#define ENOTEMPTY	66
#define EPROCLIM	67
#define EUSERS		68
#define EDQUOT		69
#define ESTALE		70
#define EREMOTE		71

#define EIDRM		77

#endif /* _ERRNO */
