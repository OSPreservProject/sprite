/* 
 * ip.c --
 *
 *      Routines to handle the IP protocol. IP packets that are received
 *      from the network layer are validated and passed to the next
 *      protocol layer. If the packet is fragmented, it is reassembled
 *      before passed to the next layer. This file also contains
 *      routines to format IP header in a packet and to send the packet to
 *      the network for output. The packet may be sent in fragments if it
 *      is too large for the network.
 *
 *	The IP protocol specification used by the routines in this file
 *	is RFC 791 "Internet Protocol", Sept. 1981.
 *
 *
 *	Based on 4.3 BSD code:
 *	"@(#)ip_input.c  7.9 (Berkeley) 3/15/88"
 *	"@(#)ip_output.c 7.9 (Berkeley) 3/15/88"
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/ip.c,v 1.9 89/10/22 12:15:17 nelson Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ipServer.h"
#include "stat.h"
#include "ip.h"
#include "icmp.h"
#include "route.h"
#include "raw.h"

#include "list.h"
#include "spriteTime.h"
#include "fs.h"

/*
 * Information needed to aid in reassembly of a datagram from its fragments. 
 *
 * The fragInfoList is a list of DgramFragInfo elements with one element
 * per fragmented datagram. The fragments are kept on the dataList linked
 * list within each DgramFragInfo element.  All fragments of a datagram
 * have the same source, destination, protocol and identification fields
 * in their IP headers.  If the datagram isn't reassembled before the
 * timeToLive field goes to 0, all fragments are freed and the DgramFragInfo
 * element is removed from the fragInfoList.
 */

typedef struct {
    List_Links		links;		/* Links to next and prev. items. */

    /*
     * The following 4 fields in the IP header uniquely identify the datagram. 
     * A fragment belongs to this datagram if all 4 fields match.
     */
    Net_InetAddress	source;		/* Sender of packet. */
    Net_InetAddress	dest;		/* Destination of packet. */
    unsigned char	protocol;	/* Protocol. */
    unsigned short	ident;		/* Datagram ID. */

    int			numFrags;	/* Number of fragments. */
    int			timeToLive;	/* # of seconds before the fragments 
					 * are dropped. */
    List_Links		dataList;	/* List head of data info elements for 
					 * fragments that have arrived. */
} DgramFragInfo;

/*
 * List head of the list of DgramFragInfo elements.
 */
static List_Links	fragInfoList;


/*
 * Information about a particular fragment within a datagram.
 *
 * Each fragment of a datagram has one of these structures in the 
 * dataList list of the DgramFragInfo element.
 */
typedef struct {
    List_Links		links;		/* Links to next and prev. items. */
    unsigned short	fragOffset;	/* Offset from the beginning of 
					 * the datagram for the data in
					 * this frgament. */
    int			dataOffset;	/* Offset from beginning of this 
					 * fragment data where usable data 
					 * starts. */
    int			dataLen;	/* Size in bytes of usable data. */
    IPS_Packet		fragPacket;	/* Address of fragment packet. */
} FragDataInfo;

/*
 * Amount of time between calls to FragTimeoutHandler.
 */
static Time	fragTimeout = { 1, 0 };


/*
 * The protocolCallback array of functions is used to pass an IP packet
 * to the next protocol layer. A higher-level protocol establishes a 
 * callback using IP_SetProtocolHandler. The maximum number of callbacks is 
 * 255 since the protocol field in the IP header 8 bits wide.
 */

#define MAX_PROTOCOL_NUM	255
static void (*protocolCallback[MAX_PROTOCOL_NUM+1])();

/*
 * Forward declarations.
 */
static Boolean		ReassemblePacket();
static ReturnStatus	ProcessOptions();
static void		ForwardPacket();
static DgramFragInfo	*FreeFragments();
static void		SaveRoute();
static void		FragTimeoutHandler();
static int 		CopyHeader();
static Boolean		CanForward();

#ifdef DEBUG
static void 		TestInputProc();
#endif

/*
 * Macro to swap the fragOffset field.
 */
#define SWAP_FRAG_OFFSET_HOST_TO_NET(ptr) { \
    short	*shortPtr; \
    shortPtr = ((short *)&ptr->ident) + 1; \
    *shortPtr = Net_HostToNetShort(*shortPtr); \
} 

#define SWAP_FRAG_OFFSET_NET_TO_HOST(ptr) { \
    short	*shortPtr; \
    shortPtr = ((short *)&ptr->ident) + 1; \
    *shortPtr = Net_NetToHostShort(*shortPtr); \
} 


/*
 *----------------------------------------------------------------------
 *
 * IP_MemBin --
 *
 *	Optimize memory allocation of data structures that are dynamically
 *	allocated by IP.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls Mem_Bin.
 *
 *----------------------------------------------------------------------
 */

void
IP_MemBin()
{
    Mem_Bin(sizeof(DgramFragInfo));
    Mem_Bin(sizeof(FragDataInfo));
    Mem_Bin(sizeof(Net_IPHeader));
    Mem_Bin(NET_ETHER_MAX_BYTES);
}

/*
 *----------------------------------------------------------------------
 *
 * IP_Init --
 *
 *	Initializes the DgramFragInfo list. This routine must be
 *	called before any packets are processed. Arranges to have 
 *	FragTimeoutHandler to be called at regular intervals.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The DgramFragInfo list is initialized. A timeout handler is
 *	created.
 *
 *----------------------------------------------------------------------
 */

void
IP_Init()
{
    List_Init(&fragInfoList);

    (void) Fs_TimeoutHandlerCreate(fragTimeout, TRUE, FragTimeoutHandler, 
		(ClientData) 0);
}


/*
 *----------------------------------------------------------------------
 *
 * IP_SetProtocolHandler --
 *
 *	Each IP packet has a protocol field that specifies the next protocol
 *	layer to be called once the IP input routine has finished with
 *	the datagram.  This routine establishes the procedure that will be
 *	called for the given protocol.
 *
 *	The proc parameter should be declared as follows:
 *
 *	void
 *	Proc(netID, packetPtr)
 *	    Rte_NetID	netID;
 *	    IPS_Packet	*packetPtr;
 *	{
 *	}
 *
 *	It is responsible for freeing the packet once it is done with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The table of protocol callbacks is updated.
 *
 *----------------------------------------------------------------------
 */

void
IP_SetProtocolHandler(protocol, proc)
    int		protocol;	/* Assigned protocol number from RFC 1010. */
    void	(*proc)();	/* Procedure to call to handle packets with
				 * the protocol #. */
{
    if (protocol < 0 || protocol > MAX_PROTOCOL_NUM) {
	(void) fprintf(stderr, "Warning: IP_SetProtocolHandler: bad protocol # (%d)\n",
			protocol);
	return;
    }
    if (proc == NULL) {
	(void) fprintf(stderr, 
		"Warning: IP_SetProtocolHandler: NULL procedure address\n");
    }
    protocolCallback[protocol] = proc;
}


