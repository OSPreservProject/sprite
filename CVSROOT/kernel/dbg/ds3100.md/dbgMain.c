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

#ifdef KDBX
#include <user/signal.h>
#endif

static unsigned sstepInst;			/* The instruction that was
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
static char requestBuffer[DBG_MAX_REQUEST_SIZE];/* Buffer to receive request
						 * into. */
static int	requestOffset;			/* Offset in buffer where next
						 * bytes should be read from.*/
static char replyBuffer[DBG_MAX_REPLY_SIZE + 2];/* Buffer to hold reply. */
static int	replyOffset = 0;		/* Offset in buffer where next
						 * bytes in reply should go. */
static unsigned int	curMsgNum;		/* The current message that
						 * is being processed. */
int	dbgTraceLevel = 0;			/* The debugger tracing
						 * level. */

/*
 * Number of times to poll before timing out and resending (about 2 seconds).
 */
int	dbgTimeout = 15000;

/*
 * Information about the latest packet received.
 */
Boolean			dbgGotPacket;	
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
#define	PACKET_HDR_SIZE (sizeof(Net_EtherHdr) + Dbg_PacketHdrSize() + 4 + 2)
#define PACKET_DATA_SIZE (DBG_MAX_REPLY_SIZE - PACKET_HDR_SIZE)


/*
 * Strings which describe each of the opcodes that kdbx can send us.
 */
static char *opcodeNames[] = DBG_OPCODE_NAMES ;

/*
 * Strings which describe the different exceptions that can occur.
*/
static char *exceptionNames[] = DBG_EXCEPTION_NAMES;

/*
 * Whether syslog should remain diverted on continue or not.
 */
static Boolean	syslogDiverted = FALSE;

/*
 * Declare global variables.
 */
int		dbgTermReason;
int		dbgInDebugger;
int		dbgIntPending;
Boolean		dbgPanic;
Boolean		dbg_UsingSyslog = FALSE;
Boolean		dbgCanUseSyslog = TRUE;
int		dbgMaxStackAddr;
#ifdef KDBX
int		dbgSignal;
static Boolean	useKdbx = FALSE;
#endif
/*
 * Trap causes (same numbering as in ptrace.h).
 */
#define CAUSE_SINGLE	4
#define CAUSE_BREAK	5

/*
 * Trap instruction.
 */
#define SSTEP_INST	(MACH_SSTEP_VAL | 0xd)

/* 
 * Forward declarations:
 */

static char *	TranslateOpcode _ARGS_((int opcode));
static char *	TranslateException _ARGS_((int exception));
static Boolean	ReadRequest _ARGS_((Boolean timeout));
static void	SendReply _ARGS_((int dataSize));
static void	DebugToRegState _ARGS_((Mach_DebugState *debugPtr, 
			Mach_RegState *regPtr));
static void	RegStateToDebug _ARGS_((Mach_RegState *regPtr, 
		    Mach_DebugState *debugPtr)); 

#ifdef KDBX
static int	sigMap[] = {
    /* MACH_EXC_INT 		*/		SIGILL,
    /* MACH_EXC_TLB_MOD 	*/		SIGSEGV,
    /* MACH_EXC_TLB_LD_MISS 	*/		SIGSEGV, 
    /* MACH_EXC_TLB_ST_MISS 	*/		SIGSEGV, 
    /* MACH_EXC_ADDR_ERR_LD 	*/ 		SIGBUS,
    /* MACH_EXC_ADDR_ERR_ST 	*/		SIGBUS,
    /* MACH_EXC_BUS_ERR_IFETCH 	*/		SIGBUS,
    /* MACH_EXC_BUS_ERR_LD_ST 	*/		SIGBUS,
    /* MACH_EXC_SYSCALL 	*/		SIGSYS,
    /* MACH_EXC_BREAK 		*/		SIGTRAP,
    /* MACH_EXC_RES_INST 	*/		SIGILL,
    /* MACH_EXC_COP_UNUSABLE 	*/		SIGILL, 
    /* MACH_EXC_OVFLOW 		*/		SIGILL,
};

#endif


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
	Mach_MonPrintf("Dbg_InRange: Object spans pages\n");
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
#ifdef KDBX
    extern void DbgDbxInit();
