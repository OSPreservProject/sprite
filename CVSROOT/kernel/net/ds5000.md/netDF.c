/* 
 * netDF.c --
 *
 *	The main routines for the device driver for the
 *	DEC FDDI controller 700.
 *
 *
 * Copyright 1992 Regents of the University of Californiaf
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
static char rcsid[] = "$Header$";
#endif not lint

#include <sprite.h>
#include <sys.h>
#include <list.h>
#include <netInt.h>
#include <vm.h>
#include <vmMach.h>
#include <mach.h>
#include <machMon.h>
#include <dbg.h>
#include <assert.h>
#include <dev/fddi.h>
#include <fmt.h>
#include <netDFInt.h>

#ifdef sun4c
#include <devSCSIC90.h>
#endif

/*
 * Macro to get the next descriptor on the COMMAND ring.
 */
#define	NEXT_CMD_DESC(p) ( (((p)+1) > statePtr->comLastPtr) ? \
				statePtr->comRingPtr : ((p)+1))
/*
 * Macro to access the buffer corresponding to the CMD descriptor.
 */
#define CmdBufFromDesc(statePtr, descPtr) \
  (statePtr->comBufPtr + \
  (NET_DF_COMMAND_BUF_SIZE * (unsigned long)(descPtr - statePtr->comRingPtr)))


static void NetDFRestartCallback _ARGS_((ClientData data,
					 Proc_CallInfo *infoPtr));
/*
 * Debug flag
 */
int netDFDebug;


#ifdef NET_DF_USE_UNCACHED_MEM

NetDFState uncachedNetDFState[3];

#else

extern Net_Interface		*netInterfaces[NET_MAX_INTERFACES];
extern int			netNumInterfaces;

#endif

void NetDFRestart();

/*
 *----------------------------------------------------------------------
 *
 * NetDFPrintStateAddrs --
 *
 *	Print out the addresses of the NetDFState structs corresponding
 *      to the FDDI interfaces.  Used for debugging only.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
void
NetDFPrintStateAddrs(void)
{
    /*
     * When the stack gets trashed, etc., it's hard to get a handle
     * on the state structure being used.  So you can get the address
     * in kgdb by calling this routine.
     */
#ifdef NET_DF_USE_UNCACHED_MEM
    printf("slot 0: 0x%x\tslot 1: 0x%x\tslot 2: 0x%x\n",
	   &uncachedNetDFState[0], &uncachedNetDFState[1],
	   &uncachedNetDFState[2]);
#else
    int i;

    for (i = 0; i < netNumInterfaces; i++) {
	if (netInterfaces[i]->netType == NET_NETWORK_FDDI) {
	    printf("slot %d: statePtr: 0x%x\n", 
		   netInterfaces[i]->unit, netInterfaces[i]->interfaceData);
	}
    }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFInit --
 *
 *	Initialize the DEC FDDIcontroller 700 board.
 *
 * Results:
 *	SUCCESS if the controller was found and initialized,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	Initializes the NetDFState record, as well as the chip.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFInit(interPtr)
    Net_Interface	*interPtr; 	/* Network interface. */
{
    register NetDFState		*statePtr;
    List_Links	        	*itemPtr;
    register int 		i;
    ReturnStatus	        status;

    MASTER_LOCK(&interPtr->mutex)

    /*
     * Just to make sure that the structures are the correct sizes.
     */
    assert(sizeof(Net_DFReg) == 2);
    assert(sizeof(NetDFCommandDesc) == 16);
    assert(sizeof(NetDFHostRcvDesc) == 16);
    assert(sizeof(NetDFSmtRcvDesc) == 16);
    assert(sizeof(NetDFSmtXmtDesc) == 16);
    assert(sizeof(NetDFRmcXmtDesc) == 16);
    assert(sizeof(NetDFUnsolDesc) == 16);

    netDFDebug = NET_DF_DEBUG_OFF;

    MAKE_NOTE("Initializing adapter.");

#ifdef NET_DF_USE_UNCACHED_MEM
    statePtr = &uncachedNetDFState[(int)interPtr->ctrlAddr];
    statePtr = (NetDFState *)MACH_UNCACHED_ADDR(statePtr);
#else
    statePtr = (NetDFState *) malloc (sizeof(NetDFState));
#endif
    bzero((char *) statePtr, sizeof(NetDFState)); 

    statePtr->running = FALSE;
    status = NetDFMachInit(interPtr, statePtr);
    if (status != SUCCESS) {
	MASTER_UNLOCK(&interPtr->mutex);
#ifndef NET_DF_USE_UNCACHED_MEM
	free((char *) statePtr); 
#endif
	return status;
    }

    /*
     * Initialize the transmission list.  
     */
    statePtr->xmitList = &statePtr->xmitListHdr;
    List_Init(statePtr->xmitList);
    statePtr->xmitFreeList = &statePtr->xmitFreeListHdr;
    List_Init(statePtr->xmitFreeList);

    for (i = 0; i < NET_DF_NUM_XMIT_ELEMENTS; i++) {
	itemPtr = (List_Links *) malloc(sizeof(NetDFXmtElement)), 
	List_InitElement(itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(statePtr->xmitFreeList));
    }

    interPtr->init	= NetDFInit;
    interPtr->output 	= NetDFOutput;
    interPtr->intr	= NetDFIntr;
    interPtr->ioctl	= NetDFIOControl;
    interPtr->reset 	= Net_DFRestart;        /**/
    interPtr->getStats	= NetDFGetStats;
    interPtr->netType	= NET_NETWORK_FDDI;
    interPtr->maxBytes	= NET_FDDI_MAX_BYTES - sizeof(Net_FDDIHdr);
    interPtr->minBytes	= NET_FDDI_MIN_BYTES;
    interPtr->interfaceData = (ClientData) statePtr;

    statePtr->interPtr = interPtr;
    statePtr->recvMemInitialized = FALSE;
    statePtr->recvMemAllocated = FALSE;
    statePtr->xmitMemInitialized = FALSE;
    statePtr->xmitMemAllocated = FALSE;
    statePtr->resetPending = FALSE;
    statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;

    /*
     * Initialize buffer for storing the results of an INIT command.
     * These results will later be used by a PARAM command.
     */
    statePtr->initComPtr = (NetDFInitCommand *)malloc(NET_DF_COMMAND_BUF_SIZE);

    /*
     * Reset the world.  
     */
    NetDFReset(interPtr);


    /*
     * Now we are running.
     */
    statePtr->running = TRUE;
    statePtr->flags |= NET_DF_FLAGS_NORMAL;

    MASTER_UNLOCK(&interPtr->mutex);

    return (SUCCESS); 
}

/*
 * Simple debug ring to hold strings.
 */
char *netDFDebugRing[NET_DF_DEBUG_RING_SIZE];
int  netDFDebugRingIndex;



