/*
 * pfs.h --
 *
 *	Declarations of the kernel/user-level-server interface for
 *	pseudo-file-systems. The pseudo-file-system interface uses
 *	the request-response protocol defined for pseudo-devices
 *	to forward file system naming operations to the pseudo-file-system
 *	server process.
 *
 *	A more complete explaination of pseudo-file-systems should be in
 *	the man page (/sprite/doc/ref/devices/pfs), and the pseudo-device
 *	interface document is (/sprite/doc/ref/devices/pdev).
 *
 *	The kernel-to-server interface is based on a set of typed messages,
 *	one for each operation implemented by the server.  Each message
 *	has a header of type Pfs_Request, which in turn embeds some
 *	operation-specific parameters.  Following the parameters is a variable
 *	length block of data, generally a pathname.  Most of the parameters
 *	are kernel structures defined in fsNameOps.h.
 *	
 * Copyright 1989 Regents of the University of California
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
 * $Header: /sprite/src/lib/include/dev/RCS/pfs.h,v 1.6 89/08/24 15:54:03 mendel Exp $ SPRITE (Berkeley)
 */

#ifndef _PFS
#define _PFS

#include "dev/pdev.h"

#ifdef KERNEL
#include "fsNameOps.h"
#else
#include <kernel/fsNameOps.h>
#endif

/*
 * Pseudo-filesystem pathname operations
 *	PFS_OPEN		Open a file in the pseudo-filesystem.
 *	PFS_GET_ATTR		Get attributes given a pathname in a pfs.
 *	PFS_SET_ATTR		Set attributes given a pathname in a pfs.
 *	PFS_MAKE_DEVICE		Make a device
 *	PFS_MAKE_DIR		Make a directory
 *	PFS_REMOVE		Remove a file
 *	PFS_REMOVE_DIR		Remove a directory
 *	PFS_RENAME		Rename a file
 *	PFS_HARD_LINK		Make a hard link between two files.
 *	PFS_DOMAIN_INFO		Return information about the pseudo-file-system
 */

#define PFS_OPEN		10
#define PFS_GET_ATTR		11
#define PFS_SET_ATTR		12
#define PFS_MAKE_DEVICE		13
#define PFS_MAKE_DIR		14
#define PFS_REMOVE		15
#define PFS_REMOVE_DIR		16
#define PFS_RENAME		17
#define PFS_HARD_LINK		18
#define PFS_SYM_LINK		19
#define PFS_DOMAIN_INFO		20

/*
 * When a naming operation is done in a pseudo-filesystem the pfs server
 * gets a message of the following format.  Data, usually a single pathname,
 * follows this message/parameter block.  The results of most of these
 * operations is only an error status and there are no associated typedefs.
 * Exceptions are GET_ATTR, which returns a Fs_GetAttributes struct,
 * OPEN, and DOMAIN_INFO.  See IOC_PFS_OPEN or IOC_PFS_PASS_STREAM for an
 * explaination of how opens are handled.  The DOMAIN_INFO operation takes
 * the fileID that is the handle on the top-level directory (prefix) of
 * the domain and the return value is a Fs_DomainInfo struct.
 */

typedef struct {
    Pdev_RequestHdr	hdr;	/* with PFS_REQUEST_MAGIC */
    union {			/* Additional parameters to the operation. */
	Fs_OpenArgs		open;
	Fs_OpenArgs		getAttr;
	Fs_OpenArgs		setAttr;
	Fs_MakeDeviceArgs	makeDevice;
	Fs_OpenArgs		makeDir;
	Fs_LookupArgs		remove;
	Fs_LookupArgs		removeDir;
	Fs_2PathParams		rename;
	Fs_2PathParams		hardLink;
	Fs_OpenArgs		symLink;
	Fs_FileID		domainInfo;
    } param;
} Pfs_Request;

#define PFS_REQUEST_MAGIC	0x73657C6B

/*
 * When a PFS_SET_ATTR operation is done the data that follows the message hdr
 * has the following format.  The file name comes at the end and its true
 * length is given in the nameLength field.
 */

typedef struct {
    Fs_Attributes	attr;		/* Attribute values */
    int			flags;		/* Indicates which attributes to set */
    int			nameLength;	/* Number of bytes in name */
    char		name[4];	/* Actually larger */
} Pfs_SetAttrData;

/*
 * I/O Controls specific to pseudo-filesystem servers
 *	IOC_PFS_OPEN		This is used instead of IOC_PDEV_REPLY to
 *				respond to a PFS_OPEN request.  This reply
 *				causes a new pseudo-device connection between
 *				the server and the opening client.  The result
 *				of this I/O control is a new streamID for
 *				the server's half of the connection.  Unlike
 *				pseudo-device servers, there is no need to
 *				read a control stream to get this streamID,
 *				it is returned as a result of this IOC.
 *	IOC_PFS_SET_ID		This is used to set the file ID associated
 *				with a connection to a pseudo-filesystem server.
 *				There will be a naming connection, and perhaps
 *				many more pdev-like connections that represent
 *				files.  The ID set by this call we be passed in
 *				as the 'prefixID' and 'rootID' fields of
 *				the arguments to naming operations.  If this
 *				call is not used then the kernel's own
 *				fileIDs (which are unique, anyway) are used.
 *	IOC_PFS_PASS_STREAM	This is used instead of IOC_PDEV_REPLY to
 *				respond to a PFS_OPEN request.  This reply
 *				passes the stream indicated by the input
 *				streamID to the client as the result of its
 *				open request.  Thus the pseudo-filesystem
 *				server can open a file/device/whatever on
 *				behalf of its client and return the open file.
 *				The pseudo-filesystem server won't see any
 *				more I/O operations, or the close.
 */

#define IOC_PFS			(6 << 16)
#define IOC_PFS_OPEN		(IOC_PFS | 0x1)
#define IOC_PFS_SET_ID		(IOC_PFS | 0x2)
#define IOC_PFS_PASS_STREAM	(IOC_PFS | 0x3)

#endif _PFS