#endif
    dbgInDebugger = 0;
    dbgIntPending = 0;
    dbgPanic = FALSE;
    dbg_BeingDebugged = FALSE;
#ifdef KDBX
    DbgDbxInit();
#endif
}



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
 *     dbgGotPacket is set to true if we got a packet that we liked.
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
	Mach_MonPrintf("Got a debugger packet on non-ethernet interface %s\n",
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
	    Mach_MonPrintf("Non-IP (Type=0x%x) ",
		    Net_NetToHostShort(etherHdrPtr->type));
	}
	return;
    }
    if (dbgGotPacket) {
	if (dbgTraceLevel >= 4) {
	    Mach_MonPrintf("Dbg_InputPacket: already have a packet\n");
	}
	return;
    }
    if (dbgTraceLevel >= 4) {
	Mach_MonPrintf("Validating packet\n");
    }
    { 
	static char alignedBuffer[NET_ETHER_MAX_BYTES];
        /*
	 * Make sure the packet starts on a 32-bit boundry so that we can
	 * use structures for describe the data.
	 */
	if ( (unsigned int) (packetPtr + sizeof(Net_EtherHdr)) & 0x3 ) {
	      bcopy (packetPtr + sizeof(Net_EtherHdr), alignedBuffer,
			    packetLength - sizeof(Net_EtherHdr));
	      packetPtr = alignedBuffer;
	} else {
	      packetPtr = packetPtr + sizeof(Net_EtherHdr);
	}

	if (Dbg_ValidatePacket(packetLength - sizeof(Net_EtherHdr),
			       (Net_IPHeader *)(packetPtr),
			       &dataLength, &dataPtr,
			       &dbgMyIPAddr, &dbgSrcIPAddr, &dbgSrcPort)) {
	    if (dbgTraceLevel >= 4) {
		Mach_MonPrintf("Got a packet: length=%d\n", dataLength);
	    }
	    bcopy((Address)etherHdrPtr, (Address)&dbgEtherHdr,
		    sizeof(Net_EtherHdr));
	    dbgGotPacket = TRUE;
	    bcopy(dataPtr, requestBuffer, dataLength);
#ifdef KDBX
	    DbgDbxStoreRequest(dataLength, requestBuffer);
#endif
	    /*
	     * Set the interface we are using. 
	     */
	    dbgInterPtr = interPtr;
	    return;
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

    dbgGotPacket = FALSE;
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
	replyOffset = PACKET_HDR_SIZE;
	requestOffset = 4;
	curMsgNum = *(unsigned int *)(requestBuffer);
#ifdef KDBX
	if (curMsgNum > 0x40000000) {
	    useKdbx = FALSE;
	} else {
	    useKdbx = TRUE;
	}
#endif
	if (dbgTraceLevel >= 4) {
	    Mach_MonPrintf("MsgNum = %d\n", curMsgNum);
	}
    }

    return(dbgGotPacket);
}

/*
 * ----------------------------------------------------------------------------
 *
 * GetRequestBytes --
 *
 *     Get the next numBytes bytes from the current request.
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
GetRequestBytes(numBytes, dest)
    int		numBytes;
    Address	dest;
{
	bcopy(requestBuffer + requestOffset, dest, numBytes);
	requestOffset += numBytes;
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutReplyBytes --
 *
 *     Put the given bytes into the reply buffer.
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
PutReplyBytes(numBytes, src)
    int		numBytes;
    Address	src;
{
    if (replyOffset + numBytes > DBG_MAX_REPLY_SIZE) {
	Mach_MonPrintf("PutReplyBytes: Buffer overflow\n");
	numBytes = DBG_MAX_REPLY_SIZE - replyOffset;
    }
    bcopy(src, &replyBuffer[replyOffset], numBytes);
    replyOffset += numBytes;
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
	Mach_MonPrintf("Sending reply\n");
    }

    etherHdrPtr = (Net_EtherHdr *) (replyBuffer+2);
    etherHdrPtr->source = dbgEtherHdr.destination;
    etherHdrPtr->destination = dbgEtherHdr.source;
    etherHdrPtr->type = dbgEtherHdr.type;
    dbgGather.bufAddr = replyBuffer + sizeof(Net_EtherHdr)+2;
    dbgGather.length = replyOffset - sizeof(Net_EtherHdr)-2;
    dbgGather.mutexPtr = (Sync_Semaphore *) NIL;
    bcopy((char *)&curMsgNum,(char *)(replyBuffer + PACKET_HDR_SIZE - 4),4);
    Dbg_FormatPacket(dbgMyIPAddr, dbgSrcIPAddr, dbgSrcPort,
		 replyOffset - sizeof(Net_EtherHdr) - Dbg_PacketHdrSize()-2,
		 replyBuffer + sizeof(Net_EtherHdr) + 2);
    Net_RawOutput(dbgInterPtr, etherHdrPtr, &dbgGather, 1);
    if (dbgTraceLevel >= 4) {
	Mach_MonPrintf("Sent reply\n");
    }
}


/*
 * Should we sync the disks on entering the debugger?
 */

