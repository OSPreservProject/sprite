/*
 * netDFInt.h --
 *
 *	External definitions for the DEC 700 FDDI controller.
 *
 * The description of the definitions here come from the DEC 
 * FDDIcontroller 700 Port Specification, Revision 1.1.
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
 * $Header$
 */

#ifndef _NETDFINT
#define _NETDFINT

#include <netInt.h>
#include <mach.h>
#include <netDFMachInt.h>

typedef unsigned short Net_DFReg;

#define NET_DF_CLEAR                         0x0000

/*
 * RESET bitfields 
 */
#define NET_DF_RESET_UNDEFINED               0xFFFC
#define NET_DF_RESET_DLU_MODE                0x0002
#define NET_DF_RESET_RESET                   0x0001

/*
 * PORT_CONTROL_A bitfields
 */
#define NET_DF_CTRLA_MB_PARITY_ERROR         0x8000
#define NET_DF_CTRLA_NXM_ERR                 0x4000
#define NET_DF_CTRLA_UNDEFINED               0x3F80
#define NET_DF_CTRLA_SMT_RCV_OVERFLOW        0x0040
#define NET_DF_CTRLA_FLUSH_DONE              0x0020
#define NET_DF_CTRLA_SHUT                    0x0010
#define NET_DF_CTRLA_HALT                    0x0008
#define NET_DF_CTRLA_CMD_POLL_DEMAND         0x0004
#define NET_DF_CTRLA_SMT_RCV_POLL_DEMAND     0x0002
#define NET_DF_CTRLA_XMT_POLL_DEMAND         0x0001

/*
 * PORT_CONTROL_B bitfields
 */
#define NET_DF_CTRLB_UNDEFINED               0xFFFC
#define NET_DF_CTRLB_CONSOLE_MODE            0x0002
#define NET_DF_CTRLB_DRIVER_MODE             0x0001

/*
 * PORT_INTERRUPT_EVENT bitfields
 */
#define NET_DF_EVENT_UNDEFINED               0xF000
#define NET_DF_EVENT_DLU_DONE                0x0800
#define NET_DF_EVENT_FLUSH_TX                0x0400
#define NET_DF_EVENT_PM_PARITY_ERROR         0x0200
#define NET_DF_EVENT_MB_PARITY_ERROR         0x0100
#define NET_DF_EVENT_NXM_ERR                 0x0080
#define NET_DF_EVENT_LINK_STATUS_CHANGE      0x0040
#define NET_DF_EVENT_STATE_CHANGE            0x0020
#define NET_DF_EVENT_UNS_POLL_DEMAND         0x0010
#define NET_DF_EVENT_CMD_DONE                0x0008
#define NET_DF_EVENT_SMT_XMT_POLL_DEMAND     0x0004
#define NET_DF_EVENT_RCV_POLL_DEMAND         0x0002
#define NET_DF_EVENT_XMT_PKT_DONE            0x0001

/*
 * PORT_STATUS bitfields
 */
#define NET_DF_STATUS_UNDEFINED              0xC000
#define NET_DF_STATUS_DLU_STATUS             0x3000
#define NET_DF_STATUS_LINK_STATUS            0x0800
#define NET_DF_STATUS_ADAPTER_STATE          0x0700
#define NET_DF_STATUS_TEST_HALT_ID           0x00FF

/*
 * Definitions of the LINK_STATUS field.
 */
#define NET_DF_LINK_UNAVAILABLE              0x0000
#define NET_DF_LINK_AVAILABLE                0x0800


/*
 * States enumerating the NET_DF_STATUS_ADAPTER_STATE field
 */
#define NET_DF_STATE_RESETTING               0x0000
#define NET_DF_STATE_UNINITIALIZED           0x0100
#define NET_DF_STATE_INITIALIZED             0x0200
#define NET_DF_STATE_RUNNING                 0x0300
#define NET_DF_STATE_MAINTENANCE             0x0400
#define NET_DF_STATE_HALTED                  0x0500
#define NET_DF_STATE_UNDEFINED1              0x0600
#define NET_DF_STATE_UNDEFINED2              0x0700

#define NET_DF_DISABLE_ALL_INT(S)  ((S) = 0x0000)
#define NET_DF_ENABLE_ALL_INT(S)   ((S) = 0xFFFF)
#define NET_DF_CLEAR_ALL_EVENTS(S)   ((S) = 0xFFFF)

/*
 * Command structures used on the COMMAND ring to configure the FDDI
 * adapter.  The COMMAND RING uses command descriptors located on
 * the adapter:
 *
 *    31                   0
 *    +--------------------+
 *    | Host Own | Command |
 *    +--------------------+
 *    |   Command Status   |
 *    +--------------------+
 *    |   Buffer Address   |
 *    +--------------------+
 *    |      Reserved      |
 *    +--------------------+
 *
 * The following command structures are used as command buffers:
 *
 *    INIT: Configure packet memory, # HOST RCV ring entries, and
 *          initialize counters maintained by the adapter.  Using this
 *          command will change the state of the adapter from 
 *          UNINITIALIZED to INITIALIZED.
 *  MODCAM: Configure the CAM chip for address recognition.
 *   PARAM: Configure Loop Back Mode and default Link Characterists
 *          returned from the successful execution of an INIT command.
 *          The adapter will transition to RUNNING, and will connect
 *          to the FDDI ring.
 */

