/* dbgMain.c -
 *
 *     This contains the routines which read and execute commands from kdbx.
 *
 *     All reads and writes to kdbx occur over channel A of the Rs232 line.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <dbg.h>
#include <dbgInt.h>
#include <main.h>
#include <mach.h>
#include <proc.h>
#include <vm.h>
#include <vmInt.h>
#include <vmMach.h>
#include <vmMachInt.h>
#include <machMon.h>
#include <net.h>
#include <netEther.h>
#include <netInet.h>
#include <dev.h>
#include <devVid.h>
#include <bstring.h>
#include <string.h>
#include <stdio.h>

Boolean	dbg_BeingDebugged = FALSE;		/* TRUE if are under control
						 * of kdbx.*/
Boolean	dbg_UsingNetwork = FALSE;		/* TRUE if the debugger is
						 * using the network interface*/
char	requestBuffer[DBG_MAX_REQUEST_SIZE];	/* Buffer to receive request
						 * into. */
int	requestOffset;				/* Offset in buffer where next
						 * bytes should be read from.*/
char	replyBuffer[DBG_MAX_REPLY_SIZE];	/* Buffer to hold reply. */
int	replyOffset = 0;			/* Offset in buffer where next
						 * bytes in reply should go. */
int	curMsgNum;				/* The current message that
						 * is being processed. */
int	dbgMonPC;				/* Place to get the PC from
						 * if trap via the monitor.*/
int	dbgTraceLevel;				/* The debugger tracing
						 * level. */

/*
 * Number of times to poll before timing out and resending (about 2 seconds).
 */
#ifdef sun3
int	dbgTimeout = 50000;
#else
int	dbgTimeout = 15000;
#endif

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
#define	PACKET_HDR_SIZE (sizeof(Net_EtherHdr) + Dbg_PacketHdrSize() + 4)
#define PACKET_DATA_SIZE (DBG_MAX_REPLY_SIZE - PACKET_HDR_SIZE)

/*
 * Strings which describe each of the opcodes that kdbx can send us.
 */
static char *opcodeNames[] =  {
	"Read all GPRS",
	"Write GPR",
	"Continue",
	"Single Step",
	"Detach",
	"Inst Read",
	"Inst Write",
	"Data Read",
	"Data Write",
	"Process to walk stack for",
	"Read information after stopped",
	"Return version string",
	"Divert syslog to the console",
	"Reboot the machine",
	"Set up things to start a call command",
	"Clean up things after a call command has executed",
	"UNKNOWN OPCODE"
};