Boolean dbgSyncDisks = TRUE;

extern Mach_DebugState	mach_DebugState;

/*
 * Place where the TLB is dumped when the kernel enters the debugger.
 */
unsigned dbgTLB[VMMACH_NUM_TLB_ENTRIES][2];

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
Dbg_Main()
{
    unsigned	cause;
    Boolean	  	done;		/* Boolean to tell us whether to leave
					 * the main debugger loop */
    Dbg_Opcode	  	opcode;	        /* The operation that was requested */
					/* Process table entry that we switched
					 * stacks to. */
    Proc_ControlBlock	*procPtr = (Proc_ControlBlock *) NIL;
    Boolean		atInterruptLevel;/* TRUE if we were entered from an
					  * interrupt handler. */
    extern int Mach_SwitchPoint();
#ifdef NOTDEF
/*
 * This code causes machines to "pop out" of the debugger.
 */
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
#endif NOTDEF

    dbg_InDebugger = TRUE;

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
#ifdef KDBX
    if ((cause >= 0) && (cause < MACH_EXC_MAX)) {
	dbgSignal = sigMap[cause];
    } else {
	dbgSignal = SIGILL;
    }
#endif
    if (cause == MACH_EXC_BREAK) {
	unsigned		*pc;

	if (mach_DebugState.causeReg & MACH_CR_BR_DELAY) {
	    pc = (unsigned *)(mach_DebugState.excPC + 4);
	} else {
	    pc = (unsigned *)mach_DebugState.excPC;
	}
	if (dbgTraceLevel >= 1) {
	    Mach_MonPrintf("break inst: %x\n", *pc);
	}
	if ((*pc & MACH_BREAK_CODE_FIELD) == MACH_SSTEP_VAL) {
	    if (dbgTraceLevel >= 1) {
		Mach_MonPrintf("sstep\n");
	    }
	    mach_DebugState.trapCause = CAUSE_SINGLE;
	    if (dbgTraceLevel >= 1) {
		Mach_MonPrintf("sstep (%x) = %x\n", pc, sstepInst);
	    }
	    Mach_FlushCode((Address)pc, 4);
	    *pc = sstepInst;
	    Mach_EmptyWriteBuffer();
	} else {
	    mach_DebugState.trapCause = CAUSE_BREAK;
	}
    }

    dbg_UsingNetwork = TRUE;

    if (dbg_BeingDebugged) {
        unsigned        char    ch;
	int	timeout = 5;
        ch = 0;
#ifdef KDBX
	if (useKdbx) {
	    Dbg_DbxMain();
	    goto there;
	}
#endif
        PutReplyBytes(1, (Address)&ch);
        SendReply();
        do {
            if (ReadRequest(TRUE)) {
                GetRequestBytes(4, (Address)&opcode);
		if (opcode != DBG_CONTINUE) {
		    break;
		} else {
		    PutReplyBytes(4, (Address) &opcode);
		    SendReply();
		    continue;
		}
            }
            /*
             * We can only timeout if we are using network debugging.
             */
	    Net_RawOutput(dbgInterPtr, (Address) replyBuffer, 
		    &dbgGather, 1);
            if (dbgTraceLevel >= 5) {
                Mach_MonPrintf("DBG: Timeout\n");
            }
            Mach_MonPrintf("TI ");
        } while (timeout-- > 0);
    } else {
        (void) ReadRequest(FALSE);
#ifdef KDBX
	if (useKdbx) {
	    Dbg_DbxMain();
	    goto there;
	}
#endif
        GetRequestBytes(4, (Address)&opcode);
    }

    Vm_MachDumpTLB(dbgTLB);

    done = FALSE;
    while (!done) {
	if (dbgTraceLevel >= 2) {
	    Mach_MonPrintf("Request: (%d) %s\n", opcode, TranslateOpcode(opcode));
	}
	switch (opcode) {

	    /*
	     * The client wants to read some data from us ...
	     */

	    case DBG_GET_STOP_INFO: {
		StopInfo	stopInfo;
		stopInfo.codeStart = (int)mach_CodeStart;
		if (procPtr != (Proc_ControlBlock *) NIL &&
		    procPtr->machStatePtr != (Mach_State *)NIL) {
		    stopInfo.regs = procPtr->machStatePtr->switchRegState;
		    /*
		     * The pc isn't stored in switchRegState, but we know
		     * that we had to be in Mach_ContextSwitch.  Also,
		     * when the context switch is done the status register
		     * and a magic number are pushed on the stack. We
		     * need to adjust the sp so that the debugger doesn't
		     * get confused by them. 
		     */
		    stopInfo.regs.pc = (Address) 
			    ((char *) Mach_ContextSwitch + 16);
		    stopInfo.regs.regs[SP] += 8;
		} else {
		    DebugToRegState(&mach_DebugState, &stopInfo.regs);
		}
		stopInfo.trapType = cause;
		PutReplyBytes(sizeof(stopInfo), (Address)&stopInfo);
		SendReply();
		break;
	    }
	    case DBG_READ_ALL_REGS: {
		Mach_RegState	regState;
		if (procPtr != (Proc_ControlBlock *) NIL &&
		    procPtr->machStatePtr != (Mach_State *)NIL) {
		    regState = procPtr->machStatePtr->switchRegState;
		    regState.pc = (Address) (Mach_SwitchPoint);
		    regState.pc = (Address) 
			    ((char *) Mach_ContextSwitch + 16);
		    regState.regs[SP] += 8;
		} else {
		    DebugToRegState(&mach_DebugState, &regState);
		}
		PutReplyBytes(sizeof(regState), (Address) &regState);
		SendReply();
	    }
#if 0
	    case DBG_GET_DUMP_BOUNDS: {
		Dbg_DumpBounds bounds;
		extern unsigned int end;
		bounds.pageSize = vm_PageSize;
		bounds.stackSize = mach_KernStackSize;
		bounds.kernelCodeStart = (unsigned int) mach_KernStart;
		bounds.kernelCodeSize  = 
			(unsigned int) (((Address)(&end)) - mach_KernStart);
		bounds.kernelDataStart	= ((unsigned int)(&end));
		bounds.kernelDataSize	= (unsigned int) 
				(vmMemEnd - ((Address)(&end)));
		bounds.kernelStacksStart = (unsigned int)vmStackBaseAddr;
		bounds.kernelStacksSize = (unsigned int) 
				(vmStackEndAddr - vmStackBaseAddr);
		bounds.fileCacheStart	= (unsigned int)vmBlockCacheBaseAddr;
		bounds.fileCacheSize	= (unsigned int) (vmBlockCacheEndAddr - 
						vmBlockCacheBaseAddr);

		PutReplyBytes(sizeof(bounds), (char *)&bounds);
		SendReply();
		break;
	    }
#endif
	    case DBG_GET_VERSION_STRING: {
		char	*version;

		version = SpriteVersion();
		PutReplyBytes(strlen(version) + 1, version);
		SendReply();
		break;
	    }
	    case DBG_INST_READ:
	    case DBG_DATA_READ: {
		Dbg_ReadMem	readMem;
		int		status;

		GetRequestBytes(sizeof(readMem), (Address) &readMem); 
		if (dbgTraceLevel >= 2) {
		    Mach_MonPrintf("Addr=%x Numbytes=%d ",
				readMem.address, readMem.numBytes);
		}
		if (Dbg_InRange(readMem.address, readMem.numBytes, FALSE)) {
		    status = 1;
		    PutReplyBytes(sizeof(status), (Address)&status);
		    PutReplyBytes(readMem.numBytes, (Address)readMem.address);
		} else {
		    if (dbgTraceLevel >= 2) {
			Mach_MonPrintf("FAILURE ");
		    }
		    status = 0;
		    PutReplyBytes(sizeof(status), (Address)&status);
		}
		SendReply();
		break;
	    }
	    case DBG_SET_PID: {
		Proc_PID	pid;

		GetRequestBytes(sizeof(pid), (Address) &pid);
		 {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}
		if (dbgTraceLevel >= 2) {
		    Mach_MonPrintf("pid %x ", pid);
		}
		if (pid == 0) {
		    procPtr = (Proc_ControlBlock *) NIL;
		} else {
		    procPtr = Proc_GetPCB(pid);
		    if (procPtr == (Proc_ControlBlock *) NIL ||
		        procPtr == (Proc_ControlBlock *) 0 ||
			procPtr->state == PROC_UNUSED ||
		        procPtr->state == PROC_DEAD ||
			procPtr->state == PROC_NEW) {
			Mach_MonPrintf("Can't backtrace stack for process %x\n",
					pid);
			procPtr = (Proc_ControlBlock *) NIL;
		    }
		}
		break;
	    }
	    case DBG_REBOOT: {
		int	stringLength;
		char	rebootString[100];
		/*
		 * For a reboot command first read the size of the string and
		 * then the string itself.
		 */
		GetRequestBytes(sizeof(int), (Address)&stringLength);
		if (stringLength != 0) {
		    GetRequestBytes(stringLength, (Address)rebootString);
		}
		rebootString[stringLength] = '\0';
		 {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}
		Mach_MonReboot(rebootString);
	    }
	    case DBG_INST_WRITE:
	    case DBG_DATA_WRITE: {
		Dbg_WriteMem		writeMem;
		unsigned	char	ch;
		/*
		 * For an instruction or a data write we first have to find out 
		 * which address to write to and how many bytes to write.  Next
		 * we have to make sure that the address is valid.  If it is
		 * then we read the data and write it to the given address.  If
		 * not we just report an error to kdbx.
		 */
		GetRequestBytes(2 * sizeof(int), (Address) &writeMem);
		if (dbgTraceLevel >= 2) {
		    Mach_MonPrintf("Addr=%x Numbytes=%d ",
				writeMem.address, writeMem.numBytes);
		}
		if (Dbg_InRange((unsigned int) writeMem.address,
			    writeMem.numBytes, opcode == DBG_DATA_WRITE)) {
		    GetRequestBytes(writeMem.numBytes,
				    (Address) writeMem.address);
		    if (opcode == DBG_INST_WRITE) {
			Mach_FlushCode((Address)writeMem.address, 
			    writeMem.numBytes);
			Mach_EmptyWriteBuffer();
		    }
		    ch = 1;
		} else {
		    char	buf[100];

		    if (dbgTraceLevel >= 2) {
			Mach_MonPrintf("FAILURE ");
		    }
		    GetRequestBytes(writeMem.numBytes, buf);
		    ch = 0;
		}
		PutReplyBytes(1, (char *) &ch);
		SendReply();

		break;
	    }
	    case DBG_WRITE_REG: {                
		Mach_RegState	regState;
		Dbg_WriteReg	writeReg;

		/*
		 * First find out which register is being written and
		 * then read the value.
		 */
		GetRequestBytes(sizeof(writeReg), (Address)&writeReg);
		 {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}
		if (dbgTraceLevel >= 2) {
		    Mach_MonPrintf("register %d data %x ", writeReg.regNum, 
				writeReg.regVal);
		}
		if (procPtr != (Proc_ControlBlock *) NIL &&
		    procPtr->machStatePtr != (Mach_State *)NIL) {
		    ((int *) 
		    &procPtr->machStatePtr->switchRegState)[(writeReg.regNum)] 
			= writeReg.regVal;
		} else {
		    DebugToRegState(&mach_DebugState, &regState);
		    ((int *) &regState)[(writeReg.regNum)] = writeReg.regVal;
		    RegStateToDebug(&regState, &mach_DebugState);
		}
	    }
	    case DBG_DIVERT_SYSLOG: 
		GetRequestBytes(sizeof(Boolean), (Address)&syslogDiverted);
		 {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}
		break;
	    case DBG_BEGIN_CALL: {
		/*
		 * We are beginning a call command.  Fix up the stack
		 * so that we will be able to continue.  We will put
		 * it back when we are done.
		 */
		int	dummy;
		if (dbgCanUseSyslog) {
		    dbg_UsingSyslog = TRUE;
		}

		PutReplyBytes(4, (Address) &dummy);
		SendReply();

		break;
	    }
	    case DBG_END_CALL: {
		char	*buffer;
		int	*firstIndexPtr;
		int	*lastIndexPtr;
		int	bufSize;
		int	length;
		/*
		 * Dump the syslog buffer.
		 */
		Dev_SyslogReturnBuffer(&buffer, &firstIndexPtr,
				       &lastIndexPtr, &bufSize);
		if (*firstIndexPtr == -1) {
		    length = 0;
		    PutReplyBytes(4, (Address) &length);
		    dbg_UsingSyslog = FALSE;
		} else if (*firstIndexPtr <= *lastIndexPtr) {
		    length = *lastIndexPtr - *firstIndexPtr + 1;
		    if (length + 4 > PACKET_DATA_SIZE) {
			length = PACKET_DATA_SIZE - 4;
		    }
		    PutReplyBytes(4, (Address) &length);
		    PutReplyBytes(length,
				  (Address)&buffer[*firstIndexPtr]);
		    *firstIndexPtr += length;
		    if (*firstIndexPtr > *lastIndexPtr) {
			*firstIndexPtr = *lastIndexPtr = -1;
		    }
		} else {
		    length = bufSize - *firstIndexPtr;
		    if (length + 4 > PACKET_DATA_SIZE) {
			length = PACKET_DATA_SIZE - 4;
		    }
		    PutReplyBytes(4, (Address) &length);
		    PutReplyBytes(length,
				  (Address)buffer[*firstIndexPtr]);
		    *firstIndexPtr += length;
		    if (*firstIndexPtr == bufSize) {
			*firstIndexPtr = 0;
		    }
		}
		SendReply();
		break;
	    }
	    case DBG_CALL_FUNCTION: {
		Dbg_CallFunc		callFunc;
		int			returnVal;
		static int		argBuf[128];
		GetRequestBytes(2 * sizeof(int), (Address) &callFunc);
		if (dbgTraceLevel >= 2) {
		    Mach_MonPrintf("Addr=%x Numbytes=%d ",
				callFunc.address, callFunc.numBytes);
		}
		if ((callFunc.numBytes >= 0 && callFunc.numBytes < 128) &&
		     Dbg_InRange((unsigned int) callFunc.address,4,FALSE)) {
		    GetRequestBytes(callFunc.numBytes,(Address) argBuf);
		    returnVal = (* ((int (*)()) callFunc.address))(argBuf[0],
		    argBuf[1],argBuf[2],argBuf[3],argBuf[4],argBuf[5],argBuf[6],
		    argBuf[7],argBuf[8],argBuf[9]);
		} else {

		    if (dbgTraceLevel >= 2) {
			Mach_MonPrintf("FAILURE ");
		    }
		    GetRequestBytes(callFunc.numBytes,(Address)argBuf);
		    returnVal = -1;
		}
		PutReplyBytes(4, (char *) &returnVal);
		SendReply();

		break;
	    }
	    case DBG_CONTINUE: {
		/*
		 * The client wants to continue execution.
		 */
		int	foo;
		GetRequestBytes(sizeof(int), 
			    (Address) &foo);
		if (dbgTraceLevel >= 2) {
		    Mach_MonPrintf("Continuing from pc %x ",debugStatePtr->excPC);
		}
		{
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}

		dbg_BeingDebugged = TRUE;
		done = TRUE;
		break;
	    }
	    case DBG_SINGLESTEP: {
		/*
		 * The client wants to single step.
		 */
		unsigned		*pc;
		int			status;
		int			dummy;

		GetRequestBytes(sizeof(int),  (Address) &dummy);
		pc = DbgGetDestPC((Address)(debugStatePtr->excPC));
		if (dbgTraceLevel >= 1) {
		    Mach_MonPrintf("Single-step PC=%x\n", pc);
		}
		if (!Dbg_InRange((unsigned int)pc, 4, TRUE)) {
		    Mach_MonPrintf("Bad SSTEP PC\n");
		    status = 0;
		} else {
		    sstepInst = *pc;
		    Mach_FlushCode((Address)pc, 4);
		    *pc = SSTEP_INST;
		    Mach_EmptyWriteBuffer();
		    dbg_BeingDebugged = TRUE;
		    done = TRUE;
		    status = 1;
		}
		PutReplyBytes(sizeof(status), (Address) &status);
		SendReply();
		break;
	    }
	    case DBG_DETACH:
		/*
		 * The debugger has terminated and wants to let us go about our
		 * business.
		 */
		if (dbgTraceLevel >= 2) {
		    Mach_MonPrintf("Detaching at pc %x ", debugStatePtr->excPC);
		}
	        {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}

		dbg_BeingDebugged = FALSE;
		done = TRUE;
		printf("Sprite is now detached from the debugger\r\n");
		break;
	    case DBG_UNKNOWN:
		Mach_MonPrintf("debugger: unrecognized request\n");
		break;
	}
	if (dbgTraceLevel >= 2) {
	    Mach_MonPrintf("\r\n");
	}
	if (!done) {
	    (void)ReadRequest(FALSE);
	    GetRequestBytes(4, (Address)&opcode);
	}
    }

#ifdef KDBX
there:
#endif
    /*
     * Don't force system log output to the console.
     */
    if (!syslogDiverted) {
	Dev_SyslogDebug(FALSE);
    }

    if (dbgTraceLevel >= 1) {
	Mach_MonPrintf("Returning to %x: %x\n", debugStatePtr->excPC, 
				*(unsigned *)debugStatePtr->excPC);
    }

    mach_AtInterruptLevel = atInterruptLevel;
    dbg_UsingNetwork = FALSE;
    /*
     * Flush out the old TLB mapping.
     */
    VmMachWriteIndexedTLB(0, 0, 0);

    dbg_InDebugger = FALSE;

    return(debugStatePtr->excPC);
}