/*
 * Types used in command structures:
 *
 *     NET_DF_ULONG: unsigned 32 bit integer
 *     NET_DF_TIMER: unsigned 32 bit integer in increments of 80ns
 *       NET_DF_PMD: unsigned 32 bit integer
 *   NET_DF_SMT_VER: unsigned 32 bit version number --  Version 6 SMT =
 *                      +----+----+----+----+
 *            offset 0  | 00 | 00 | 00 | 06 |
 *                      +----+----+----+----+
 *      NET_DF_BOOL: unsigned 32 bit integer
 * NET_DF_LOOP_MODE: unsigned 32 bit integer
 *      NET_DF_ADDR: 8 bytes representing a network address, of which
 *                   6 are only used.  The address 08-00-2B-01-02-03
 *                   would be:
 *                      +----+----+----+----+
 *            offset 0  | 01 | 2B | 00 | 08 |
 *                      +----+----+----+----+
 *                      +----+----+----+----+
 *            offset 4  | 00 | 00 | 03 | 02 |
 *                      +----+----+----+----+
 *   NET_DF_VERSION: ROM and FW version numbers represented as ASCII
 *                   characters.  Version 1.0A would be:
 *                      +----+----+----+----+
 *            offset 0  | 40 | 30 | 2E | 31 |
 *                      +----+----+----+----+
 *     NET_DF_COUNT: unsigned 64 bit quantity using 8 consecutive bytes.
 *                   The hex value 0x0011223344556677 would be:
 *                      +----+----+----+----+
 *            offset 0  | 00 | 01 | 02 | 03 |
 *                      +----+----+----+----+
 *                      +----+----+----+----+
 *            offset 4  | 04 | 05 | 06 | 07 |
 *                      +----+----+----+----+
 */

typedef unsigned long NET_DF_ULONG;
typedef unsigned long NET_DF_TIMER;
typedef unsigned long NET_DF_PMD;
typedef unsigned long NET_DF_SMT_VER;
typedef unsigned long NET_DF_BOOL;

#define NET_DF_FALSE                 0x0
#define NET_DF_TRUE                  0x1

/*
 * NET_DF_NO_LOOP is used for normal operation.
 */
typedef unsigned long NET_DF_LOOP_MODE;
#define NET_DF_NO_LOOP               0x0
#define NET_DF_INTERNAL_LOOPBACK     0x1
#define NET_DF_EXTERNAL_LOOPBACK     0x2

typedef char NET_DF_ADDR[8];
typedef char NET_DF_VERSION[4];

typedef struct NET_DF_COUNT {
    unsigned long count1;
    unsigned long count2;
} NET_DF_COUNT;

/*
 * Number of command descriptors and size of command buffers.
 */
#define NET_DF_NUM_COMMAND_DESC      64
#define NET_DF_COMMAND_BUF_SIZE      512
/*
 * A command descriptor.
 */
typedef struct NetDFCommandDesc {
    NET_DF_ULONG    command;   /* [31] Host own; [30-0] The command type */
    NET_DF_ULONG    status;    /* Status of command */
    NET_DF_ULONG    bufAddr;   /* Address of command buffer */
    NET_DF_ULONG    reserved; 
} NetDFCommandDesc;


/*
 * Buffers used with command descriptors.  They are type cast to
 * the structure appropriate for the command being used 
 * (e.g. NetDFInitCommand), and must be NET_DF_COMMAND_BUF_SIZE bytes long.
 */
typedef char NetDFCommandBuf;

#define NET_DF_OWN                   0x80000000
#define NET_DF_HOST_OWN              0x80000000
#define NET_DF_ADAPTER_OWN           0x00000000

#define NET_DF_COMMAND_NOP           0x00000000
#define NET_DF_COMMAND_INIT          0x00000001
#define NET_DF_COMMAND_MODCAM        0x00000002
#define NET_DF_COMMAND_PARAM         0x00000003
#define NET_DF_COMMAND_MODPROM       0x00000004
#define NET_DF_COMMAND_SETCHAR       0x00000005
#define NET_DF_COMMAND_RDCNTR        0x00000006
#define NET_DF_COMMAND_STATUS        0x00000007
#define NET_DF_COMMAND_RDCAM         0x00000008

#define NET_DF_COMMAND_MASK          0x0000000F

#define NET_DF_COMMAND_STATUS_OK     0x00000000

/* 
 * Location of the COMMAND ring in adapter memory.
 */
