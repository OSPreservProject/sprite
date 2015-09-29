/*
 * fddi.h --
 *
 *	Declarations for ioctls specific to the DEC FDDI device.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/fddi.h,v 1.2 92/06/08 14:51:58 voelker Exp $ SPRITE (Berkeley)
 */

#ifndef _FDDI_IOC
#define _FDDI_IOC

#define IOC_FDDI	(21 << 16)

/*
 * Ioctl definitions specific to the FDDI device.
 */

#define IOC_FDDI_RESET                 (IOC_FDDI | 0x1)
#define IOC_FDDI_DEBUG                 (IOC_FDDI | 0x2)  /* Toggle debug */
#define IOC_FDDI_SEND_PACKET           (IOC_FDDI | 0x3)
#define IOC_FDDI_REG_CONTENTS          (IOC_FDDI | 0x4)
#define IOC_FDDI_ERR_LOG               (IOC_FDDI | 0x5)
#define IOC_FDDI_FLUSH_XMT_Q           (IOC_FDDI | 0x6)
#define IOC_FDDI_ADDRESS               (IOC_FDDI | 0x7)
#define IOC_FDDI_PRINT_DEBUG_RING      (IOC_FDDI | 0x8)
#define IOC_FDDI_RPC_ECHO              (IOC_FDDI | 0x9)
#define IOC_FDDI_HALT                  (IOC_FDDI | 0xA)
#define IOC_FDDI_SOURCE                (IOC_FDDI | 0xB)
#define IOC_FDDI_STATS                 (IOC_FDDI | 0xC)

/*
 * IOC_FDDI_SEND_PACKET input parameter.
 */
typedef struct Dev_FDDISendPacket {
    char 	buffer[IOC_MAX_BYTES - sizeof(int) - sizeof(Net_Address)]; 
                                        /* The data. */
    int		length;			/* Length of the data. */
    Net_FDDIAddress dest;
} Dev_FDDISendPacket;

/*
 * IOC_FDDI_REG_CONTENTS output parameter.
 */
typedef struct Dev_FDDIRegContents {
    unsigned short regReset;
    unsigned short regCtrlA;
    unsigned short regCtrlB;
    unsigned short regStatus;
    unsigned short regEvent;
    unsigned short regMask;
} Dev_FDDIRegContents;

/*
 * IOC_FDDI_ERR_LOG output parameter.
 */
typedef struct Dev_FDDIErrLog {
    unsigned long internal;      /* Internal Error Code */
    unsigned long external;      /* External Error Code (should match error
				  * code in the STATUS register */
} Dev_FDDIErrLog;

/*
 * IOC_FDDI_ADDRESS output parameter.  The link address of the
 * adapter.
 */
typedef struct Dev_FDDILinkAddr {
    Net_FDDIAddress source;
} Dev_FDDILinkAddr;

/*
 * IOC_FDDI_RPC_ECHO input parameter.
 */
typedef struct Dev_FDDIRpcEcho {
    unsigned long packetSize;
    unsigned long numEchoes;
    unsigned long serverID;
    Boolean       printSyslog;
} Dev_FDDIRpcEcho;

/*
 * IOC_FDDI_RPC_ECHO output parameter.
 */
typedef struct Dev_FDDIRpcEchoReturn {
    Time rpcTime;
} Dev_FDDIRpcEchoReturn;

/*
 * IOC_FDDI_SEND_PACKET input parameter.
 */
typedef struct Dev_FDDISourcePacket {
    int		length;			/* Length of the data. */
    Time        time;
    Net_FDDIAddress dest;
} Dev_FDDISourcePacket;

/*
 * IOC_FDDI_STATS output structure.  It should look very similar
 * to the Net_FDDIStats structure.
 */
typedef struct Dev_FDDIStats {
    int		packetsSent;		/* Number of packets sent. */
    int		bytesSent;	        /* Number of bytes sent. */
    int         transmitHistogram[NET_FDDI_STATS_HISTO_NUM];
                                        /* Histogram of packet sizes sent */
    int		packetsReceived;	/* Number of packets received. */
    int		bytesReceived;		/* Number of bytes received. */
    int         receiveHistogram[NET_FDDI_STATS_HISTO_NUM];
                                        /* Histogram of rcved packet sizes */
    int         receiveReaped[NET_FDDI_STATS_RCV_REAPED];
                                        /* Number of packets reaped per
					 * receive interrupt */
    int         xmtPacketsDropped;      /* Packets dropped because 
					 * of lack of transmit buffer space. */
    int         packetsQueued;          /* Number of packets written
					 * to adapter transmit buffers. */
} Dev_FDDIStats;

#endif /* _FDDI_IOC */





