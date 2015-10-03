/* dbgMain.c -
 *
 *	This contains the routines which read and execute commands from kdbx.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <dbgDbxInt.h>
#include <dbgInt.h>
#include <mach.h>
#include <machConst.h>
#include <proc.h>
#include <vm.h>
#include <vmMach.h>
#include <machMon.h>
#include <net.h>
#include <netEther.h>
#include <netInet.h>
#include <dev.h>
#include <sys.h>
#include <sync.h>
#include <main.h>

static unsigned sstepInst;			/* The instruction that was
						 * replaced when we tried to
						 * single step. */
static char requestBuffer[DBG_MAX_REQUEST_SIZE + 2];/* Buffer to receive request
						 * into. */
static char *requestBufPtr = requestBuffer + 2;	/* Offset the buffer by 2 so
						 * we four byte align the 
						 * data in the packet. */
static char replyBuffer[DBG_MAX_REPLY_SIZE + 2];/* Buffer to hold reply. */
static char *replyBufPtr = replyBuffer + 2;	/* Offset the buffer by 2 so
						 * we four byte align the 
						 * data in the packet. */


/*
 * Size of debugging packet header and data.
 */
#define	PACKET_HDR_SIZE (sizeof(Net_EtherHdr) + Dbg_PacketHdrSize())
#define PACKET_DATA_SIZE \
    (DBG_MAX_REPLY_SIZE - PACKET_HDR_SIZE - sizeof(Dbg_Reply) + 4)

/*
 * Message buffers.
 */

static Dbg_Request	*requestPtr;
static Dbg_Reply	*replyPtr;
static char		*dataPtr;

/*
 * Strings which describe each of the opcodes that kdbx can send us.
 */
static char *opcodeNames[] =  {
    "Unknown",
    "IREAD",
    "DREAD",
    "UREAD",
    "IWRITE",
    "DWRITE",
    "UWRITE",
    "CONTP",
    "PKILL",
    "SSTEP",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "DBREAD",
    "DBWRITE",
    "DHREAD",
    "DHWRITE",
    "QUERY",
    "BEGINCALL",
    "ENDCALL",
    "DETACH",
    "GETMAXSTACK",
    "GETSTATUS",
};

/*
 * Strings which describe the different exceptions that can occur.
*/
static char *exceptionNames[] = {
    "Interrupt",
    "TLB Mod",
    "TLB LD miss",
    "TLB ST miss",
    "TLB load address error",
    "TLB store address error",
    "TLB ifetch bus error",
    "TLB load or store bus error",
    "System call",
    "Breakpoint trap",
    "Reserved instruction",
    "Coprocessor unusable",
    "Overflow"
};

/*
 * Declare global variables.
 */
extern int			dbgTermReason;
extern int			dbgInDebugger;
extern int			dbgIntPending;
extern Boolean			dbgPanic;
extern int			dbgMaxStackAddr;
extern Boolean			dbg_UsingSyslog;
extern Boolean			dbgCanUseSyslog;
extern int			dbgPacketLength;
extern Net_InetAddress		dbgMyIPAddr;
extern Net_InetAddress		dbgSrcIPAddr;
extern Net_InetAddress		dbgSrcPort;
extern Net_EtherHdr		dbgEtherHdr;
extern Net_ScatterGather	dbgGather;
extern Net_Interface		*dbgInterPtr;
extern int			dbgTimeout;
extern unsigned			dbgTLB[VMMACH_NUM_TLB_ENTRIES][2];
extern Boolean			dbgGotPacket;

/*
 * Trap causes (same numbering as in ptrace.h).
 */
#define CAUSE_SINGLE	4
#define CAUSE_BREAK	5

/*
 * Trap instruction.
 */
#define SSTEP_INST	(MACH_SSTEP_VAL | 0xd)

Boolean	dbg_OthersCanUseNetwork = TRUE;

/* 
 * Forward declarations:
 */

static char *	TranslateOpcode _ARGS_((int opcode));
static char *	TranslateException _ARGS_((int exception));
static Boolean	ReadRequest _ARGS_((Boolean timeout));
static void	SendReply _ARGS_((int dataSize));