#define NET_DF_COMMAND_RING_OFFSET   NET_DF_MACH_COMMAND_RING_OFFSET


/*
 * INIT command values supplied by the driver (us).  
 */
#define NET_DF_INIT_TRANSMIT_MODE    0x0
#define NET_DF_INIT_HOST_RCV_ENTRIES 0x20
#define NET_DF_INIT_COUNTER_VALUE    0x0

/*
 * INIT command structure.
 */
typedef struct NetDFInitCommand {
    NET_DF_ULONG    transmitMode;    /*
				      * setting  #RMC XMT  #RMC RCV
				      *   0         512       1024
				      *   1        1024        512
				      */
    NET_DF_ULONG    rcvEntries;      /* # Host RCV entries (2-256) */
    NET_DF_COUNT    sysBufUnavail;   /* System Buffer Unavailable Counter */
    NET_DF_COUNT    xmtUnder;        /* Transmit Underruns */
    NET_DF_COUNT    xmtFail;         /* Transmit Failures */
    NET_DF_COUNT    rcvDataOver;     /* Receive Data Overruns */ 
    NET_DF_COUNT    frameCnt;        /* Frame Count */
    NET_DF_COUNT    errorCnt;        /* Error Count */
    NET_DF_COUNT    lostCnt;         /* Lost Count */
    NET_DF_COUNT    ringInitStart;   /* Ring Initialization Initiated */
    NET_DF_COUNT    ringInitRcvd;    /* Ring Initialization Received */
    NET_DF_COUNT    ringBeaconStart; /* Ring Beacon Initiated */
    NET_DF_COUNT    dupAddrTestFail; /* Duplicate Address Test Failures */
    NET_DF_COUNT    dupTokenDetect;  /* Duplicate Token Detected */
    NET_DF_COUNT    ringPurgeErr;    /* Ring Purge Errors */
    NET_DF_COUNT    bridgeStripErr;  /* Bridge Strip Errors */
    NET_DF_COUNT    tracesStart;     /* Traces Initiated */
    NET_DF_COUNT    tracesRcvd;      /* Traces Received */
    NET_DF_COUNT    lemReject;       /* LEM Rejects */
    NET_DF_COUNT    tneExpReject;    /* TNE Exp Rejects */
    NET_DF_COUNT    lemEvents;       /* LEM Events */
    NET_DF_COUNT    lctRejects;      /* LCT Rejects */
    NET_DF_COUNT    connectComplete; /* Connections Completed */
    NET_DF_COUNT    elastBufErr;     /* Elasticity Buffer Errors */
    NET_DF_VERSION  pmcRevision;     /* NOT DOCUMENTED */
    NET_DF_VERSION  romRevison;      /* ROM Revision # */
    NET_DF_VERSION  fwRevison;       /* Firmware Revision # */
    NET_DF_ULONG    mopDeviceType;   /* MOP Device Type */
    NET_DF_ULONG    hostRcvBase;     /* Base Addr of Host RCV Descriptor */
    NET_DF_ULONG    rmcXmtBase;      /* Base Addr of RMC XMT Descriptor */
    NET_DF_ULONG    rmcXmtEntries;   /* Number of RMC XMT Descriptor Entries */
    NET_DF_ULONG    smtXmtBase;      /* Base Addr of SMT XMT Descriptor */
    NET_DF_ULONG    smtXmtEntries;   /* Number of SMT XMT Descriptor Entries */
    NET_DF_ULONG    smtRcvBase;      /* Base Addr of SMT RCV Descriptor */
    NET_DF_ULONG    smtRcvEntries;   /* Number of SMT RCV Descriptor Entries */
    NET_DF_ADDR     linkAddress;     /* Link Address */
    NET_DF_TIMER    defaultTReq;     /* Default T_REQ */
    NET_DF_TIMER    defaultTvx;      /* Default TVX */
    NET_DF_TIMER    defaultTMax;     /* NOT DOCUMENTED */
    NET_DF_ULONG    lemThresh;       /* NOT DOCUMENTED */
    NET_DF_COUNT    stationID;       /* NOT DOCUMENTED */
    NET_DF_ULONG    reserved6;       /* The following below may be outdated */
    NET_DF_SMT_VER  smtVersionID;    /* SMT Verison ID */
    NET_DF_TIMER    tokenTimeout;    /* Restricted Token Timeout */
    NET_DF_BOOL     ringPurgeEnable; /* Ring Purger Enable */
    NET_DF_SMT_VER  smtMaxVersionID; /* SMT Maximum Version ID */
    NET_DF_SMT_VER  smtMinVersionID; /* SMT MinimumVersion ID */
    NET_DF_PMD      pmdType;         /* PMD Type */
} NetDFInitCommand;

/*
 * A MODCAM command structure is just 64 NET_DF_ADDRs
 */
typedef NET_DF_ADDR NetDFModcamCommand;