/*
 *----------------------------------------------------------------------
 *
 * IP_AcceptedProtocol --
 *
 *	Determine if a IP protocol has a handler established for it.  This
 *	is used at open time in order to deny opening an IP protocol that
 *	is not implemented.
 *
 * Results:
 *	TRUE if a callback has been installed for the protocol.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
IP_AcceptedProtocol(protocol)
    int		protocol;	/* Assigned protocol number from RFC 1010. */
{
    if (protocol < 0 || protocol > MAX_PROTOCOL_NUM) {
	return(FALSE);
    } else if (protocolCallback[protocol] != NULL) {
	return(TRUE);
    } else {
	return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * IP_Input --
 *
 *	The main routine to deal with new IP packets from the network.
 *	The packet is checked to make sure that it has the proper sizes
 *	and that it has not been corrupted. The packet's destination is 
 *	checked to see if it is for us or whether it needs to be forwarded
 *	to the appropriate gateway. IP options processing and fragment 
 *	reassembly is also initiated here.  Once processed, the packet 
 *	may be handed to the next protocol layer.
 *
 *	Terminology: a packet is the data received from the network.
 *	It may a complete IP datagram or a fragment of a datagram.
 *
 * Results:
 *	SUCCESS	- The packet was processed successfully or was a fragment. 
 *		  The caller can NOT deallocate the packet memory.
 *	FAILURE	- The packet was malformed or was not used. The caller 
 *		  must deallocate the packet memory.
 *
 * Side effects:
 *	The fragment queue may be changed. Various statistics counters 
 *	are incremented.
 *
 *
 *	The following fields are converted from network order to host order.
 *	 - totalLen
 *	 - ident
 *	 - offset + flags
 *	 - checksum
 *	 - source
 *	 - dest
 *	They must be changed back if the packet needs to be output 
 *	to the network.
 *
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
IP_Input(netID, packetPtr)
    Rte_NetID	netID;		/* ID of network interface that this packet
				 * arrived on. */
    IPS_Packet	*packetPtr;	/* Packet layout descriptor. */
{
    register Net_IPHeader	*ipHeaderPtr;
    register int		headerLenInBytes;


    stats.ip.totalRcv++;

    if (packetPtr->ipLen < sizeof(Net_IPHeader)) {
	stats.ip.shortPacket++;
	if (ips_Debug) {
	    fprintf(stderr, "IP_Input: Short packet, len=%d\n", 
			    packetPtr->ipLen);
	}
	return(FAILURE);
    }

    ipHeaderPtr =  packetPtr->ipPtr;
    headerLenInBytes = ipHeaderPtr->headerLen * 4;

    if (headerLenInBytes < sizeof(Net_IPHeader)) {
	stats.ip.shortHeader++;
	if (ips_Debug) {
	    fprintf(stderr, "IP_Input: Short header, len=%d\n", 
			    headerLenInBytes);
	}
	return(FAILURE);
    }

    /*
     * See if the header checksum is ok. RFC791 says the header checksum 
     * is computed as if ipHeaderPtr->checksum is 0 but we recompute it with
     * whatever value that's there. If the packet is good, then the sum not
     * including ipHeaderPtr->checksum should be the 1's complement of 
     * ipHeaderPtr->checksum and when added together (during the checksum 
     * computation), they should equal 0.
     */
    if (Net_InetChecksum(headerLenInBytes, (Address) ipHeaderPtr) != 0) {
	stats.ip.badChecksum++;
	if (ips_Debug) {
	    fprintf(stderr, "IP_Input: Bad check sum\n");
	}
	return(FAILURE);
    }

    ipHeaderPtr->totalLen = Net_NetToHostShort(ipHeaderPtr->totalLen);
    ipHeaderPtr->ident    = Net_NetToHostShort(ipHeaderPtr->ident);
    SWAP_FRAG_OFFSET_NET_TO_HOST(ipHeaderPtr);

    /*
     * Change the length in the packet descriptor to the true length.
     * The previous value for the length was calculated from the
     * packet size from the network and that might have been too large.
     */
    packetPtr->ipLen = ipHeaderPtr->totalLen;

    if (ipHeaderPtr->totalLen < ipHeaderPtr->headerLen) {
	stats.ip.shortLen++;
	if (ips_Debug) {
	    fprintf(stderr, "IP_Input: Short length, len=%d\n", 
			    ipHeaderPtr->totalLen);
	}
	return(FAILURE);
    }

    if (headerLenInBytes > sizeof(Net_IPHeader)) {
	/*
	 * Process the options that follow the basic IP header.
	 */
	if (!ProcessOptions(headerLenInBytes, ipHeaderPtr)) {
	    if (ips_Debug) {
		fprintf(stderr, "IP_Input: Bad options\n");
	    }
	    return(FAILURE);
	}
    }

    if (ips_Debug) {
	(void) fprintf(stderr, 
		    "IP Input: %d bytes, prot %d <%x> <-- <%x,#%d> flags %x\n",
			packetPtr->ipLen,
			ipHeaderPtr->protocol,
			Net_NetToHostInt(ipHeaderPtr->dest), 
			Net_NetToHostInt(ipHeaderPtr->source),
			ipHeaderPtr->ident,
			ipHeaderPtr->flags);

    }


    /*
     * See if the packet is for us. If it isn't then forward it to
     * the appropriate host.
     */
    
    if (!Rte_AddrIsForUs(ipHeaderPtr->dest)) {
	stats.ip.forwards++;
	if (ips_Debug) {
	    fprintf(stderr, "IP_Input: Forwarding packet\n");
	}
	ForwardPacket(packetPtr);
	return(FAILURE);
    }


    /*
     * Fragmentation test: 
     * See if this packet is a complete datagram or a fragment of one.
     * If it's a fragment, then search the queue and try to reassemble the
     * complete datagram.  If it's a complete datagram already, then 
     * don't search the queue; even though there may be fragments for the
     * datagram on the queue, let the timeout routine remove them.
     */
    if ((ipHeaderPtr->flags & NET_IP_MORE_FRAGS) ||
	(ipHeaderPtr->fragOffset != 0)) {

	register DgramFragInfo	*fragInfoPtr;
	Boolean	foundFrags;

	stats.ip.fragsRcv++;
	if (ips_Debug) {
	    fprintf(stderr, "IP_Input: Received fragment, offset=%d\n",
			ipHeaderPtr->fragOffset);
	}

	foundFrags = FALSE;
	LIST_FORALL(&fragInfoList, (List_Links *) fragInfoPtr) {
	    if ((ipHeaderPtr->ident == fragInfoPtr->ident) &&
		(ipHeaderPtr->protocol == fragInfoPtr->protocol) &&
		(ipHeaderPtr->source == fragInfoPtr->source) &&
		(ipHeaderPtr->dest == fragInfoPtr->dest)) {
		foundFrags = TRUE;
		break;
	    }
	}
	if (!foundFrags) {
	    /*
	     * First fragment to arrive so there's no information yet.
	     * ReassemblePacket will create a list for this datagram.
	     */
	    if (ips_Debug) {
		fprintf(stderr, "IP_Input: First fragment\n");
	    }
	    fragInfoPtr = (DgramFragInfo *) NULL;
	}

	if (!ReassemblePacket(fragInfoPtr, packetPtr)) {
	    /*
	     * The fragment did not complete the packet.
	     */
	    return(SUCCESS);
	}
	if (ips_Debug) {
	    fprintf(stderr, "IP_Input: Fragment completes packet\n");
	}
	stats.ip.fragsReass++;
    }

    /*
     * IP Processing is now complete. Pass the datagram to the next protocol
     * for additional processing.  If a callback has not been established for
     * a protocol, give the datagram to the raw socket layer. 
     */

    if ((ipHeaderPtr->protocol <= MAX_PROTOCOL_NUM) &&
	(protocolCallback[ipHeaderPtr->protocol] != NULL)) {

	 (*protocolCallback[ipHeaderPtr->protocol]) (netID, packetPtr);

    } else {
	Raw_Input((int)ipHeaderPtr->protocol, ipHeaderPtr->source, 
			ipHeaderPtr->dest, packetPtr);
	return(FAILURE);	/* So caller will deallocate the packet. */
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FragTimeoutHandler --
 *
 *	Examines the fragment queue to see if any fragments should be
 *	discarded because they have not been reassembled within a certain
 *	amount of time. This routine is called from the Fs_Dispatch's
 *	timeout mechanism.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fragment info list may be changed.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
FragTimeoutHandler(data, time)
    ClientData	data;		/* ignored. */
    Time	time;		/* ignored. */
{
    register DgramFragInfo	*fragInfoPtr;

    stats.ip.fragTimeouts++;

    fragInfoPtr = (DgramFragInfo *) List_First(&fragInfoList);
    while (!List_IsAtEnd(&fragInfoList, (List_Links *) fragInfoPtr)) {
	if (fragInfoPtr->timeToLive <= 0) {
	    /*
	     * FreeFragments will remove the element from the list
	     * but it will return the pointer to the next element
	     * so the while loop will work.
	     */

	    if (ips_Debug) {
		fprintf(stderr, "IP Frag timeout: src <%x, #%d>\n",
			fragInfoPtr->source, fragInfoPtr->ident);
	    }

	    fragInfoPtr = FreeFragments(fragInfoPtr);

	    stats.ip.fragsTimedOut++;

	} else {
	    fragInfoPtr->timeToLive--;
	    fragInfoPtr = (DgramFragInfo *) 
				List_Next((List_Links *) fragInfoPtr);
	}

    }
}


/*
 *----------------------------------------------------------------------
 *
 * ReassemblePacket --
 *
 *	Given a packet that is a fragment of a datagram, try to reassemble
 *	a complete datagram.
 *
 * Results:
 *	TRUE		- the datagram was reassembled.
 *	FALSE		- the fragment did not complete the datagram.
 *
 * Side effects:
 *	The fragment will be added to the list of frgaments for this
 *	datagram. If all fragments for the datagram have arrived, they
 *	are consolidated into 1 buffer and the memory held by the fragments 
 *	is freed.
 *
 *----------------------------------------------------------------------
 */

static Boolean
ReassemblePacket(fragInfoPtr, packetPtr)
    register DgramFragInfo *fragInfoPtr;/* Info about existing fragments
					 * for this datagram. */
    IPS_Packet	*packetPtr;		/* Addr of ptr to Fragment (in) -- Addr
					 * of ptr to completed datagram (out).*/
{
    register FragDataInfo	*dataPtr;	/* Ptr to element on the
						 * fragment queue. */
    register FragDataInfo	*newDataPtr;
    register List_Links		*fragDataListPtr;
    register Net_IPHeader	*ipHeaderPtr;
    register int		fragOffset;
    Address			ptr;
    Boolean			first;
    int				next;
    int				lastByte;

    ipHeaderPtr = packetPtr->ipPtr;

    /*
     * The fragOffset "is measured in units of 8 octets (64 bits)".
     * It is necessary to use it as # of bytes, hence the multiplication
     * by 8.  Note that we already byte swapped it before this routine was
     * called.
     */
    fragOffset = ipHeaderPtr->fragOffset * 8;

    /*
     * For the first fragment, allocate the fragment info data structure 
     * and initialize it.
     */
    if (fragInfoPtr == (DgramFragInfo *) NULL) {
	fragInfoPtr = (DgramFragInfo *) malloc(sizeof(DgramFragInfo));
	fragInfoPtr->source	= ipHeaderPtr->source;
	fragInfoPtr->dest	= ipHeaderPtr->dest;
	fragInfoPtr->ident	= ipHeaderPtr->ident;
	fragInfoPtr->protocol	= ipHeaderPtr->protocol;
	fragInfoPtr->numFrags	= 0;
	fragInfoPtr->timeToLive	= NET_IP_MAX_FRAG_TTL;
	fragDataListPtr =  &(fragInfoPtr->dataList);
	dataPtr = (FragDataInfo	*) fragDataListPtr;
	List_Init(fragDataListPtr);

	List_InitElement((List_Links *) fragInfoPtr);
	List_Insert((List_Links *) fragInfoPtr, LIST_ATFRONT(&fragInfoList));
    } else {

	fragDataListPtr = &(fragInfoPtr->dataList);

	/*
	 * Insert a new fragment into the appropriate spot in the fragment
	 * list. Depending on the location, new fragments may or may not
	 * replace data in existing fragments.  (Note: this is contrary
	 * to RFC791 which says data in an arriving fragment should
	 * replace existing data. The algorithm to do that is a bit more
	 * complex than the one use below.)
	 *
	 * Step 1: search the list to find the first existing fragment
	 * that begins after the new fragment. When the loop is exited via
	 * the break, dataPtr will point to this existing fragment.
	 * If we don't take the break, dataPtr will point to the head of 
	 * the list and the new fragment belongs at the end of the list.
	 */

	LIST_FORALL(fragDataListPtr, (List_Links *) dataPtr) {
	    /*
	     * Find the first existing fragment that begins after 
	     * the new fragment.
	     */ 
	    if (fragOffset < dataPtr->fragOffset) {
		break;
	    }
	}

	/*
	 * Step 2: The new fragment is before dataPtr and after
	 * newDataPtr. newDataPtr could be the head of list which means
	 * the new fragment fits in front of all the other fragments.
	 * dataPtr could be the list head which means that the new
	 * fragment goes at the end of the list.
	 */

	newDataPtr = (FragDataInfo *) (List_Prev((List_Links *)dataPtr));

	if (!List_IsAtEnd(fragDataListPtr, (List_Links *)newDataPtr)) {

	    int i = newDataPtr->fragOffset + newDataPtr->dataLen - 
				fragOffset;
	    /*
	     * "if (i > 0)" ==  if (fragOffset < last byte in preceeding 
	     * fragment) then the new fragment overlaps or is contained in 
	     * the preceeding fragment.
	     */

	    if (i > 0) {
		if (i >= ipHeaderPtr->totalLen) {
		    /*
		     * The new fragment is totally contained in the preceeding 
		     * fragment so drop the new fragment. 
		     *
		     * (If the new data were to replace the old, the preceeding 
		     * fragment would have to be split or the data from 
		     * the new fragment copied into the preceeding fragment.)
		     */

		    if (ips_Debug) {
			fprintf(stderr, "ReassemblePacket: Frag dropped\n");
		    }
		    stats.ip.fragsDropped++;
		    free(packetPtr->base);
		    return(FALSE);

		} else {
		    /*
		     * The end of the preceeding fragment needs to be trimmed 
		     * because the new fragment replaces that data.
		     * (4.3BSD keeps the old data and trims the front of
		     * the new fragment).
		     */
		     newDataPtr->dataLen -= i;
		}
	    }
	}

	/*
	 * Step 3:  see if the succeeding fragments need to be trimmed 
	 * because the end of the new fragment partially or completely 
	 * overlaps them.
	 */

	lastByte = fragOffset + ipHeaderPtr->totalLen;
	while ((!List_IsAtEnd(fragDataListPtr, (List_Links *)dataPtr)) &&
	       (dataPtr->fragOffset < lastByte)) {

	    if ((dataPtr->fragOffset + dataPtr->dataLen) < lastByte) {
		/*
		 * The new fragment covers the old fragment so the
		 * old one can be freed.
		 */

		newDataPtr =  /* temp. holder */
			(FragDataInfo *) (List_Next((List_Links *)dataPtr));
		List_Remove((List_Links *)dataPtr);
		free((char *) dataPtr->fragPacket.base);
		free((char *) dataPtr);
		dataPtr = newDataPtr;

	    } else {
		/*
		 * The new fragment covers some, but not all, of the old 
		 * fragment. Trim data from the start of the old one. 
		 * At this point we are also done trimming.
		 */
		int trimAmt = lastByte - dataPtr->fragOffset;

		dataPtr->fragOffset	+= trimAmt;
		dataPtr->dataOffset	+= trimAmt;
		dataPtr->dataLen	-= trimAmt;
		break;
	    }
	}
    }

    /*
     * Insert the new fragment into the list of received fragments.
     */

    fragInfoPtr->numFrags++;
    newDataPtr = (FragDataInfo *) malloc(sizeof(FragDataInfo));
    newDataPtr->fragOffset	= fragOffset;
    newDataPtr->dataOffset	= ipHeaderPtr->headerLen * 4;
    newDataPtr->dataLen		= ipHeaderPtr->totalLen -newDataPtr->dataOffset;
    newDataPtr->fragPacket	= *packetPtr;

    List_InitElement((List_Links *) newDataPtr);
    List_Insert((List_Links *)newDataPtr, LIST_BEFORE((List_Links *) dataPtr));


    /*
     * Now scan the data list for this datagram to see if all the 
     * fragments have been received. "Next" is the next expected fragment
     * offset.
     */

    next = 0;
    LIST_FORALL(fragDataListPtr, (List_Links *) dataPtr) {
	if (dataPtr->fragOffset != next) {
	    /*
	     * Oops! Found a hole so the datagram is not yet complete.
	     */
	    if (ips_Debug) {
		fprintf(stderr, "ReassemblePacket: Incomplete datagram\n");
	    }
	    return(FALSE);
	} else {
	    next += dataPtr->dataLen;
	}
    }

    /*
     * All the fragments are contiguous but we have to check if the
     * last fragment has been received. This is done by checking 
     * the NET_IP_MORE_FRAGS bit in the header of the queue's last fragment.
     * If that bit is set, then the reassembly is not yet complete.
     *
     * (dataPtr should point to the head of the list so we need to look
     * at the previous element.)
     */

    newDataPtr = (FragDataInfo *) (List_Prev((List_Links *)dataPtr));
    if (newDataPtr->fragPacket.ipPtr->flags & NET_IP_MORE_FRAGS) {
	if (ips_Debug) {
	    fprintf(stderr, "ReassemblePacket: More frags\n");
	}
	return(FALSE);
    }

    /*
     * At this point, all the fragments have been received. A new buffer
     * is allocated so all the fragments can be concatenated together into
     * 1 datagram.  The datagram's IP header comes from the first fragment.
     * After copying the fragment, the FragDatInfo element is removed from
     * the list and freed, along with the fragment.
     *
     * (IPS_InitPacket will resuse packetPtr and allocate new memory. 
     * We don't free(packetPtr->base) before calling IPS_InitPacket 
     * because that buffer is already used in one of elements in the fragment 
     * data list for this datagram.)
     */

    if (ips_Debug) {
	fprintf(stderr, "ReassemblePacket: All frags received\n");
    }

    IPS_InitPacket(next, packetPtr);
    ptr = packetPtr->dbase + sizeof(IPS_PacketNetHdr);
    packetPtr->ipPtr = (Net_IPHeader *) ptr;

    first = TRUE;
    dataPtr = (FragDataInfo *) List_First(fragDataListPtr);
    while (!List_IsAtEnd(fragDataListPtr, (List_Links *) dataPtr)) {
	if (first) {
	    int amtToCopy = dataPtr->fragPacket.ipPtr->totalLen;
	    first = FALSE;

	    /*
	     * The header of the first fragment needs to be copied, too.
	     */
	    bcopy((Address) dataPtr->fragPacket.ipPtr, (Address) ptr,amtToCopy);

	    /*
	     * Reset the size of the packet in the header.
	     */
	    packetPtr->ipLen = ((Net_IPHeader *) ptr)->totalLen = 
			    next + (dataPtr->fragPacket.ipPtr->headerLen * 4);

	    ptr += amtToCopy;
	} else {
	    bcopy((Address) dataPtr->fragPacket.ipPtr+dataPtr->dataOffset, 
		    (Address) ptr, dataPtr->dataLen);
	    ptr += dataPtr->dataLen;
	}

	newDataPtr = (FragDataInfo *) (List_Next((List_Links *)dataPtr));
	List_Remove((List_Links *) dataPtr);
	free((char *) dataPtr->fragPacket.base);
	free((char *) dataPtr);
	dataPtr = newDataPtr;
    }
    List_Remove((List_Links *) fragInfoPtr);
    free((char *) fragInfoPtr);

    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeFragments --
 *
 *	Releases the fragments held while trying to reassemble a datagram.
 *
 * Results:
 *	Address of the previous element in the fragInfoList.
 *
 * Side effects:
 *	The saved packets are freed and the *fragInfoPtr is removed
 *	from the fragment queue.
 *
 *----------------------------------------------------------------------
 */

static DgramFragInfo *
FreeFragments(fragInfoPtr)
    register DgramFragInfo *fragInfoPtr; /* Info about existing fragments
					  * for this datagram. */
{
    register List_Links	*dataPtr;
    register List_Links	*nextPtr;
    DgramFragInfo *savePtr; 

    /*
     * We need to return the ptr to the next element so this routine
     * can be called inside of a list-examining loop and still be able 
     * to remove the element from the list.
     */
    savePtr = (DgramFragInfo *) List_Next((List_Links *) fragInfoPtr);
    List_Remove((List_Links *) fragInfoPtr);

    LIST_FORALL(&(fragInfoPtr->dataList), dataPtr) {
	nextPtr = List_Next(dataPtr);
	List_Remove(dataPtr);
	free((char *) ((FragDataInfo *)dataPtr)->fragPacket.base);
	free((char *) dataPtr);
	dataPtr = nextPtr;
    }
    free((char *) fragInfoPtr);
    return(savePtr);
}


/*
 *----------------------------------------------------------------------
 *
 * ForwardPacket --
 *
 *	Given an packet that's not for us, this routine determines which 
 *	host the packet should be forwarded to and tries to send it to
 *	that host (possibly via gateways).
 *
 *	For now, we don't do forwarding -- the packet is just dropped.
 *	This will need to change when Sprite machines want to act
 *	as gateways/routers.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
ForwardPacket(packetPtr)
    IPS_Packet	*packetPtr;	/* Packet layout descriptor. */
{
    stats.ip.cannotForward++;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessOptions --
 *
 *	This routine decodes the option(s) that follow the IP header
 *	and processes it/them.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An ICMP packet is sent to the packet sender if if an option is 
 *	malformed. The Record Route and Timestamp options modify the
 *	the packet.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ProcessOptions(hdrLen, ipHeaderPtr)
    int hdrLen;				/* Header length in bytes. */
    register Net_IPHeader *ipHeaderPtr;	/* IP header with trailing options. */
{
    register Address optionPtr;		/* Ptr to the current option in the
					 * IP header that's being processed. */
    int		optionType;
    int		optionLen;
    int		totalOptionSize;
    int		ptr;
    int		icmpCode;
    int		icmpType;


    /*
     * Go through the option area of the packet and process each option.
     * The options begin just after the standard header info.
     */
    
    optionPtr = ((Address) ipHeaderPtr) + sizeof(Net_IPHeader) + 1;

    for (totalOptionSize = hdrLen - sizeof(Net_IPHeader); 
	 totalOptionSize > 0; 
	 totalOptionSize -= optionLen, optionPtr += optionLen) {

	optionType = optionPtr[NET_IP_OPT_TYPE_OFFSET];

	/*
	 * Decode the option. END_OF_LIST and NOP options are only 1
	 * octet long. The rest of the options have a length field that 
	 * must be checked.
	 */

	if (optionType == NET_IP_OPT_END_OF_LIST) {
	    break;
	} else if (optionType == NET_IP_OPT_NOP) {
	    optionLen = 1;
	} else {
	    optionLen = optionPtr[NET_IP_OPT_LEN_OFFSET];
	    if (optionLen <= 0 || optionLen > totalOptionSize) {
		icmpType = NET_ICMP_PARAM_PROB;
		icmpCode = &optionPtr[NET_IP_OPT_LEN_OFFSET] - 
				(Address) ipHeaderPtr;
		goto bad;
	    }

	    switch (optionType) {

		/*
		 * Loose and strict source routing and record options:
		 *
		 * If the packet is not directed towards one of our
		 * Internet addresses then send an ICMP error packet if
		 * strict routing is wanted or do nothing if loosely
		 * routed.  Also, record our address in the option area
		 * and make the next address in the routing list as the
		 * destination.  If strictly-routed, make sure the next
		 * address is on a directly-accessible network.
		 */

		case NET_IP_OPT_LOOSE_ROUTE:
		case NET_IP_OPT_STRICT_ROUTE:

		    ptr = optionPtr[NET_IP_OPT_PTR_OFFSET];
		    if (ptr < NET_IP_OPT_MIN_PTR) {
			icmpType = NET_ICMP_PARAM_PROB;
			icmpCode = &optionPtr[NET_IP_OPT_PTR_OFFSET]  -
				    (Address) ipHeaderPtr;
			goto bad;
		    }

		    if (!Rte_AddrIsForUs(ipHeaderPtr->dest)) {
			if (optionType == NET_IP_OPT_STRICT_ROUTE) {
				icmpType = NET_ICMP_UNREACHABLE;
				icmpCode = NET_ICMP_UNREACH_SRC_ROUTE;
				goto bad;
			}
			/*
			 * Loose routing, and not at next destination
			 * yet; nothing to do except forward.
			 */
			break;
		    }

		    ptr--;			/* 0 origin */
		    if (ptr > optionLen - sizeof(Net_InetAddress)) {
			/*
			 * End of source route.  Should be for us.
			 */
			SaveRoute(optionPtr, ipHeaderPtr->source);

		    } else {
			Net_InetAddress addr;

			/*
			 * We are an intermediate host in the routing list.
			 * Change the packet's destination to the next host in 
			 * the list and record our address there.
			 */
			addr = *((Net_InetAddress *)(optionPtr + ptr));
			    
			if ((optionType == NET_IP_OPT_STRICT_ROUTE) && 
			    !CanForward(addr)) {
				icmpType = NET_ICMP_UNREACHABLE;
				icmpCode = NET_ICMP_UNREACH_SRC_ROUTE;
				goto bad;
			}
			*((Net_InetAddress *)(optionPtr + ptr)) =
					(ipHeaderPtr->dest);
			optionPtr[NET_IP_OPT_PTR_OFFSET] += 
					sizeof(Net_InetAddress);
			ipHeaderPtr->dest = addr;
		    }
		    break;


		/*
		 * Record Route option: 
		 *
		 *  Add our address in the area reserved for routes.
		 */

		case NET_IP_OPT_REC_ROUTE: 
		    {
			ptr = optionPtr[NET_IP_OPT_PTR_OFFSET];
			if (ptr < NET_IP_OPT_MIN_PTR) {
			    icmpType = NET_ICMP_PARAM_PROB;
			    icmpCode= &optionPtr[NET_IP_OPT_PTR_OFFSET]  -
					(Address) ipHeaderPtr;
			    goto bad;
			}

			/*
			 * If no space remains, do nothing.
			 */
			ptr--;			/* 0 origin */
			if (ptr <= (optionLen - sizeof(Net_InetAddress))) {
			    /*
			     * See if we can forward this packet before our
			     * address is recorded in the routing list.
			     */

			    if (!CanForward(ipHeaderPtr->dest)) {
				    icmpType = NET_ICMP_UNREACHABLE;
				    icmpCode = NET_ICMP_UNREACH_SRC_ROUTE;
				    goto bad;
			    }
			    *((Net_InetAddress *)(optionPtr + ptr)) =
				 (Rte_GetOfficialAddr(FALSE));

			    optionPtr[NET_IP_OPT_PTR_OFFSET] += 
				    sizeof(Net_InetAddress);
			}
		    }
		    break;

		/*
		 * Timestamp option: 
		 *
		 * Add a timestamp (and optionally our address) in the area 
		 * reserved for it.
		 */

		case NET_IP_OPT_TIMESTAMP: 
		    {
			Net_InetTime    time;
			Net_InetAddress *addrPtr;
			Net_IPTimestampHdr *tsPtr =
					(Net_IPTimestampHdr *)optionPtr;

			if (tsPtr->len < 5) {
			    icmpType = NET_ICMP_PARAM_PROB;
			    icmpCode = optionPtr - (Address) ipHeaderPtr;
			    goto bad;
			}

			if (tsPtr->pointer > (tsPtr->len - sizeof(*tsPtr))) {
			    tsPtr->overflow++;
			    if (tsPtr->overflow == 0) {
				icmpType = NET_ICMP_PARAM_PROB;
				icmpCode = optionPtr - (Address) ipHeaderPtr;
				goto bad;
			    }
			    break;
			}

			addrPtr = (Net_InetAddress *)
					(optionPtr + tsPtr->pointer -1);

			switch (tsPtr->flags) {

			    case NET_IP_OPT_TS_ONLY:
				break;

			    /*
			     * Add our address to the packet if there's room.
			     */
			    case NET_IP_OPT_TS_AND_ADDR:
				if ((tsPtr->pointer + sizeof(Net_InetTime) +
				    sizeof(Net_InetAddress)) > tsPtr->len) {
					icmpType = NET_ICMP_PARAM_PROB;
					icmpCode = optionPtr -
						    (Address) ipHeaderPtr;
					goto bad;
				}
				*addrPtr = (Rte_GetOfficialAddr(FALSE));
				tsPtr->pointer += sizeof(*addrPtr);
				break;

			    /*
			     * Add our address to the packet only if our
			     * address in the list of prespecified addresses.
			     */
			    case NET_IP_OPT_TS_ADDR_SPEC:
				if ((tsPtr->pointer + sizeof(Net_InetTime) +
				    sizeof(Net_InetAddress)) > tsPtr->len) {

					icmpType = NET_ICMP_PARAM_PROB;
					icmpCode = optionPtr - 
						    (Address) ipHeaderPtr;
					goto bad;
				}
				if (!Rte_AddrIsForUs((*addrPtr))) {
				    continue;
				}
				tsPtr->pointer += sizeof(*addrPtr);
				break;

			    default:
				    /*
				     * Unknown timestamp flag.
				     */
				    icmpType = NET_ICMP_PARAM_PROB;
				    icmpCode = optionPtr -(Address) ipHeaderPtr;
				    goto bad;
			}
			time = Net_HostToNetInt(IPS_GetTimestamp());
			bcopy( (Address)&time, 
		              (Address) (optionPtr + tsPtr->pointer -1),
			      sizeof(time));
			tsPtr->pointer += sizeof(time);
		    }
		    break;

		/*
		 * The following options aren't handled yet.
		 */
		case NET_IP_OPT_STREAM_ID:
		case NET_IP_OPT_SECURITY:
		default:
		    break;
	    }
	}
    }
    return(SUCCESS);

    /*
     * We are sent here if there was an error with an option's format.
     * Send an ICMP error packet to the source.
     */
bad:
    ICMP_SendErrorMsg(ipHeaderPtr, icmpType, icmpCode);
    return (FAILURE);
}


/*
 *----------------------------------------------------------------------
 *
 * CanForward --
 *
 * 	Given an address of the next destination (final or next hop),
 * 	indicate whether we can foraward the packet.
 *
 * Results:
 *	TRUE	- we are able to forward the packet.
 *	FALSE	- we are not able to forward the packet.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static Boolean
CanForward(dest)
     Net_InetAddress dest;
{
    /*
     * For now, say we can't forward anything.
     */
    return(FALSE);
}


/*
 * We need to save the IP options in case a higher-level protocol wants 
 * to respond to an incoming packet over the same route if the packet got
 * here using IP source routing.  This allows connection establishment and
 * maintenance when the remote end is on a network that is not known to us.
 *
 * MAX_SAVE_ADDRS is the max. # of IP addresses that can be stored in
 * an IP record route option. Subtract 3 for the record route info (type,
 * length and pointer).
 */

#define MAX_SAVE_ADDRS  ((NET_IP_OPT_MAX_LEN - 3)/sizeof(Net_InetAddress))
static struct {
    int numHops;				/* # of addresses in addr. */
    struct {
	struct {
	    char	nop;		/* One NOP to align when 
					 * returning the route. */
	    char	type;		/* Option type */
	    char	len;		/* Totol length of info + addr. */
	    char	ptr;		/* Offset where routes start. */
	} info;
	Net_InetAddress addr[MAX_SAVE_ADDRS];	/* List of routes. */
    } route;
    Net_InetAddress 	dest;		/* In host byte order. */
} savedSrcRoute = { 0, {{NET_IP_OPT_NOP,}, }, };

/*
 *----------------------------------------------------------------------
 *
 * SaveRoute --
 *
 *	Save incoming source route for use in replies,
 *	to be picked up later by IP_SrcRoute if the receiver is interested.
 *	The options are kept in network byte order.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The route is stored in a static buffer.
 *
 *----------------------------------------------------------------------
 */

static void
SaveRoute(optionPtr, dest)
    Address	optionPtr;	/* Ptr to IP options buffer containing source
				 * routing option. */
    Net_InetAddress dest;	/* Next destination address to put in the
				 * saved source route. */
{
    int optLen;

    optLen = optionPtr[NET_IP_OPT_LEN_OFFSET];
    if (optLen > sizeof(savedSrcRoute.route) - 1) {
	(void) fprintf(stderr, "Warning: SaveRoute (IP): optLen (%d) bad\n", optLen);
	return;
    }
    bcopy(optionPtr, (Address)&savedSrcRoute.route.info.type, optLen);
    savedSrcRoute.numHops = (optLen - 3) / sizeof(Net_InetAddress);
    savedSrcRoute.dest = dest;
}


/*
 *----------------------------------------------------------------------
 *
 * IP_GetSrcRoute --
 *
 *	Get the incoming source route that was saved in savedSrcRoute
 *	for use in replies. The first hop is returned in *destPtr.
 *
 * Results:
 *	The address of a buffer containing the saved source route from the
 *	most recent IP packet.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Address 
IP_GetSrcRoute(lenPtr, destPtr)
    int	*lenPtr;		/* Out: Size of source-route buffer. */
    Net_InetAddress *destPtr;	/* Out: address of the first destination. */
{
    register Net_InetAddress *oldPtr, *newPtr;
    register Address routePtr;
    int	len;

    if (savedSrcRoute.numHops == 0) {
	*lenPtr = 0;
	return((Address) NULL);
    }

    len = ((1 + savedSrcRoute.numHops) * sizeof(Net_InetAddress)) +
		sizeof(savedSrcRoute.route.info);
    *lenPtr = len;

    /*
     * Allocate a buffer to hold the source route.
     */
    routePtr = malloc((unsigned int) len);

    /*
     * First the save first hop for return route.
     */
    *destPtr = savedSrcRoute.dest;

    /*
     * Copy the route info.
     */
    bcopy( (Address) &savedSrcRoute.route.info, routePtr,
		sizeof(savedSrcRoute.route.info));
    newPtr = (Net_InetAddress *) (routePtr + sizeof(savedSrcRoute.route.info));
			    
    /*
     * Record return path as an IP source route, reversing the path.
     */
    oldPtr = &savedSrcRoute.route.addr[savedSrcRoute.numHops -1];
    while (oldPtr >= savedSrcRoute.route.addr) {
	*newPtr++ = *oldPtr--;
    }
    return(routePtr);
}


/*
 *----------------------------------------------------------------------
 *
 * IP_Output --
 *
 *	Causes an IP packet to sent to the network. The packet may be
 *	fragmented if it's too large for the chosen network.
 *
 * Results:
 *	SUCCESS			- the datagram was sent out.
 *	FS_BUFFER_TOO_BIG	- the datagram was too large and had to be
 *				  fragmented but the DONT_FRAG bit was set
 *				  in the header.
 *	NET_UNREACHABLE		- couldn't find a network to send the datagram
 *				  to.
 *
 * Side effects:
 *	Packet(s) may be output.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
IP_Output(packetPtr, canTrash)
    register IPS_Packet *packetPtr;	/* Packet layout descriptor. IP-related
					 * fields are filled in. If the packet
					 * is fragmented, the data and hdrLen
					 * fields are modified. */
    Boolean canTrash;			/* TRUE if the packet will be discarded
					 * after output.  If so, then we can
					 * optimize fragmentation by writing
					 * packet headers over some data. */
{
    ReturnStatus	status;
    int			headerLenInBytes;
    int			maxOutSize;
    Rte_NetID		netID;
    register Net_IPHeader *headerPtr = packetPtr->ipPtr;


    headerLenInBytes = headerPtr->headerLen * 4;

    if (headerLenInBytes > NET_IP_MAX_HDR_SIZE) {
	return(FAILURE);
    }


    /*
     * Determine which network interface must be used to output this
     * datagram.
     */

    if (!Rte_FindOutputNet(headerPtr->dest,  &netID, &maxOutSize)) {
	return(NET_UNREACHABLE_NET);
    }

#ifdef notdef
    headerPtr->source = Net_HostToNetInt(headerPtr->source);
    headerPtr->dest   = Net_HostToNetInt(headerPtr->dest);
#endif

    /*
     * See if the datagram is small enough to be sent in 1 piece.
     */

    if (headerPtr->totalLen <= maxOutSize) {
	int	totalLen;

	stats.ip.wholeSent++;
	totalLen = headerPtr->totalLen;
	headerPtr->totalLen = Net_HostToNetShort(totalLen);
	headerPtr->fragOffset = 0;
	headerPtr->flags = 0;
	headerPtr->checksum = 0;
	headerPtr->checksum = Net_InetChecksum(headerLenInBytes, 
					(Address) headerPtr);
	packetPtr->ipLen = headerLenInBytes;
	status = Rte_OutputPacket(netID, packetPtr);

    } else if (headerPtr->flags & NET_IP_DONT_FRAG) {
	/*
	 * The datagram is too big but the higher-level protocol doesn't want
	 * it to be fragmented. Better tell the sender that it's too big.
	 */

	stats.ip.dontFragment++;

	status = FS_BUFFER_TOO_BIG;
    } else {

	register Address fragPtr;
	register Address dataPtr;
	int		totalDataLen;
	int		fragDataLen;
	int		fragHdrLen;
	int		offset;
	IPS_Packet 	packet;
	char		ipHeader[NET_IP_MAX_HDR_SIZE];

	stats.ip.fragOnSend++;

	/*
	 * The datagram is too big to send as 1 packet on the network
	 * (identified by netID) but we can fragment the datagram into 
	 * several packets and send them out individually. 
	 *
	 * Each fragment contains an IP header based on the original
	 * header plus some portion of the data.  Options in the IP header
	 * require special handling. All options in the original header
	 * need to go in the first fragment's header.  In succeeding
	 * fragments, only some of the options are included in the header.
	 *
	 * TotalDataLen is the number of bytes of data in the datagram and
	 * fragDataLen is the amount of data that can fit into a fragment.
	 * Actually fragDataLen must be a multiple of 8 (c.f. p. 25 in RFC791);
	 * this done by masking off the low-order 3 bits.
	 */

	totalDataLen = headerPtr->totalLen - headerLenInBytes;
	fragDataLen = (maxOutSize - headerLenInBytes) & ~7;

	/*
	 * Output the first fragment. We use the orginal datagram by modifying 
	 * the header and telling the output routine how many bytes to output 
	 * so we can avoid copying.
	 */

	headerPtr->flags = NET_IP_MORE_FRAGS;
	headerPtr->fragOffset = 0;
	SWAP_FRAG_OFFSET_HOST_TO_NET(headerPtr);

	headerPtr->totalLen = Net_HostToNetShort(fragDataLen+headerLenInBytes);
	headerPtr->checksum = 0;
	headerPtr->checksum = Net_InetChecksum(headerLenInBytes, 
					(Address) headerPtr);
	packetPtr->dataLen = fragDataLen;
	packetPtr->ipLen = headerLenInBytes;

	/*
	 * Reset the packet's higher-level protocol header length to 0.
	 * That header is now considered part of the data.
	 */
	packetPtr->hdrLen = 0;
	status = Rte_OutputPacket(netID, packetPtr);
	stats.ip.fragsSent++;
	if (status != SUCCESS) {
	    return(status);
	}

	/*
	 * The first fragDataLen bytes have been sent. DataPtr is set to
	 * point to the next byte in the original datagram to be sent.
	 */
	dataPtr = (Address) (((Address) headerPtr) + headerLenInBytes + 
				fragDataLen);

	/*
	 * Now allocate a buffer that will be used to send the rest of the
	 * fragments.  We must over-allocate the space because we don't
	 * know how much room the header options need until we copy them
	 * (only some options are copied). The rest of the buffer will 
	 * hold part of the data from the original packet.  We reuse this 
	 * buffer for all remaining fragments, adjusting the header if 
	 * necessary.
	 *
	 * CAN_TRASH:  If we can trash the packet then we do so by writing
	 * the fragment headers over existing data.  This eliminates the
	 * need to allocate a buffer and copy data into it.  The variable
	 * dataPtr always references the next data byte to be output.
	 * packet.base is set to be in front of this enough to fit the header.
	 */

#define fragHdrPtr ((Net_IPHeader *)fragPtr)

	packet.totalLen = fragDataLen + headerLenInBytes + 
				   sizeof(IPS_PacketNetHdr);
	packet.hdrLen = 0;
	if (!canTrash) {
	    /*
	     * Allocate a packet buffer and copy the header into it.  Note that
	     * we add two the allocated pointer so that we four byte align
	     * everything but the 14 byte ethernet header.
	     */
	    packet.base = malloc((unsigned int) packet.totalLen + 2);
	    packet.dbase = packet.base + 2;
	    fragPtr = packet.dbase + sizeof(IPS_PacketNetHdr);
	    fragHdrLen = CopyHeader(headerLenInBytes, headerPtr, 
				    (Net_IPHeader *)fragPtr);
	    packet.ipPtr = fragHdrPtr;
	    fragHdrPtr->headerLen = fragHdrLen / 4;
	} else {
	    /*
	     * Just save the fragment header and get its length.
	     */
	    fragHdrLen = CopyHeader(headerLenInBytes, headerPtr, ipHeader);
	}
	packet.ipLen = fragHdrLen;

	for (offset = fragDataLen; 
	     offset < totalDataLen; 
	     offset += fragDataLen, dataPtr += fragDataLen) {

	    if (canTrash) {
		/*
		 * Copy the fragment header in front of the next data to go out.
		 */
		fragPtr = dataPtr - fragHdrLen;
		bcopy((char *)ipHeader, fragPtr, fragHdrLen);
		packet.base = packet.dbase = fragPtr - sizeof(IPS_PacketNetHdr);
		packet.ipPtr = fragHdrPtr;
		fragHdrPtr->headerLen = fragHdrLen / 4;
	    }
	    if (offset + fragDataLen >= totalDataLen) {
		/*
		 * Last fragment reached. Must turn off MORE_FRAGS so the
		 * receiver won't expect more data.
		 */
		fragDataLen =  totalDataLen - offset;
		fragHdrPtr->flags = 0;
	    } else {
		fragHdrPtr->flags = NET_IP_MORE_FRAGS;
	    }
	    fragHdrPtr->fragOffset = offset >> 3;
	    SWAP_FRAG_OFFSET_HOST_TO_NET(fragHdrPtr);

	    fragHdrPtr->totalLen = Net_HostToNetShort(fragDataLen + fragHdrLen);


	    if (!canTrash) {
		/*
		 * Copy the data into the extra packet.
		 */
		bcopy( (Address) dataPtr, 
			(Address) fragPtr+fragHdrLen, fragDataLen);
	    }

	    fragHdrPtr->checksum = 0;
	    fragHdrPtr->checksum = Net_InetChecksum(fragHdrLen, 
						(Address) fragHdrPtr);
	    packet.data = fragPtr + packet.ipLen;
	    packet.dataLen = fragDataLen;
	    status = Rte_OutputPacket(netID, &packet);
	    stats.ip.fragsSent++;
	    if (status != SUCCESS) {
		break;
	    }
	}
	if (!canTrash) {
	    free(packet.base);
	}
    }
    return (status);
}


/*
 *----------------------------------------------------------------------
 *
 * IP_QueueOutput --
 *
 *	Queues up a packet to be sent at a later time by IP_DelayedOutput.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A queued packet may be sent.
 *	The packet is freed once IP_Output is called.
 *
 *----------------------------------------------------------------------
 */

static IPS_Packet queuedPacket;
static Boolean     isPacketQueued = FALSE;

void
IP_QueueOutput(packetPtr)
    IPS_Packet *packetPtr;	/* Packet layout descriptor. */
{
    if (isPacketQueued) {
	isPacketQueued = FALSE;
	(void) IP_Output(&queuedPacket, TRUE);
	free(queuedPacket.base);

	(void) IP_Output(packetPtr, TRUE);
	free(packetPtr->base);
    } else {
	isPacketQueued = TRUE;
	queuedPacket = *packetPtr;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * IP_DelayedOutput --
 *
 *	Causes a queued IP packet to be output.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The packet is freed once IP_Output is called.
 *
 *----------------------------------------------------------------------
 */

void
IP_DelayedOutput()
{
    if (isPacketQueued) {
	isPacketQueued = FALSE;
	(void) IP_Output(&queuedPacket, TRUE);
	free(queuedPacket.base);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * CopyHeader --
 *
 *	Copies the IP header from a unfragmented datagram to a fragmented
 *	datagram. The basic IP header is copied from the source datagram 
 *	along with some of the options to the destination datagram.
 *	According to RFC791, only certain options are copied to fragment
 *	headers.
 *
 * Results:
 *	The length in bytes of the copied header.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CopyHeader(srcHdrLen, srcHdrPtr, destHdrPtr)
    int		srcHdrLen;		/* # of bytes in the src header. */
    Net_IPHeader *srcHdrPtr;		/* Ptr to original datagram header. */
    Net_IPHeader *destHdrPtr;		/* Ptr to fragment datagram header. */
{
    register unsigned char *srcPtr;	/* Ptr to the source's options area. */
    register unsigned char *destPtr;	/* Ptr to the dest's options area. */
    int		optionAreaLen;		/* # of bytes of options that have been
					 * copied from the source to dest. */
    int		option;			/* Current option. */
    int		optionLen;		/* Lenght in bytes of current option.*/

    /*
     * First copy the basic IP header that is required for every IP datagram.
     */
    *destHdrPtr = *srcHdrPtr;

    /*
     * Now selectively copy options from the source packet to the dest
     * packet header. The dest packet is a fragment and only some of the
     * options need to be copied to it.
     */
    srcPtr  = (unsigned char *)((Address)srcHdrPtr  + sizeof(Net_IPHeader));
    destPtr = (unsigned char *)((Address)destHdrPtr + sizeof(Net_IPHeader));

    optionAreaLen = srcHdrLen - sizeof(Net_IPHeader);

    for ( ; optionAreaLen > 0; optionAreaLen -= optionLen, srcPtr += optionLen){
	    option = srcPtr[0];
	    if (option == NET_IP_OPT_END_OF_LIST) {
		break;
	    }
	    if (option == NET_IP_OPT_NOP) {
		optionLen = 1;
	    } else {
		optionLen = srcPtr[NET_IP_OPT_LEN_OFFSET];
	    }

	    if (optionLen > optionAreaLen) {
		(void) fprintf(stderr, 
		    "Warning: (IP) CopyOption: bad option length %d\n", 
		    optionLen);
		optionLen = optionAreaLen;
	    }

	    /*
	     * Copy the option if it's supposed to be copied.
	     */
	    if (NET_IP_OPT_COPIED(option)) {
		bcopy((Address)srcPtr, (Address)destPtr, optionLen);
		destPtr += optionLen;
	    }
    }

    /*
     * Pad the options area in the dest. packet with END_OF_LIST until 
     * a 32-bit boundary is reached.
     */
    for (optionLen = destPtr - (unsigned char *)(destHdrPtr+1); 
	     optionLen & 0x3; optionLen++) {
	*destPtr++ = NET_IP_OPT_END_OF_LIST;
    }

    /*
     * Return the true size of the dest.'s datagram header.
     */
    return(sizeof(Net_IPHeader) + optionLen);
}


/*
 *----------------------------------------------------------------------
 *
 * IP_FormatPacket --
 *
 *	Given a partially-formed packet, this routine fills in most
 *	of the fields in the IP header and computes a checksum for
 *	the header. After this routine completes, the packet can be
 *	sent out on the network.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All IP fields in the packet are modified.
 *
 *----------------------------------------------------------------------
 */

void
IP_FormatPacket(protocol, timeToLive, srcAddr, destAddr, packetPtr)
    int			protocol;	/* Assigned protocol #. */
    int			timeToLive;	/* # of seconds before packet should
					 * be discarded. Value depends on
					 * the protocol. */
    Net_InetAddress	srcAddr;	/* One of our IP addresses. */
    Net_InetAddress	destAddr;	/* Where to send the packet. */
    register IPS_Packet *packetPtr;	/* Partially formed packet. */
{
    register Net_IPHeader	*ipPtr;
    static int	ident = 0;

    /*
     * Fill in the IP-specific stuff in the packet.
     */
    packetPtr->ipLen = sizeof(Net_IPHeader);
    if (packetPtr->hdrLen > 0) {
	ipPtr = packetPtr->ipPtr = (Net_IPHeader *) 
		    (((Address)packetPtr->hdr.hdrPtr) - packetPtr->ipLen);
    } else {
	ipPtr = packetPtr->ipPtr = (Net_IPHeader *) 
		    (packetPtr->data - packetPtr->ipLen);
    }

    /*
     * Fill in the IP header and checksum it.
     */
    ipPtr->version	= NET_IP_VERSION;
    ipPtr->headerLen	= sizeof(Net_IPHeader) / 4;
    ipPtr->typeOfService = 0;
    ipPtr->totalLen	= sizeof(Net_IPHeader) + packetPtr->hdrLen + 
				packetPtr->dataLen;
    ipPtr->ident	= Net_HostToNetShort(ident);
    ident += 1;
    ipPtr->fragOffset	= 0;
    ipPtr->flags	= 0;
    ipPtr->timeToLive	= timeToLive;
    ipPtr->protocol	= protocol;
    ipPtr->source	= srcAddr;
    ipPtr->dest		= destAddr;
    ipPtr->checksum	= 0;
    ipPtr->checksum	= Net_InetChecksum(sizeof(Net_IPHeader),(Address)ipPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * TestInputProc --
 *
 *	Debugging code to print the header of an IP datagram.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef DEBUG

static char srcAddr[18];
static char destAddr[18];
static char *ProtNumToName();

/*ARGSUSED*/
static void
TestInputProc(netID, packetPtr)
    Rte_NetID	netID;
    IPS_Packet	*packetPtr;
{
    register Net_IPHeader       *headerPtr;
    unsigned short		checksum;

    headerPtr = packetPtr->ipPtr;

    (void) fflush(stdout);

    (void) Net_InetAddrToString(headerPtr->source, srcAddr);
    (void) Net_InetAddrToString(headerPtr->dest, destAddr);

    (void) printf("IP Packet: size = %d\n", packetPtr->ipLen);
    (void) printf("Protocol, version:	%s, %d\n", 
		    ProtNumToName(headerPtr->protocol),
		    headerPtr->version);
    (void) printf("Src, dest addrs:	%s, %s\n", srcAddr, destAddr);

    (void) printf("Header, total len:	%d, %d\n", 
		    headerPtr->headerLen, headerPtr->totalLen);
    (void) printf("TOS, ttl:		%x, %d\n", 
		    headerPtr->typeOfService, headerPtr->timeToLive);
    checksum = headerPtr->checksum, 
    headerPtr->checksum = 0;
    (void) printf("checksum, recomp:	%x, %x\n", checksum, 
		Net_InetChecksum((int)headerPtr->headerLen*4, 
					(Address)headerPtr));
    (void) printf("Frag flags, offset, ID:	%x, %d, %x\n", 
		    headerPtr->flags, headerPtr->fragOffset, 
		    headerPtr->ident);
    (void) printf("\n");

    (void) fflush(stdout);

    free(packetPtr->base);
    return;
}

static char *
ProtNumToName(num) 
    unsigned char num;
{
    static char buffer[5];

    switch (num) {
	case NET_IP_PROTOCOL_IP:
	    return("IP");
	case NET_IP_PROTOCOL_ICMP:
	    return("ICMP");
	case NET_IP_PROTOCOL_TCP:
	    return("TCP");
	case NET_IP_PROTOCOL_EGP:
	    return("EGP");
	case NET_IP_PROTOCOL_UDP:
	    return("UDP");
	default:
#ifndef KERNEL
	    (void) sprintf(buffer, "%4d", num);
	    return(buffer);
#else
	    return("???");
#endif
    }
}
#endif DEBUG
