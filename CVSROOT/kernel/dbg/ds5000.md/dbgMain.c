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
#include <dbg.h>
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

unsigned sstepInst;				/* The instruction that was
						 * replaced when we tried to
						 * single step. */
Boolean dbg_InDebugger = FALSE;			/* TRUE if are currently in
						 * the debug command loop. */
Boolean	dbg_BeingDebugged = FALSE;		/* TRUE if are under control
						 * of kdbx.*/
Boolean	dbg_Rs232Debug = FALSE;			/* TRUE if are using the RS232
						 * line to debug, FALSE if are
						 * using the network. */
Boolean	dbg_UsingNetwork = FALSE;		/* TRUE if the debugger is
						 * using the network interface*/
char	requestBuffer[DBG_MAX_REQUEST_SIZE + 2];/* Buffer to receive request
						 * into. */
char	*requestBufPtr = requestBuffer + 2;	/* Offset the buffer by 2 so
						 * we four byte align the 
						 * data in the packet. */
char	replyBuffer[DBG_MAX_REPLY_SIZE + 2];	/* Buffer to hold reply. */
char	*replyBufPtr = replyBuffer + 2;		/* Offset the buffer by 2 so
						 * we four byte align the 
						 * data in the packet. */
int	dbgTraceLevel = 0;			/* The debugger tracing
						 * level. */

/*
 * Number of times to poll before timing out and resending (about 2 seconds).
 */
int	dbgTimeout = 15000;

/*
 * Information about the latest packet received.
 */
Boolean			gotPacket;	
int			dbgPacketLength;
Net_InetAddress		dbgMyIPAddr;
Net_InetAddress		dbgSrcIPAddr;
Net_InetAddress		dbgSrcPort;
Net_EtherHdr		dbgEtherHdr;
Net_ScatterGather	dbgGather;
Net_Interface		*dbgInterPtr = (Net_Interface *) NIL;

/*
 * Size of debugging packet header and data.
 */
#define	PACKET_HDR_SIZE (sizeof(Net_EtherHdr) + Dbg_PacketHdrSize())
#define PACKET_DATA_SIZE \
    (DBG_MAX_REPLY_SIZE - PACKET_HDR_SIZE - sizeof(Dbg_Reply) + 4)

/*
 * Message buffers.
 */

Dbg_Request	*requestPtr;
Dbg_Reply	*replyPtr;
char		*dataPtr;

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
 * The type of machine that we are on.
 */
int		machineType;

/*
 * Declare global variables.
 */
int		dbgTermReason;
int		dbgInDebugger;
int		dbgIntPending;
Boolean		dbgPanic;
int		dbgMaxStackAddr;
Boolean		dbg_UsingSyslog = FALSE;
Boolean		dbgCanUseSyslog = TRUE;

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
 * Dbg_InRange --
 *
 *     Return true if the given address is a valid kernel address and false
 *     otherwise.
 *
 * Results:
 *     True if the given address is a valid kernel address and false
 *     otherwise.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