/*
 * Strings which describe the different exceptions that can occur.
*/
static char *exceptionNames[] = {
  /* 0 */   "Reset",
  /* 1 */   "UNKNOWN EXCEPTION",
  /* 2 */   "Bus Error",
  /* 3 */   "Address Error",
  /* 4 */   "Illegal Instruction",
  /* 5 */   "Zero Div",
  /* 6 */   "CHK Instruction",
  /* 7 */   "TrapV",
  /* 8 */   "Privilege Violation",
  /* 9 */   "Trace Trap",
  /* 10 */  "EMU 1010",
  /* 11 */  "EMU 1111",
  /* 12 */  "UNKNOWN EXCEPTION",
  /* 13 */  "Coprocessor Protocol Violation",
  /* 14 */  "Stack Format Error",
  /* 15 */  "Uninitialized Vector",
  /* 16 */  "UNKNOWN EXCEPTION",
  /* 17 */  "UNKNOWN EXCEPTION",
  /* 18 */  "UNKNOWN EXCEPTION",
  /* 19 */  "UNKNOWN EXCEPTION",
  /* 20 */  "UNKNOWN EXCEPTION",
  /* 21 */  "UNKNOWN EXCEPTION",
  /* 22 */  "UNKNOWN EXCEPTION",
  /* 23 */  "UNKNOWN EXCEPTION",
  /* 24 */  "Spurious Interrupt",
  /* 25 */  "Level 1 Interrupt",
  /* 26 */  "Level 2 Interrupt",
  /* 27 */  "Level 3 Interrupt",
  /* 28 */  "Level 4 Interrupt",
  /* 29 */  "Level 5 Interrupt",
  /* 30 */  "Level 6 Interrupt",
  /* 31 */  "Level 7 Interrupt",
  /* 32 */  "TRAP #0",
  /* 33 */  "Syscall Trap",
  /* 34 */  "Signal Return Trap",
  /* 35 */  "Bad Trap",
  /* 36 */  "TRAP #4",
  /* 37 */  "TRAP #5",
  /* 38 */  "TRAP #6",
  /* 39 */  "TRAP #7",
  /* 40 */  "TRAP #8",
  /* 41 */  "TRAP #9",
  /* 42 */  "TRAP #10",
  /* 43 */  "TRAP #11",
  /* 44 */  "TRAP #12",
  /* 45 */  "TRAP #13",
  /* 46 */  "TRAP #14",
  /* 47 */  "Breakpoint Trap",
  /* 48 */  "FPU Unordered Condition",
  /* 49 */  "FPU Inexact Result",
  /* 50 */  "FPU Zero Divide",
  /* 51 */  "FPU Underflow",
  /* 52 */  "FPU Operand Error",
  /* 53 */  "FPU Overflow",
  /* 54 */  "FPU NaN",
  /* 55 */  "UNKNOWN EXCEPTION",
};

/*
 * The type of machine that we are on.
 */
int		machineType;

/*
 * Whether syslog should remain diverted on continue or not.
 */
static Boolean	syslogDiverted = FALSE;

/*
 * This is a typedef which is used to take care of the hole that is put in
 * the stack when we are called.
 */
typedef struct {
    char	hole[DBG_STACK_HOLE];
} StackHole;

/*
 * All of the stuff that is put onto the stack when the debugger is entered.
 */
typedef struct {
    int			gprs[MACH_NUM_GPRS];
    Mach_TrapStack	trapStack;
} DbgStack;

/*
 * Declare global variables.
 */
int		dbgSfcReg;
int		dbgDfcReg;
int 		dbgUserContext;
int 		dbgKernelContext;
DbgStack	dbgGlobalStack;
int		dbgTermReason;
int		dbgInDebugger;
int		dbgIntPending;
int		dbgExcType;
Boolean		dbgPanic;
int		dbgSavedSP;
int		dbgMaxStackAddr;
Boolean		dbg_UsingSyslog = FALSE;
Boolean		dbgCanUseSyslog = TRUE;
static	   int	oldContext;

/*
 * Saved exception stack stuff.
 */
static	int		savedDbgStackLength;
static	int		savedExcStackLength;
static	DbgStack	savedDbgStack;
static	Boolean		callInProgress = FALSE;

/* 
 * Forward declarations:
 */
static void DbgCheckNmis _ARGS_((void));
static char *	TranslateOpcode _ARGS_((Dbg_Opcode opcode));
static char *	TranslateException _ARGS_((int exception));
static Boolean	ReadRequest _ARGS_((Boolean timeout));
static void	SendReply _ARGS_((void));
static void	GetRequestBytes _ARGS_((int numBytes, Address dest));
static void	PutReplyBytes _ARGS_((int numBytes, Address src));

extern	void	Dbg_Main _ARGS_((StackHole stackHole,
				 DbgStack dbgStack));
				/* called from assembly code? */