/*
 *----------------------------------------------------------------------
 *
 * NetDFPrintDebugRing --
 *
 *	Print out the contents of the debug ring, from most remote to
 *      most recent in time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Debug messages are printed.
 *
 *----------------------------------------------------------------------
 */
void
NetDFPrintDebugRing(statePtr)
    register NetDFState *statePtr;
{
    char *messagePtr;
    int  index;
    
    index = netDFDebugRingIndex;

    printf("<---\n");
    while (TRUE) {
	/*
	 * Start at the last one.
	 */
	messagePtr = netDFDebugRing[index];
	if (messagePtr !=  NULL) {
	    printf("%d\t%s\n", index, messagePtr);
	}
	index = (index + 1) % NET_DF_DEBUG_RING_SIZE;
	if (index == netDFDebugRingIndex) {
	    break;
	}
    }
    printf("--->\n");
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFPrintRegContents --
 *
 *	Print out the contents in each of the six registers on the adapter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The register contents are printed.
 *
 *----------------------------------------------------------------------
 */
void
NetDFPrintRegContents(statePtr)
    register NetDFState *statePtr;
{
    register unsigned short status;

    status = *(statePtr->regReset);
    printf("Reset: 0x%x\t", status);
    status = *(statePtr->regCtrlA);
    printf("CtrlA: 0x%x\t", status);
    status = *(statePtr->regCtrlB);
    printf("CtrlB: 0x%x\n", status);
    status = *(statePtr->regStatus);
    printf("Status: 0x%x\t", status);
    status = *(statePtr->regEvent);
    printf("Event: 0x%x\t", status);
    status = *(statePtr->regMask);
    printf("Mask: 0x%x\n", status);
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFPrintErrLog --
 *
 *	Print out selected contents of the error log.  Error codes
 *      are placed in a log buffer on the adapter when the
 *      adapter goes wacky.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Error log contents are printed.
 *
 *----------------------------------------------------------------------
 */
void
NetDFPrintErrorLog(statePtr)
    register NetDFState *statePtr;
{
    register volatile unsigned char  *errPtr;
    register unsigned long           large;

    /*
     * The internal code specifies a more detailed error than the
     * external code.
     */
    errPtr = statePtr->errLogPtr;
    large = (unsigned long)*(errPtr + NET_DF_MACH_ERR_INTERNAL_OFFSET);
    /*
     * The external code is the one that can be found in the STATUS
     * regsiter.
     */
    printf("Internal: 0x%x\t", large);
    large = (unsigned long)*(errPtr + NET_DF_MACH_ERR_EXTERNAL_OFFSET);
    printf("External: 0x%x\t", large);
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFAssertState --
 *
 *	If the adapter is not in the given state, then panic!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	We could panic.
 *
 *----------------------------------------------------------------------
 */
void
NetDFAssertState(statePtr, state)
    register NetDFState *statePtr;
    unsigned short state; 
{
    unsigned short  status;
    
    status = *(statePtr->regStatus) & NET_DF_STATUS_ADAPTER_STATE;
    if (status != state) {
	status = status;
	panic("DEC FDDI: adapter in wrong state (is %x, want %x)\n", 
	      status, state);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFCommandTimeout --
 *
 *	Wait for a command to finish.  If it does not finish within
 *      a finite time, then stop waiting and return an error.
 *
 * Results:
 *	SUCCESS if the command finished within its allotted time,
 *      and FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFCommandTimeout(statePtr, comDescPtr)
    register NetDFState                *statePtr;
    register volatile NetDFCommandDesc *comDescPtr;
{
    register volatile Net_DFReg status;
    register volatile Net_DFReg event;
    register unsigned long      n;

    /*
     * Clear the CMD_DONE bit, and give ownership of the command descriptor to
     * the adapter.  Then poke the CTRLA register, telling it that we've set
     * up a command to be executed.
     */
    *(statePtr->regEvent) = NET_DF_EVENT_CMD_DONE;
    n = comDescPtr->command & NET_DF_COMMAND_MASK;
    comDescPtr->command = n | NET_DF_ADAPTER_OWN;
    *(statePtr->regCtrlA) |= NET_DF_CTRLA_CMD_POLL_DEMAND;

    /*
     * Wait for the CMD_DONE event.
     */
    status = FAILURE;
    for (n = 0; n < 10000000; n++) {
	event = *(statePtr->regEvent);
	if (event & NET_DF_EVENT_CMD_DONE) {
	    /*
	     * Clear the event we just noticed
	     */
	    *(statePtr->regEvent) = NET_DF_EVENT_CMD_DONE;
	    status = SUCCESS;
	    break;
	}
    }

    if (status == SUCCESS && comDescPtr->status == NET_DF_COMMAND_STATUS_OK) {
	return status;
    }
    NetDFPrintRegContents(statePtr);
    NetDFPrintErrorLog(statePtr);
    if (status == SUCCESS) {
	printf("DEC FDDI: command %d did not work\n", 
	       comDescPtr->command & NET_DF_COMMAND_MASK);
    } else {
	printf("DEC FDDI: command %d timed out\n", 
	       comDescPtr->command & NET_DF_COMMAND_MASK);
    }
    printf("command addr: 0x%x\n", comDescPtr);
    printf("command status: 0x%x\n", comDescPtr->status);
    printf("command field: 0x%x\n", comDescPtr->command);
    printf("bufAddr field: 0x%x\n", comDescPtr->bufAddr);
    return FAILURE;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFDoInitCommand --
 *
 *	Execute the INIT command on the DEC FDDI adapter.
 *
 * Results:
 *	SUCCESS if the INIT command succeeded, FAILURE otherwise.
 *
 * Side effects:
 *	If successful, the adapter will transition to the INITIALIZED
 *      state, and the interface will have its pointers updated with
 *      the base addresses for the adapter rings and the link address
 *      for the adapter.  The link address is reported to the console.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFDoInitCommand(statePtr)
    register NetDFState         *statePtr; /* Interface to FDDI adapter. */
{
    register volatile NetDFInitCommand   *comInitPtr;
    register volatile NetDFCommandDesc   *comDescPtr;
    register char                        *linkAddr;
    register char                        *slotAddr;
    register int			 i;
    int                                  slot;
    Mach_SlotInfo                        slotInfo;
    char                                 buffer[32];
    ReturnStatus                         result;

    /*
     * The adapter must be in the UNINITIALIZED state.
     */
    NetDFAssertState(statePtr, NET_DF_STATE_UNINITIALIZED);

    comDescPtr = statePtr->comNextPtr;
    if ((comDescPtr->command & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	panic("DEC FDDI: Command Descriptor owned by adapter!");
    }
    statePtr->comNextPtr = NEXT_CMD_DESC(comDescPtr);
    comInitPtr = (NetDFInitCommand *)CmdBufFromDesc(statePtr, comDescPtr);
    /*
     * Set up a command descriptor for an INIT command.
     */
    comDescPtr->command = NET_DF_HOST_OWN | NET_DF_COMMAND_INIT;
    comDescPtr->status = 0x0;
    comDescPtr->bufAddr = (NET_DF_ULONG)comInitPtr;

    /*
     * Set up the command buffer with proper input values.
     */
    bzero((Address)comInitPtr, NET_DF_COMMAND_BUF_SIZE);
    comInitPtr->transmitMode = NET_DF_INIT_TRANSMIT_MODE;
    comInitPtr->rcvEntries = NET_DF_INIT_HOST_RCV_ENTRIES;
    Mach_EmptyWriteBuffer();

    result = NetDFCommandTimeout(statePtr, comDescPtr);
    if (result != SUCCESS) {
	panic("DEC FDDI: INIT command failed!\n");
    }

    /*
     * We should have transitioned into the INITIALIZED state.
     * Clear the STATE_CHANGE event bit.
     */
    NetDFAssertState(statePtr, NET_DF_STATE_INITIALIZED);
    *(statePtr->regEvent) = NET_DF_EVENT_STATE_CHANGE;

    /*
     * Extract and report the FDDI address.
     */
    linkAddr = (char *)comInitPtr->linkAddress;
    for (i = 0; i < 6; i++) {
	((char *)&statePtr->fddiAddress)[i] = linkAddr[i];
    }

    slot = (int) statePtr->interPtr->ctrlAddr;
    slotAddr = statePtr->slotAddr;
    result = Mach_GetSlotInfo(slotAddr + NET_DF_MACH_OPTION_ROM_OFFSET, 
			      &slotInfo);
    if (result != SUCCESS) {
	return result;
    }
    (void) Net_FDDIAddrToString(&statePtr->fddiAddress, buffer);
    printf("FDDI in slot %d, address %s (%s %s %s %s)\n",
	   slot, buffer, slotInfo.module, slotInfo.vendor, 
	   slotInfo.revision, slotInfo.type);
    result = Net_SetAddress(NET_ADDRESS_FDDI, 
		(Address) &statePtr->fddiAddress,
		&statePtr->interPtr->netAddress[NET_PROTO_RAW]);
    if (result != SUCCESS) {
	panic("NetDFDoInitCommand: Net_SetAddress failed.\n");
    }
    /*
     * Save the INIT command buffer...we will need its values for
     * the PARAM command that gets done a little later.  After the command
     * ring goes around once, this buffer becomes invalid.
     */
    bcopy(comInitPtr, statePtr->initComPtr, NET_DF_COMMAND_BUF_SIZE);

    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFDoModcamCommand --
 *
 *	Execute the MODCAM command on the DEC FDDI adapter.
 *
 * Results:
 *	SUCCESS if the MODCAM command succeeded, FAILURE otherwise.
 *
 * Side effects:
 *	If successful, the adapter will be loaded with the default
 *      CAM entries for address recognition (i.e.,
 *      NET_DF_MODCAM_RING_PURGE, NET_DF_MODCAM_BEACON)
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFDoModcamCommand(statePtr)
    register NetDFState         *statePtr; /* Interface to FDDI adapter. */
{
    register volatile NetDFModcamCommand *comModcamPtr;
    register volatile NetDFCommandDesc   *comDescPtr;
    register unsigned long               *intAddr;
    ReturnStatus                         result;

    comDescPtr = statePtr->comNextPtr;
    if ((comDescPtr->command & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	panic("DEC FDDI: Command Descriptor owned by adapter!");
    }
    statePtr->comNextPtr = NEXT_CMD_DESC(comDescPtr);
    comModcamPtr = (NetDFModcamCommand *)CmdBufFromDesc(statePtr, comDescPtr);
    /*
     * Set up a command descriptor for a MODCAM command.
     */
    comDescPtr->command = NET_DF_HOST_OWN | NET_DF_COMMAND_MODCAM;
    comDescPtr->status = 0x0;
    comDescPtr->bufAddr = (NET_DF_ULONG)comModcamPtr;

    /*
     * Set up the command buffer with proper input values.
     */
    bzero((Address)comModcamPtr, NET_DF_COMMAND_BUF_SIZE);

    intAddr = (unsigned long *)comModcamPtr;
    intAddr[0] = NET_DF_MODCAM_BEACON_LOW;
    intAddr[1] = NET_DF_MODCAM_BEACON_HIGH;
    intAddr[2] = NET_DF_MODCAM_RING_PURGE_LOW;
    intAddr[3] = NET_DF_MODCAM_RING_PURGE_HIGH;
    Mach_EmptyWriteBuffer();

    result = NetDFCommandTimeout(statePtr, comDescPtr);
    if (result != SUCCESS) {
	panic("DEC FDDI: MODCAM command did not work!\n");
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFDoStatusCommand --
 *
 *	Execute the STATUS command on the DEC FDDI adapter. 
 *
 * Results:
 *	SUCCESS if the STATUS command succeeded, FAILURE otherwise.
 *
 * Side effects:
 *      The statusComPtr slot of the statePtr is updated with the
 *      results of the status command.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFDoStatusCommand(statePtr)
    register NetDFState         *statePtr; /* Interface to FDDI adapter. */
{
    register volatile NetDFStatusCommand *comStatusPtr;
    register volatile NetDFCommandDesc   *comDescPtr;
    ReturnStatus                         result;

    comDescPtr = statePtr->comNextPtr;
    if ((comDescPtr->command & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	panic("DEC FDDI: Command Descriptor owned by adapter!");
    }
    statePtr->comNextPtr = NEXT_CMD_DESC(comDescPtr);
    comStatusPtr = (NetDFStatusCommand *)CmdBufFromDesc(statePtr, comDescPtr);
    /*
     * Set up a command descriptor for a STATUS command.
     */
    comDescPtr->command = NET_DF_HOST_OWN | NET_DF_COMMAND_STATUS;
    comDescPtr->status = 0x0;
    comDescPtr->bufAddr = (NET_DF_ULONG)comStatusPtr;
    
    /*
     * Set up the command buffer with proper input values.
     * For a status command, the adapter fills it all in.
     */
    bzero((Address)comStatusPtr, NET_DF_COMMAND_BUF_SIZE);
    Mach_EmptyWriteBuffer();

    result = NetDFCommandTimeout(statePtr, comDescPtr);
    if (result != SUCCESS) {
	printf("DEC FDDI: STATUS command did not work, darnit!\n");
	return FAILURE;
    } 
    
    printf("LED state 0x%x\t", comStatusPtr->ledState);
    printf("Link state 0x%x\t", comStatusPtr->linkState);
    printf("Phy state 0x%x\n", comStatusPtr->phyState);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFDoRdcamCommand --
 *
 *	Execute the RDCAM command on the DEC FDDI adapter.
 *
 * Results:
 *	SUCCESS if the RDCAM command succeeded, FAILURE otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFDoRdcamCommand(statePtr)
    register NetDFState   	         *statePtr;
{
    register volatile NetDFRdcamCommand  *comRdcamPtr;
    register volatile NetDFCommandDesc   *comDescPtr;
    register int			 j, k;
    register char                        *addr;
    char                                 buffer[32];
    Net_FDDIAddress                      fddiAddress;
    ReturnStatus                         result;

    comDescPtr = statePtr->comNextPtr;
    if ((comDescPtr->command & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	panic("DEC FDDI: Command Descriptor owned by adapter!");
    }
    statePtr->comNextPtr = NEXT_CMD_DESC(comDescPtr);
    comRdcamPtr = (NetDFRdcamCommand *)CmdBufFromDesc(statePtr, comDescPtr);
    /*
     * Set up a command descriptor for a RDCAM command.
     */
    comDescPtr->command = NET_DF_HOST_OWN | NET_DF_COMMAND_RDCAM;
    comDescPtr->status = 0x0;
    comDescPtr->bufAddr = (NET_DF_ULONG)comRdcamPtr;
    
    /*
     * Set up the command buffer with proper input values.
     */
    bzero((Address)comRdcamPtr, NET_DF_COMMAND_BUF_SIZE);
    Mach_EmptyWriteBuffer();

    result = NetDFCommandTimeout(statePtr, comDescPtr);
    if (result != SUCCESS) {
	printf("DEC FDDI: shoot! the RDCAM command did not work!\n");
	return result;
    }
    /*
     * Print out the addresses returned in the buffer.
     */
    for (j = 0; j < 2; j++) {
	addr = (char *)(comRdcamPtr + j);
	for (k = 0; k < 6; k++) {
	    ((char *)&fddiAddress)[k] = addr[k];
	}
	(void) Net_FDDIAddrToString(&fddiAddress, buffer);
	DFprintf("RDCAM entry %d addr 0x%x -- %s\n", j, 
	       comRdcamPtr + j, buffer);
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFDoParamCommand --
 *
 *	Execute the PARAM command on the DEC FDDI adapter.
 *
 * Results:
 *	SUCCESS if the PARAM command succeeded, FAILURE otherwise.
 *
 * Side effects:
 *	If successful, the adapter will transition to the RUNNING state
 *      and connect to the FDDI ring.  Processing of receive and
 *      transmit frames will begin.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFDoParamCommand(statePtr)
    register NetDFState  	*statePtr; /* Interface to FDDI adapter. */
{
    register volatile NetDFParamCommand  *comParamPtr;
    register volatile NetDFCommandDesc   *comDescPtr;
    ReturnStatus                         result;

    comDescPtr = statePtr->comNextPtr;
    if ((comDescPtr->command & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	panic("DEC FDDI: Command Descriptor owned by adapter!");
    }
    statePtr->comNextPtr = NEXT_CMD_DESC(comDescPtr);
    comParamPtr = (NetDFParamCommand *)CmdBufFromDesc(statePtr, comDescPtr);
    /*
     * Set up a command descriptor for a PARAM command.
     */
    comDescPtr->command = NET_DF_HOST_OWN | NET_DF_COMMAND_PARAM;
    comDescPtr->status = 0x0;
    comDescPtr->bufAddr = (NET_DF_ULONG)comParamPtr;
    
    /*
     * Set up the command buffer with proper input values.
     * Most of these values are default values that are given
     * to us from the adapter using the INIT command.
     */
    bzero((Address)comParamPtr, NET_DF_COMMAND_BUF_SIZE);
    comParamPtr->loopMode = NET_DF_NO_LOOP;
    comParamPtr->tMax = statePtr->initComPtr->defaultTMax;
    comParamPtr->tReq = statePtr->initComPtr->defaultTReq;
    comParamPtr->tvx = statePtr->initComPtr->defaultTvx;
    comParamPtr->lemThresh = statePtr->initComPtr->lemThresh;
    comParamPtr->stationID.count1 = statePtr->initComPtr->stationID.count1;
    comParamPtr->stationID.count2 = statePtr->initComPtr->stationID.count2;
    comParamPtr->tokenTimeout =	statePtr->initComPtr->tokenTimeout;
    comParamPtr->ringPurgeEnable = statePtr->initComPtr->ringPurgeEnable;
    Mach_EmptyWriteBuffer();

    result = NetDFCommandTimeout(statePtr, comDescPtr);
    if (result != SUCCESS) {
	panic("DEC FDDI: uh oh...the PARAM command did not work\n");
    }
    /*
     * We should have transitioned into the RUNNING state.
     * Make sure to clear the STATE_CHANGE event bit.
     */
    NetDFAssertState(statePtr, NET_DF_STATE_RUNNING);
    *(statePtr->regEvent) = NET_DF_EVENT_STATE_CHANGE;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFDoModpromCommand --
 *
 *	Execute the MODPROM command on the DEC FDDI adapter.
 *
 * Results:
 *	SUCCESS if the MODPROM command succeeded, FAILURE otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFDoModpromCommand(statePtr)
    register NetDFState  	          *statePtr; 
{
    register volatile NetDFModpromCommand *comModpromPtr;
    register volatile NetDFCommandDesc    *comDescPtr;
    ReturnStatus                          result;

    comDescPtr = statePtr->comNextPtr;
    if ((comDescPtr->command & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	panic("DEC FDDI: Command Descriptor owned by adapter!");
    }
    statePtr->comNextPtr = NEXT_CMD_DESC(comDescPtr);
    comModpromPtr = (NetDFModpromCommand *)
	CmdBufFromDesc(statePtr, comDescPtr);
    /*
     * Set up a command descriptor for a MODPROM command.
     */
    comDescPtr->command = NET_DF_HOST_OWN | NET_DF_COMMAND_MODPROM;
    comDescPtr->status = 0x0;
    comDescPtr->bufAddr = (NET_DF_ULONG)comModpromPtr;
    
    /*
     * Set up the command buffer with proper input values.
     */
    bzero((Address)comModpromPtr, NET_DF_COMMAND_BUF_SIZE);

    comModpromPtr->llcPromEnable = NET_DF_FALSE;
    comModpromPtr->smtPromEnable = NET_DF_FALSE;
    comModpromPtr->llcMultiEnable = NET_DF_FALSE;
    comModpromPtr->llcBroadEnable = NET_DF_FALSE;

    result = NetDFCommandTimeout(statePtr, comDescPtr);
    if (result != SUCCESS) {
	printf("DEC FDDI: MODPROM command flopped all over the place\n");
	return FAILURE;
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcessUnsolicited --
 *
 *	Process the UNSOLICITED ring entries.  We just report
 *      what they say;  we don't really act on them.
 *
 * Results:
 *	SUCCESS if we succeeded to read them, FAILURE otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
ProcessUnsolicited(statePtr)
    register NetDFState *statePtr;
{
    register volatile NetDFUnsolDesc  *descPtr;
    register Net_FDDIAddress          *addrPtr;
    register NetDFDirectedBeacon      *beaconPtr;
    char                              buffer[32];

    static char     *eventNames[] = {
	"UNDEFINED",
	"RING_INIT_INIT",
	"RING_INIT_RCV",
	"BEACON_INIT",
	"DUP_ADDR",
	"DUP_TOKEN",
	"PURGE_ERROR",
	"STRIP_ERROR",
	"OP_OSCILLAT",
	"BEACON_RCV",
	"PC_TRACE_INIT",
	"PC_TRACE_RECV",
	"XMT_UNDERRUN",
	"XMT_FAILURE",
	"RCV_OVERRUN"
    };
    
    descPtr = statePtr->unsolNextDescPtr;
    
    if ((descPtr->own & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	if (statePtr->lastUnsolCnt == 0) {
	    printf("DEC FDDI: Unsolicited Descriptor owned by adapter.\n");
	    return (FAILURE);
	} else {
	    statePtr->lastUnsolCnt == 0;
	    return (SUCCESS);
	}
    }
    statePtr->lastUnsolCnt = 0;
    while (TRUE) {
	switch (descPtr->eventID) {
	case NET_DF_UNSOL_UNDEFINED:
	case NET_DF_UNSOL_RING_INIT_INIT:
	case NET_DF_UNSOL_RING_INIT_RCV:
	case NET_DF_UNSOL_BEACON_INIT:
	case NET_DF_UNSOL_DUP_ADDR:
	case NET_DF_UNSOL_DUP_TOKEN:
	case NET_DF_UNSOL_PURGE_ERROR:
	case NET_DF_UNSOL_STRIP_ERROR:
	case NET_DF_UNSOL_OP_OSCILLAT:
	case NET_DF_UNSOL_PC_TRACE_INIT:
	case NET_DF_UNSOL_PC_TRACE_RECV:
	case NET_DF_UNSOL_XMT_UNDERRUN:
	case NET_DF_UNSOL_XMT_FAILURE:
	case NET_DF_UNSOL_RCV_OVERRUN:
	    printf("DEC FDDI: Received Unsolicited Event: %s\n", 
		   eventNames[descPtr->eventID]);
	    break;
	case NET_DF_UNSOL_BEACON_RCV:
	    printf("DEC FDDI: Received Unsolicited Event: %s\n", 
		   eventNames[descPtr->eventID]);
	    beaconPtr = (NetDFDirectedBeacon *)
		(descPtr->bufAddr + statePtr->slotAddr);
	    addrPtr = (Net_FDDIAddress *)beaconPtr->sourceAddr;
	    (void) Net_FDDIAddrToString(addrPtr, buffer);
	    printf("          Source Address: %s", buffer);
	    addrPtr = (Net_FDDIAddress *)beaconPtr->una;
	    (void) Net_FDDIAddrToString(addrPtr, buffer);
	    printf("          UNA of Source: %s", buffer);

	    break;
	default:
	    printf("DEC FDDI: Unknown Unsolicited Event.\n");
	    break;
	}
	statePtr->lastUnsolCnt++;
	descPtr = (descPtr + 1) > statePtr->unsolLastDescPtr ?
	    statePtr->unsolFirstDescPtr : (descPtr + 1);
	if ((descPtr->own & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	    break;
	}
    }
    statePtr->unsolNextDescPtr = descPtr;
    return (SUCCESS);
}

static void FinishReset();


/*
 *----------------------------------------------------------------------
 *
 * NetDFReset --
 *
 *	Reset the interface.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All of the pointers in the interface structure are initialized.
 *
 *----------------------------------------------------------------------
 */

void
NetDFReset(interPtr)
    Net_Interface	*interPtr; /* Interface to reset. */
{
    register NetDFState		        *statePtr;
    register int			i;
    unsigned short	                status;
    unsigned short	                event;
    unsigned long	                own;
    Boolean                             test;

    statePtr = (NetDFState *) interPtr->interfaceData;
    /*
     * If there isn't a reset pending already, and the chip is currently
     * transmitting then just set the pending flag.  With any luck
     * this mechanism will prevent the chip from being reset right in
     * the middle of a packet.
     */

    if (!(statePtr->resetPending) && (statePtr->transmitting)) {
	printf("Deferring reset.\n");
	statePtr->resetPending = TRUE;
	return;
    }
    statePtr->resetPending = FALSE;

    /* 
     * Reset (and stop) the chip.
     */
    interPtr->flags &= ~NET_IFLAGS_RUNNING;
    /*
     * Reset the chip.  First put the chip into driver mode by setting
     * the DRIVER_MODE bit in PORT_CONTROL_B, and then set and clear
     * the RESET bit in PORT_RESET.  The chip will then run self-tests,
     * and, if the self-tests pass, should transition to UNINITIALIZED
     * in less than 30 seconds.  Otherwise it has failed the self-tests,
     * and will remain in RESETTING.
     */
    
    NET_DF_DISABLE_ALL_INT(*(statePtr->regMask));
    Mach_EmptyWriteBuffer();

    status = *(statePtr->regStatus);
    status = (status & NET_DF_STATUS_ADAPTER_STATE) >> 8;

    *(statePtr->regCtrlB) = NET_DF_CTRLB_DRIVER_MODE;
    Mach_EmptyWriteBuffer();
    *(statePtr->regReset) = NET_DF_RESET_RESET;
    Mach_EmptyWriteBuffer();
    *(statePtr->regReset) = NET_DF_CLEAR;
    Mach_EmptyWriteBuffer();
    
    NetDFAssertState(statePtr, NET_DF_STATE_RESETTING);
    printf("DEC FDDI: RESETTING -> ");

    statePtr->flags |= NET_DF_FLAGS_RESETTING;
    for (i = 0; i < 50000000; i++) {
	test = *(statePtr->regEvent) & NET_DF_EVENT_STATE_CHANGE;
	if (test) {
	    status = *(statePtr->regStatus) & NET_DF_STATUS_ADAPTER_STATE;
	    if (status == NET_DF_STATE_UNINITIALIZED) {
		printf("UNINITIALIZED -> ");
		break;
	    }
	}
    }
    test = *(statePtr->regEvent) & NET_DF_EVENT_STATE_CHANGE;
    if (!test) {
	NetDFPrintRegContents(statePtr);
	panic("DEC FDDI: Adapter will not initialize!\n");
	/*
	 * should read the TEST_ID field of PORT_STATUS to see which
	 * self-test failed
	 */
    }
    NET_DF_ENABLE_ALL_INT(*(statePtr->regMask));
    NET_DF_CLEAR_ALL_EVENTS(*(statePtr->regEvent));

    /*
     * Reset the COMMAND and UNSOLICITED rings before we execute commands.
     */
    statePtr->comNextPtr = statePtr->comRingPtr;
    statePtr->unsolNextDescPtr = statePtr->unsolFirstDescPtr;
    statePtr->lastUnsolCnt = 0;

    printf("INITIALIZED\n");
    NetDFDoInitCommand(statePtr);
    /*
     * Once the INIT command completes, we can initialize the HOST RCV,
     * SMT RCV, RMC XMT, and SMT XMT rings.
     */
    NetDFRecvInit(statePtr);
    NetDFXmitInit(statePtr);

    /*
     * Now that the rings are initialized, massage the adapter into the
     * RUNNING state and connect to the network.  Please please please.
     */
    NetDFDoModcamCommand(statePtr);
    NetDFDoParamCommand(statePtr);
    NetDFDoModpromCommand(statePtr);

    NET_DF_CLEAR_ALL_EVENTS(*(statePtr->regEvent));

    statePtr->numResets++;
    statePtr->flags &= ~NET_DF_FLAGS_RESETTING;
    interPtr->flags = NET_IFLAGS_RUNNING;
    Sync_Broadcast(&statePtr->doingReset);
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * NetDFRestart --
 *
 *	Reinitialize the DEC FDDI adapter.  Assumes that the
 *      interface mutex is held.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the adapter.
 *
 *----------------------------------------------------------------------
 */
void
NetDFRestart(interPtr)
    Net_Interface	*interPtr; 	/* Interface to restart. */
{

    NetDFState	*statePtr = (NetDFState *) interPtr->interfaceData;

    /*
     * Drop the current packet so the sender doesn't get hung.
     */
    DFprintf("DEC FDDI: Dropping current packet.\n");
    NetDFXmitDrop(statePtr);

    /*
     * Reset the world.
     */
    NetDFReset(interPtr);

    /*
     * Restart transmission of packets.
     */
    DFprintf("DEC FDDI: Restarting transmission queue.\n");
    NetDFXmitRestart(statePtr);

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFRestartCallback --
 *
 *	This routine is called by the Proc_ServerProc during the
 *	callback to reset the adapter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The adapter is reset.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
NetDFRestartCallback(data, infoPtr)
    ClientData		data;		/* Ptr to the interface to reset. */
    Proc_CallInfo	*infoPtr;	/* Unused. */
{
    Net_DFRestart((Net_Interface *) data);
}

/*
 *----------------------------------------------------------------------
 *
 * Net_DFRestart --
 *
 *	This is a version of the reset routine that can be called
 * 	from outside the module since it locks the mutex.
 *
 * Results:
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Net_DFRestart(interPtr)
    Net_Interface	*interPtr;	/* Interface to reset. */
{
    register NetDFState *statePtr;

    statePtr = (NetDFState *) interPtr->interfaceData;

    MASTER_LOCK(&interPtr->mutex);

    if (statePtr->flags & NET_DF_FLAGS_RESETTING) {
	MAKE_NOTE("Restarting while restarting.  Doing nothing.");
	goto exit;
    }

    MAKE_NOTE("_Restarting FDDI.");
    /*
     * If we are at interrupt level we have to do a callback to reset
     * the adapter since we can't wait for the response from
     * the adapter (there may not be a current process and we can't
     * get the interrupt).
     */
    if (Mach_AtInterruptLevel()) {
	Proc_CallFunc(NetDFRestartCallback, (ClientData) interPtr, 0);
    } else {
	(void) NetDFRestart(interPtr);
    }
exit:
    MASTER_UNLOCK(&interPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFIntr --
 *
 *	Process an interrupt from the DEC FDDI adapter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
NetDFIntr(interPtr, polling)
    Net_Interface	*interPtr;	/* Interface to process. */
    Boolean		polling;	/* TRUE if are being polled instead of
					 * processing an interrupt. */
{
    register NetDFState *statePtr;
    register volatile unsigned short event;
    int                 i;
    List_Links          *xmitElem;
    static unsigned short linkStatus = NET_DF_LINK_UNAVAILABLE;
    unsigned short      status;
    ReturnStatus        statusRcv, statusXmt;
    static Boolean      shouldNotice = FALSE;
    static Boolean      halt = FALSE;
    Boolean             test;
    unsigned short      result;


    statePtr = (NetDFState *) interPtr->interfaceData;

    MASTER_LOCK(&interPtr->mutex);

    if (statePtr->flags & NET_DF_FLAGS_RESETTING) {
	test = *(statePtr->regEvent) & NET_DF_EVENT_STATE_CHANGE;
	if (test) {
	    result = *(statePtr->regStatus) & NET_DF_STATUS_ADAPTER_STATE;
	    if (result == NET_DF_STATE_UNINITIALIZED) {
		/*
		 * Put code here to wakeup process waiting in the
		 * reset code.
		 */
	    } else {
		NetDFPrintRegContents(statePtr);
		panic("DEC FDDI: Adapter did not initialize during reset!\n");
	    }
	} else {
	    /*
	     * Clear those pesky events that aren't what we are looking for.
	     */
	    *(statePtr->regEvent) = test;
	}
	goto exit;
    }

/*
 * useful for halting in the interrupt routine when debugging
 
    if (halt == TRUE) {
	NET_DF_CLEAR_ALL_EVENTS(*statePtr->regEvent);
	NET_DF_DISABLE_ALL_INT(*statePtr->regMask);
	Mach_EmptyWriteBuffer();

	goto exit;
    }
 */
    event = *(statePtr->regEvent);
    *(statePtr->regEvent) = event;
    if (shouldNotice && !(event & NET_DF_EVENT_XMT_PKT_DONE)) {
	MAKE_NOTE("clearly missed the transmit done.");
	shouldNotice = FALSE;
    }
    status = *(statePtr->regStatus);
    DFprintf("\nDEC FDDI: NetDFIntr --->");
    /*
     * LINK_STATUS_CHANGE tells us either that we just connected
     * to the ring, or that the ring has become unavailable.
     * We are not supposed to transmit USER_LLC or USER_SMT
     * packets, although we must handle all others.
     */
    if (event & NET_DF_EVENT_LINK_STATUS_CHANGE) {
	DFprintf("DEC FDDI: Interrupt: Link status changed to 0x%x.\n",
		 status & NET_DF_STATUS_LINK_STATUS);
	if ((status & NET_DF_STATUS_LINK_STATUS) == NET_DF_LINK_UNAVAILABLE) {
	    printf("DEC FDDI: Link became unavailable.\n");
	    linkStatus = NET_DF_LINK_UNAVAILABLE;
	    halt = TRUE;
	} else {
	    printf("DEC FDDI: Link now available.\n");
	    linkStatus = NET_DF_LINK_AVAILABLE;
	}
    }
    /*
     * For the PM_PARITY_ERROR and MB_PARITY_ERROR, nothing seems
     * to happen....so do nothing.  For NXM_ERROR, the adapter
     * will have transitioned into the HALTED state.  
     */
    if (event & NET_DF_EVENT_PM_PARITY_ERROR) {
	DFprintf("DEC FDDI: Interrupt: PM parity error.\n");
    }
    if (event & NET_DF_EVENT_MB_PARITY_ERROR) {
	DFprintf("DEC FDDI: Interrupt: MB parity error.\n");
    }
    if (event & NET_DF_EVENT_NXM_ERR) {
	DFprintf("DEC FDDI: Interrupt: Non existant memory error.\n");
    }
    /*
     * Should be a rare event.
     */
    if (event & NET_DF_EVENT_FLUSH_TX) {
	DFprintf("DEC FDDI: Interrupt: Flush XMT rings requested.\n");
    }
    /*
     * The adapter has changed state.  Most likely this means that
     * we have halted for some reason.
     */
    if (event & NET_DF_EVENT_STATE_CHANGE) {
	DFprintf("DEC FDDI: Interrupt: Adapter state changed to 0x%x.\n",
		 *(statePtr->regStatus) & NET_DF_STATUS_ADAPTER_STATE);
    }
    if ((*(statePtr->regStatus) & NET_DF_STATUS_ADAPTER_STATE)
	== NET_DF_STATE_HALTED) {
	printf("DEC FDDI: Adapter halted.\n");

#ifdef NOTDEF
	NetDFPrintRegContents(statePtr);
	NetDFPrintErrorLog(statePtr);

	NET_DF_CLEAR_ALL_EVENTS(*statePtr->regEvent);
	NET_DF_DISABLE_ALL_INT(*statePtr->regMask);
	Mach_EmptyWriteBuffer();

	NetDFPrintDebugRing(statePtr);
	DBG_CALL;
	Mach_EmptyWriteBuffer();
	Mach_EmptyWriteBuffer();
	halt = TRUE;
#endif
	/*
	 * Note that this reset does not lock the mutex.
	 */
	NetDFRestart(interPtr);    
	goto exit;
    }
    /*
     * An unsolicited event has arrived.  Just process it and
     * report it.
     */
    if (event & NET_DF_EVENT_UNS_POLL_DEMAND) {
	DFprintf("DEC FDDI: Interrupt: unsolicited event.\n");
	ProcessUnsolicited(statePtr);
    }
    /*
     * A command has finished.  And there was rejoicing.
     */
    if (event & NET_DF_EVENT_CMD_DONE) {
	DFprintf("DEC FDDI: Interrupt: Command done.\n");
    }
    if ((*(statePtr->regStatus) & NET_DF_STATUS_ADAPTER_STATE) 
	!= NET_DF_STATE_RUNNING) {
	/*
	 * If we're not in in the HALTED state and we're not in the
	 * RUNNING state, then don't process any of the other events.
	 */
	MAKE_NOTE("stopped processing events --> not in running state");
	goto exit;
    }
    statusRcv = statusXmt = SUCCESS;
    /*
     * We have finished transmitting a packet, so send another
     * one if there are any left.
     */
    if ((*(statePtr->regEvent) & NET_DF_EVENT_XMT_PKT_DONE)
	&& !(event & NET_DF_EVENT_XMT_PKT_DONE)) {
	MAKE_NOTE("skipped a transmit done, level 1");
	shouldNotice = TRUE;
    }
    if (event & NET_DF_EVENT_XMT_PKT_DONE) {
	DFprintf("DEC FDDI: Interrupt: XMT packet done.\n");
	MAKE_NOTE("---before XMT done---");
	statusXmt = NetDFXmitDone(statePtr);
	MAKE_NOTE("---after XMT done---");
    }
    if ((*(statePtr->regStatus) & NET_DF_STATUS_ADAPTER_STATE)
	== NET_DF_STATE_HALTED) {
	MAKE_NOTE("---halted after XMT done---");
    }
    /*
     * We have received a packet, so give the contents to the lucky
     * process that's waiting for it.
     */
    if (event & NET_DF_EVENT_RCV_POLL_DEMAND) {
	DFprintf("DEC FDDI: Interrupt: RCV poll demand.\n");
	statusRcv = NetDFRecvProcess(FALSE, statePtr);
	MAKE_NOTE("---after RCV done---");
    }
    /*
     * The adapter wants us to transfer a SMT packet from the
     * SMT_XMT ring to the RMC_XMT ring.   Oblige it.
     */
    if (event & NET_DF_EVENT_SMT_XMT_POLL_DEMAND) {
	DFprintf("DEC FDDI: Interrupt: SMT XMT poll demand.\n");

	MAKE_NOTE("SMT XMT poll demand");
	NetDFSmtOutput(interPtr);
    }

    if (netDFDebug == NET_DF_DEBUG_ON) {
	i = 0;
	LIST_FORALL(statePtr->xmitList, xmitElem) {
	    i++;
	}
	DFprintf("DEC FDDI: <--------------- XMT Queue: %d\n", i);
    }
    
    if (statusRcv != SUCCESS || statusXmt != SUCCESS) {
	if (statusRcv != SUCCESS) {
	    MAKE_NOTE("RCV wasn't successful.");
	}
	if (statusXmt != SUCCESS) {
	    MAKE_NOTE("XMT wasn't successful.");
	}
    }
exit:
    MASTER_UNLOCK(&interPtr->mutex);
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * NetDFGetStats --
 *
 *	Return the statistics for the interface.
 *
 * Results:
 *	A pointer to the statistics structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetDFGetStats(interPtr, statPtr)
    Net_Interface	*interPtr;		/* Current interface. */
    Net_Stats		*statPtr;		/* Statistics to return. */
{
    NetDFState *statePtr = (NetDFState *) interPtr->interfaceData;

    MASTER_LOCK(&interPtr->mutex);

    if (statePtr->flags & NET_DF_FLAGS_RESETTING) {
	MAKE_NOTE("Process waiting in NetDFIOControl while resetting.\n");
	do {
	    Sync_MasterWait(&statePtr->doingReset, &interPtr->mutex, FALSE);
	} while (statePtr->flags & NET_DF_FLAGS_RESETTING);
    }

    MAKE_NOTE("Returning FDDI stats.");
    statPtr->fddi = statePtr->stats;

    MASTER_UNLOCK(&interPtr->mutex);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * NetDFIOControl --
 *
 *	Perform ioctls for the adapter.  Right now we don't support any.
 *
 * Results:
 *	DEV_INVALID_ARG
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
NetDFIOControl(interPtr, ioctlPtr, replyPtr)
    Net_Interface *interPtr;	/* Interface on which to perform ioctl. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    register NetDFState *statePtr;
    ReturnStatus status;

    statePtr = (NetDFState *)interPtr->interfaceData;

    DFprintf("NetDFIOControl: command = %d\n", ioctlPtr->command);
    if ((ioctlPtr->command & ~0xffff) != IOC_FDDI) {
	return DEV_INVALID_ARG;
    }

    MASTER_LOCK(&interPtr->mutex);

    if (statePtr->flags & NET_DF_FLAGS_RESETTING) {
	MAKE_NOTE("Process waiting in NetDFIOControl while resetting.\n");
	do {
	    Sync_MasterWait(&statePtr->doingReset, &interPtr->mutex, FALSE);
	} while (statePtr->flags & NET_DF_FLAGS_RESETTING);
    }

    status = SUCCESS;
    switch(ioctlPtr->command) {
    case IOC_FDDI_RESET:
	MASTER_UNLOCK(&interPtr->mutex);
	Net_DFRestart(interPtr);
	MASTER_LOCK(&interPtr->mutex);
	status = SUCCESS;
	break;
    case IOC_FDDI_DEBUG:
	if (netDFDebug == NET_DF_DEBUG_ON) {
	    printf("DEC FDDI: Debug turned off.\n");
	    netDFDebug = NET_DF_DEBUG_OFF;
	} else {
	    printf("DEC FDDI: Debug turned on.\n");
	    netDFDebug = NET_DF_DEBUG_ON;
	}
	break;
    case IOC_FDDI_REG_CONTENTS: {
	Dev_FDDIRegContents *regContentsPtr;
	
	regContentsPtr = (Dev_FDDIRegContents *)ioctlPtr->outBuffer;
	if (regContentsPtr == NULL) {
	    status = FAILURE;
	    goto exit;
	}
	regContentsPtr->regReset = *(statePtr->regReset);
	regContentsPtr->regCtrlA = *(statePtr->regCtrlA);
	regContentsPtr->regCtrlB = *(statePtr->regCtrlB);
	regContentsPtr->regStatus = *(statePtr->regStatus);
	regContentsPtr->regEvent = *(statePtr->regEvent);
	regContentsPtr->regMask = *(statePtr->regMask);
	break;
    }
    case IOC_FDDI_ERR_LOG: {
	Dev_FDDIErrLog *errLogPtr;

	errLogPtr = (Dev_FDDIErrLog *)ioctlPtr->outBuffer;
	if (errLogPtr == NULL) {
	    status = FAILURE;
	    goto exit;
	}
	errLogPtr->internal = *(statePtr->errLogPtr +
				NET_DF_MACH_ERR_INTERNAL_OFFSET);
	errLogPtr->external = *(statePtr->errLogPtr +
				NET_DF_MACH_ERR_EXTERNAL_OFFSET);
	break;
    }
    case IOC_FDDI_SEND_PACKET: {
	Dev_FDDISendPacket *packetPtr;
	ReturnStatus       retval;
	Net_FDDIHdr        *fddiHdrPtr;
	Net_ScatterGather  *scatterPtr;

	packetPtr = (Dev_FDDISendPacket *)ioctlPtr->inBuffer;
	if (packetPtr == NULL) {
	    status = FAILURE;
	    goto exit;
	}
	fddiHdrPtr = &statePtr->headerArray[statePtr->scatterIndex];
	scatterPtr = &statePtr->scatterArray[statePtr->scatterIndex];
	IncScatterIndex(statePtr);

	fddiHdrPtr->prh[0] = NET_DF_PRH0;
	fddiHdrPtr->prh[1] = NET_DF_PRH1;
	fddiHdrPtr->prh[2] = NET_DF_PRH2;
	fddiHdrPtr->frameControl = NET_DF_FRAME_HOST_LLC;
	NET_FDDI_ADDR_COPY(packetPtr->dest, fddiHdrPtr->dest);
	
	scatterPtr->bufAddr = packetPtr->buffer;
	scatterPtr->length = packetPtr->length;
	scatterPtr->mutexPtr = NULL;
	scatterPtr->done = FALSE;

	/*
	 * NetDFOutput locks the mutex.
	 */
	MASTER_UNLOCK(&interPtr->mutex);
	retval = NetDFOutput(interPtr, fddiHdrPtr, scatterPtr, 1, FALSE, NIL);
	MASTER_LOCK(&interPtr->mutex);

	break;
    }
    case IOC_FDDI_FLUSH_XMT_Q: {
	printf("DEC FDDI: Dropping current packet.\n");
	NetDFXmitDrop(statePtr);
	printf("DEC FDDI: Flushing transmit queue.\n");
	NetDFXmitFlushQ(statePtr);
	break;
    }
    case IOC_FDDI_ADDRESS: {
	Dev_FDDILinkAddr *infoPtr;

	infoPtr = (Dev_FDDILinkAddr *)ioctlPtr->outBuffer;
	if (infoPtr == NULL) {
	    status = FAILURE;
	    goto exit;
	}
	NET_FDDI_ADDR_COPY(statePtr->fddiAddress, infoPtr->source);
	break;
    }
    case IOC_FDDI_RPC_ECHO: {
	Dev_FDDIRpcEcho       *echoPtr;
	Dev_FDDIRpcEchoReturn *returnPtr;
	char                  *bufPtr;

	echoPtr = (Dev_FDDIRpcEcho *)ioctlPtr->inBuffer;
	if (echoPtr == NULL || echoPtr == (Dev_FDDIRpcEcho *)NULL) {
	    status = FAILURE;
	    goto exit;
	}
	returnPtr = (Dev_FDDIRpcEchoReturn *)ioctlPtr->outBuffer;
	if (returnPtr == NULL || returnPtr == (Dev_FDDIRpcEchoReturn *)NULL) {
	    status = FAILURE;
	    goto exit;
	}
	bufPtr = (char *)malloc(echoPtr->packetSize);
	if (echoPtr->printSyslog == TRUE) {
	    status = Rpc_EchoTest(echoPtr->serverID, echoPtr->numEchoes,
				  echoPtr->packetSize, bufPtr, bufPtr, 
				  (Time *)NIL);
	} else {
	    status = Rpc_EchoTest(echoPtr->serverID, echoPtr->numEchoes,
				  echoPtr->packetSize, bufPtr, bufPtr, 
				  &returnPtr->rpcTime);
	}
	free(bufPtr);
	break;
    }
    case IOC_FDDI_HALT: {
	*(statePtr->regCtrlA) |= NET_DF_CTRLA_HALT;
	break;
    }
    case IOC_FDDI_STATS: {
	Dev_FDDIStats *statsPtr;

	statsPtr = (Dev_FDDIStats *)ioctlPtr->outBuffer;
	if (statsPtr == NULL || statsPtr == (Dev_FDDIStats *)NIL) {
	    status = DEV_INVALID_ARG;
	    goto exit;
	}
	bcopy((Address)&statePtr->stats, (Address)statsPtr,
	      sizeof(Dev_FDDIStats));
	break;
    }
    default:
	printf("NetDFIOControl: unknown ioctl 0x%x\n", ioctlPtr->command);
    }
    
exit:
    MASTER_UNLOCK(&interPtr->mutex);
    return status;
}