Boolean Dbg_InRange(addr, numBytes, writeable) 
    unsigned 	int addr; 	/* Beginning address to check. */
    int		numBytes; 	/* Number of bytes to check. */
    Boolean	writeable;	/* TRUE => address must be writeable. */
{
    int			firstPage;
    int			lastPage;

    firstPage = addr >> VMMACH_PAGE_SHIFT;
    lastPage = (addr + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    if (firstPage != lastPage) {
	printf("Dbg_InRange: Object spans pages\n");
	return(FALSE);
    }
    return(VmMach_MakeDebugAccessible(addr));
}


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
 * Dbg_Init --
 *
 *     Initialize the debugger.
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
Dbg_Init()
{
    dbgInDebugger = 0;
    dbgIntPending = 0;
    dbgPanic = FALSE;
    dbg_BeingDebugged = FALSE;
    dbgMaxStackAddr = (int)mach_StackBottom + mach_KernStackSize;
    replyPtr = (Dbg_Reply *)(replyBufPtr + PACKET_HDR_SIZE);
    dataPtr = replyBufPtr + PACKET_HDR_SIZE + sizeof(Dbg_Reply) - 4;
}

char	pingBuffer[DBG_MAX_REQUEST_SIZE + 2];
char	*pingBufPtr = pingBuffer + 2;


/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_InputPacket --
 *
 *     See if the current packet is for us.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     gotPacket is set to true if we got a packet that we liked.
 *
 * ----------------------------------------------------------------------------
 */
void
Dbg_InputPacket(interPtr, packetPtr, packetLength)
    Net_Interface	*interPtr;
    Address		packetPtr;
    int			packetLength;
{
    Net_EtherHdr	*etherHdrPtr;
    Net_IPHeader	*ipPtr;
    Address		dataPtr;
    int			dataLength;

    if (interPtr->netType != NET_NETWORK_ETHER) {
	printf("Got a debugger packet on non-ethernet interface %s\n",
	    interPtr->name);
	return;
    }
    etherHdrPtr = (Net_EtherHdr *)packetPtr;

    if (dbgTraceLevel >= 5) {
	if ((unsigned char)etherHdrPtr->destination.byte1 != 0xff) {
	    Mach_MonPrintf("Size=%d S: %x:%x:%x:%x:%x:%x D: %x:%x:%x:%x:%x:%x T=%x\n",
			packetLength,
			(unsigned char)etherHdrPtr->source.byte1,
			(unsigned char)etherHdrPtr->source.byte2,
			(unsigned char)etherHdrPtr->source.byte3,
			(unsigned char)etherHdrPtr->source.byte4,
			(unsigned char)etherHdrPtr->source.byte5,
			(unsigned char)etherHdrPtr->source.byte6,
			(unsigned char)etherHdrPtr->destination.byte1,
			(unsigned char)etherHdrPtr->destination.byte2,
			(unsigned char)etherHdrPtr->destination.byte3,
			(unsigned char)etherHdrPtr->destination.byte4,
			(unsigned char)etherHdrPtr->destination.byte5,
			(unsigned char)etherHdrPtr->destination.byte6,
			Net_NetToHostShort(etherHdrPtr->type));
	}
    }

    if (Net_NetToHostShort(etherHdrPtr->type) != NET_ETHER_IP) {
	if (dbgTraceLevel >= 5) {
	    printf("Non-IP (Type=0x%x) ",
		    Net_NetToHostShort(etherHdrPtr->type));
	}
	return;
    }
    if (gotPacket) {
	return;
    }
    if (dbgTraceLevel >= 4) {
	printf("Validating packet\n");
    }
    if (Dbg_ValidatePacket(packetLength - sizeof(Net_EtherHdr),
			   (Net_IPHeader *)(packetPtr + sizeof(Net_EtherHdr)),
			   &dataLength, &dataPtr,
			   &dbgMyIPAddr, &dbgSrcIPAddr, &dbgSrcPort)) {
	if (dbgTraceLevel >= 4) {
	    printf("Got a packet: length=%d\n", dataLength);
	}
	bcopy((Address)etherHdrPtr, (Address)&dbgEtherHdr,
		sizeof(Net_EtherHdr));
	gotPacket = TRUE;
	bcopy(dataPtr, requestBuffer, dataLength);
	/*
	 * Set the interface we are using. 
	 */
	dbgInterPtr = interPtr;
	return;
    }

    if (Net_NetToHostShort(etherHdrPtr->type) != 0x800) {
	return;
    }
    /*
     * Handle ICMP echo requests.  This is a good test of the
     * basic networking code.
     */
    bcopy(packetPtr, pingBufPtr, packetLength);
    etherHdrPtr = (Net_EtherHdr *)pingBufPtr;
    ipPtr = (Net_IPHeader *)(pingBufPtr + sizeof(Net_EtherHdr));
    if (ipPtr->protocol == NET_IP_PROTOCOL_ICMP) {
	Net_InetAddress	tAddr;
	Net_EtherAddress	tEtherAddr;
	Net_ICMPHeader	*icmpPtr;

	tEtherAddr = etherHdrPtr->source;
	etherHdrPtr->source = etherHdrPtr->destination;
	etherHdrPtr->destination = tEtherAddr;
	tAddr = ipPtr->source;
	ipPtr->source = ipPtr->dest;
	ipPtr->dest = tAddr;
	icmpPtr = (Net_ICMPHeader *)((Address)ipPtr + ipPtr->headerLen * 4);
	if (icmpPtr->type == 8) {
	    Mach_MonPrintf("ICMP Echo\n");
	    icmpPtr->type = 0;
	    icmpPtr->checksum = 0;
	    icmpPtr->checksum = 
		Net_InetChecksum(
		(int)(Net_NetToHostShort(ipPtr->totalLen)-ipPtr->headerLen*4),
		(Address)icmpPtr);
	    dbgGather.length = packetLength - sizeof(Net_EtherHdr);
	    dbgGather.bufAddr = pingBufPtr + sizeof(Net_EtherHdr);
	    Net_RawOutput(dbgInterPtr, etherHdrPtr, &dbgGather, 1);
	}
    }
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

    gotPacket = FALSE;
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
	    if (gotPacket) {
		break;
	    }
	}
	if (timeout) {
	    timeOutCounter--;
	}
    } while(!gotPacket && timeOutCounter != 0);
    if (gotPacket) {
	requestPtr = (Dbg_Request *)requestBuffer;
	if (dbgTraceLevel >= 4) {
	    printf("MsgNum = %d\n", requestPtr->num);
	}
    }

    return(gotPacket);
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

    if (dbgTraceLevel >= 4) {
	printf("Sending reply\n");
    }
    replyPtr->num = requestPtr->num;

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
	printf("Sent reply\n");
    }
}