/*
 * CAM addresses that must be loaded:
 *
 * Ring Purger Multicast:       09-00-2b-02-01-05
 * Directed Beacon Multicast:   01-80-c2-00-01-00
 *
 * The adapter memory does not seem to be byte-writeable (although
 * it is byte-readable), so load the entries by words.  This is
 * probably not machine independent, but then the DEC FDDI will probably
 * only run on DECstations...(an excuse)
 */

#define NET_DF_MODCAM_RING_PURGE_LOW     0x022b0009
#define NET_DF_MODCAM_RING_PURGE_HIGH    0x00000501
#define NET_DF_MODCAM_BEACON_LOW         0x00c28001
#define NET_DF_MODCAM_BEACON_HIGH        0x00000001


/*
 * A RDCAM command structure is just 64 NET_DF_ADDRs
 */
typedef NET_DF_ADDR NetDFRdcamCommand;

/*
 * A MODPROM command structure.
 */
typedef struct NetDFModpromCommand {
    NET_DF_BOOL      llcPromEnable;   /* LLC Promiscuous Enable */
    NET_DF_BOOL      smtPromEnable;   /* SMT Promiscuous Enable */
    NET_DF_BOOL      llcMultiEnable;  /* LLC Multicast Promiscuous Enable */
    NET_DF_BOOL      llcBroadEnable;  /* LLC Broadcast Promiscuous Enable */
} NetDFModpromCommand;

/*
 * PARAM command structure.
 */
typedef struct NetDFParamCommand {
    NET_DF_LOOP_MODE loopMode;        /* Loop Back Mode */
    NET_DF_ULONG     tMax;            /* NOT DOCUMENTED */
    NET_DF_TIMER     tReq;            /* Requested token rotation timer */
    NET_DF_TIMER     tvx;             /* Valid transmission time */
    NET_DF_ULONG     lemThresh;       /* NOT DOCUMENTED */
    NET_DF_COUNT     stationID;       /* NOT DOCUMENTED */
    NET_DF_TIMER     tokenTimeout;    /* Restricted token timeout */
    NET_DF_BOOL      ringPurgeEnable; /* Ring Purger Enable */
} NetDFParamCommand;

/*
 * A STATUS command structure.
 */
typedef struct NetDFStatusCommand {
    NET_DF_ULONG     ledState;        /* State of the LED in the back */
    NET_DF_ULONG     reserved1;
    NET_DF_ULONG     linkState;       /* State of the link to the ring */
    NET_DF_ULONG     dupAddrFlag;     /* Duplicate Address Flag */
    NET_DF_ULONG     ringPurgeState;  /* Ring Purger State */
    NET_DF_TIMER     negTrt;          /* Negotiated Target Rotation Time */
    NET_DF_ADDR      upNeighborAddr;  /* Upstream Neighbor Address */
    NET_DF_ULONG     unaTimedout;     /* UNA Timed Out */
    NET_DF_ULONG     frameStrip;      /* Frame Strip Mode */
    NET_DF_ULONG     claimToken;      /* Claim Token Yield Mode */
    NET_DF_ULONG     phyState;        /* Phy State */
} NetDFStatusCommand;

/*
 * Host RCV descriptor and buffer.  The DEC FDDI adapter transfers data
 * from the ring into these buffers, which are referenced by the
 * descriptors.  FDDI packets can have a size of 4.5K, and the DEC
 * adapter requires that the host split the buffer up into two buffers,
 * one of size 4K and the other 0.5K.  The Host RCV descpritor points to
 * these two buffers, in addition to the own bit.
 */
typedef struct NetDFHostRcvDesc {
    unsigned long *bufAPtr;    /* [31] = host own; [30-25] = reserved;
			        * [24-0] = (4K buffer address >> 9) */
    unsigned long *bufBPtr;    /* [31-25] = reserved;
			        * [24-0] = (0.5K buffer address >> 9) */
    unsigned long rmcRcvDesc;  /* RMC RCV descriptor */
    unsigned long reserved;
} NetDFHostRcvDesc;

/*
 * The manual says the lower 25 bits form the address, but its picture
 * says 22 bits...which is all we need anyways.
 */
#define NET_DF_HOST_RCV_BUFA_MASK    0x007FFFFF
#define NET_DF_HOST_RCV_BUFB_MASK    0x007FFFFF

#define NET_DF_HOST_RCV_BUF_SIZE     4608
#define NET_DF_HOST_RCV_BUFA_SIZE    4096
#define NET_DF_HOST_RCV_BUFB_SIZE    512
/*
 * RMC RCV descriptor fields
 */
#define NET_DF_RMC_RCV_SOP           0x80000000
#define NET_DF_RMC_RCV_EOP           0x40000000
#define NET_DF_RMC_RCV_FSC           0x38000000
#define NET_DF_RMC_RCV_FSB           0x07C00000
#define NET_DF_RMC_RCV_BAD           0x00200000
#define NET_DF_RMC_RCV_CRC           0x00100000
#define NET_DF_RMC_RCV_RRR           0x000E0000  /* MAC receive status */
#define NET_DF_RMC_RCV_DD            0x00018000
#define NET_DF_RMC_RCV_SS            0x00006000
#define NET_DF_RMC_RCV_PBC           0x00001FFF
/*
 * RRR (MAC receive status) definitions
 */
