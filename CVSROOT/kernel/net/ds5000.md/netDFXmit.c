/* 
 * netDFXmit.c --
 *
 *	Routines to transmit packets on the DEC 700 FDDI controller.
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
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif

#include <sprite.h>
#include <sys.h>
#include <netDFInt.h>
#include <vm.h>
#include <vmMach.h>
#include <list.h>
#include <sync.h>
#include <machMon.h>

#include <dbg.h>

/*
 * Macros to step RMC XMT ring pointers.
 */
#define	NEXT_RMC_SEND(p) ( (((p)+1) > statePtr->rmcXmtLastDescPtr) ? \
				statePtr->rmcXmtFirstDescPtr : \
				((p)+1))
#define	PREV_RMC_SEND(p) ( (((p)-1) < statePtr->rmcXmtFirstDescPtr) ? \
				statePtr->rmcXmtLastDescPtr : \
				((p)-1))
/*
 * Macros to step SMT XMT ring pointers.
 */
#define	NEXT_SMT_SEND(p) ( (((p)+1) > statePtr->smtXmtLastDescPtr) ? \
				statePtr->smtXmtFirstDescPtr : \
				((p)+1))

static ReturnStatus OutputPacket _ARGS_((Net_FDDHdr *fddiHdrPtr, 
					 Net_ScatterGather *scatterGatherPtr, 
					 int scatterGatherLength, 
					 NetDFState *statePtr));

/*
 * To access the buffer corresponding to the RMC XMT descriptor
 */
#define RmcXmtBufFromDesc(statePtr, descPtr) \
  (statePtr->rmcXmtFirstBufPtr + \
   (unsigned long)(descPtr - statePtr->rmcXmtFirstDescPtr))