/*
 * Whether syslog should remain diverted on continue or not.
 */
static Boolean	syslogDiverted = FALSE;

/*
 * Should we sync the disks on entering the debugger?
 */

Boolean dbgSyncDisks = TRUE;

extern Mach_DebugState	mach_DebugState;

/*
 * Place where the TLB is dumped when the kernel enters the debugger.
 */
unsigned dbgTLB[VMMACH_NUM_TLB_ENTRIES][2];

Mach_DebugState	tmpDebugState;
Mach_DebugState	*debugStatePtr;


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
Dbg_Main()
{
    Boolean	done;
    Boolean	atInterruptLevel;
    unsigned	cause;
    int		signal;
    int		origSignal;
    int		dataSize;
    int		origMaxStackAddr;

    if (!dbg_BeingDebugged && dbgSyncDisks) {
	/*
	 * Try to sync the disks if we aren't at interrupt level.  If we
	 * are don't bother because we'll just hang waiting for interrupts.
	 * Of course I could force interrupts to be enabled but I'm not sure
	 * if that's a great idea.
	 */
	if (mach_NumDisableIntrsPtr[0] == 0 &&
	    !mach_AtInterruptLevel) {
	    Mach_EnableIntr();
	    Sys_SyncDisks(MACH_OTHER_TRAP_TYPE);
	    Mach_DisableIntr();
	}
    }

    dbg_InDebugger = TRUE;

    origMaxStackAddr = dbgMaxStackAddr;
    debugStatePtr = &mach_DebugState;

    if (dbgTraceLevel >= 1) {
	unsigned int	*sp;
	extern int etext;
    
	Mach_MonPrintf("\nCause=%x SR=%x excPC=%x SP=%x BVA=%x\n", 
		       mach_DebugState.causeReg, mach_DebugState.statusReg, 
		       mach_DebugState.excPC, mach_DebugState.regs[SP],
		       mach_DebugState.badVaddr);
	Mach_MonPrintf("Stack:\n");
	for (sp = (unsigned *)mach_DebugState.regs[SP]; 
	     sp < (unsigned *)0x80030000; 
	     sp++) {
	    if ((*sp < (unsigned int)&etext) && (*sp >= (unsigned)0x80030000)) {
		Mach_MonPrintf("%x\n", *sp);
	    }
	}
    }

    atInterruptLevel = mach_AtInterruptLevel;
    mach_AtInterruptLevel = TRUE;

    /*
     * Force system log output to the console.
     */
    if (!syslogDiverted) {
	Dev_SyslogDebug(TRUE);
    }

    /*
     * We want to inform the user what caused the problem.  However we only
     * tell him if: 1) we are debugging the debugger; 2) we are not under 
     * debugger control (i.e. we don't want to inform the user on every trace 
     * trap), 3) we got something besides a trace trap or a breakpoint trap
     * exception.
     */
    cause = (mach_DebugState.causeReg & MACH_CR_EXC_CODE) >> 
						MACH_CR_EXC_CODE_SHIFT;
    if (dbgPanic) {
	dbgPanic = FALSE;
	mach_DebugState.excPC = mach_DebugState.regs[RA];
    }

    if (dbgTraceLevel >= 1 || !dbg_BeingDebugged || 
        cause != MACH_EXC_BREAK) {
	(void)Dev_VidEnable(TRUE); /* unblank the screen */
	printf("Entering debugger with a %s exception at PC 0x%x\r\n",
		   TranslateException((int)cause),
		   mach_DebugState.excPC);
    }

    switch (cause) {
        case MACH_EXC_INT:
	    signal = 2;
	    break;
        case MACH_EXC_TLB_MOD:
        case MACH_EXC_TLB_LD_MISS:
        case MACH_EXC_TLB_ST_MISS:
	    signal = 11;
	    break;
        case MACH_EXC_ADDR_ERR_LD:
        case MACH_EXC_ADDR_ERR_ST:
	    signal = 10;
	    break;
        case MACH_EXC_BUS_ERR_IFETCH:
        case MACH_EXC_BUS_ERR_LD_ST:
	    signal = 10;
	    break;
        case MACH_EXC_SYSCALL:
	    signal = 12;
	    break;
        case MACH_EXC_BREAK: {
	    unsigned		*pc;

	    signal = 5;
	    if (mach_DebugState.causeReg & MACH_CR_BR_DELAY) {
		pc = (unsigned *)(mach_DebugState.excPC + 4);
	    } else {
		pc = (unsigned *)mach_DebugState.excPC;
	    }
	    if (dbgTraceLevel >= 1) {
		printf("break inst: %x\n", *pc);
	    }
	    if ((*pc & MACH_BREAK_CODE_FIELD) == MACH_SSTEP_VAL) {
		if (dbgTraceLevel >= 1) {
		    printf("sstep\n");
		}
		mach_DebugState.trapCause = CAUSE_SINGLE;
		if (dbgTraceLevel >= 1) {
		    printf("sstep (%x) = %x\n", pc, sstepInst);
		}
		Mach_FlushCode((Address)pc, 4);
		*pc = sstepInst;
		Mach_EmptyWriteBuffer();
	    } else {
		mach_DebugState.trapCause = CAUSE_BREAK;
	    }
	    break;
	}
        case MACH_EXC_RES_INST:
	    signal = 4;
	    break;
        case MACH_EXC_COP_UNUSABLE:
	    signal = 4;
	    break;
        case MACH_EXC_OVFLOW:
	    signal = 4;
	    break;
	default:
	    printf("Bad cause\n");
	    signal = 4;
	    break;
    }
    origSignal = signal;

    dbg_UsingNetwork = TRUE;

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
	    printf("TI: %d ", requestPtr->request);
	} while (TRUE);
    } else {
	(void) ReadRequest(FALSE);
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
	    printf("Request: %d, %s at %x\n", request, 
			TranslateOpcode(request), requestPtr->addr);
	}
	switch (request) {
            case DBG_UREAD:
		if (requestPtr->addr > sizeof(mach_DebugState) / 4) {
		    printf("Bogus UREAD addr %x\n", requestPtr->addr);
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
			    printf("Bad process table index %d\n", 
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
			printf("Bogus UWRITE addr %x\n", requestPtr->addr);
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
		    printf("Single-step PC=%x\n", pc);
		}
		if (!Dbg_InRange((unsigned int)pc, 4, TRUE)) {
		    printf("Bad SSTEP PC\n");
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
		printf("Sprite is now detached from the debugger\r\n");
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

    /*
     * Don't force system log output to the console.
     */
    if (!syslogDiverted) {
	Dev_SyslogDebug(FALSE);
    }

    if (dbgTraceLevel >= 1) {
	printf("Returning to %x: %x\n", debugStatePtr->excPC, 
				*(unsigned *)debugStatePtr->excPC);
    }

    mach_AtInterruptLevel = atInterruptLevel;
    if (dbg_OthersCanUseNetwork) {
	dbg_UsingNetwork = FALSE;
    }
    /*
     * Flush out the old TLB mapping.
     */
    VmMachWriteIndexedTLB(0, 0, 0);

    dbg_InDebugger = FALSE;

    return(debugStatePtr->excPC);
}
