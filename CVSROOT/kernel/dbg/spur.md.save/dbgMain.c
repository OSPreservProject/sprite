/* 
 * dbgMain.c --
 *
 *	Routines for the SPUR kernel debugger.
 *
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPR
ITE (Berkeley)";
#endif not lint

#define GOOD_SYSLOG

#include "sprite.h"
#include "mach.h"
#include "proc.h"
#include "vmMachInt.h"
#include "vm.h"
#include "vmInt.h"
#include "machMon.h"
#include "net.h"
#include "netEther.h"
#include "netInet.h"
#include "dev.h"
#include "dbg.h"
#include "dbgInt.h"

Boolean	dbg_UsingSyslog = FALSE;
Boolean	dbgCanUseSyslog = TRUE;

Boolean	dbg_BeingDebugged = FALSE;		/* TRUE if are under control
						 * of kdbx.*/
Boolean	dbg_Rs232Debug = TRUE;			/* TRUE if are using the RS232
						 * line to debug, FALSE if are
						 * using the network. */
Boolean	dbg_UsingNetwork = FALSE;		/* TRUE if the debugger is
						 * using the network interface*/
int	dbgTraceLevel;				/* The debugger tracing
						 * level. */
int	machineType;

Dbg_Reply			*dbgReplyPtr; 
Dbg_Request 			*dbgRequestPtr;
Dbg_RawReply			dbgRawReply;
Dbg_RawRequest			dbgRawRequest;
int				dbgTimeout = 50000;
static	int			sequence;
static	char			*replyEndPtr;
static	int			replyBytesUsed;
static	char			replyContents[DBG_MAX_CONTENT];
static	ReturnStatus		replyStatus;

static	char			*opcodeNames[] = DBG_OPCODE_NAMES;
static	char			*exceptionNames[] = DBG_EXCEPTION_NAMES;
static	Net_ScatterGather	gather[3];

static	void			ReadSerial();
static	void			WriteSerial();
static	Boolean			InRange();
static	char			*TranslateOpcode();
static	char			*TranslateException();
static	Boolean			ReadRequest();
static	void			PutReplyBytes();
static	void			SendReply();

static struct {
    char	*opcode;
    char	*stopInfo;
    char	*regs;
    char	*returnVal;
} contentStrings;