/*
 *----------------------------------------------------------------------
 *
 * DebugToRegState --
 *
 *	Converts a Mach_DebugState to Mach_RegState.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
DebugToRegState(debugPtr, regPtr)
    Mach_DebugState	*debugPtr;
    Mach_RegState	*regPtr;
{
    regPtr->pc = (Address) debugPtr->excPC;
    bcopy((char *) debugPtr->regs, (char *) regPtr->regs, 
	MACH_NUM_GPRS * sizeof(int));
    bcopy((char *) debugPtr->fpRegs, (char *) regPtr->fpRegs, 
	MACH_NUM_FPRS * sizeof(int));
    regPtr->fpStatusReg = debugPtr->fpCSR;
    regPtr->mfhi = debugPtr->multHi;
    regPtr->mflo = debugPtr->multLo;
}

/*
 *----------------------------------------------------------------------
 *
 * RegStateToDebug --
 *
 *	Converts a Mach_RegState to Mach_DebugState.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
RegStateToDebug(regPtr, debugPtr)
    Mach_RegState	*regPtr;
    Mach_DebugState	*debugPtr;
{
    debugPtr->excPC = (unsigned) regPtr->pc;
    bcopy((char *) regPtr->regs, (char *) debugPtr->regs, 
	MACH_NUM_GPRS * sizeof(int));
    bcopy((char *) regPtr->fpRegs, (char *) debugPtr->fpRegs, 
	MACH_NUM_FPRS * sizeof(int));
    debugPtr->fpCSR = regPtr->fpStatusReg;
    debugPtr->multHi = regPtr->mfhi;
    debugPtr->multLo = regPtr->mflo;
}

