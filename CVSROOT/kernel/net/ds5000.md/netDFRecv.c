/* 
 * netDFRecv.c -
 *
 * Routines to manage the receive unit of the DEC FDDI controller 700.
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
#endif not lint

#include <sprite.h>
#include <netDFInt.h>
#include <vm.h>
#include <vmMach.h>
#include <sys.h>
#include <list.h>
#include <machMon.h>
#include <dbg.h>

/*
 * Macros to step HOST RCV ring pointers.
 */
#define	NEXT_HOST_RECV(p) ( (((p)+1) > statePtr->hostRcvLastDescPtr) ? \
				statePtr->hostRcvFirstDescPtr : \
				((p)+1))
/*
 * Macros to step SMT RCV ring pointers.
 */
#define	NEXT_SMT_RECV(p) ( (((p)+1) > statePtr->smtRcvLastDescPtr) ? \
				statePtr->smtRcvFirstDescPtr : \
				((p)+1))
#define	PREV_SMT_RECV(p) ( (((p)-1) < statePtr->smtRcvFirstDescPtr) ? \
				statePtr->smtRcvLastDescPtr : \
				((p)-1))

/*
 * Macro to assign buffer addresses to HOST RCV descriptor fields
 */
#define ASSIGN_BUF_ADDR(ptr, bufAddr) \
{ \
    register unsigned long _addr_; \
 \
    _addr_ = (unsigned long)(bufAddr); \
    _addr_ = _addr_ >> 9; \
    Bf_WordSet(((unsigned long *)ptr), 9, 23, _addr_) \
}

/*
 * Macro to test a descriptor to see if the adapter owns it.
 */
#define ADAPTER_RCV_OWN(ownWord) \
    (((unsigned long)(ownWord) & NET_DF_OWN) == NET_DF_ADAPTER_OWN)

/*
 * Macros to translate from RCV descriptors to the buffers
 * that they are associated with.
 */
#ifdef NET_DF_USE_UNCACHED_MEM
#define HostRcvBufFromDesc(statePtr, descPtr) \
  (NetDFHostRcvBuf *)(statePtr->hostRcvFirstBufPtr + \
   (unsigned long)(descPtr - statePtr->hostRcvFirstDescPtr))
#else
#define HostRcvBufFromDesc(statePtr, descPtr) \
  (NetDFHostRcvBuf *)(statePtr->hostRcvBuffers[ \
   (unsigned long)(descPtr - statePtr->hostRcvFirstDescPtr)])
#endif

#define SmtRcvBufFromDesc(statePtr, descPtr) \
  (NetDFSmtRcvBuf *)(statePtr->smtRcvFirstBufPtr + \
   (unsigned long)(descPtr - statePtr->hostRcvFirstDescPtr))

/*
 * For now use kernel initialized memory.  The buffers must start
 * on a 512-byte boundary
 */
unsigned char hostRcvBufferMem[512 +
       NET_DF_NUM_HOST_RCV_ENTRIES * NET_DF_HOST_RCV_BUF_SIZE];

/*
 *----------------------------------------------------------------------
 *
 * AllocateRecvMem --
 *
 *	Allocate kernel memory for receive ring and data buffers.	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Device state structure is updated.
 *
 *----------------------------------------------------------------------
 */