/*
 * ----------------------------------------------------------------------------
 *
 * TranslateOpcode --
 *
 *     Return the string which describes the given opcode.
 *
 * Results:
 *     Pointer to the string which describes the given opcode.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
static char *
TranslateOpcode(opcode)
    int opcode;		/* The opcode which is to be translated. */
{
	int index;

	index = (int) opcode;
	if (index < 0 || index >= sizeof(opcodeNames) / 4) {
	    index = 0;
	}

	return(opcodeNames[index]);
}


/*
 * ----------------------------------------------------------------------------
 *
 * TranslateException --
 *
 *     Return the string that describes the given exception.
 *
 * Results:
 *     Pointer to string which describes the given exception.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
static char *
TranslateException(exception)
    int exception;		/* The exception which is to be translated. */
{

	if (exception < 0 || exception > MACH_EXC_OVFLOW) {
	    return("Unknown");
	} else {
	    return(exceptionNames[exception]);
	}
}


/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_DbxInit --
 *
 *     Initialize the kdbx aspects of the debugger.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Global variables are initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
DbgDbxInit()
{
    dbgMaxStackAddr = (int)mach_StackBottom + mach_KernStackSize;
    replyPtr = (Dbg_Reply *)(replyBufPtr + PACKET_HDR_SIZE);
    dataPtr = replyBufPtr + PACKET_HDR_SIZE + sizeof(Dbg_Reply) - 4;
}


/*
 * ----------------------------------------------------------------------------
 *
 * DbgDbxStoreRequest --
 *
 *     Put the request into the request buffer.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Stuff is put into requestBuffer.
 *
 * ----------------------------------------------------------------------------
 */

void 
DbgDbxStoreRequest(size, reqPtr)
    int		size;
    char	*reqPtr;
{
    if (dbgTraceLevel >= 4) {
	Mach_MonPrintf("DbgDbxStoreRequest\n");
    }
    bcopy(reqPtr, requestBuffer, size);
    requestPtr = (Dbg_Request *)requestBuffer;
}

/*
 * ----------------------------------------------------------------------------
 *
 * ReadRequest --
 *
 *     Read the next request from kdbx.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     TRUE if didn't time out.
 *
 * ----------------------------------------------------------------------------
 */
static Boolean
ReadRequest(timeout)
    Boolean	timeout;	/* TRUE if should timeout after waiting a 
				 * while. */
{
    int			timeOutCounter;
    Net_Interface	*interPtr;
    int			i;

    dbgGotPacket = FALSE;
    if (dbgTraceLevel >= 4) {
	Mach_MonPrintf("ReadRequest\n");
    }
    timeOutCounter = dbgTimeout;
    do {
	/*
	 * Listen on all the interfaces. The debugger is relatively
	 * stateless so its easiest to just listen on them all.
	 */
	for (i = 0; ; i++) {
	    interPtr = Net_NextInterface(TRUE, &i);
	    if (interPtr == (Net_Interface *) NIL) {
		break;
	    }
	    Net_RecvPoll(interPtr);
	    if (dbgGotPacket) {
		break;
	    }
	}
	if (timeout) {
	    timeOutCounter--;
	}
    } while(!dbgGotPacket && timeOutCounter != 0);
    if (dbgGotPacket) {
	requestPtr = (Dbg_Request *)requestBuffer;
	if (dbgTraceLevel >= 4) {
	    Mach_MonPrintf("MsgNum = %d\n", requestPtr->num);
	}
    }

    return(dbgGotPacket);
}


/*
 * ----------------------------------------------------------------------------
 *
 * SendReply --
 *
 *     Send a reply to kdbx.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
static void
SendReply(dataSize)
    int	dataSize;
{
    Net_EtherHdr		*etherHdrPtr;

    replyPtr->num = requestPtr->num;

    if (dbgTraceLevel >= 4) {
	Mach_MonPrintf("Sending reply %d\n", replyPtr->num);
    }
    etherHdrPtr = (Net_EtherHdr *) replyBufPtr;
    etherHdrPtr->source = dbgEtherHdr.destination;
    etherHdrPtr->destination = dbgEtherHdr.source;
    etherHdrPtr->type = dbgEtherHdr.type;
    dbgGather.bufAddr = replyBufPtr + sizeof(Net_EtherHdr);
    dbgGather.length = PACKET_HDR_SIZE + sizeof(Dbg_Reply) + dataSize - 
		       sizeof(Net_EtherHdr);
    dbgGather.mutexPtr = (Sync_Semaphore *) NIL;
    Dbg_FormatPacket(dbgMyIPAddr, dbgSrcIPAddr, dbgSrcPort,
		     dataSize + sizeof(Dbg_Reply),
		     replyBufPtr + sizeof(Net_EtherHdr));
    Net_RawOutput(dbgInterPtr, etherHdrPtr, &dbgGather, 1);
    if (dbgTraceLevel >= 4) {
	Mach_MonPrintf("Sent reply\n");
    }
}

/*
 * Whether syslog should remain diverted on continue or not.
 */