#define NET_DF_RRR_MAC_RESET         0x000E0000
#define NET_DF_RRR_FORMAT_ERROR      0x000C0000
#define NET_DF_RRR_FRAGMENT          0x000A0000
#define NET_DF_RRR_INV_LENGTH        0x00080000
#define NET_DF_RRR_RMC_ABORT         0x00060000
#define NET_DF_RRR_DADDR_UNMATCH     0x00040000
#define NET_DF_RRR_SADDR_MATCH       0x00020000
#define NET_DF_RRR_NORMAL            0x00000000
/*
 * DD (Destination Address match status) definitions
 */
#define NET_DF_DD_LOCAL              0x00018000
#define NET_DF_DD_CAM                0x00010000
#define NET_DF_DD_PROM               0x00008000
#define NET_DF_DD_NONE               0x00000000
/*
 * SS (Source Address match status) definitions.  These are only
 * valid when the adapter is in loopback mode.
 */
#define NET_DF_SS_LOCAL              0x00006000
#define NET_DF_SS_CAM                0x00004000
#define NET_DF_SS_ALIAS              0x00002000
#define NET_DF_SS_NONE               0x00000000

typedef struct NetDFHostRcvBuf {
    unsigned char bufA[NET_DF_HOST_RCV_BUFA_SIZE];
    unsigned char bufB[NET_DF_HOST_RCV_BUFB_SIZE];
} NetDFHostRcvBuf;

#ifdef NET_DF_USE_UNCACHED_MEM
#undef NET_DF_USE_UNCACHED_MEM
#endif 
/*
 * Remove to have the receive buffers cached.
 *
 * #define NET_DF_USE_UNCACHED_MEM
 */

/*
 * The number of HOST RCV entries
 */
#define NET_DF_NUM_HOST_RCV_ENTRIES  NET_DF_INIT_HOST_RCV_ENTRIES

/*
 * SMT RCV ring descriptor and buffer definitions
 */
typedef struct NetDFSmtRcvDesc {
    unsigned long own;         /* [31] = 0 adapter own; = 1 host own
				* [30-0] reserved */
    unsigned long rmcRcvDesc;  /* RMC RCV Descriptor */
    unsigned long bufAddr;    /* Address of buffer */
    unsigned long reserved;  
} NetDFSmtRcvDesc;

#define NET_DF_SMT_RCV_BUF_SIZE      4608
typedef char NetDFSmtRcvBuf[NET_DF_SMT_RCV_BUF_SIZE];

/*
 * UNSOLICITED ring descriptor definition.
 */
typedef struct NetDFUnsolDesc {
    unsigned long own;         /* [31] = 0 adapter own; = 1 host own
				* [30-0] reserved */
    unsigned long eventID;     /* Unsolicited event ID */
    unsigned long bufAddr;     /* Address of event buffer */
    unsigned long reserved;
} NetDFUnsolDesc;

#define NET_DF_UNSOL_UNDEFINED       0x0
#define NET_DF_UNSOL_RING_INIT_INIT  0x1
#define NET_DF_UNSOL_RING_INIT_RCV   0x2
#define NET_DF_UNSOL_BEACON_INIT     0x3
#define NET_DF_UNSOL_DUP_ADDR        0x4
#define NET_DF_UNSOL_DUP_TOKEN       0x5
#define NET_DF_UNSOL_PURGE_ERROR     0x6
#define NET_DF_UNSOL_STRIP_ERROR     0x7
#define NET_DF_UNSOL_OP_OSCILLAT     0x8
#define NET_DF_UNSOL_BEACON_RCV      0x9
#define NET_DF_UNSOL_PC_TRACE_INIT   0xA
#define NET_DF_UNSOL_PC_TRACE_RECV   0xB
#define NET_DF_UNSOL_XMT_UNDERRUN    0xC
#define NET_DF_UNSOL_XMT_FAILURE     0xD
#define NET_DF_UNSOL_RCV_OVERRUN     0xE

/*
 * Only one of the unsolicited events (Directed Beacon Received Event)
 * actually uses a buffer.
 */
typedef struct NetDFDirectedBeacon {
    NET_DF_ADDR sourceAddr;  /* Directed Beacon source address */
    NET_DF_ADDR una;         /* UNA of Directed Beacon Source */
} NetDFDirectedBeacon;

/*
 * SMT XMT ring descriptor definition.  The 4.5 Kbyte buffers associated with
 * the descriptors are on the adapter board itself.  We don't really
 * have to do much with these except transfer valid ones to the RMC XMT ring.
 */
