/*
 * nfs.h --
 *	Definitions for the Sprite user-level NFS interface.  A Sprite process
 *	runs as the server of a Sprite pseudo-filesystem that provides
 *	access to a remote NFS filesystem.
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
 * $Header: /sprite/src/cmds/nfsmount/RCS/nfs.h,v 1.8 91/10/20 12:38:26 mottsmth Exp $ SPRITE (Berkeley)
 */

#include "sys/time.h"
#include "rpc/rpc.h"
#include "mount.h"
#include "nfs_prot.h"
#include "errno.h"

#undef SUCCESS
#include "status.h"
#include <kernel/fs.h>
#include "pfs.h"
#include "pdev.h"

extern char myhostname[];

/*
 * Top-level state.  This has a NFS handle for the mount point,
 * and state for the naming request-response protocol with the
 * Sprite kernel.
 */
typedef struct {
    char	*host;			/* Hostname of NFS server */
    char	*nfsName;		/* NFS directory being mounted */
    char	*prefix;		/* Prefix for pseudo-filesystem */
    Pfs_Token	pfsToken;		/* Handle for Pfs library package */
    CLIENT	*mountClnt;		/* Handle for SUN RPC to mount prog. */
    nfs_fh	*mountHandle;		/* NFS handle from mount protocol */
    CLIENT	*nfsClnt;		/* Handle for SUN RPC to nfs server */
} NfsState;

/*
 * Timeout period for SUN RPC.
 */
extern struct timeval nfsTimeout;

/*
 * Open file table.
 */
typedef struct {
    nfs_fh *handlePtr;		/* NFS file handle */
    AUTH *authPtr;		/* NFS authentication */
    int openFlags;		/* Saved open flags so we can remember
				 * to do append-mode writes */
} NfsOpenFile;

extern NfsOpenFile **nfsFileTable;
extern NfsOpenFile **nextFreeSlot;
extern int nfsFileTableSize;

/*
 * Type mappings
 */
extern int nfsToSpriteFileType[];
extern int spriteToNfsModeType[];

/*
 * Internal types needed to differentiate between the NFS root,
 * directories, regular files, and links.  The lookup routines have to
 * recognize the root, and the read routine has to recognize directories,
 * files, and links.
 * This type is kept in the type field of the fileIDs we pass to the
 * kernel each time an NFS file is opened.
 *	TYPE_ROOT has to be zero because the Sprite kernel sets up the
 *	fileID of our root to have all zero fields.
 */
#define TYPE_ROOT	0x0
#define TYPE_DIRECTORY	0x1
#define TYPE_FILE	0x2
#define TYPE_SYMLINK	0x4

/*
 * Macro to map Unix errno to Sprite int
 */
extern int nfsStatusMap[];

#define NfsStatusMap(errno) \
    ((errno >= 0 && errno < sys_nerr) ? nfsStatusMap[errno] : errno)

/*
 * Write-behind flag for testing.
 */
extern int nfs_PdevWriteBehind;

extern CLIENT *Nfs_MountInitClient();
extern void Nfs_MountTest();
extern void Nfs_MountDump();
extern void Nfs_Unmount();

extern CLIENT *Nfs_InitClient();
extern Pfs_CallBacks nfsNameService;
extern Pdev_CallBacks nfsFileService;
extern int BadProc();

extern int NfsOpen();
extern int NfsClose();
extern int NfsRead();
extern int NfsWrite();
extern int NfsIoctl();
extern int NfsGetAttrStream();
extern int NfsSetAttrStream();
extern int NfsGetAttrPath();
extern int NfsSetAttrPath();
extern int NfsMakeDevice();
extern int NfsMakeDir();
extern int NfsRemove();
extern int NfsRemoveDir();
extern int NfsRename();
extern int NfsHardLink();
extern int NfsSymLink();
extern int NfsDomainInfo();

extern void Nfs_UnmountAll();
extern void Nfs_Exports();
extern nfs_fh *Nfs_Mount();

extern int NfsRecordMountPointProc();

/*
 * Attribute handling.
 */
extern void NfsToSpriteAttr();
extern void SpriteToNfsAttr();
extern void NfsToSpriteDirectory();
extern void NfsFindCookie();