/*
 * ----------------------------------------------------------------------------
 *
 * InRange --
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
static Boolean
InRange(addr, numBytes, writeable) 
    unsigned 	int addr; 	/* Beginning address to check. */
    int		numBytes; 	/* Number of bytes to check. */
    Boolean	writeable;	/* TRUE => address must be writeable. */
{

    /*
     * We ignore the writable argument because we can't determine the
     * write status of the memory right now. Maybe there is a routine
     * in the vm module to do this, but I haven't found it. -- jhh
     */
    if (dbgTraceLevel >= 5) {
	printf("InRange called with addr 0x%x %d bytes, and writable = %d\n",
	    addr, numBytes, (unsigned int) writeable);
    }
    if (!Vm_ValidateRange(addr, numBytes)) {
	return(FALSE);
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
static char *
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
static char *
TranslateException(exception)
    int exception;		/* The exception which is to be translated. */
{

	if (exception < 0 || exception > MACH_MAX_TRAP_TYPE) {
	    exception = MACH_MAX_TRAP_TYPE+1;
	}

	return(exceptionNames[exception]);
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
 *     None..
 *
 * ----------------------------------------------------------------------------
 */
void
Dbg_Init()
{
    dbgTraceLevel = 0;
    dbg_BeingDebugged = FALSE;
    dbgReplyPtr = &dbgRawReply.reply;
    dbgRequestPtr = &dbgRawRequest.request;
    contentStrings.opcode = DBG_OPCODE_REP_CNTS;
    contentStrings.stopInfo = DBG_STOPINFO_REP_CNTS;
    contentStrings.regs = DBG_REGSTATE_REP_CNTS;
    contentStrings.returnVal = DBG_RETVAL_REP_CNTS;
    machineType = 0;
    Mach_MonPrintf("Machine type %d\n", machineType);
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
    int	flasher;
    
    led_display(REQUEST_LED,LED_OK,FALSE);
    if (dbg_Rs232Debug) {
	ReadSerial(sizeof(Dbg_Request), (Address) dbgRequestPtr, TRUE);
	dbgGotPacket = TRUE;
    } else {
	int	timeOutCounter;

	dbgGotPacket = FALSE;
	timeOutCounter = dbgTimeout;
	do {
	    Net_RecvPoll();
	    if (timeout) {
		timeOutCounter--;
	    }
	} while(!dbgGotPacket && timeOutCounter != 0);
    }
    if (dbgGotPacket) {
	sequence = dbgRequestPtr->header.sequence;
	if (dbgTraceLevel >= 4) {
	    printf("MsgNum = %d\n", sequence);
	}
    }
    replyEndPtr = (char *) &dbgReplyPtr->data;
    replyBytesUsed = 0; 
    replyContents[0] = '\0';
    replyStatus = SUCCESS;
    return(dbgGotPacket);
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
    if (replyBytesUsed + numBytes > DBG_MAX_REPLY_DATA_SIZE) {
	printf("PutReplyBytes: Buffer overflow\n");
	numBytes = DBG_MAX_REPLY_DATA_SIZE - replyBytesUsed;
    }
    bcopy(src, &dbgReplyPtr->data.bytes[replyBytesUsed], numBytes);
    replyBytesUsed += numBytes;
    replyEndPtr = &dbgReplyPtr->data.bytes[replyBytesUsed+1];
    strcpy(replyContents, DBG_BYTES_REP_CNTS);
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
    void	Dbg_FormatPacket();
    int		replySize;
    int		tempContents[DBG_MAX_CONTENT];

    if (dbgTraceLevel >= 4) {
	printf("Sending reply\n");
    }
    led_display(REPLY_LED,LED_OK,FALSE);
    replySize = (int) (replyEndPtr - (char *) dbgReplyPtr);
    dbgReplyPtr->header.magic = DBG_MAGIC;
    dbgReplyPtr->header.sequence = sequence;
    dbgReplyPtr->header.dataSize = (int) (replyEndPtr - 
				   (char *) &dbgReplyPtr->data);
    dbgReplyPtr->header.status = replyStatus;


    bcopy(replyContents, tempContents, DBG_MAX_CONTENT);
    sprintf(dbgReplyPtr->header.contents, tempContents, 
	DBG_MAX_REPLY_DATA_SIZE);
    if (dbg_Rs232Debug) { 
	WriteSerial(replySize, (char *) dbgReplyPtr);
    } else {

	Dbg_FormatPacket(replySize,gather);
	gather[2].bufAddr = (Address) dbgReplyPtr;
	gather[2].length = replySize;
	gather[2].mutexPtr = (Sync_Semaphore *) NIL;
	Net_OutputRawEther(dbgRawReply.etherHeader, gather, 3);
    }
    if (dbgTraceLevel >= 4) {
	printf("Sent reply\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReadSerial --
 *
 *	Read bytes from the serial line. If ack is true then acknowledge
 *	every third character;
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
ReadSerial(size, buffer, ack)
    int		size;
    char	*buffer;
    Boolean	ack;
{
    int		i;
    Boolean	justAcked;

    for (i = 0; i < size; i++) {
	justAcked = FALSE;
	buffer[i] = readUart();
	if (dbgTraceLevel >= 4) {
	    printf("Read byte = %x\n", buffer[i]);
	}
	if (ack && (i + 1) % 3 == 0) {
	    if (dbgTraceLevel >= 4) {
		printf("Acking read\n");
	    }
	    justAcked = TRUE;
	    writeUart(DBG_REQUEST_ACK);
	}
	led_display(size - i - 1, LED_ONE, FALSE);
    }
    if (ack && !justAcked) {
	if (dbgTraceLevel >= 4) {
	    printf("Acking read\n");
	}
	writeUart(DBG_REQUEST_ACK);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WriteSerial --
 *
 *	Write bytes to the serial line.
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
WriteSerial(size, buffer)
    int		size;
    char	*buffer;
{
    int		i;
    Boolean	justAcked;
    char	ackValue;

    for (i = 0; i < size; i++) {
	justAcked = FALSE;
	if (dbgTraceLevel >= 4) {
	    printf("\r\nWriting byte: %x", buffer[i]);
	}
	writeUart(buffer[i]);
	if ((i + 1) % DBG_ACK_SIZE == 0) {
	    ackValue = readUart();
	    if (ackValue != DBG_REPLY_ACK) {
		led_display(ackValue, LED_ERROR, TRUE);
	    }
	    justAcked = TRUE;
	}
	led_display(size - i - 1, LED_TWO, FALSE);
    }
    if (!justAcked) {
	if (dbgTraceLevel >= 4) {
	    printf("Waiting for ack\n");
	}
	ackValue = readUart();
	if (ackValue != DBG_REPLY_ACK) {
	    led_display(ackValue, LED_ERROR, TRUE);
	}
    }
}

/*
 * Whether syslog should remain diverted on continue or not.
 */
static Boolean	syslogDiverted = FALSE;


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
/*ARGSUSED*/
void
Dbg_Main(trapType, trapStatePtr)
    int			trapType;	/* The reason we were called. */
    Mach_RegState	*trapStatePtr;	/* The CPU's state at the trap. */
{
    Boolean	  	done;		/* Boolean to tell us whether to leave
					 * the main debugger loop */
    Dbg_Opcode	  	opcode;	        /* The operation that was requested */
					/* Process table entry that we switched
					 * stacks to. */
    Proc_ControlBlock	*procPtr = (Proc_ControlBlock *) NIL;
    Boolean		atInterruptLevel;/* TRUE if we were entered from an
					  * interrupt handler. */
    int			switches;


    switches = read_physical_word(0x40000);
    dbg_Rs232Debug =  (switches & 0x80) ;
    dbg_UsingNetwork =  !dbg_Rs232Debug;
    led_display(TRAPPED_LED | trapType,LED_ONE & LED_FOUR,FALSE);



    /*
     * Put us at interrupt level so that printf won't accidently enable
     * interrupts.
     */
    atInterruptLevel = mach_AtInterruptLevel[Mach_GetProcessorNumber()];
    mach_AtInterruptLevel[Mach_GetProcessorNumber()] = TRUE;

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
	trapType != MACH_BREAKPOINT_TRAP) { 
	printf("Entering debugger with a %s (%d) exception at PC 0x%x\r\n",
		   TranslateException(trapType),trapType,
		   (unsigned) trapStatePtr->curPC);
    }
    if (dbg_BeingDebugged) {
        unsigned        char    ch;
	int	timeout = 5;

        SendReply();
        do {
            if (ReadRequest(TRUE)) {
		if (dbgRequestPtr->header.opcode != DBG_CONTINUE) {
		    break;
		} else {
		    SendReply();
		    continue;
		}
            }
	    Net_OutputRawEther(dbgRawReply.etherHeader, gather, 3);
            if (dbgTraceLevel >= 5) {
                printf("DBG: Timeout\n");
            }
            printf("TI ");
        } while (timeout-- > 0);
    } else {
	if (dbg_Rs232Debug) {
	    unsigned	char	ch;
	    /*
	     * Read in bytes until we get the sequence of 3 characters
	     * 127, 27, 7.
	     */

	    while (TRUE) {
		ReadSerial(1, (Address) &ch, FALSE);
		if (ch == 127) {
		    ReadSerial(1, (Address) &ch, FALSE);
		    if (ch == 27) {
			ReadSerial(1, (Address) &ch, FALSE);
			if (ch == 7) {
			    break;
			}
		    }
		}
	    }
	}
	(void) ReadRequest(FALSE);
    }
    opcode = dbgRequestPtr->header.opcode;
    /*
     * Now read commands until kdbx tells us that we can return.
     */
    done = FALSE;
    while (!done) {
	if (dbgTraceLevel >= 2) {
	    printf("Request: %s ", TranslateOpcode(opcode));
	}
	led_display(PROCESS_LED, LED_OK, FALSE);

	/*
	 * Process the request 
	 */
	switch (opcode) {

	    /*
	     * The client wants to read some data from us ...
	     */

	    case DBG_GET_STOP_INFO: {
		Dbg_StopInfo	stopInfo;

		led_display(GET_STOP_INFO_LED,LED_OK,FALSE);
		stopInfo.codeStart = (int)mach_CodeStart;
		if (procPtr != (Proc_ControlBlock *) NIL &&
		    procPtr->machStatePtr != (Mach_State *)NIL) {
		    stopInfo.regs = procPtr->machStatePtr->switchRegState;
		    stopInfo.regStateAddr = 
			(int) &(procPtr->machStatePtr->switchRegState.regs);
		} else {
		    stopInfo.regs = *trapStatePtr;
		    stopInfo.regStateAddr = &(trapStatePtr->regs);
		}
		stopInfo.trapType = trapType;
		FillReplyField(stopInfo, stopInfo);
		SendReply();
		break;
	    }
	    case DBG_READ_ALL_REGS: 

		led_display(READ_ALL_REGS_LED,LED_OK,FALSE);
		if (procPtr != (Proc_ControlBlock *) NIL &&
		    procPtr->machStatePtr != (Mach_State *)NIL) {
		    FillReplyField(regs, procPtr->machStatePtr->switchRegState);
		} else {
		    FillReplyField(regs, *trapStatePtr);
		}
		SendReply();
		break;

	    case DBG_GET_VERSION_STRING: {
		char	*SpriteVersion();
		char	*version;

		led_display(GET_VERSION_STRING_LED,LED_OK,FALSE);
		version = SpriteVersion();
		PutReplyBytes(strlen(version) + 1, version);
		SendReply();
		break;
	    }
	    
	    case DBG_INST_READ:
	    case DBG_DATA_READ: {
		Dbg_ReadMem	readMem;

		led_display(DATA_READ_LED,LED_OK,FALSE);
		readMem = dbgRequestPtr->data.readMem;
		if (dbgTraceLevel >= 2) {
		    printf("Addr=%x Numbytes=%d ",
				readMem.address, readMem.numBytes);
		}
		if (InRange((unsigned int) readMem.address, readMem.numBytes,
			    FALSE)) {
		    replyStatus = SUCCESS;
		    PutReplyBytes(readMem.numBytes, (Address)readMem.address);
		} else {
		    led_display(DATA_READ_LED, LED_ERROR, TRUE);
		    if (dbgTraceLevel >= 2) {
			printf("FAILURE ");
		    }
		    replyStatus = FAILURE;
		}
		SendReply();
		break;
	    }

	    /*
	     * The client wants to write something to us.
	     */
	    case DBG_SET_PID: {
		Proc_PID	pid;

		led_display(SET_PID_LED,LED_OK,FALSE);
		pid = dbgRequestPtr->data.pid;
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
			replyStatus = FAILURE;
		    }
		}
		SendReply();
		break;
	    }
	    case DBG_REBOOT: {
		int	stringLength;
		char	*string;
		/*
		 * For a reboot command first read the size of the string and
		 * then the string itself.
		 */
		led_display(REBOOT_LED,LED_OK,FALSE);
		stringLength = dbgRequestPtr->data.reboot.stringLength;
		string = dbgRequestPtr->data.reboot.string;
		string[stringLength] = '\0';
		SendReply();
		Mach_MonReboot(string);
	    }
	    case DBG_INST_WRITE:
	    case DBG_DATA_WRITE: {
		Dbg_WriteMem		writeMem;
		/*
		 * For an instruction or a data write we first have to find out 
		 * which address to write to and how many bytes to write.  Next
		 * we have to make sure that the address is valid.  If it is
		 * then we read the data and write it to the given address.  If
		 * not we just report an error to kdbx.
		 */
		led_display(DATA_WRITE_LED,LED_OK,FALSE);
		writeMem = dbgRequestPtr->data.writeMem;
		if (dbgTraceLevel >= 2) {
		    printf("Addr=%x Numbytes=%d ",
				writeMem.address, writeMem.numBytes);
		}

		if (InRange((unsigned int) writeMem.address,
			    writeMem.numBytes, opcode == DBG_DATA_WRITE)) {
		    if (opcode == DBG_INST_WRITE) {
			VmMach_SetProtForDbg(TRUE, writeMem.numBytes, 
					     (Address)writeMem.address);
		    }
		    bcopy(writeMem.buffer, writeMem.address, writeMem.numBytes);
		    if (opcode == DBG_INST_WRITE) {
			VmMach_SetProtForDbg(FALSE, writeMem.numBytes, 
					     (Address)writeMem.address);
		    }
		    replyStatus = SUCCESS;
		} else {
		    if (dbgTraceLevel >= 2) {
			printf("FAILURE ");
		    }
		    replyStatus = FAILURE;
		}
		SendReply();
		break;
	    }

	    case DBG_WRITE_REG: {                
		Dbg_WriteReg	writeReg;

		/*
		 * First find out which register is being written and
		 * then read the value.
		 */

		led_display(WRITE_REG_LED,LED_OK,FALSE);
		writeReg = dbgRequestPtr->data.writeReg;
		if (dbgTraceLevel >= 2) {
		    printf("register %d data %x ", writeReg.regNum, 
				writeReg.regVal);
		}
		if (writeReg.regNum < MACH_NUM_ACTIVE_REGS) {
		    trapStatePtr->regs[writeReg.regNum][0] = writeReg.regVal;
		} else {
		    switch (writeReg.regNum) {
			case DBG_PC_REGNUM : 
			    trapStatePtr->curPC = (Address) writeReg.regVal;
			    break;
			case DBG_NEXT_PC_REGNUM : 
			    trapStatePtr->nextPC = (Address) writeReg.regVal;
			    break;
			case DBG_SWP_REGNUM : 
			    trapStatePtr->swp = (Address) writeReg.regVal;
			    break;
			case DBG_CWP_REGNUM : 
			    trapStatePtr->cwp = writeReg.regVal;
			    break;
			default:
			    printf("%d is a bogus register value.\n", 
				   writeReg.regNum);
			    replyStatus = FAILURE;
		    }
		}
		SendReply();
		break;
	    }

	    case DBG_DIVERT_SYSLOG: 
		led_display(DIVERT_SYSLOG_LED,LED_OK,FALSE);
		if (dbgRequestPtr->data.syslogCmd == DBG_SYSLOG_TO_ORIG) {
		    syslogDiverted = FALSE;
		} else {
		    syslogDiverted = TRUE;
		}
		SendReply();
		break;

	    case DBG_BEGIN_CALL: {
		/*
		 * We are beginning a call command.  Fix up the stack
		 * so that we will be able to continue.  We will put
		 * it back when we are done.
		 */
		if (dbgCanUseSyslog) {
		    dbg_UsingSyslog = TRUE;
		}
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
		    dbg_UsingSyslog = FALSE;
		} else if (*firstIndexPtr <= *lastIndexPtr) {
		    length = *lastIndexPtr - *firstIndexPtr + 1;
		    if (length > DBG_MAX_REPLY_DATA_SIZE) {
			length = DBG_MAX_REPLY_DATA_SIZE;
		    }
		    PutReplyBytes(length, (Address)&buffer[*firstIndexPtr]);
		    *firstIndexPtr += length;
		    if (*firstIndexPtr > *lastIndexPtr) {
			*firstIndexPtr = *lastIndexPtr = -1;
		    }
		} else {
		    length = bufSize - *firstIndexPtr;
		    if (length > DBG_MAX_REPLY_DATA_SIZE) {
			length = DBG_MAX_REPLY_DATA_SIZE;
		    }
		    PutReplyBytes(length, (Address)&buffer[*firstIndexPtr]);
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
		int			*args;

		led_display(CALL_FUNCTION_LED,LED_OK,FALSE);
		callFunc = dbgRequestPtr->data.callFunc;
		args = callFunc.args;
		if (dbgTraceLevel >= 2) {
		    printf("Addr=%x Numargs=%d ",
				callFunc.address, callFunc.numArgs);
		}

		if (InRange((unsigned int) callFunc.address,4,FALSE)) {
		    returnVal = (* ((int (*)()) callFunc.address))(args[0],
		    args[1],args[2],args[3],args[4],args[5],args[6],
		    args[7],args[8],args[9]);
		    replyStatus = SUCCESS;
		} else {

		    if (dbgTraceLevel >= 2) {
			printf("FAILURE ");
		    }
		    replyStatus = FAILURE;
		    returnVal = -1;
		}
		FillReplyField(returnVal, returnVal);
		SendReply();
		break;
	    }
	    case DBG_CONTINUE: {
		/*
		 * The client wants to continue execution.
		 */
		led_display(CONTINUE_LED,LED_OK,FALSE);
		if (dbgTraceLevel >= 2) {
		    printf("Continuing from pc %x ",
				trapStatePtr->curPC);
		}
		SendReply();
		dbg_BeingDebugged = TRUE;
		done = TRUE;
		break;
	    }
	    case DBG_SINGLESTEP: {
		/*
		 * The client wants to single step.
		 */
		led_display(SINGLESTEP_LED,LED_OK,FALSE);
		printf("Spur's can't single step\n");
		replyStatus = FAILURE;
		SendReply();
		break;
	    }
	    case DBG_DETACH:
		/*
		 * The debugger has terminated and wants to let us go about our
		 * business.
		 */
		led_display(DETACH_LED,LED_OK,FALSE);
		if (dbgTraceLevel >= 2) {
		    printf("Detaching at pc %x ",
				trapStatePtr->curPC);
		}
		SendReply();
		dbg_BeingDebugged = FALSE;
		done = TRUE;
		printf("Sprite is now detached from the debugger\r\n");
		break;
	    case DBG_SET_PROCESSOR: {
		/*
		 * Set the trap state to that of the given processor.
		 */
		int	proc;

		led_display(SET_PROCESSOR_LED,LED_OK,FALSE);
		proc = dbgRequestPtr->data.processor;
		if (dbgTraceLevel >= 2) {
		    printf("proc %d ", proc);
		}
		if (proc < 0 || proc >= Mach_GetNumProcessors()) {
		    printf("Bad processor number %d.\n", proc);
		    replyStatus = FAILURE;
		} else {
		    trapStatePtr = Mach_GetDebugStateInfo(proc);
		}
		SendReply();
		break;
	    }
	}
	if (dbgTraceLevel >= 2) {
	    printf("\r\n");
	}
	if (!done) {
	    (void) ReadRequest(FALSE);
	    opcode = dbgRequestPtr->header.opcode;
	}
    }
    mach_AtInterruptLevel[Mach_GetProcessorNumber()] = atInterruptLevel;
    dbg_UsingNetwork = FALSE;
    trapStatePtr->kpsw |= MACH_KPSW_USE_CUR_PC;
    /*
     * Don't force system log output to the console.
     */
    if (!syslogDiverted) {
	Dev_SyslogDebug(FALSE);
    }
    led_display(RUNNING_LED,LED_OK,FALSE);
}