typedef struct NetDFSmtXmtDesc {
    unsigned long own;         /* [31] = 0 adapter own; = 1 host own
				* [30-0] reserved */
    unsigned long pbc;         /* RMC XMT PBC */
    unsigned long bufAddr;     /* Address of buffer, long word boundary */
    unsigned long reserved;
} NetDFSmtXmtDesc;    

#define NET_DF_SMT_XMT_BUF_SIZE      4608

typedef char NetDFSmtXmtBuf[NET_DF_SMT_XMT_BUF_SIZE];

/*
 * RMC XMT ring descriptor definition.  The 512 byte buffers associated with
 * the descriptors are on the adapter.  To transmit stuff > 512 bytes, we
 * have to chain these things.  Yummy yummy.
 */
typedef struct NetDFRmcXmtDesc {
    unsigned long rmcXmtDesc;    /* RMC XMT Descriptor */
    unsigned long hostReserved; /* Reserved for us? Wow! */
    unsigned long own;          /* [31] = 1 adapter own; = 0 host own
				 * Note that this protocol is opposite
				 * all other own-bit protocols.
				 * [30-0] reserved */
    unsigned long reserved;
} NetDFRmcXmtDesc;    

#define NET_DF_RMC_OWN               0x80000000
#define NET_DF_RMC_ADAPTER_OWN       0x80000000
#define NET_DF_RMC_HOST_OWN          0x00000000

#define NET_DF_RMC_XMT_BUF_SIZE      512
#define NET_DF_RMC_XMT_BUF_HDR_SIZE  16
#define NET_DF_RMC_XMT_BUF_DATA_SIZE 496

typedef char NetDFRmcXmtBuf[NET_DF_RMC_XMT_BUF_SIZE];

#define NET_DF_MAX_PACKET_SIZE       4480
#define NET_DF_MIN_MAC_PACKET_SIZE   17
#define NET_DF_MIN_SMT_PACKET_SIZE   37
#define NET_DF_MIN_LLC_PACKET_SIZE   20
#define NET_DF_MIN_IMP_PACKET_SIZE   17
#define NET_DF_MIN_RES_PACKET_SIZE   17

#define NET_DF_RMC_XMT_SOP           0x80000000
#define NET_DF_RMC_XMT_EOP           0x40000000
#define NET_DF_RMC_XMT_DTP           0x20000000
#define NET_DF_RMC_XMT_VBC           0x10000000
#define NET_DF_RMC_XMT_DCC           0x0F000000
#define NET_DF_RMC_XMT_RES           0x00C00000
#define NET_DF_RMC_XMT_XPO           0x003FE000
#define NET_DF_RMC_XMT_PBC           0x00001FFF
/*
 * Typical assignment values to the RMC XMT descriptor
 */
#define NET_DF_RMC_XMT_ONE_PAGE      0xD0000000
#define NET_DF_RMC_XMT_FIRST_PAGE    0x90000000
#define NET_DF_RMC_XMT_MIDDLE_PAGE   0x00000000
#define NET_DF_RMC_XMT_LAST_PAGE     0x40000000
/*
 * DCC (DMA Completion Code) definitions.
 */
#define NET_DF_DCC_SUCCESS           0x01000000

/*
 * Typical PRH (Packet Request Header) byte values
 */
#define NET_DF_PRH0                  0x20
#define NET_DF_PRH1                  0x28
#define NET_DF_PRH2                  0x00
/*
 * Default Frame Control types and values.
 */
#define	NET_DF_FRAME_CLASS            0x80
#define	NET_DF_FRAME_ADDR_LENGTH      0x40
#define	NET_DF_FRAME_FORMAT           0x30
#define	NET_DF_FRAME_CONTROL          0x0F

#define NET_DF_FRAME_HOST_LLC         0x54
#define NET_DF_FRAME_VOID             0x40
#define NET_DF_FRAME_NON_REST_TOK     0x80
#define NET_DF_FRAME_REST_TOK         0xc0             
#define NET_DF_FRAME_SMT_INFO         0x41
#define NET_DF_FRAME_SMT_NSA          0x4F
#define NET_DF_FRAME_MAC_BEACON       0xc2
#define NET_DF_FRAME_MAC_CLAIM        0xc3
#define NET_DF_FRAME_LLC_ASYNC        0x50
#define NET_DF_FRAME_LLC_SYNC         0xD0
#define NET_DF_FRAME_IMP_ASYNC        0x60
#define NET_DF_FRAME_IMP_SYNC         0xe0
#define NET_DF_FRAME_SMT              0x40
#define NET_DF_FRAME_MAC              0xc0

/*
 * A transmission queue used to shelve packets while one is 
 * being sent.
 */

/*
 * Length of the transmission queue.
 */
#define NET_DF_NUM_XMIT_ELEMENTS     32
#define NET_DF_XMT_HOST              0x1
#define NET_DF_XMT_SMT               0x2

