/*
 * dbgDbxInt.h --
 *
 *	Exported types and procedure headers for the debugger module.
 *	This file is needed for the kdbx interface.  Once kdbx is abandoned
 *	in favor of kgdb this file should be deleted.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBG
#define _DBG

#ifndef _SPRITE
#include <sprite.h>
#endif

/*
 * Variable to indicate if are using the rs232 debugger or the network debugger.
 */
extern	Boolean	dbg_Rs232Debug;

/*
 * Variable to indicate that dbg wants a packet.
 */
extern	Boolean	dbg_UsingNetwork;

/*
 * Variable that indicates that we are under control of the debugger.
 */
extern	Boolean	dbg_BeingDebugged;

/*
 * Variable that indicates that we are in the debugger command loop.
 */
extern	Boolean	dbg_InDebugger;

/*
 * The maximum stack address.
 */
extern	int	dbgMaxStackAddr;

/*
 * Debugger using syslog to dump output of call command or not.
 */
extern	Boolean	dbg_UsingSyslog;

#define	DBG_MAX_REPLY_SIZE	1024
#define	DBG_MAX_REQUEST_SIZE	1024

/*
 * The UDP port number that the kernel and kdbx use to identify a packet as
 * a debugging packet.  (composed from "uc": 0x75 = u, 0x63 = c)
 */

#define DBG_UDP_PORT 	0x7563

/*
 * Request and reply buffer headers. 
 */

typedef struct {
    int		num;
    int		request;
    unsigned	addr;
    int		data;
} Dbg_Request;

typedef struct {
    int		num;
    int		status;
    int		data;
} Dbg_Reply;

/*
 * Values for the request field of a Dbg_Request.
 */

#define DBG_UREAD   3       /* read from process's user structure */
#define DBG_UWRITE  6       /* write to process's user structure */
#define DBG_IREAD   1       /* read from process's instruction space */
#define DBG_IWRITE  4       /* write to process's instruction space */
#define DBG_DREAD   2       /* read from process's data space */
#define DBG_DWRITE  5       /* write to process's data space */
#define DBG_CONTP   7       /* continue stopped process */
#define DBG_SSTEP   9       /* continue for approximately one instruction */
#define DBG_PKILL   8       /* terminate the process */
#define DBG_DBREAD  14      /* read in bytes */
#define DBG_DBWRITE 15      /* write in bytes */
#define DBG_DHREAD  16      /* read in halfwords */
#define DBG_DHWRITE 17      /* write in halfwords */
#define DBG_QUERY   18      /* query returns flags about target environment */
#define DBG_BEGINCALL 19
#define DBG_ENDCALL   20
#define DBG_DETACH    21
#define DBG_GETMAXSTACK 22
#define DBG_GETSTATUS 23
/*
 * These ones are used by kmsg.
 */
#define DBG_GET_VERSION_STRING	100
#define DBG_REBOOT		101

/*
 * Variable that is set to true when we are called through the DBG_CALL macro.
 */
extern	Boolean	dbgPanic;

/*
 * Macro to call the debugger from kernel code.
 */
extern	void Dbg_Call _ARGS_((void));
#define DBG_CALL	dbgPanic = TRUE; Dbg_Call();

/*
 * Number of bytes between acknowledgements when the the kernel is writing
 * to kdbx.
 */
#define DBG_ACK_SIZE	256

extern	void	Dbg_Init _ARGS_((void));
extern	void	Dbg_InputPacket _ARGS_((Address packetPtr, int packetLength));
extern	Boolean	Dbg_InRange _ARGS_((unsigned int addr, int numBytes,
				    Boolean writeable));
extern	unsigned	Dbg_Main _ARGS_((void));

extern Boolean
    Dbg_ValidatePacket _ARGS_((int size, Net_IPHeader *ipPtr, int *lenPtr,
			       Address *dataPtrPtr,
			       Net_InetAddress *destIPAddrPtr,
			       Net_InetAddress *srcIPAddrPtr,
			       unsigned int *srcPortPtr));
extern void
    Dbg_FormatPacket _ARGS_((Net_InetAddress srcIPAddress,
			     Net_InetAddress destIPAddress,
			     unsigned int destPort, int dataSize,
			     Address dataPtr));
extern int	Dbg_PacketHdrSize _ARGS_((void));

#endif /* _DBG */