/*
 *----------------------------------------------------------------------
 *
 * NetDFBcopy --
 *
 *      Just like bcopy(), except that all data is transfered in words
 *      on word-aligned boundaries.  The adapter requires that this
 *      be so.
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
NetDFBcopy(startPtr, destPtr, numBytes)
    unsigned char *startPtr;
    unsigned char *destPtr;
    unsigned long numBytes;
{
    register unsigned char *bytePtr, *toPtr, *fromPtr;
    register unsigned long *div;
    register unsigned long mod, length, aligned;
    register unsigned long n;
    unsigned long          value;

    /*
     * One of the most annoying things about this adapter is that,
     * although you can read bytes, you can't write bytes that
     * are not word-aligned.  This is a pain, because we can't just
     * flat out use bcopy()...we have to massage the beginnings and
     * ends of the buffers so that we use bcopy only on an integral
     * number of words.
     */
    fromPtr = startPtr;
    toPtr = destPtr;
    length = numBytes;

    if (length == 0) {
	return;
    }

    mod = (unsigned long)toPtr % sizeof(unsigned long);
    div = (unsigned long *)(((unsigned long)toPtr >> 2) << 2);

    /*
     * Write bytes to the destination array until the pointer
     * is word-aligned.
     */
    value = *div;
    bytePtr = (unsigned char *)&value;
    switch((int)mod) {
    case 1:
	bytePtr[1] = fromPtr[0];
	fromPtr++;
    case 2:
	bytePtr[2] = fromPtr[0];
	fromPtr++;
    case 3:
	bytePtr[3] = fromPtr[0];
	fromPtr++;
	*div = value;
	length -= (4 - mod);
	toPtr += (4 - mod);
    default:
	break;
    }

    aligned = (length >> 2) << 2;
	/*
	 * We are really hosed if the source buffer is not word aligned
	 * after we align the dest buffer.  The adapter write addressing
	 * really stinks.  Luckily this appears to be a rare occurance.
	 */
    mod = (unsigned long)fromPtr % sizeof(unsigned long);
    if (mod) {
	n = aligned;
	div = (unsigned long *)toPtr;
	while (n > 0) {
	    value = *div;
	    bytePtr = (unsigned char *)&value;
	    bytePtr[0] = fromPtr[0];
	    bytePtr[1] = fromPtr[1];
	    bytePtr[2] = fromPtr[2];
	    bytePtr[3] = fromPtr[3];
	    *div = value;
	    div++;
	    fromPtr += 4;
	    n -= 4;
	}
	toPtr += aligned;
    } else { 
	/*
	 * Only bcopy() an integral number of words. 
	 */
	if (aligned != 0) {
	    bcopy(fromPtr, toPtr, aligned);
	    toPtr += aligned;
	    fromPtr += aligned;
	}
    } 

    /*
     * finish up the tail end of the buffers
     */
    if (aligned != length) {
	mod = length - aligned;
	div = (unsigned long *)toPtr;
	
	value = *div;
	bytePtr = (unsigned char *)&value;
	switch((int)mod) {
	case 3:
	    bytePtr[2] = fromPtr[2];
	case 2:
	    bytePtr[1] = fromPtr[1];
	case 1:
	    bytePtr[0] = fromPtr[0];
	    *div = value;
	default:
	    break;
	}
    }

    /*
     * Extra anal debugging.
     */
    if (netDFDebug == NET_DF_DEBUG_ON) {
	for (n = 0; n < numBytes; n++) {
	    if (startPtr[n] != destPtr[n]) {
		printf("DEC FDDI: NetDFBcopy failed!\n");
		DBG_CALL;
		Mach_EmptyWriteBuffer();
		Mach_EmptyWriteBuffer();
		Mach_EmptyWriteBuffer();
		Mach_EmptyWriteBuffer();
		return;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * OutputXmtElement --
 *
 *      Transmit the given packet that was waiting on the transmit
 *      queue.  The element is removed from the queue.
 *
 * Results:
 *	SUCCESS if the packet was successfully transmitted; FAILURE
 *      otherwise.
 *
 * Side effects:
 *	A packet is output.
 *
 *----------------------------------------------------------------------
 */
OutputXmtElement(statePtr, xmtElementPtr)
    register NetDFState      *statePtr;
    register NetDFXmtElement *xmtElementPtr;
{
    int status;
    
    switch(xmtElementPtr->xmtType) { 
    case NET_DF_XMT_HOST: 
	DFprintf("DEC FDDI: Outputting LLC packet.\n"); 
	status = OutputPacket(xmtElementPtr->fddiHdrPtr, 
				xmtElementPtr->scatterGatherPtr, 
				xmtElementPtr->scatterGatherLength,  
				statePtr); 
	DFprintf("DEC FDDI: Finished outputting LLC packet.\n"); 
	break; 
    case NET_DF_XMT_SMT: 
	status = NetDFOutputSmtPacket(statePtr,  
				      xmtElementPtr->smtDescPtr); 
	break; 
    default: 
	printf("DEC FDDI: Unknown XMT element packet type.\n"); 
	status = FAILURE; 
	break; 
    } 
    List_Move((List_Links *) xmtElementPtr, 
	      LIST_ATREAR(statePtr->xmitFreeList)); 
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * CastOffPacket --
 *
 *      Given pointers to the first and last RMC XMT descriptors,
 *      transfer ownerships of those descriptors to the adapter
 *      and tell it to start transmitting those descriptors.
 *
 * Results:
 *	FAILURE if something went wrong.
 *
 * Side effects:
 *	The adapter will start transmitting.
 *
 *----------------------------------------------------------------------
 */
static void
CastOffPacket(statePtr, firstDescPtr, lastDescPtr)
    register NetDFState               *statePtr;
    register volatile NetDFRmcXmtDesc *firstDescPtr;
    register volatile NetDFRmcXmtDesc *lastDescPtr;
{
    register volatile NetDFRmcXmtDesc *descPtr;
    unsigned long                     own;

    descPtr = lastDescPtr;
    /*
     * Give the buffers used to the adapter.  Avoid race conditions
     * by starting with the last RMC XMT buffer and ending with
     * the first.
     */
    while (TRUE) {
	descPtr->own = NET_DF_RMC_ADAPTER_OWN;
	if (descPtr == firstDescPtr) {
	    break;
	}
	descPtr = PREV_RMC_SEND(descPtr);
    }
    /*
     * Due to pipelineing in the adapter, we must read the own bit back
     * on the first descriptor to ensure that it was placed there.
     */
    own = descPtr->own;
    if ((own & NET_DF_RMC_OWN) == NET_DF_RMC_HOST_OWN) {
	MAKE_NOTE("host owns RMC XMT descriptor!");
    }
    /*
     * Give the adapter a little kick.
     */
    *(statePtr->regCtrlA) |= NET_DF_CTRLA_XMT_POLL_DEMAND;
    Mach_EmptyWriteBuffer();
    statePtr->stats.packetsQueued++;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFXmitInit --
 *
 *	Initialize the transmission queue structures.  This includes setting
 *	up the transmission ring buffers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The transmission ring is initialized.
 *
 *----------------------------------------------------------------------
 */
void
NetDFXmitInit(statePtr)
    register NetDFState		*statePtr; 	/* State of the interface. */
{
    if (!statePtr->xmitMemAllocated) {
	/*
	 * SMT XMT ring
	 */
	statePtr->smtXmtFirstDescPtr = (NetDFSmtXmtDesc *)
	    (statePtr->initComPtr->smtXmtBase + statePtr->slotAddr);
	/*
	 * RMC XMT ring
	 */
	statePtr->rmcXmtFirstDescPtr = (NetDFRmcXmtDesc *)
	    (statePtr->initComPtr->rmcXmtBase + statePtr->slotAddr);
	statePtr->xmitMemAllocated = TRUE;
    }
    /*
     * Reset the SMT and RMC XMT ring pointers.
     */
    statePtr->smtXmtNextDescPtr = statePtr->smtXmtFirstDescPtr;
    statePtr->smtXmtLastDescPtr = statePtr->smtXmtFirstDescPtr +
	(statePtr->initComPtr->smtXmtEntries - 1);
    statePtr->rmcXmtNextDescPtr = statePtr->rmcXmtFirstDescPtr;
    statePtr->rmcXmtLastDescPtr = statePtr->rmcXmtFirstDescPtr +
	(statePtr->initComPtr->rmcXmtEntries - 1);

    statePtr->xmitMemInitialized = TRUE;

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFOutputSmtPacket --
 *
 *      Transfer an SMT packet from the SMT XMT ring and place it
 *      onto the RMC XMT ring.  
 *
 * Results:
 *	FAILURE if something went wrong.
 *
 * Side effects:
 *	Transmit ring is modified to contain the SMT packet.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFOutputSmtPacket(statePtr, smtDescPtr)
    register NetDFState               *statePtr;
    register volatile NetDFSmtXmtDesc *smtDescPtr;
{
    register volatile NetDFRmcXmtDesc *descPtr;
    register volatile NetDFRmcXmtBuf  *rmcBufPtr;
    register unsigned char            *smtBufPtr;
    register int                      length;

    descPtr = statePtr->rmcXmtNextDescPtr;

    /*
     * Do some sanity checks. Why the polarity of the own bits on the
     * two buffers are different, I don't know.
     */
    if ((descPtr->own & NET_DF_RMC_OWN) == NET_DF_RMC_ADAPTER_OWN) {
	printf("DEC FDDI: RMC transmit buffer owned by adapter in SMT XMT.\n");
	return (FAILURE);
    }
    if ((smtDescPtr->own & NET_DF_OWN) == NET_DF_ADAPTER_OWN) {
	printf("DEC FDDI: SMT transmit buffer owned by adapter in SMT XMT.\n");
	return (FAILURE);
    }

    statePtr->transmitting = TRUE;
    statePtr->hostTransmit = FALSE;
    statePtr->curScatGathPtr = (Net_ScatterGather *)NIL;
    statePtr->curSmtXmtDescPtr = smtDescPtr;
    rmcBufPtr = RmcXmtBufFromDesc(statePtr, descPtr);
    smtBufPtr = (unsigned char *)
	(statePtr->slotAddr + (unsigned long)(smtDescPtr->bufAddr));
    length = smtDescPtr->pbc;

    /*
     * Transfer the data from the SMT XMT ring to the RMC XMT ring.
     */
    descPtr->rmcXmtDesc = NET_DF_RMC_XMT_FIRST_PAGE | (unsigned long)length;
    while (length > 0) {
	if (length < NET_DF_RMC_XMT_BUF_SIZE) {
	    NetDFBcopy(smtBufPtr, rmcBufPtr, length);
	    break;
	} else {
	    NetDFBcopy(smtBufPtr, rmcBufPtr, NET_DF_RMC_XMT_BUF_SIZE);
	    length -= NET_DF_RMC_XMT_BUF_SIZE;
	    smtBufPtr += NET_DF_RMC_XMT_BUF_SIZE;
	    descPtr = NEXT_RMC_SEND(descPtr);
	    descPtr->rmcXmtDesc = NET_DF_RMC_XMT_MIDDLE_PAGE;
	    rmcBufPtr = RmcXmtBufFromDesc(statePtr, descPtr);
	}
    }
    descPtr->rmcXmtDesc |= NET_DF_RMC_XMT_LAST_PAGE;

    MAKE_NOTE("begin transmitting SMT packet.");
    CastOffPacket(statePtr, statePtr->rmcXmtNextDescPtr, descPtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * OutputPacket --
 *
 *	Assemble and output the packet in the given scatter/gather element.
 *	The FDDI header contains the address of the destination host
 *	and the higher level protocol type already.
 *
 * Results:
 *	FAILURE if something went wrong.
 *
 * Side effects:
 *	Transmit command list is modified to contain the packet.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
OutputPacket(fddiHdrPtr, scatterGatherPtr, scatterGatherLength, statePtr)
    Net_FDDIHdr		       *fddiHdrPtr;       /* FDDI header of packet */
    register Net_ScatterGather *scatterGatherPtr; /* Data portion of packet */
    int			       scatterGatherLength; /* Length of data portion 
						     * gather array. */
    register NetDFState	       *statePtr;	    /* The interface state. */
{
    register volatile NetDFRmcXmtDesc	*descPtr;
    register volatile unsigned char	*descBufPtr;
    register Address			bufPtr;
    register int			bufCount;
    register int			length;
    register int                        remaining;
    int					totalLength;
    int					i, n, l;

    descPtr = statePtr->rmcXmtNextDescPtr;
    /*
     * Do some sanity checks.
     */
    if ((descPtr->own & NET_DF_RMC_OWN) == NET_DF_RMC_ADAPTER_OWN) {
	printf("DEC FDDI: Transmit buffer owned by adapter.\n");
	return (FAILURE);
    }

    statePtr->transmitting = TRUE;
    statePtr->hostTransmit = TRUE;
    statePtr->curScatGathPtr = scatterGatherPtr;
    /*
     * Total up the size of the packet.  Do a sanity check to make sure
     * that the packet is not too large, and then make sure that we 
     * own enough RMC XMT descriptors to dump the packet into.
     */
    totalLength = NET_DF_RMC_XMT_BUF_HDR_SIZE;
    for (i = 0; i < scatterGatherLength; i++) {
	totalLength += scatterGatherPtr[i].length;
    } 
    if (totalLength > NET_DF_MAX_PACKET_SIZE) {
	printf("DEC FDDI: OutputPacket: packet too large (%d)\n", totalLength);
	return FAILURE;
    }
    i = (totalLength / NET_DF_RMC_XMT_BUF_SIZE);
    n = statePtr->rmcXmtLastDescPtr - statePtr->rmcXmtFirstDescPtr;
    l = descPtr - statePtr->rmcXmtFirstDescPtr;
    descPtr = statePtr->rmcXmtFirstDescPtr + ((i + l) % (n + 1));
    if ((descPtr->own & NET_DF_RMC_OWN) == NET_DF_RMC_ADAPTER_OWN) {
	printf("DEC FDDI: Last transmit buffer owned by adapter.\n");
	return (FAILURE);
    }
    /*
     * Set up the header.
     */
    descPtr = statePtr->rmcXmtNextDescPtr;
    descBufPtr = (unsigned char *)RmcXmtBufFromDesc(statePtr, descPtr);
    fddiHdrPtr->prh[0] = NET_DF_PRH0;
    fddiHdrPtr->prh[1] = NET_DF_PRH1;
    fddiHdrPtr->prh[2] = NET_DF_PRH2;
    NET_FDDI_ADDR_COPY(statePtr->fddiAddress, fddiHdrPtr->source);
    NetDFBcopy(fddiHdrPtr, descBufPtr, sizeof(Net_FDDIHdr));
    descBufPtr += sizeof(Net_FDDIHdr);

    /*
     * Transfer the scatter/gather array to the RMC XMT ring.
     */
    descPtr->rmcXmtDesc = NET_DF_RMC_XMT_FIRST_PAGE | totalLength;
    remaining = NET_DF_RMC_XMT_BUF_SIZE - sizeof(Net_FDDIHdr);

    for (bufCount = 0; bufCount < scatterGatherLength; 
	 bufCount++, scatterGatherPtr++ ) {
	/*
	 * If it is an empty buffer then skip it.
	 */
	length = scatterGatherPtr->length;
	if (length == 0) {
	    continue;
	}
	bufPtr = scatterGatherPtr->bufAddr;
	
	/*
	 * When the while loop exits, the current RMC XMT buffer will have
	 * enough space left in it to place what is left of the scatter 
	 * gather array.
	 */
	while (length > remaining) {
	    /*
	     * Top off the current RMC XMT buffer.
	     */
	    if (remaining > 0) {
		NetDFBcopy(bufPtr, descBufPtr, remaining);
		bufPtr += remaining;
		length -= remaining;
	    }
	    /*
	     * Obtain and initialize the next RMC XMT buffer.
	     */
	    descPtr = NEXT_RMC_SEND(descPtr);
	    if ((descPtr->own & NET_DF_RMC_OWN) == 
		NET_DF_RMC_ADAPTER_OWN) {
		printf("DEC FDDI: Transmit buffer owned by adapter.\n");
		return (FAILURE);
	    }
	    descPtr->rmcXmtDesc = NET_DF_RMC_XMT_MIDDLE_PAGE;
	    descBufPtr = (unsigned char *)RmcXmtBufFromDesc(statePtr, descPtr);
	    remaining = NET_DF_RMC_XMT_BUF_SIZE;
	}
	/*
	 * length <= remaining
	 */
	NetDFBcopy(bufPtr, descBufPtr, length);
	remaining -= length;
	descBufPtr += length;
	/*
	 * We've used up the current scatter gather.  Get the next.
	 * If remaining == 0, then the above while loop will initialize
	 * another RMC XMT buffer and descriptor
	 */
    }
    /*
     * Upon exit of the for-loop, descPtr points to the last RMC XMT
     * buffer used.  Change the descriptor to be an EOP descriptor.
     */
    descPtr->rmcXmtDesc |= NET_DF_RMC_XMT_LAST_PAGE;
    CastOffPacket(statePtr, statePtr->rmcXmtNextDescPtr, descPtr);
    MAKE_NOTE("finished casting off HOST packet.");
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFXmitDone --
 *
 *	This routine will process a completed transmit command.  It will
 *	check for errors and update the transmission ring pointers.
 *
 * Results:
 *	FAILURE if problem is found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFXmitDone(statePtr)
    register NetDFState		        *statePtr; /* State of the interface */
{
    register volatile NetDFRmcXmtDesc   *descPtr;
    register NetDFXmtElement  	        *xmitElementPtr;
    register Net_FDDIStats              *stats;
    register int                        size;
    ReturnStatus			status;

    descPtr = statePtr->rmcXmtNextDescPtr;
    stats = &statePtr->stats;

    /*
     * If there is nothing that is currently being sent then something is
     * wrong.
     */
    if ((statePtr->hostTransmit == TRUE) && 
	(statePtr->curScatGathPtr == (Net_ScatterGather *) NIL)) {
	printf( "DEC FDDI: NetDFXmitDone: No current packet\n.");
	status = FAILURE;
	goto exit;
    }

    /*
     * Check for errors.
     */
    if ((descPtr->own & NET_DF_RMC_OWN) == NET_DF_RMC_ADAPTER_OWN) {
	printf("DEC FDDI: Bogus XMT interrupt. Buffer owned by adapter.\n");
	status = FAILURE;
	goto exit;
    }
    if (!(descPtr->rmcXmtDesc & NET_DF_RMC_XMT_SOP)) {
	printf("DEC FDDI: Bogus XMT interrupt. Buffer not start of packet\n");
	status = FAILURE;
	goto exit;
    }
    if ((descPtr->rmcXmtDesc & NET_DF_RMC_XMT_DCC) != NET_DF_DCC_SUCCESS) {
	printf("DEC FDDI: Unsuccessful completion code: 0x%x",
	       descPtr->rmcXmtDesc & NET_DF_RMC_XMT_DCC);
	return (FAILURE);
    }

    /*
     * Traverse the packet.
     */
    while (TRUE) {
	if (descPtr->rmcXmtDesc & NET_DF_RMC_XMT_EOP) {
	    break;
	}
	descPtr = NEXT_RMC_SEND(descPtr);
	if (descPtr == statePtr->rmcXmtNextDescPtr) {
	    panic("DEC FDDI: Transmit ring with no end of packet.\n");
	}
	if ((descPtr->rmcXmtDesc & NET_DF_RMC_OWN) ==
	    NET_DF_RMC_ADAPTER_OWN) {
	    printf("DEC FDDI: XMT done: buffer owned by adapter.\n");
	    status = FAILURE;
	    goto exit;
	}
    }
    /*
     * Collect some stats.
     */
    stats->packetsSent++;
    size = statePtr->rmcXmtNextDescPtr->rmcXmtDesc & NET_DF_RMC_XMT_PBC;
    stats->bytesSent += size;
    stats->transmitHistogram[size >> NET_FDDI_STATS_HISTO_SHIFT]++;

    /*
     * Update the ring pointer to point at the next buffer to use.
     */
    statePtr->rmcXmtNextDescPtr = NEXT_RMC_SEND(descPtr);

    if (statePtr->hostTransmit == TRUE) {
	/*
	 * Mark the packet as done.
	 */
	MAKE_NOTE("finished transmitting HOST packet.");
	statePtr->curScatGathPtr->done = TRUE;
	if (statePtr->curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	    MAKE_NOTE("waking up process waiting for transmit to finish.");
	    NetOutputWakeup(statePtr->curScatGathPtr->mutexPtr);
	}
    } else {
	MAKE_NOTE("finished transmitting SMT packet.");
    }
    /*
     * If there are more packets to send then send the first one on
     * the queue.  Otherwise there is nothing being transmitted.
     */
    status = SUCCESS;
    if (statePtr->resetPending == TRUE) {
	goto exit;
    }
	
    if (!List_IsEmpty(statePtr->xmitList)) {
	xmitElementPtr = (NetDFXmtElement *) List_First(statePtr->xmitList);
	status = OutputXmtElement(statePtr, xmitElementPtr);
    } else {
	statePtr->transmitting = FALSE;
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
exit:
    /*
     * This assumes that whatever calls us will reset the chip if we return
     * anything other than SUCCESS.  This way we avoid resetting the chip 
     * twice in a row.
     */
    if ((statePtr->resetPending == TRUE) && (status == SUCCESS)) {
	statePtr->transmitting = FALSE;
	NetDFRestart(statePtr->interPtr);
    }
    if (status != SUCCESS) {
	statePtr->transmitting = FALSE;
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFOutput --
 *
 *	Output a packet.  The procedure is to either put the packet onto the 
 *	queue of outgoing packets if packets are already being sent, or 
 *	otherwise to send the packet directly.
 *
 * Results:
 *	SUCCESS if the packet was sent or queued successfully, otherwise
 *	FAILURE.
 *
 * Side effects:
 *	Queue of packets modified.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
NetDFOutput(interPtr, hdrPtr, scatGathPtr, scatGathLength, rpc, statusPtr)
    Net_Interface	       *interPtr;      /* The network interface. */
    Address		       hdrPtr;	       /* Packet header. */
    register Net_ScatterGather *scatGathPtr;   /* Data portion of packet. */
    register int               scatGathLength; /* Length of data portion 
						* gather array. */
    Boolean		       rpc;	       /* An RPC packet? */
    ReturnStatus	       *statusPtr;     /* Status from sending
						* packet.*/
{
    register NetDFState			*statePtr;
    register NetDFXmtElement		*xmitPtr;
    Net_FDDIHdr			        *fddiHdrPtr = (Net_FDDIHdr *)hdrPtr;
    Boolean				restart = FALSE;
    ReturnStatus			status;

    statePtr = (NetDFState *) interPtr->interfaceData;

    MASTER_LOCK(&interPtr->mutex);

    if (statePtr->flags & NET_DF_FLAGS_RESETTING) {
	MAKE_NOTE("Process waiting in NetDFOutput while resetting.\n");
	do {
	    Sync_MasterWait(&statePtr->doingReset, &interPtr->mutex, FALSE);
	} while (statePtr->flags & NET_DF_FLAGS_RESETTING);
    }

    /*
     * See if the packet is for us.  In this case just copy in the packet
     * and call the higher level routine.
     */
    if (!Net_FDDIAddrCmp(statePtr->fddiAddress, fddiHdrPtr->dest)) {
	int i, length;

        length = sizeof(Net_FDDIHdr);
        for (i = 0; i < scatGathLength; i++) {
            length += scatGathPtr[i].length;
        }
        if (length <= NET_DF_MAX_PACKET_SIZE) {
	    register Address bufPtr;

	    MAKE_NOTE("--- Loopback buffer used ---");

	    NET_FDDI_ADDR_COPY(statePtr->fddiAddress, fddiHdrPtr->source);

	    bufPtr = (Address)statePtr->loopBackBuffer;
	    bcopy((Address)fddiHdrPtr, bufPtr, sizeof(Net_FDDIHdr));
	    bufPtr += sizeof(Net_FDDIHdr);
            Net_GatherCopy(scatGathPtr, scatGathLength, bufPtr);

	    Net_Input(interPtr, (Address)statePtr->loopBackBuffer, length);
        }

        scatGathPtr->done = TRUE;

	status = SUCCESS;
	if (statusPtr != (ReturnStatus *) NIL) {
	    *statusPtr = SUCCESS;
	} 
	goto exit;
    }

    /*
     * If no packet is being sent then go ahead and send this one.
     */
    if (!statePtr->transmitting) {
	status = OutputPacket(fddiHdrPtr, scatGathPtr, 
			      scatGathLength, statePtr);
	if (status != SUCCESS) {
	    restart = TRUE;
	} else if (statusPtr != (ReturnStatus *) NIL) {
	    *statusPtr = SUCCESS;
	} 
	goto exit;
    }

    /*
     * There is a packet being sent so this packet has to be put onto the
     * transmission queue.  Get an element off of the transmission free list.  
     * If none available then drop the packet.
     */
    if (List_IsEmpty(statePtr->xmitFreeList)) {
	statePtr->stats.xmtPacketsDropped++;
        scatGathPtr->done = TRUE;
	status = FAILURE;
	goto exit;
    }

    xmitPtr = (NetDFXmtElement *) 
	List_First((List_Links *) statePtr->xmitFreeList);
    List_Remove((List_Links *) xmitPtr);

    /*
     * Initialize the list element and place on the queue.
     */
    xmitPtr->fddiHdrPtr = fddiHdrPtr;
    xmitPtr->scatterGatherPtr = scatGathPtr;
    xmitPtr->scatterGatherLength = scatGathLength;
    xmitPtr->xmtType = NET_DF_XMT_HOST;
    List_Insert((List_Links *) xmitPtr, LIST_ATREAR(statePtr->xmitList)); 

    if (statusPtr != (ReturnStatus *) NIL) {
	*statusPtr = SUCCESS;
    } 
    status = SUCCESS;
exit:
    MASTER_UNLOCK(&interPtr->mutex);
    if (restart) {
	Net_DFRestart(interPtr);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFSmtOutput --
 *
 *	Transfer a packet from the SMT XMT ring to the RMC XMT ring.
 *      If a packet is currently being sent, then the SMT packet is
 *      placed on the transmit queue;  otherwise, it is sent directly.
 *
 * Results:
 *	SUCCESS if the packet was sent or queued successfully, FAILURE
 *	otherwise.
 *
 * Side effects:
 *	Queue of packets modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFSmtOutput(interPtr)
    Net_Interface *interPtr;
{
    register NetDFState      *statePtr;
    register NetDFXmtElement *xmitPtr;
    ReturnStatus	     status;
    Boolean		     restart = FALSE;

    statePtr = (NetDFState *)interPtr->interfaceData;

    /*
     * If no packet is being sent then go ahead and send this one.
     */
    if (!statePtr->transmitting) {
	status = NetDFOutputSmtPacket(statePtr, statePtr->smtXmtNextDescPtr);
	if (status != SUCCESS) {
	    restart = TRUE;
	}
	goto exit;
    }
    /*
     * There is a packet being sent so this packet has to be put onto the
     * transmission queue.  Get an element off of the transmission free list.  
     * If none available then drop the packet.
     */
    if (List_IsEmpty(statePtr->xmitFreeList)) {
	statePtr->stats.xmtPacketsDropped++;
	status = FAILURE;
	goto exit;
    }

    xmitPtr = (NetDFXmtElement *) 
	List_First((List_Links *) statePtr->xmitFreeList);
    List_Remove((List_Links *) xmitPtr);

    /*
     * Initialize the list element.
     */
    xmitPtr->fddiHdrPtr = NULL;
    xmitPtr->scatterGatherPtr = NULL;
    xmitPtr->scatterGatherLength = 0;
    xmitPtr->smtDescPtr = statePtr->smtXmtNextDescPtr;
    xmitPtr->xmtType = NET_DF_XMT_SMT;

    statePtr->smtXmtNextDescPtr = NEXT_SMT_SEND(statePtr->smtXmtNextDescPtr);
    /* 
     * Put onto the transmission queue.
     */
    List_Insert((List_Links *) xmitPtr, LIST_ATREAR(statePtr->xmitList)); 
    status = SUCCESS;
exit:
    if (restart) {
	NetDFRestart(interPtr);
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFXmitDrop --
 *
 *	Drop the current packet.  Called at the beginning of the
 *	restart sequence, before curScatGathPtr is reset to NIL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Current scatter gather pointer is reset and processes waiting
 *	for synchronous output are notified.
 *
 *----------------------------------------------------------------------
 */
void
NetDFXmitDrop(statePtr)
    register NetDFState *statePtr; 	/* State of the interface. */
{
    MAKE_NOTE("Dropping current packet.");
    if (statePtr->curScatGathPtr != (Net_ScatterGather *) NIL) {
	statePtr->curScatGathPtr->done = TRUE;
	if (statePtr->curScatGathPtr->mutexPtr != (Sync_Semaphore *) NIL) {
	    NetOutputWakeup(statePtr->curScatGathPtr->mutexPtr);
	}
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    statePtr->transmitting = FALSE;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFXmitRestart --
 *
 *	Restart transmission of packets at the end of the restart
 *	sequence, after a chip reset.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output queue started up.
 *
 *----------------------------------------------------------------------
 */
void
NetDFXmitRestart(statePtr)
    register NetDFState		*statePtr; 	/* State of the interface. */
{
    register NetDFXmtElement	*xmitElementPtr;
    ReturnStatus	        status;

    /*
     * Start output if there are any packets queued up.
     */
    if (!List_IsEmpty(statePtr->xmitList)) {
	xmitElementPtr = (NetDFXmtElement *) List_First(statePtr->xmitList);
	status = OutputXmtElement(statePtr, xmitElementPtr);
	List_Move((List_Links *) xmitElementPtr, 
		  LIST_ATREAR(statePtr->xmitFreeList));
	/*
	 * If we cannot successfully output the packet, then flush the
	 * transmit queue.
	 */
	if (status != SUCCESS) {
	    printf("DEC FDDI: Cannot output first packet on restart.\n");
	    NetDFXmitFlushQ(statePtr);
	}
    } else {
	statePtr->transmitting = FALSE;
	statePtr->curScatGathPtr = (Net_ScatterGather *) NIL;
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFXmitFlushQ --
 *
 *	Drop all packets in the transmit queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Processes waiting for synchronous output are notified.
 *
 *----------------------------------------------------------------------
 */
void
NetDFXmitFlushQ(statePtr)
    register NetDFState      *statePtr;	/* State of the interface. */
{
    register NetDFXmtElement *xmtElementPtr;

    while (TRUE) {
	if (List_IsEmpty(statePtr->xmitList)) {
	    break;
	}
	xmtElementPtr = (NetDFXmtElement *) List_First(statePtr->xmitList);
	switch(xmtElementPtr->xmtType) {
	case NET_DF_XMT_HOST:
	    xmtElementPtr->scatterGatherPtr->done = TRUE;
	    if (xmtElementPtr->scatterGatherPtr->mutexPtr != 
		(Sync_Semaphore *) NIL) {
		NetOutputWakeup(xmtElementPtr->scatterGatherPtr->mutexPtr);
	    }
	    break;
	case NET_DF_XMT_SMT:
	default:
	    break;
	}
	List_Move((List_Links *) xmtElementPtr, 
		  LIST_ATREAR(statePtr->xmitFreeList));
    }
}

