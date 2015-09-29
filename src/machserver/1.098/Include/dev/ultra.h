/*
 * ultra.h --
 *
 *	Declarations for ioctls specific to the Ultranet VME adapter.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/ultra.h,v 1.4 90/10/19 15:51:49 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _ULTRA
#define _ULTRA

#define IOC_ULTRA	(14 << 16)

/*
 * Ioctl definitions specific to the Ultranet device.
 */

#define IOC_ULTRA_SET_FLAGS		(IOC_ULTRA | 0x1)
#define IOC_ULTRA_RESET_FLAGS		(IOC_ULTRA | 0x2)
#define IOC_ULTRA_GET_FLAGS		(IOC_ULTRA | 0x3)
#define IOC_ULTRA_RESET			(IOC_ULTRA | 0x4)
#define IOC_ULTRA_CLR			(IOC_ULTRA | 0x5)
#define IOC_ULTRA_INT			(IOC_ULTRA | 0x6)
#define IOC_ULTRA_WFI			(IOC_ULTRA | 0x7)
#define IOC_ULTRA_LOAD			(IOC_ULTRA | 0x8)
#define IOC_ULTRA_GO			(IOC_ULTRA | 0x9)
#define IOC_ULTRA_EXTENDED_DIAG		(IOC_ULTRA | 0xa)
#define IOC_ULTRA_DIAG			(IOC_ULTRA | 0xb)
#define IOC_ULTRA_DUMP			(IOC_ULTRA | 0xc)
#define IOC_ULTRA_GET_ADAP_INFO		(IOC_ULTRA | 0xd)
#define IOC_ULTRA_DEBUG			(IOC_ULTRA | 0xe)
#define IOC_ULTRA_INIT			(IOC_ULTRA | 0xf)
#define IOC_ULTRA_START			(IOC_ULTRA | 0x10)
#define IOC_ULTRA_ADDRESS		(IOC_ULTRA | 0x11)
#define IOC_ULTRA_SEND_DGRAM		(IOC_ULTRA | 0x12)
#define IOC_ULTRA_ECHO			(IOC_ULTRA | 0x13)
#define IOC_ULTRA_TRACE			(IOC_ULTRA | 0x14)
#define IOC_ULTRA_SOURCE		(IOC_ULTRA | 0x15)
#define IOC_ULTRA_SINK			(IOC_ULTRA | 0x16)
#define IOC_ULTRA_COLLECT_STATS		(IOC_ULTRA | 0x17)
#define IOC_ULTRA_CLEAR_STATS		(IOC_ULTRA | 0x18)
#define IOC_ULTRA_GET_STATS		(IOC_ULTRA | 0x19)
#define IOC_ULTRA_MAP_THRESHOLD		(IOC_ULTRA | 0x1a)
#define IOC_ULTRA_BCOPY_TEST		(IOC_ULTRA | 0x1b)
#define IOC_ULTRA_SG_BCOPY_TEST		(IOC_ULTRA | 0x1c)
#define IOC_ULTRA_HARD_RESET		(IOC_ULTRA | 0x1d)
/*
 * IOC_ULTRA_LOAD_BLOCK parameter.
 */

typedef struct Dev_UltraLoadBlock {
    int		length;			/* Length of the data. */
    int 	address;			/* Load address. */
    char 	buffer[IOC_MAX_BYTES - 2 * sizeof(int)]; /* The data. */
} Dev_UltraLoadBlock;

/*
 * IOC_ULTRA_GO parameter. 
 */

typedef struct Dev_UltraGo {
    int		address;		/* Starting address. */
} Dev_UltraGo;

/* 
 * IOC_ULTRA_DIAG parameter.
 */

typedef struct Dev_UltraDiag {
    int		version;		/* Firmware version. */
    int		error;			/* 0 if tests passed, otherwise
					 * the number of the test that
					 * failed. */
    int		hwModel;		/* Hardware model. */
    int		hwVersion;		/* Hardware version. */
    int		hwRevision;		/* Hardware revision. */
    int		hwOption;		/* Hardware option. */
    int		hwSerial;		/* Hardware serial number. */
} Dev_UltraDiag;

/*
 * IOC_ULTRA_GET_ADAP_INFO parameter. This is the same as IOC_ULTRA_DIAG,
 * except that the 'error' field isn't used because diagnostics aren't
 * run.
 */

typedef Dev_UltraDiag Dev_UltraAdapterInfo;

/*
 * IOC_ULTRA_EXTENDED_DIAG parameter.
 */

typedef struct Dev_UltraExtendedDiag {
    int		version;		/* Firmware version. */
    int		error;			/* 0 if tests passed, otherwise
					 * the number of the test that
					 * failed. */
    Boolean	externalLoopback;	/* TRUE => use external loopback
					 * (there had better be a loopback
					 * connector attached to the
					 * adapter!) */
} Dev_UltraExtendedDiag;

/*
 * IOC_ULTRA_LOAD parameter.
 */

typedef struct Dev_UltraLoad {
    int		address;	/* Load address. */
    int		length;		/* Length of the block. */
    char	data[IOC_MAX_BYTES - 2 * sizeof(int)];	/* The block to load. */
} Dev_UltraLoad;

/*
 * IOC_ULTRA_SEND_DGRAM parameter.
 */

typedef struct Dev_UltraSendDgram {
    Net_Address		address;
    int			count;
    Time		time;
    int			size;
    Boolean		useBuffer;
    char		buffer[100];
} Dev_UltraSendDgram;

/*
 * IOC_ULTRA_ECHO parameter.
 */

typedef struct Dev_UltraEcho {
    Boolean		echo;	/* TRUE => echo received datagrams back
				 * to sender. */
} Dev_UltraEcho;

typedef struct Dev_UltraSink {
    int		packets;
    Time	time;
} Dev_UltraSink;

typedef struct Dev_UltraStats {
    int		packetsSent;
    int		bytesSent;
    int		sentHistogram[33];
    int		packetsReceived;
    int		bytesReceived;
    int		receivedHistogram[33];
} Dev_UltraStats;    

#endif /* _ULTRA */