typedef struct NetDFXmtElement {
    List_Links        links;
    Net_FDDIHdr       *fddiHdrPtr;          /* FDDI header with address of
					     * receiver already set */
    Net_ScatterGather *scatterGatherPtr;    /* Scatter/gather array */
    int               scatterGatherLength;  /* Size of scatter/gather array */
    NetDFSmtXmtDesc   *smtDescPtr;          /* SMT Descriptor to transmit */
    Boolean           xmtType;              /* Either XMT_HOST or XMT_SMT */
} NetDFXmtElement;

/*
 * Structure to hold all state information associated with 
 * the FDDI adapter board.
 */
typedef struct {
    List_Links		xmitListHdr;	/* List of packets to be transmited. */
    List_Links		*xmitList;	/* Pointer to the front of the list of
					   packets to be transmited. */
    List_Links		xmitFreeListHdr; /* List of unused packets. */
    List_Links      	*xmitFreeList;	/* Pointer to a list of unused 
					   transmission queue elements. */
    volatile char       *slotAddr;         /* option slot base address */

    volatile Net_DFReg  *regReset;         /* RESET Register */
    volatile Net_DFReg  *regCtrlA;         /* PORT_CONTROL_A register */
    volatile Net_DFReg  *regCtrlB;         /* PORT_CONTROL_B register */
    volatile Net_DFReg  *regStatus;        /* PORT_STATUS register */
    volatile Net_DFReg  *regEvent;         /* PORT_INTERRUPT_EVENT register */
    volatile Net_DFReg  *regMask;          /* PORT_INTERRUPT_MASK register */

    volatile NetDFCommandDesc *comRingPtr; /* Base address for the command
					    * descriptor memory */
    volatile NetDFCommandDesc *comNextPtr; /* Command descriptor after last
					    * one previously used */
    volatile NetDFCommandDesc *comLastPtr; /* Last command descriptor that
					    * can be used */
    volatile NetDFCommandBuf  *comBufPtr;  /* Base address for the command
					    * buffer memory region */
    volatile NetDFInitCommand *initComPtr;    /* INIT command buffer */
    volatile unsigned char    *errLogPtr;  /* Base address of Error Log */

    volatile NetDFHostRcvDesc *hostRcvFirstDescPtr;
    volatile NetDFHostRcvDesc *hostRcvNextDescPtr;
    volatile NetDFHostRcvDesc *hostRcvLastDescPtr;

    volatile NetDFHostRcvBuf  *hostRcvFirstBufPtr;
    volatile NetDFHostRcvBuf  *hostRcvNextBufPtr;
    volatile NetDFHostRcvBuf  *hostRcvLastBufPtr;
#ifndef NET_DF_USE_UNCACHED_MEM
    volatile NetDFHostRcvBuf  *hostRcvBuffers[NET_DF_NUM_HOST_RCV_ENTRIES];
     int              hostRcvBufIndex;
#endif

    volatile NetDFSmtRcvDesc  *smtRcvFirstDescPtr;
    volatile NetDFSmtRcvDesc  *smtRcvNextDescPtr;
    volatile NetDFSmtRcvDesc  *smtRcvLastDescPtr;

    volatile NetDFSmtXmtDesc  *smtXmtFirstDescPtr;
    volatile NetDFSmtXmtDesc  *smtXmtNextDescPtr;
    volatile NetDFSmtXmtDesc  *smtXmtLastDescPtr;

    volatile NetDFRmcXmtDesc  *rmcXmtFirstDescPtr;
    volatile NetDFRmcXmtDesc  *rmcXmtBatchDescPtr;
    volatile NetDFRmcXmtDesc  *rmcXmtNextDescPtr;
    volatile NetDFRmcXmtDesc  *rmcXmtLastDescPtr;
    volatile NetDFRmcXmtBuf   *rmcXmtFirstBufPtr;

    volatile NetDFUnsolDesc   *unsolFirstDescPtr;
    volatile NetDFUnsolDesc   *unsolNextDescPtr;
    volatile NetDFUnsolDesc   *unsolLastDescPtr;

    unsigned long             flags;    /* State of the adapter. */
                                        /* Used for iocontrol transmits. */
    Net_ScatterGather         scatterArray[NET_DF_NUM_XMIT_ELEMENTS];
    Net_FDDIHdr               headerArray[NET_DF_NUM_XMIT_ELEMENTS];
    int                       scatterIndex;

    Sync_Condition      doingReset;     /* Condition to wait for resetting
					   to finish. */
    Boolean		transmitting;	/* Set if are currently transmitting a
					   packet. */
    Boolean             hostTransmit;   /* Set if we are transmitting a
					 * host packet (non-SMT) */
    Boolean		running;	/* Is the chip currently active. */
    Boolean             booting;        /* Is this during boot? */
    Boolean		recvMemInitialized;	/* Flag for initializing
						 * kernel memory. */
    Boolean		recvMemAllocated;	/* Flag for allocating
						 * memory for ring buffers. */
    Net_ScatterGather 	*curScatGathPtr;  /* Pointer to scatter gather element 
					   * for current packet being sent. */
    volatile NetDFSmtXmtDesc  *curSmtXmtDescPtr; /* Pointer to SMT XMT desc
						  * for current packet */
    char		*firstDataBuffer; /* Buffer used to ensure that
					   * first element is of a minimum
					   * size. */
    Boolean		xmitMemInitialized; /* Flag to note if xmit memory
					     * has been initialized. */
    Boolean		xmitMemAllocated; /* Flag to note if xmit memory
					   * has been allocated. */
    Net_Interface	*interPtr;    /* Pointer back to network interface. */
    Net_FDDIStats	stats;		/* Performance statistics. */
    int			numResets;	/* Number of times the chip has
					 * been reset. */
    Boolean		resetPending;	/* TRUE => chip should be reset when
					 * current transmit is done. */
    int			lastRecvCnt;	/* Number of packets done during
					 * last receive interrupt. */
    int			lastUnsolCnt;	/* Number of unsolicited events done
					 * during last unsol interrupt. */
    unsigned char      	loopBackBuffer[NET_DF_MAX_PACKET_SIZE]; 
                                        /* Buffer for the
					 * loopback address. */
    Net_FDDIAddress	fddiAddress;	   /* The FDDI address */
} NetDFState;