/*
 * ----------------------------------------------------------------------------
 *
 * DbgCheckNmis --
 *
 *	Turn Non-maskable-interrupts on and off to allow keyboard events to
 *	take place.  Only needed on Sun-2's because of the funny mapping
 *	between kernel and user address spaces - they are turned on in the
 *	main debugging loop on Sun-3's.
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
DbgCheckNmis()
{
#ifdef sun2
    int	oldContext;
    oldContext = VmMachGetKernelContext(); 
    VmMachSetKernelContext(VMMACH_KERN_CONTEXT);
    Mach_MonStartNmi();
    Mach_MonStopNmi();
    VmMachSetKernelContext(oldContext);
#endif
}


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
Boolean Dbg_InRange(addr, numBytes, writeable) 
    unsigned 	int addr; 	/* Beginning address to check. */
    int		numBytes; 	/* Number of bytes to check. */
    Boolean	writeable;	/* TRUE => address must be writeable. */
{
    VmMachPTE		pte;
    int			i;
    unsigned	int	prot;
    int			firstPage;
    int			lastPage;
    unsigned	int	maxAddr;

#ifdef sun2
    maxAddr = 0x1000000;
#else
    /*
     * Don't look at anything in device space. 
     */
    maxAddr = 0x10000000;
#endif
    if (addr > maxAddr || (addr + numBytes - 1) > maxAddr) {
	return(FALSE);
    }
    /*
     * Don't look at anything in device space. 
     */
    if (!(((addr + numBytes - 1) < VMMACH_DEV_START_ADDR) ||
	 (addr >= VMMACH_DMA_START_ADDR))) {
	return FALSE;
    }
    if ((int) (addr) & 0x1) {
	printf("Dbg: odd address: %x\n", addr);
	return(FALSE);
    }

    firstPage = ((unsigned int) addr) >> VMMACH_PAGE_SHIFT;
    lastPage = (((unsigned int) addr) + numBytes - 1) >> VMMACH_PAGE_SHIFT;
    for (i = firstPage; i <= lastPage; i++) {
	pte = VmMachGetPageMap((Address)(i << VMMACH_PAGE_SHIFT));
	prot = pte & VMMACH_PROTECTION_FIELD;
	if (!(pte & VMMACH_RESIDENT_BIT)) {
	    return(FALSE);
	} else if (writeable) {
	    if (prot != VMMACH_KRW_PROT && prot != VMMACH_URW_PROT) {
		return(FALSE);
	    }
	} else {
	    if (prot != VMMACH_KRW_PROT && prot != VMMACH_URW_PROT &&
		prot != VMMACH_KR_PROT && prot != VMMACH_UR_PROT) {
		return(FALSE);
	    }
	}
    }

    return(TRUE);
}


