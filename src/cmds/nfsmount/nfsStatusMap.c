/*
 * nfsStatusMap.c --
 * 
 *	This defines a mapping from UNIX to Sprite error codes.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/nfsmount/RCS/nfsStatusMap.c,v 1.4 90/05/21 14:29:14 mgbaker Exp $ SPRITE (Berkeley)";
#endif not lint

#include "status.h"
#include "errno.h"

ReturnStatus nfsStatusMap[] = {
	SUCCESS,		/* 			0	 */
	FS_NOT_OWNER,		/* UNIX_EPERM		1	 */
	FS_FILE_NOT_FOUND,	/* UNIX_ENOENT		2	 */
	PROC_INVALID_PID,	/* UNIX_ESRCH		3	 */
	GEN_ABORTED_BY_SIGNAL,	/* UNIX_EINTR		4	 */
	DEV_DMA_FAULT,		/* UNIX_EIO		5	 */
	FS_FILE_NOT_FOUND,	/* UNIX_ENXIO		6	 */
	FAILURE,		/* UNIX_E2BIG		7	 */
	PROC_BAD_AOUT_FORMAT,	/* UNIX_ENOEXEC		8	 */
	FS_INVALID_ARG,		/* UNIX_EBADF		9	 */
	PROC_NO_CHILDREN,	/* UNIX_ECHILD		10	 */
	FAILURE,		/* UNIX_EAGAIN		11	 */
	VM_NO_SEGMENTS,		/* UNIX_ENOMEM		12	 */
	FS_NO_ACCESS,		/* UNIX_EACCES		13	 */
	SYS_ARG_NOACCESS,	/* UNIX_EFAULT		14	 */
	FS_DEVICE_OP_INVALID,	/* UNIX_ENOTBLK		15	 */
	FAILURE,		/* UNIX_EBUSY		16	 */
	FS_FILE_EXISTS,		/* UNIX_EEXIST		17	 */
	FS_CROSS_DOMAIN_OPERATION, /* UNIX_EXDEV	18	 */
	FS_FILE_NOT_FOUND,	/* UNIX_ENODEV		19	 */
	FS_NOT_DIRECTORY,	/* UNIX_ENOTDIR		20	 */
	FS_IS_DIRECTORY,	/* UNIX_EISDIR		21	 */
	FS_INVALID_ARG,		/* UNIX_EINVAL		22	 */
	FAILURE,		/* UNIX_ENFILE		23	 */
	FAILURE,		/* UNIX_EMFILE		24	 */
	FAILURE,		/* UNIX_ENOTTY		25	 */
	FS_FILE_BUSY,		/* UNIX_ETXTBSY		26	 */
	FAILURE,		/* UNIX_EFBIG		27	 */
	FS_NO_DISK_SPACE,	/* UNIX_ENOSPC		28	 */
	FS_LOCAL_OP_INVALID,	/* UNIX_ESPIPE		29	 */
	FAILURE,		/* UNIX_EROFS		30	 */
	FS_NAME_LOOP,		/* UNIX_EMLINK		31	 */
	FS_BROKEN_PIPE,		/* UNIX_EPIPE		32	 */
	FAILURE,		/* UNIX_EDOM		33	 */
	FAILURE,		/* UNIX_ERANGE		34	 */

	FS_WOULD_BLOCK,		/* UNIX_EWOULDBLOCK	35	 */
	FAILURE,		/* UNIX_EINPROGRESS	36	 */
	FAILURE,		/* UNIX_EALREADY	37	 */

	FAILURE,		/* UNIX_ENOTSOCK	38	 */
	FAILURE,		/* UNIX_EDESTADDRREQ	39	 */
	FS_BUFFER_TOO_BIG,	/* UNIX_EMSGSIZE	40	 */
	FAILURE,		/* UNIX_EPROTOTYPE	41	 */
	FAILURE,		/* UNIX_ENOPROTOOPT	42	 */
	FAILURE,		/* UNIX_EPROTONOSUPPORT	43	 */
	FAILURE,		/* UNIX_ESOCKTNOSUPPORT	44	 */
	FAILURE,		/* UNIX_EOPNOTSUPP	45	 */
	FAILURE,		/* UNIX_EPFNOSUPPORT	46	 */
	FAILURE,		/* UNIX_EAFNOSUPPORT	47	 */
	FAILURE,		/* UNIX_EADDRINUSE	48	 */
	FAILURE,		/* UNIX_EADDRNOTAVAIL	49	 */

	FAILURE,		/* UNIX_ENETDOWN	50	 */
	FAILURE,		/* UNIX_ENETUNREACH	51	 */
	FAILURE,		/* UNIX_ENETRESET	52	 */
	FAILURE,		/* UNIX_ECONNABORTED	53	 */
	FAILURE,		/* UNIX_ECONNRESET	54	 */
	FAILURE,		/* UNIX_ENOBUFS		55	 */
	FAILURE,		/* UNIX_EISCONN		56	 */
	FAILURE,		/* UNIX_ENOTCONN	57	 */
	FAILURE,		/* UNIX_ESHUTDOWN	58	 */
	FAILURE,		/* UNIX_ETOOMANYREFS	59	 */
	RPC_TIMEOUT,		/* UNIX_ETIMEDOUT	60	 */
	FAILURE,		/* UNIX_ECONNREFUSED	61	 */

	FS_NAME_LOOP,		/* UNIX_ELOOP		62	 */
	FAILURE,		/* UNIX_ENAMETOOLONG	63	 */

	FAILURE,		/* UNIX_EHOSTDOWN	64	 */
	FAILURE,		/* UNIX_EHOSTUNREACH	65	 */
	FAILURE,		/* UNIX_ENOTEMPTY	66	 */

	FAILURE,		/* UNIX_EPROCLIM	67	 */
	FAILURE,		/* UNIX_EUSERS		68	 */
	FAILURE,		/* UNIX_EDQUOT		69	 */

	FS_FILE_NOT_FOUND,	/* UNIX_ESTALE		70	 */
	EREMOTE,		/* UNIX_EREMOTE		71	 */

};