/*
 * Flags to determine adapter state
 */
#define NET_DF_FLAGS_NORMAL          0x00000001
#define NET_DF_FLAGS_RESETTING       0x00000002
#define NET_DF_FLAGS_SYSLOG          0x00000004

/*
 * Debug ring.
 */
#define NET_DF_DEBUG_RING_SIZE       20
extern char *netDFDebugRing[];
extern int  netDFDebugRingIndex;

#define MAKE_NOTE(message) \
netDFDebugRing[netDFDebugRingIndex] = (message); \
netDFDebugRingIndex = (netDFDebugRingIndex + 1) % NET_DF_DEBUG_RING_SIZE

extern void             NetDFPrintDebugRing _ARGS_((NetDFState *statePtr));

/*
 * General routines.
 */
#define IncScatterIndex(statePtr) \
    statePtr->scatterIndex = (statePtr->scatterIndex + 1) % \
        NET_DF_NUM_XMIT_ELEMENTS


#define NET_DF_DEBUG_OFF             0x0
#define NET_DF_DEBUG_ON              0x1

extern int netDFDebug;

#define DFprintf \
  if (netDFDebug == NET_DF_DEBUG_ON) printf

extern	ReturnStatus	NetDFInit _ARGS_((Net_Interface *interPtr));
extern	ReturnStatus	NetDFOutput _ARGS_((Net_Interface *interPtr,
			    Address hdrPtr,Net_ScatterGather *scatterGatherPtr,
			    int scatterGatherLength, Boolean rpc,
			    ReturnStatus *statusPtr));

extern ReturnStatus     NetDFSmtOutput _ARGS_((Net_Interface *interPtr));

extern void             NetDFPrintStateAddrs _ARGS_((void));
extern void             NetDFPrintNextRmc _ARGS_((NetDFState *statePtr));

extern	void		NetDFIntr _ARGS_((Net_Interface *interPtr, 
			    Boolean polling));
extern ReturnStatus 	NetDFIOControl _ARGS_((Net_Interface *interPtr, 
			    Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));

extern void             Net_DFRestart _ARGS_((Net_Interface *interPtr));
extern	void		NetDFReset _ARGS_((Net_Interface *interPtr));

extern	ReturnStatus	NetDFGetStats _ARGS_((Net_Interface *interPtr, 
			    Net_Stats *statPtr));
extern	ReturnStatus	NetDFMachInit _ARGS_ ((Net_Interface *interPtr,
				NetLEState *statePtr));

extern void NetDFPrintRegContents _ARGS_((NetDFState *statePtr));
extern void NetDFPrintErrorLog _ARGS_((NetDFState *statePtr));

/*
 * Routines for transmitting.
 */

extern  void            NetDFBcopy    _ARGS_((unsigned char *startPtr,
					      unsigned char *destPtr,
					      unsigned long numBytes));
extern	void		NetDFXmitInit _ARGS_((NetDFState *statePtr));
extern	ReturnStatus	NetDFXmitDone _ARGS_((NetDFState *statePtr));

extern	void		NetDFXmitRestart _ARGS_((NetDFState *statePtr));

extern	void		NetDFXmitDrop _ARGS_((NetDFState *statePtr));
extern	void		NetDFXmitFlushQ _ARGS_((NetDFState *statePtr));
extern	ReturnStatus	NetDFOutputSmtPacket _ARGS_((NetDFState *statePtr));

/*
 * Routines for the receive unit.
 */

extern	void		NetDFRecvInit _ARGS_((NetDFState *statePtr));
extern	ReturnStatus	NetDFRecvProcess _ARGS_((Boolean dropPackets,
			    NetLEState *statePtr));

#endif