static void
AllocateRecvMem(statePtr)
    register NetDFState		*statePtr; /* The state of the interface. */
{
    register unsigned long	memBase;
    register int                i;

    /*
     * SMT RCV descriptor ring.
     */
    statePtr->smtRcvFirstDescPtr = (NetDFSmtRcvDesc *)
	(statePtr->initComPtr->smtRcvBase + statePtr->slotAddr);

    /*
     * HOST RCV descriptor ring.
     */
    statePtr->hostRcvFirstDescPtr = (NetDFHostRcvDesc *)
	(statePtr->initComPtr->hostRcvBase + statePtr->slotAddr);

#ifdef NET_DF_USE_UNCACHED_MEM

    printf("DEC FDDI: Using uncached memory for receive buffers.\n");
    /*
     * Allocate the ring of receive buffer descriptors.  The ring must start
     * on 512-byte boundary.  For now, use kernel initialized memory
     * so that we don't have to worry about the effects of DMA.
     */
    memBase = (unsigned long) (&hostRcvBufferMem[0]);
    /*
     * Ensure that the ring starts on the 512-byte boundary.
     */
    if (memBase & 0x1FF) {
	memBase = (memBase + 512) & ~0x1FF;
    }
    /*
     * Use uncached unmapped addresses
     */
    memBase = (unsigned long)MACH_UNCACHED_ADDR(memBase);
    statePtr->hostRcvFirstBufPtr = (volatile NetDFHostRcvBuf *) memBase;

#else /* NET_DF_USE_UNCACHED_MEM */

    printf("DEC FDDI: Using cached memory for receive buffers.\n");
    for (i = 0; i < NET_DF_NUM_HOST_RCV_ENTRIES; i++) {
	/*
	 * Ensure that the ring starts on the 512-byte boundary.
	 */
	memBase = (unsigned long) malloc(NET_DF_HOST_RCV_BUF_SIZE + 512);
	if (memBase & 0x1FF) {
	    memBase = (memBase + 512) & ~0x1FF;
	}
	memBase = memBase - MACH_KSEG2_ADDR + MACH_CACHED_MEMORY_ADDR;
	statePtr->hostRcvBuffers[i] = (volatile NetDFHostRcvBuf *) memBase;
    }
    statePtr->hostRcvFirstBufPtr = statePtr->hostRcvBuffers[0];

#endif /* NET_DF_USE_UNCACHED_MEM */

    statePtr->recvMemAllocated = TRUE;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFRecvInit --
 *
 *	Initialize the receive buffer lists for the receive unit allocating
 *	memory if need.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The receive ring is initialized and the device state structure is
 *	updated.
 *
 *----------------------------------------------------------------------
 */

void
NetDFRecvInit(statePtr)
    register NetDFState		       *statePtr;  /* Interface state. */
{
    register volatile NetDFHostRcvDesc *descPtr; 
    register volatile NetDFHostRcvBuf  *bufPtr;
    register int 	               bufNum;

    if (!statePtr->recvMemAllocated) {
	AllocateRecvMem(statePtr);
    }
    /*
     * Reset the HOST and SMT RCV ring pointers.
     */
    statePtr->hostRcvNextDescPtr = statePtr->hostRcvFirstDescPtr;
    statePtr->hostRcvLastDescPtr = statePtr->hostRcvFirstDescPtr +
	(NET_DF_NUM_HOST_RCV_ENTRIES - 1);

#ifdef NET_DF_USE_UNCACHED_MEM
    statePtr->hostRcvNextBufPtr = statePtr->hostRcvFirstBufPtr;
    statePtr->hostRcvLastBufPtr = statePtr->hostRcvFirstBufPtr +
	(NET_DF_NUM_HOST_RCV_ENTRIES - 1);	
#else
    statePtr->hostRcvNextBufPtr = statePtr->hostRcvBuffers[0];
    statePtr->hostRcvBufIndex = 0;
    statePtr->hostRcvLastBufPtr = 
	statePtr->hostRcvBuffers[NET_DF_NUM_HOST_RCV_ENTRIES - 1];
#endif

    statePtr->smtRcvNextDescPtr = statePtr->smtRcvFirstDescPtr;
    statePtr->smtRcvLastDescPtr = statePtr->smtRcvFirstDescPtr +
	(statePtr->initComPtr->smtRcvEntries - 1);
    /* 
     * Initialize the Host RCV ring descriptors.
     */
    descPtr = statePtr->hostRcvFirstDescPtr;
    for (bufNum = 0; bufNum < NET_DF_NUM_HOST_RCV_ENTRIES; 
	 bufNum++, descPtr++) { 
	/*
	 * Point the descriptor at its buffer, and give it to the adapter.
	 */
#ifdef NET_DF_USE_UNCACHED_MEM
	bufPtr = statePtr->hostRcvFirstBufPtr + bufNum;
#else
	bufPtr = statePtr->hostRcvBuffers[bufNum];
#endif	
	ASSIGN_BUF_ADDR(&descPtr->bufAPtr, bufPtr->bufA);
	ASSIGN_BUF_ADDR(&descPtr->bufBPtr, bufPtr->bufB);
	descPtr->rmcRcvDesc = 0x0;
	Bf_WordSet(((unsigned long *)&(descPtr->bufAPtr)), 0, 1, 0x0);
    }
    /*
     * Initialize the SMT RCV ring descriptors.  Turns out that the
     * descriptors already point to their buffers.
     */
    statePtr->lastRecvCnt = 0;
    statePtr->recvMemInitialized = TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * HostRcvToSmtRcv --
 *
 *	Transfer an SMT packet from the HOST RCV ring to the SMT RCV ring.
 *
 * Results:
 *	SUCCESS if the transfer was successful, and FAILURE otherwise.
 *
 * Side effects:
 *	A packet is written to the SMT RCV ring on the adapter.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
HostRcvToSmtRcv(statePtr, hostDescPtr)
    register NetDFState               *statePtr;
    register NetDFHostRcvDesc         *hostDescPtr;
{
    register volatile NetDFSmtRcvDesc *smtDescPtr;
    register volatile NetDFSmtRcvBuf  *smtBufPtr;
    register NetDFHostRcvBuf          *hostBufPtr;
    register unsigned long            length;

    smtDescPtr = statePtr->smtRcvNextDescPtr;
    /*
     * Do some sanity checks.
     */
    if (ADAPTER_RCV_OWN(smtDescPtr->own)) {
	printf("DEC FDDI: SMT RCV descriptor owned by adapter.\n");
	return (FAILURE);
    }
    smtBufPtr = (NetDFSmtRcvBuf *)
	(statePtr->slotAddr + (unsigned long)(smtDescPtr->bufAddr));
    hostBufPtr = HostRcvBufFromDesc(statePtr, hostDescPtr);
    /*
     * Transfer the data.  Even though the host buffers are split
     * into two, we allocated them contiguously so we can just
     * copy them in one fell swoop.
     */
    smtDescPtr->rmcRcvDesc = hostDescPtr->rmcRcvDesc;
    length = hostDescPtr->rmcRcvDesc & NET_DF_RMC_RCV_PBC;
    /*
     * According to the description of the RMC RCV Descriptor,
     * the PBC does not include the three Packet Request Header (PRH)
     * bytes found in the first word of the buffer.  So we must
     * take those into account while copying from the HOST RCV
     * buffer to the SMT RCV buffer.
     */
    length += 3;
    NetDFBcopy((char *)hostBufPtr, (char *)smtBufPtr, length);
    /*
     * Give the SMT RCV descriptor and buffer to the adapter, and
     * poke the magic bit.
     */
    smtDescPtr->own = NET_DF_ADAPTER_OWN;
    *(statePtr->regCtrlA) |= NET_DF_CTRLA_SMT_RCV_POLL_DEMAND;
    Mach_EmptyWriteBuffer();
    
    statePtr->smtRcvNextDescPtr = NEXT_SMT_RECV(smtDescPtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * GotAPacket --
 *
 *	Determine what to do with the given packet:  if the packet
 *      is an SMT packet, then transfer it to the SMT RCV ring;
 *      if the packet is a host LLC packet, give it to the
 *      proper authorities.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A packet may be written to the SMT RCV ring on the adapter.
 *
 *----------------------------------------------------------------------
 */
static void
GotAPacket(statePtr, descPtr, bufPtr)
    register NetDFState       *statePtr;
    register NetDFHostRcvDesc *descPtr;
    NetDFHostRcvBuf           *bufPtr;
{
    register unsigned char    *dataPtr;
    register int              size;
    register ReturnStatus     result;

    dataPtr = (unsigned char *)bufPtr;

    if (dataPtr[3] == NET_DF_FRAME_SMT ||
	dataPtr[3] == NET_DF_FRAME_SMT_INFO ||
	dataPtr[3] == NET_DF_FRAME_SMT_NSA) {

	MAKE_NOTE("Received an SMT packet.");
	result = HostRcvToSmtRcv(statePtr, descPtr);
	if (result != SUCCESS) {
	    printf("DEC FDDI: Failed to transfer from HOST to SMT RCV.\n");
	} else {	
	    MAKE_NOTE("processed SMT RCV packet successfully.");
	    DFprintf("DEC FDDI: Transfer from HOST to SMT RCV succeeded.\n");
	}
	return;
    }
    if (dataPtr[3] != NET_FDDI_SPRITE) {
	/*
	 * This is not a Sprite FDDI packet, so just drop it on the floor.
	 */
	return;
    }
    size = (int)(descPtr->rmcRcvDesc & NET_DF_RMC_RCV_PBC);
    /*
     * The size reported is not entirely correct.  The RMC RCV descriptor
     * determines size by counting the 1 Frame Control byte, 4 CRC bytes,
     * and the data bytes, ignoring the 3 Packet Request Header bytes. 
     * We determine size by counting the Frame Control, PRH, and data bytes,
     * ignoring the CRC bytes.  To adjust accordingly, we subtract one from
     * the reported length.
     */
    size--;
    MAKE_NOTE("received a HOST packet.");
    Net_Input(statePtr->interPtr, dataPtr, size);
}

/*
 *----------------------------------------------------------------------
 *
 * CheckRmcRcvDesc --
 *
 *      Check the RMC RCV Descriptor of the given packet for
 *      various transmission errors.
 *
 * Results:
 *	SUCCESS if the packet is `OK', and FAILURE otherwise.
 *
 * Side effects:
 *	If the packet has a problem, that problem is printed.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
CheckRmcRcvDesc(statePtr, descPtr)
    NetDFState             *statePtr; /* State of the interface */
    NetDFHostRcvDesc       *descPtr;  /* Descriptor pointing to packet. */
{
    register unsigned long bad;
    register unsigned long pbc;
    register unsigned long crc;
    register unsigned long rrr;
    register unsigned long dd;
    register unsigned long ss;
    register unsigned long fsc;
    register unsigned long fsb;
    register unsigned char frameControl;
    NetDFHostRcvBuf        *bufPtr;
    
    bad = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_BAD;
    pbc = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_PBC;

    if (bad) {
	/*
	 * We've got ourselves here what they call a `bad' little packet.
	 */
	fsc = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_FSC;
	fsb = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_FSB;
	crc = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_CRC;
	rrr = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_RRR;
	dd = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_DD;
	ss = descPtr->rmcRcvDesc & NET_DF_RMC_RCV_SS;
	if (crc && rrr == NET_DF_RRR_DADDR_UNMATCH && dd == NET_DF_DD_CAM &&
	    ss == NET_DF_SS_ALIAS) {
	    if (pbc == 8190 || pbc == 8191) {
		printf("DEC FDDI: Received frame that was too long.\n");
	    } else {
		printf("DEC FDDI: Fifo overflow with an interrupt.\n");
	    }
	} else if (crc && rrr == NET_DF_RRR_DADDR_UNMATCH &&
		   dd == NET_DF_DD_CAM && ss == NET_DF_SS_CAM) {
	    printf("DEC FDDI: RMC/MAC interface error.\n");
	} else if (rrr == NET_DF_RRR_SADDR_MATCH ||
		   rrr == NET_DF_RRR_DADDR_UNMATCH ||
		   rrr == NET_DF_RRR_RMC_ABORT) {
	    printf("DEC FDDI: Nasty hardware problem.\n");
	} else if (rrr == NET_DF_RRR_INV_LENGTH) {
	    printf("DEC FDDI: Large frame with odd # of symbols.\n");
	} else if (rrr == NET_DF_RRR_FRAGMENT) {
	    printf("DEC FDDI: Fragment error.\n");
	} else if (rrr == NET_DF_RRR_FORMAT_ERROR) {
	    printf("DEC FDDI: Format error.\n");
	} else if (rrr == NET_DF_RRR_MAC_RESET) {
	    printf("DEC FDDI: MAC reset error.\n");
	} else {
	    if (crc) {
		printf("DEC FDDI: Received packet with CRC error.\n");
	    } else if (!fsc && fsb) {
		printf("DEC FDDI: Frame status error.\n");
	    } else {
		printf("DEC FDDI: Large secondary NSA frame.\n");
	    }
	}
	return FAILURE;
    } else {
	bufPtr = HostRcvBufFromDesc(statePtr, descPtr);
	frameControl = ((unsigned char *)bufPtr)[3];
	switch(frameControl) {
	case NET_DF_FRAME_SMT_INFO:
	case NET_DF_FRAME_SMT_NSA:
	case NET_DF_FRAME_SMT:
	    if (pbc < NET_DF_MIN_SMT_PACKET_SIZE ||
		pbc > NET_DF_MAX_PACKET_SIZE) {
		printf("DEC FDDI: SMT packet too long or too short ");
		printf("(length = %d)\n", pbc);
		return FAILURE;
	    }
	    break;
	case NET_DF_FRAME_LLC_ASYNC:
	case NET_DF_FRAME_LLC_SYNC:
	case NET_DF_FRAME_HOST_LLC:
	case NET_FDDI_SPRITE:
	    /*
	     * We subtract one because the PBC is slightly off for our
	     * interpretations.  See the note in GotAPacket.
	     */
	    if (pbc < NET_DF_MIN_LLC_PACKET_SIZE ||
		((pbc - 1) > NET_DF_MAX_PACKET_SIZE)) {
		printf("DEC FDDI: LLC packet too long or too short ");
		printf("(length = %d)\n", pbc);
		return FAILURE;
	    }
	    break;
	case NET_DF_FRAME_MAC_BEACON:
	case NET_DF_FRAME_MAC_CLAIM:
	case NET_DF_FRAME_MAC:
	    if (pbc < NET_DF_MIN_MAC_PACKET_SIZE ||
		pbc > NET_DF_MAX_PACKET_SIZE) {
		printf("DEC FDDI: MAC packet too long or too short ");
		printf("(length = %d)\n", pbc);
		return FAILURE;
	    }
	    break;
	case NET_DF_FRAME_IMP_ASYNC:
	case NET_DF_FRAME_IMP_SYNC:
	    if (pbc < NET_DF_MIN_IMP_PACKET_SIZE ||
		pbc > NET_DF_MAX_PACKET_SIZE) {
		printf("DEC FDDI: IMP packet too long or too short ");
		printf("(length = %d)\n", pbc);
		return FAILURE;
	    }
	    break;
	case NET_DF_FRAME_NON_REST_TOK:
/*	case NET_DF_FRAME_REST_TOK:  == MAC */
	    if (pbc < NET_DF_MIN_RES_PACKET_SIZE ||
		pbc > NET_DF_MAX_PACKET_SIZE) {
		printf("DEC FDDI: RES packet too long or too short ");
		printf("(length = %d)\n", pbc);
		return FAILURE;
	    }
	    break;
	default:
	    printf("DEC FDDI: Hmmm....don't know this packet type: ");
	    printf("%x\n", frameControl);
	    break;
	}
	DFprintf("DEC FDDI: Frame Control %d\tPBC = %d\n", frameControl, pbc);
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * NetDFRecvProcess --
 *
 *	Process a newly received packet.
 *
 * Results:
 *	FAILURE if something went wrong, SUCCESS otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
NetDFRecvProcess(dropPackets, statePtr)
    Boolean		                dropPackets; /* Drop all packets. */
    register NetDFState		        *statePtr;   /* Interface state. */
{
    register volatile NetDFHostRcvDesc  *hostDescPtr;
    register volatile NetDFHostRcvBuf   *bufPtr;
    register Net_FDDIStats              *stats;
    unsigned long                       *addr1;
    unsigned long                       addr2;
    register int			size;
    Boolean				tossPacket;
    register int			numResets;
    ReturnStatus                        result;

    /*
     * If we are not initialized then forget the interrupt.
     */
    if (!statePtr->recvMemInitialized) {
	return (FAILURE);
    }

    hostDescPtr = statePtr->hostRcvNextDescPtr;
    if (ADAPTER_RCV_OWN(hostDescPtr->bufAPtr)) {
	/*
	 * I'm not quite sure why this is here.  But oh well.
	 *
	 * I think that I see why, now.  The problem is that we process
	 * multiple receive packets per receive interrupt.  Before we
	 * process the receive packets, however, we clear that interrupt.
	 * Packets may arrive, however, setting the interrupt again while we
	 * process those packets under the interrupt of an earlier packet.
	 * This leaves a hanging interrupt that signals something we've
	 * already done, and we have to check for it.
	 */
	if (statePtr->lastRecvCnt == 0) {
	    printf("DEC FDDI: Really bogus receive interrupt. ");
	    printf("Buffer 0x%x owned by the adapter.\n", hostDescPtr);
	    return (FAILURE);
	} else {
	    statePtr->lastRecvCnt = 0;
	    return (SUCCESS);
	}
    }
    if (!(hostDescPtr->rmcRcvDesc & NET_DF_RMC_RCV_SOP)) {
	printf("DEC FDDI: Really bogus receive interrupt. ");
	printf("Buffer 0x%x doesn't start packet.\n", hostDescPtr);
	return (FAILURE);
    }

    tossPacket = dropPackets;
    numResets = statePtr->numResets;
    statePtr->lastRecvCnt = 0;
    stats = &statePtr->stats;
    while (TRUE) {
	/* 
	 * Check to see if we have processed all our buffers. 
	 */
	if (ADAPTER_RCV_OWN(hostDescPtr->bufAPtr)) {
	    break;
	}
	/*
	 * Each buffer pair is large enough to hold a MAX packet,
	 * so the START_OF_PACKET should always be set.
	 */
	if (!(hostDescPtr->rmcRcvDesc & NET_DF_RMC_RCV_SOP)) {
	    printf("DEC FDDI: Really bogus receive interrupt. ");
	    printf("Buffer 0x%x doesn't start packet.\n", hostDescPtr);
	    return (FAILURE);
	}
	/*
	 * If the END_OF_PACKET bit is set, then the buffer has less than
	 * 512 bytes in it.  Only useful for statistical information.
	 */

	bufPtr = HostRcvBufFromDesc(statePtr, hostDescPtr);
#ifndef NET_DF_USE_UNCACHED_MEM    
	Mach_FlushCacheRange(bufPtr, 
	     (hostDescPtr->rmcRcvDesc & NET_DF_RMC_RCV_PBC) + 3);
#endif
	/*
	 * Check to make sure that the packet is valid:
	 * "A valid packet must be void of data corruption, must be of
	 * a certain frame type, and for each frame type must be of
	 * acceptable size." -- Need I say more?
	 */
	result = CheckRmcRcvDesc(statePtr, hostDescPtr);
	if (result != SUCCESS) {
	    tossPacket = TRUE;
	}
	/*
	 * Call a higher level protocol to process the packet.
	 */
	if (!tossPacket) {
	    stats->packetsReceived++;
	    size = hostDescPtr->rmcRcvDesc & NET_DF_RMC_RCV_PBC;
	    stats->bytesReceived += size;
	    stats->receiveHistogram[size >> NET_FDDI_STATS_HISTO_SHIFT]++;
	    GotAPacket(statePtr, hostDescPtr, bufPtr); 
	}
	/*
	 * The adapter trashes the buffer address when it gives
	 * ownership of the descriptor back to the host.  We must
	 * update it here, and give ownership to the chip.
	 */
	ASSIGN_BUF_ADDR(&hostDescPtr->bufAPtr, bufPtr->bufA)
	ASSIGN_BUF_ADDR(&hostDescPtr->bufBPtr, bufPtr->bufB);
	hostDescPtr->rmcRcvDesc = 0x0;
	Bf_WordSet((unsigned long *)&hostDescPtr->bufAPtr, 0, 1, 0x0);

	statePtr->lastRecvCnt++;
	/* 
	 * Check to see if we have processed all of our buffers. 
	 */
	hostDescPtr = NEXT_HOST_RECV(hostDescPtr);
	if (ADAPTER_RCV_OWN(hostDescPtr->bufAPtr)) {
		break;
	}
	if (hostDescPtr == statePtr->hostRcvNextDescPtr) {
	    break;
	}
    }
    stats->receiveReaped[statePtr->lastRecvCnt - 1]++;
    /*
     * Update the the ring pointer. We should be pointing at 
     * the next buffer in which the adapter will place a packet.
     */
    statePtr->hostRcvNextDescPtr = hostDescPtr;
    /*
     * return a SUCCESS.
     */
    return (SUCCESS);
}
