/*-
 * boot.h --
 *	 Header file for sprite tftp boot program
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$Header: /sprite/src/boot/netBoot.OpenProm/RCS/boot.h,v 1.3 91/01/13 02:35:53 dlong Exp $ SPRITE (Berkeley)"
 */
#ifndef _BOOT_H
#define _BOOT_H

#include <sprite.h>
#include <sys/types.h>
#include <netEther.h>
#ifndef OPENPROMS
#define OPENPROMS
#endif
#include "sunromvec.h"

extern struct sunromvec	*romp;
#define romVectorPtr	romp

#define printf	  		(*romp->v_printf)
#define millitime()		(*romp->v_nmiclock)

#if defined(sun4)

#include <netInet.h>

typedef struct {
    Net_EtherHdr	header;
    unsigned short	hardwareType;
    unsigned short	protocolType;
    unsigned char	hardwareAddrLen;
    unsigned char	protocolAddrLen;
    unsigned short	opcode;
    Net_EtherAddress	senderEtherAddr;
    unsigned char	senderProtAddr[4];
    Net_EtherAddress	targetEtherAddr;
    unsigned char	targetProtAddr[4];
} My_ArpPacket;

#define Net_ArpPacket My_ArpPacket

#define inet_cmp(x, y)		(bcmp((x), (y), 4) == 0)
#define inet_copy(x, y)		bcopy((y), (x), 4)

#else

#define inet_cmp(x, y)		((x) == (y))
#define inet_copy(x, y)		((x) = (y))

#endif

extern char *BootDevName();
extern char *BootFileName();
extern void PrintBootCommand();

#if defined(sun4c)

#define ExitToMon()		(*romp->v_exit_to_mon)();
#define CheckRomMagic()		(ROMVEC_MAGIC == RomMagic)
#define RomVersion		(romp->v_romvec_version)
#define RomMagic		(romp->v_magic)

#define DevOpen(dev) \
    (RomVersion >= 2 \
    ? (*romp->op_open)(dev) \
    : (*romp->v_open)(dev))

#define DevClose(fileId) \
    (RomVersion >= 2 \
    ? (*romp->op_close)(fileId) \
    : (*romp->v_close)(fileId))

#define xmit_packet(fileId, buf, len) \
    (RomVersion >= 2 \
    ? (*romp->op_write)(fileId, buf, len) \
    : (*romp->v_xmit_packet)(fileId, len, buf))

#define poll_packet(fileId, buf) \
    (RomVersion >= 2 \
    ? (*romp->op_read)(fileId, buf, NET_ETHER_MAX_BYTES) \
    : (*romp->v_poll_packet)(fileId, NET_ETHER_MAX_BYTES, buf))

#define reset(fileId)	/* don't know how to reset */

#else  /* sun4c */

#include "saio.h"
typedef struct saioreq SIP;

#define xmit_packet(fileId, buf, len) \
    (*(SIP *)(fileId)->si_sif->sif_xmit)((SIP *)(fileId)->si_devdata, buf, len)
#define poll_packet(fileId, buf)
    (*(SIP *)(fileId)->si_sif->sif_poll)((SIP *)(fileId)->si_devdata, buf)
#define reset(fileId)
    (*(SIP *)(fileId)->si_sif->sif_reset)((SIP *)(fileId)->si_devdata)

#endif /* sun4c */

#endif /* _BOOT_H */
