/*
 * devNet.h --
 *
 *	This defines the interface to the file system net device.
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
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVNET
#define _DEVNET

#include "sprite.h"
#include "user/fs.h"
#include "fs.h"

/*
 * These macros are used to access the information in the unit of the
 * device. The NetType is used to determine the type of the network
 * interface.  See net.h for definitions.  The Number is used to determine
 * which interface to open.  Interfaces of the same type are numbered
 * consecutively as they are attached, starting at 0.  The Proto information
 * is used to determine the type of packets the device handles.  See
 * below.
 */

#define DEV_NET_NETTYPE_MASK		0xf000
#define DEV_NET_NETTYPE_SHIFT 		12
#define DEV_NET_NUMBER_MASK		0xf00
#define DEV_NET_NUMBER_SHIFT		8
#define DEV_NET_PROTO_MASK		0xff
#define DEV_NET_PROTO_SHIFT		0

#define DEV_NET_NETTYPE_FROM_UNIT(unit) \
    (Net_NetworkType) ((DEV_NET_NETTYPE_MASK & (unit)) >> DEV_NET_NETTYPE_SHIFT)

#define DEV_NET_NUMBER_FROM_UNIT(unit) \
    (int) ((DEV_NET_NUMBER_MASK & (unit)) >> DEV_NET_NUMBER_SHIFT)

#define DEV_NET_PROTO_FROM_UNIT(unit) \
    (int) ((DEV_NET_PROTO_MASK & (unit)) >> DEV_NET_PROTO_SHIFT)

/*
 * We set a high bit in the unit so we can distinguish new unit values
 * from old.
 */

#define DEV_NET_COMPAT_BIT	0x4000

/*
 * The protocol field in the device unit number is used to select which
 * packets the device handles.  The following are the legal values for
 * the protocol field. Note that the packets handled by a device is also
 * dependent on the type of network associated with the device, so it is
 * ok for the protocol values to overlap between different types of
 * networks.
 */

#define DEV_NET_PROTO_NONE	0x0	/* If the protocol field is set
					 * to this value then read/write
					 * are not supported on the device,
					 * but ioctls are. */
#define DEV_NET_PROTO_ARP	0x1	/* Arp packets. */
#define DEV_NET_PROTO_RARP	0x2	/* Rarp packets. */
#define DEV_NET_PROTO_IP	0x3	/* IP packets. */
#define DEV_NET_PROTO_DEBUG	0x4	/* Sprite debugger packets. */
#define DEV_NET_PROTO_MOP	0x5	/* MOP packets. */

#define DEV_NET_NUM_PROTO	6

/*
 * Forward routines.
 */

extern ReturnStatus DevNet_FsOpen _ARGS_((Fs_Device *devicePtr, int useFlags,
    Fs_NotifyToken data, int *flagsPtr));
extern ReturnStatus DevNet_FsReopen _ARGS_((Fs_Device *devicePtr, int refs,
    int writers, Fs_NotifyToken data, int *flagsPtr));
extern ReturnStatus DevNet_FsRead _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
extern ReturnStatus DevNet_FsWrite _ARGS_((Fs_Device *devicePtr,
    Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
extern ReturnStatus DevNet_FsClose _ARGS_((Fs_Device *devicePtr, int useFlags,
    int openCount, int writerCount));
extern ReturnStatus DevNet_FsSelect _ARGS_((Fs_Device *devicePtr, int *readPtr,
    int *writePtr, int *exceptPtr));
extern ReturnStatus DevNet_FsIOControl _ARGS_((Fs_Device *devicePtr,
    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern void DevNetEtherHandler _ARGS_((Address packetPtr, int size));

#endif /* _DEVNET */