/*
 * ----------------------------------------------------------------------------
 *
 * TranslateOpcode --
 *
 *     Return the string which describes the given opcode.
 *
 * Results:
 *     The string which describes the given opcode.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
char *
TranslateOpcode(opcode)
    Dbg_Opcode opcode;		/* The opcode which is to be translated. */
{
	int index;

	index = (int) opcode;
	if (index < 0 || index > (int) DBG_UNKNOWN) {
	    index = (int) DBG_UNKNOWN;
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
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
char *
TranslateException(exception)
    int exception;		/* The exception which is to be translated. */
{

	if (exception < 0 || exception > MACH_UNKNOWN_EXC) {
	    exception = MACH_UNKNOWN_EXC;
	}

	return(exceptionNames[exception]);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DbgComplain --
 *
 *     Complain because we are already in the debugger.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
void
DbgComplain(trapStack)
    Mach_TrapStack	trapStack;
{
    printf("%s exception in the debugger at pc %x addr %x\n",
	   TranslateException(trapStack.trapType), 
	   trapStack.excStack.pc,
	   trapStack.excStack.tail.addrBusErr.faultAddr);
    Mach_MonAbort();
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
 *     dbgMonPC and dbgDoTrace are initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
Dbg_Init()
{
    dbgMonPC = 0;
    dbgTraceLevel = 6;
    dbgInDebugger = 0;
    dbgIntPending = 0;
    dbgPanic = FALSE;
    dbg_BeingDebugged = FALSE;
    machineType = Mach_GetMachineType();
    Mach_MonPrintf("Machine type %d\n", machineType);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_InputPacket --
 *
 *     See if the current packet is for us.  At the moment this only 
 *	handles ethernet packets.
 *
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
    Address	dataPtr;
    int		dataLength;
    Net_EtherHdr	*etherHdrPtr;

    if (interPtr->netType != NET_NETWORK_ETHER) {
	printf("Got a debugger packet on non-ethernet interface %s\n",
	    interPtr->name);
	return;
    }
    etherHdrPtr = (Net_EtherHdr *)packetPtr;
    if (etherHdrPtr->type != NET_ETHER_IP) {
	if (dbgTraceLevel >= 5) {
	    printf("Non-IP (Type=0x%x) ", (int)etherHdrPtr->type);
	}
	return;
    }
    if (gotPacket) {
	if (dbgTraceLevel >= 4) {
	    printf("Already have a packet.\n");
	}
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
	int	timeOutCounter;
	Net_Interface	*interPtr;
	int		i;

	gotPacket = FALSE;
	timeOutCounter = dbgTimeout;
	do {
	    DbgCheckNmis();
	    /*
	     * Listen on all the interfaces. The debugger is relatively
	     * stateless so its easiest to just listen on them all.
	     */
	    for (i = 0; ; i++) {
		interPtr = Net_NextInterface(TRUE, &i);
		printf("i = %d, interPtr = 0x%x\n", i, interPtr);
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
	    replyOffset = PACKET_HDR_SIZE;
	    requestOffset = 4;
	    curMsgNum = *(int *)(requestBuffer);
	    if (dbgTraceLevel >= 4) {
		printf("MsgNum = %d\n", curMsgNum);
	    }
	}

	return(gotPacket);
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
	printf("PutReplyBytes: Buffer overflow\n");
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
SendReply()
{
	Net_EtherHdr		*etherHdrPtr;

	if (dbgTraceLevel >= 4) {
	    printf("Sending reply\n");
	}
	etherHdrPtr = (Net_EtherHdr *) replyBuffer;
	etherHdrPtr->source = dbgEtherHdr.destination;
	etherHdrPtr->destination = dbgEtherHdr.source;
	etherHdrPtr->type = dbgEtherHdr.type;
	dbgGather.bufAddr = replyBuffer + sizeof(Net_EtherHdr);
	dbgGather.length = replyOffset - sizeof(Net_EtherHdr);
	dbgGather.mutexPtr = (Sync_Semaphore *) NIL;
	*(int *)(replyBuffer + PACKET_HDR_SIZE - 4) = curMsgNum;
	Dbg_FormatPacket(dbgMyIPAddr, dbgSrcIPAddr, dbgSrcPort,
		     replyOffset - sizeof(Net_EtherHdr) - Dbg_PacketHdrSize(),
		     replyBuffer + sizeof(Net_EtherHdr));
	Net_RawOutput(dbgInterPtr, (Address) etherHdrPtr, &dbgGather, 1);
	if (dbgTraceLevel >= 4) {
	    printf("Sent reply\n");
	}
}

/*
 * 68020 and 68010 moveml instructions work differently.  68010 stores
 * original value of sp and 68020 stores sp - 4.  This has to be fixed
 * when getting around the exception stack.
 */
#ifdef sun3
#define STACK_INC	4
#endif
#ifdef sun2
#define STACK_INC	0
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_Main --
 *
 *     The main debugger loop.  This will read commands from the rs232 line
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
/*ARGSUSED*/
void
Dbg_Main(stackHole, dbgStack)
    StackHole		stackHole;	/* The hole put in the stack so that
					 * kdbx can play around with the stack*/
    DbgStack		dbgStack;	/* All of the stuff that is put onto the
					 * stack because of the exception and
					 * the trap handler. */
{
    short		trapCode;	/* Reason that we trapped that is sent
					 * to kdbx. */
    Boolean	  	done;		/* Boolean to tell us whether to leave
					 * the main debugger loop */
    int		  	dbgStackLength;/* The length of the trap stack */
    int		  	excStackLength;	/* The length of the exception part 
					 * of the trap stack */
    Dbg_Opcode	  	opcode;		/* The operation that was requested */
    short	  	tOpcode;	/* Temporary used to receive the opcode
					 * which is sent as a short. */
					/* Process table entry that we switched
					 * stacks to. */
    Proc_ControlBlock	*procPtr = (Proc_ControlBlock *) NIL;
    Boolean		atInterruptLevel;/* TRUE if we were entered from an
					  * interrupt handler. */

#ifdef sun3
    /*
     * Turn on non-maskable interrupts.
     */
    Mach_MonStartNmi();
#endif
    /*
     * Switch to kernel context so that we can access the monitor.
     */
    oldContext = VmMachGetKernelContext();
    VmMachSetKernelContext(VMMACH_KERN_CONTEXT);

    /*
     * Put us at interrupt level so that printf won't accidently enable
     * interrupts.
     */
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
    if (dbgTraceLevel >= 1 || !dbg_BeingDebugged || 
        (dbgStack.trapStack.trapType != MACH_TRACE_TRAP && 
         dbgStack.trapStack.trapType != MACH_BRKPT_TRAP)) { 
	Dev_VidEnable(TRUE);	/* unblank the screen */
	printf("Entering debugger with a %s exception at PC 0x%x\r\n",
		   TranslateException(dbgStack.trapStack.trapType),
		   (unsigned) dbgStack.trapStack.excStack.pc);
    }

    /*
     * The saved stack pointer points to all of the junk on the stack from
     * the exception.  We need to move the saved stack pointer past all of
     * this junk to the point where kdbx sees what it expects (it doesn't 
     * understand exception stacks).  Also we need to copy all of the 
     * trap stack to a global variable.  This allows access to this stack 
     * through the debugger (kdbx doesn't know we are in this routine so the
     * parameter dbgStack cannot be printed out in kdbx).
     *
     * NOTE:
     *
     * When kdbx does a "call" command it adds things onto the stack starting 
     * from the saved stack pointer.  When it does this it will trash the 
     * trap stack that we were passed.  Thus saving a copy of the trap
     * stack also serves the purpose of preventing kdbx from trashing the
     * original copy.
     */
    excStackLength = Mach_GetExcStackSize(&dbgStack.trapStack.excStack);
    dbgStackLength = sizeof(DbgStack) - sizeof(Mach_ExcStack) + excStackLength;

    bcopy((Address) &dbgStack, (Address) &dbgGlobalStack, dbgStackLength);

    dbgGlobalStack.gprs[SP] += excStackLength + STACK_INC + 
			       MACH_TRAP_INFO_SIZE;

    /*
     * Clear the trace bit from the status register.
     */
    dbgGlobalStack.trapStack.excStack.statusReg &= ~MACH_SR_TRACEMODE;

    /*
     * We need to tell kdbx what type of exception this is.  If the
     * termReason is DBG_INTERRUPT_SIG then we set the trap code to
     * interrupted.  Otherwise we don't know what caused us to get here so
     * we have to check the exception type.  If the exception is either a
     * trace trap of a breakpoint trap then we set the term reason to
     * DBG_TRACE_TRAP_SIG and set the trap code appropriately.  Otherwise we 
     * tell kdbx that the reason was an DBG_INTERRUPT_SIG and we set the trap
     * code appropriately for the given exception.
     */
    trapCode = dbgStack.trapStack.trapType;
    if (dbgTermReason == DBG_INTERRUPT_SIG) {
	trapCode = DBG_INTERRUPT;
    } else if (dbgPanic) {
	dbgPanic = FALSE;
	trapCode = DBG_INTERRUPT;
        dbgTermReason = DBG_INTERRUPT_SIG;
    } else if (dbgStack.trapStack.trapType == MACH_TRACE_TRAP ||
	       dbgStack.trapStack.trapType == MACH_BRKPT_TRAP) {
	dbgTermReason = DBG_TRACE_TRAP_SIG;
    } else {
	dbgTermReason = DBG_INTERRUPT_SIG;
    }

    dbg_UsingNetwork = TRUE;

    /*
     * If we are stopped after a continue or single step must write a
     * null byte to the debugger at the other end.  It knows that if it sees
     * a null byte we are stopped at this end.
     */
    if (dbg_BeingDebugged) {
	unsigned	char	ch;

	ch = 0;
	PutReplyBytes(1, (Address)&ch);
	SendReply();
	do {
	    if (ReadRequest(TRUE)) {
		GetRequestBytes(2, (Address)&tOpcode);
		opcode = (Dbg_Opcode) tOpcode;
		if (opcode == DBG_GET_STOP_INFO) {
		    break;
		}
	    }
	    /*
	     * We can only timeout if we are using network debugging.
	     */
	    Net_RawOutput(dbgInterPtr, (Address) replyBuffer, 
		    &dbgGather, 1);
	    if (dbgTraceLevel >= 5) {
		printf("DBG: Timeout\n");
	    }
	    printf("TI ");
	} while (TRUE);
    } else {
	(void) ReadRequest(FALSE);
	GetRequestBytes(2, (Address)&tOpcode);
	opcode = (Dbg_Opcode) tOpcode;
    }

    /*
     * Now read commands until kdbx tells us that we can return.
     */
    done = FALSE;
    while (!done) {
	if (dbgTraceLevel >= 2) {
	    printf("Request: %s ", TranslateOpcode(opcode));
	}

	/*
	 * Process the request 
	 */
	switch (opcode) {

	    /*
	     * The client wants to read some data from us ...
	     */

	    case DBG_GET_STOP_INFO: {
		StopInfo	stopInfo;
		extern void	Mach_ContextSwitch();
		stopInfo.codeStart = (int)mach_CodeStart;
		if (procPtr != (Proc_ControlBlock *) NIL &&
		    procPtr->machStatePtr != (Mach_State *)NIL) {
		    stopInfo.maxStackAddr =
			(int)(procPtr->machStatePtr->kernStackStart + 
			      mach_KernStackSize);
		    bcopy((Address) procPtr->machStatePtr->switchRegs,
			    (Address) stopInfo.genRegs,
			    sizeof(procPtr->machStatePtr->switchRegs));
		    stopInfo.pc = (int) ((Address) Mach_ContextSwitch);
		} else {
		    stopInfo.maxStackAddr = dbgMaxStackAddr;
		    bcopy((Address) dbgGlobalStack.gprs, 
			    (Address) stopInfo.genRegs,
			    sizeof(dbgGlobalStack.gprs));
		    stopInfo.pc = dbgGlobalStack.trapStack.excStack.pc;
		}
		stopInfo.termReason = dbgTermReason;
		stopInfo.trapCode = trapCode;
		stopInfo.statusReg = 
		(unsigned short) dbgGlobalStack.trapStack.excStack.statusReg;
		PutReplyBytes(sizeof(stopInfo), (Address)&stopInfo);
		SendReply();
		break;
	    }
	    case DBG_READ_ALL_GPRS:
		if (procPtr != (Proc_ControlBlock *) NIL &&
		    procPtr->machStatePtr != (Mach_State *)NIL) {
		    PutReplyBytes(sizeof(procPtr->machStatePtr->switchRegs),
				 (Address) procPtr->machStatePtr->switchRegs);
		} else {
		    PutReplyBytes(sizeof(dbgGlobalStack.gprs),
			         (Address) dbgGlobalStack.gprs);
		}
		SendReply();
		break;

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
		    printf("Addr=%x Numbytes=%d ",
				readMem.address, readMem.numBytes);
		}

		VmMachSetKernelContext(oldContext);
		if (Dbg_InRange((unsigned int) readMem.address, readMem.numBytes,
			    FALSE)) {
		    status = 1;
		    PutReplyBytes(sizeof(status), (Address)&status);
		    PutReplyBytes(readMem.numBytes, (Address)readMem.address);
		} else {
		    if (dbgTraceLevel >= 2) {
			printf("FAILURE ");
		    }
		    status = 0;
		    PutReplyBytes(sizeof(status), (Address)&status);
		}
		VmMachSetKernelContext(VMMACH_KERN_CONTEXT);
		SendReply();
		break;
	    }

	    /*
	     * The client wants to write something to us.
	     */
	    case DBG_SET_PID: {
		Proc_PID	pid;

		GetRequestBytes(sizeof(pid), (Address) &pid);
		{
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}
		if (dbgTraceLevel >= 2) {
		    printf("pid %x ", pid);
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
			printf("Can't backtrace stack for process %x\n",
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
		    printf("Addr=%x Numbytes=%d ",
				writeMem.address, writeMem.numBytes);
		}

		VmMachSetKernelContext(oldContext);
		if (Dbg_InRange((unsigned int) writeMem.address,
			    writeMem.numBytes, opcode == DBG_DATA_WRITE)) {
		    if (opcode == DBG_INST_WRITE) {
			VmMach_SetProtForDbg(TRUE, writeMem.numBytes, 
					     (Address)writeMem.address);
		    }
		    GetRequestBytes(writeMem.numBytes,
				    (Address) writeMem.address);
		    if (opcode == DBG_INST_WRITE) {
			VmMach_SetProtForDbg(FALSE, writeMem.numBytes, 
					     (Address)writeMem.address);
		    }
		    ch = 1;
		} else {
		    char	buf[100];

		    if (dbgTraceLevel >= 2) {
			printf("FAILURE ");
		    }
		    GetRequestBytes(writeMem.numBytes, buf);
		    ch = 0;
		}
		VmMachSetKernelContext(VMMACH_KERN_CONTEXT);

		PutReplyBytes(1, (char *) &ch);
		SendReply();

		break;
	    }

	    case DBG_WRITE_GPR: {                
		Dbg_WriteGPR	writeGPR;

		/*
		 * First find out which register is being written and
		 * then read the value.
		 */
		GetRequestBytes(sizeof(writeGPR), (Address)&writeGPR);
		{
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}
		if (dbgTraceLevel >= 2) {
		    printf("register %d data %x ", writeGPR.regNum, 
				writeGPR.regVal);
		}
		dbgGlobalStack.gprs[writeGPR.regNum] = writeGPR.regVal;
		break;
	    }

	    case DBG_DIVERT_SYSLOG: 
		GetRequestBytes(sizeof(Boolean), (Address)&syslogDiverted);
		{
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}
		break;

	    case DBG_BEGIN_CALL:
		/*
		 * We are beginning a call command.  Fix up the stack
		 * so that we will be able to continue.  We will put
		 * it back when we are done.
		 */
		savedDbgStackLength = dbgStackLength;
		savedExcStackLength = excStackLength;
		bcopy((Address)&dbgGlobalStack, (Address)&savedDbgStack,
			dbgStackLength);

		dbgGlobalStack.trapStack.excStack.vor.stackFormat = MACH_SHORT;
		excStackLength =
		    Mach_GetExcStackSize(&dbgGlobalStack.trapStack.excStack);
		dbgStackLength = excStackLength + sizeof(DbgStack) - 
				 sizeof(Mach_ExcStack);

		callInProgress = TRUE;
		if (dbgCanUseSyslog) {
		    dbg_UsingSyslog = TRUE;
		}
		  {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}

		break;
	    case DBG_END_CALL: {
		char	*buffer;
		int	*firstIndexPtr;
		int	*lastIndexPtr;
		int	bufSize;
		int	length;
		if (callInProgress) {
		    /*
		     * Restore the state to the state before the
		     * call was begun.  Note that the DBG_END_CALL command will
		     * be executed until the sys log buffer is empty so only
		     * need to restore our state if the callInProgress flag is
		     * set.
		     */
		    dbgStackLength = savedDbgStackLength;
		    excStackLength = savedExcStackLength;
		    bcopy((Address)&savedDbgStack, (Address)&dbgGlobalStack,
			    sizeof(dbgGlobalStack));
		    callInProgress = FALSE;
		}
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
		    printf("Addr=%x Numbytes=%d ",
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
			printf("FAILURE ");
		    }
		    GetRequestBytes(callFunc.numBytes,(Address)argBuf);
		    returnVal = -1;
		}
		PutReplyBytes(4, (char *) &returnVal);
		SendReply();

		break;
	    }
	    case DBG_CONTINUE: 
		/*
		 * The client wants to continue execution.
		 */
		GetRequestBytes(sizeof(int), 
			    (Address) &dbgGlobalStack.trapStack.excStack.pc);
		if (dbgTraceLevel >= 2) {
		    printf("Continuing from pc %x ",
				dbgGlobalStack.trapStack.excStack.pc);
		}
		 {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}

		dbg_BeingDebugged = TRUE;
		done = TRUE;
		break;

	    case DBG_SINGLESTEP:
		/*
		 * The client wants to single step.
		 */
		GetRequestBytes(sizeof(int), 
			    (Address) &dbgGlobalStack.trapStack.excStack.pc);
		if (dbgTraceLevel >= 2) {
		    printf("Stepping from pc %x ",
				dbgGlobalStack.trapStack.excStack.pc);
		}
		 {
		    int	dummy;

		    PutReplyBytes(4, (Address) &dummy);
		    SendReply();
		}

		/* 
		 * Turn the trace bit on in the SR.
		 */
		dbgGlobalStack.trapStack.excStack.statusReg |= 
							MACH_SR_TRACEMODE;
		dbg_BeingDebugged = TRUE;
		done = TRUE;
		break;

	    case DBG_DETACH:
		/*
		 * The debugger has terminated and wants to let us go about our
		 * business.
		 */
		GetRequestBytes(sizeof(int), 
			    (Address) &dbgGlobalStack.trapStack.excStack.pc);
		if (dbgTraceLevel >= 2) {
		    printf("Detaching at pc %x ",
				dbgGlobalStack.trapStack.excStack.pc);
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
		printf("debugger: unrecognized request\n");
		break;
	}

	if (dbgTraceLevel >= 2) {
	    printf("\r\n");
	}
	if (!done) {
	    (void)ReadRequest(FALSE);
	    GetRequestBytes(2, (Address)&tOpcode);
	    opcode = (Dbg_Opcode) tOpcode;
	}
    }

    /*
     * Return from the debugger.  Before we return, we must push the trap
     * stuff back onto the stack and restore the saved sp to point to the
     * exception stuff on the stack.
     */

    dbgSavedSP = dbgGlobalStack.gprs[SP] - dbgStackLength;
    dbgGlobalStack.gprs[SP] -= excStackLength + STACK_INC  +
			          MACH_TRAP_INFO_SIZE;
    bcopy((Address) &dbgGlobalStack, (Address) dbgSavedSP, dbgStackLength);

    VmMachSetKernelContext(oldContext);
    mach_AtInterruptLevel = atInterruptLevel;
    dbg_UsingNetwork = FALSE;

    /*
     * Don't force system log output to the console.
     */
    if (!syslogDiverted) {
	Dev_SyslogDebug(FALSE);
    }
#ifdef sun3
    /*
     * Turn off non-maskable interrupts.
     */
    Mach_MonStopNmi();
#endif
}