static Boolean	syslogDiverted = FALSE;

extern Mach_DebugState	mach_DebugState;

static Mach_DebugState	tmpDebugState;
static Mach_DebugState	*debugStatePtr;


/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_Main --
 *
 *     The main debugger loop.  This will read commands from the network
 *     and call the proper routine to execute them.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
unsigned
Dbg_DbxMain()
{
    Boolean	done;
    Boolean	atInterruptLevel;
    unsigned	cause;
    int		signal;
    int		origSignal;
    int		dataSize;
    int		origMaxStackAddr;

    origMaxStackAddr = dbgMaxStackAddr;
    debugStatePtr = &mach_DebugState;

    if (dbg_BeingDebugged) {

	SendReply(0);
	do {
	    if (ReadRequest(TRUE)) {
		if (requestPtr->request == DBG_GETSTATUS) {
		    break;
		}
	    }
	    Net_RawOutput(dbgInterPtr, (Net_EtherHdr *)replyBuffer, 
		    &dbgGather, 1);
	    Mach_MonPrintf("TI: %d ", requestPtr->request);
	} while (TRUE);
    } 

    Vm_MachDumpTLB(dbgTLB);

    done = FALSE;
    while (!done) {
	int	request;

	dataSize = 0;
	replyPtr->status = 1;
	replyPtr->data = 0;
	request = requestPtr->request;
	if (dbgTraceLevel >= 1) {
	    Mach_MonPrintf("Request: %d, %s at %x\n", request, 
			TranslateOpcode(request), requestPtr->addr);
	}
	switch (request) {
            case DBG_UREAD:
		if (requestPtr->addr > sizeof(mach_DebugState) / 4) {
		    Mach_MonPrintf("Bogus UREAD addr %x\n", requestPtr->addr);
		    replyPtr->status = 0;
		} else {
		    replyPtr->data = *((int *)debugStatePtr + requestPtr->addr);
		}
		break;
            case DBG_UWRITE: {
		extern void Mach_SwitchPoint();	/* XXX - should go elsewhere */

		if (requestPtr->addr == (unsigned)-1) {
		    Proc_ControlBlock	*procPtr;

		    if (requestPtr->data == 0) {
			debugStatePtr = &mach_DebugState;
			signal = origSignal;
			dbgMaxStackAddr = origMaxStackAddr;
			break;
		    } else {
			procPtr = Proc_GetPCB(requestPtr->data);
			if (procPtr == (Proc_ControlBlock *)NIL ||
			    procPtr->machStatePtr == (Mach_State *)NIL) {
			    Mach_MonPrintf("Bad process table index %d\n", 
				    requestPtr->data);
			    break;
			}
		    }
		    debugStatePtr = &tmpDebugState;
		    bcopy((char *) procPtr->machStatePtr->switchRegState.regs,
			  (char *) tmpDebugState.regs, 32 * sizeof(int));
		    tmpDebugState.excPC = (unsigned)(Address)Mach_SwitchPoint;
		    dbgMaxStackAddr = (int)procPtr->machStatePtr->kernStackEnd;
		    signal = 2;
		} else if (requestPtr->addr == (unsigned)-2) {
		    (void)Proc_Dump();
		} else {
		    if (requestPtr->addr > sizeof(mach_DebugState) / 4) {
			Mach_MonPrintf("Bogus UWRITE addr %x\n", requestPtr->addr);
			replyPtr->status = 0;
		    } else {
			replyPtr->data =
				*((int *)debugStatePtr + requestPtr->addr);
			*((int *)debugStatePtr + requestPtr->addr) =
							requestPtr->data;
		    }
		}
		break;
	    }
            case DBG_IREAD:
            case DBG_DREAD:
		if (Dbg_InRange(requestPtr->addr, 4, FALSE)) {
		    replyPtr->data = *(int *)requestPtr->addr;
		} else {
		    replyPtr->status = 0;
		}
		break;
            case DBG_IWRITE:
		if (Dbg_InRange(requestPtr->addr, 4, TRUE)) {
		    replyPtr->data = *(int *)requestPtr->addr;
		    Mach_FlushCode((Address)requestPtr->addr, 4);
		    *(int *)requestPtr->addr = requestPtr->data;
		    Mach_EmptyWriteBuffer();
		} else {
		    replyPtr->status = 0;
		}
		break;
            case DBG_DWRITE:
		if (Dbg_InRange(requestPtr->addr, 4, TRUE)) {
		    replyPtr->data = *(int *)requestPtr->addr;
		    *(int *)requestPtr->addr = requestPtr->data;
		} else {
		    replyPtr->status = 0;
		}
		break;
            case DBG_CONTP: 
		dbg_BeingDebugged = TRUE;
		done = TRUE;
		break;
            case DBG_SSTEP: {
		unsigned		*pc;

		pc = DbgGetDestPC((Address)(debugStatePtr->excPC));
		if (dbgTraceLevel >= 1) {
		    Mach_MonPrintf("Single-step PC=%x\n", pc);
		}
		if (!Dbg_InRange((unsigned int)pc, 4, TRUE)) {
		    Mach_MonPrintf("Bad SSTEP PC\n");
		    replyPtr->status = 0;
		    break;
		}
		sstepInst = *pc;
		Mach_FlushCode((Address)pc, 4);
		*pc = SSTEP_INST;
		Mach_EmptyWriteBuffer();
		dbg_BeingDebugged = TRUE;
		done = TRUE;
		break;
	    }
            case DBG_PKILL:
		break;
            case DBG_DBREAD: 
		if (Dbg_InRange(requestPtr->addr, 1, FALSE)) {
		    replyPtr->data = *(char *)requestPtr->addr;
		} else {
		    replyPtr->status = 0;
		}
		break;
            case DBG_DBWRITE:
		if (Dbg_InRange(requestPtr->addr, 1, TRUE)) {
		    replyPtr->data = *(char *)requestPtr->addr;
		    *(char *)requestPtr->addr = requestPtr->data;
		} else {
		    replyPtr->status = 0;
		}
		break;
            case DBG_DHREAD:
		if (Dbg_InRange(requestPtr->addr, 2, FALSE)) {
		    replyPtr->data = *(short *)requestPtr->addr;
		} else {
		    replyPtr->status = 0;
		}
		break;
            case DBG_DHWRITE:
		if (Dbg_InRange(requestPtr->addr, 2, TRUE)) {
		    replyPtr->data = *(short *)requestPtr->addr;
		    *(short *)requestPtr->addr = requestPtr->data;
		} else {
		    replyPtr->status = 0;
		}
		break;
            case DBG_QUERY:
		break;
            case DBG_BEGINCALL:
		break;
            case DBG_ENDCALL:
		break;
            case DBG_DETACH:
		dbg_BeingDebugged = FALSE;
		done = TRUE;
		Mach_MonPrintf("Sprite is now detached from the debugger\r\n");
		break;
            case DBG_GETMAXSTACK:
		replyPtr->data = dbgMaxStackAddr;
		break;
            case DBG_GETSTATUS:
		replyPtr->data = 0177 | (signal << 8);
		break;
	    case DBG_GET_VERSION_STRING: {
		char *version;
		version = SpriteVersion();
		strncpy(dataPtr, version, PACKET_DATA_SIZE);
		dataSize = strlen(version) + 1 - 4;
		break;
	    }
	    case DBG_REBOOT: {
		char	*reboot;
		reboot = (char *) &requestPtr->data;
		Mach_MonReboot(reboot);
		break;
	    }
	}
	SendReply(dataSize);
	if (!done) {
	    (void)ReadRequest(FALSE);
	}
    }
}
