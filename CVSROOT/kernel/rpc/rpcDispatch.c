/*
 * rpcDispatch.c --
 *
 * The top level rpc dispatch routine.  The dispatcher is responsible for
 * taking a packet from the packet transport level and passing it to the
 * delivery level of the correct process involved in the RPC.  The
 * top-level dispatcher finds the protocol state for the client or server
 * receiving the message and calls the client or server specific dispatch
 * routine.  This file also has the utility routine to copy a message from
 * the network module's buffers into the stub's buffers.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "dbg.h"
#include "vm.h"
#include "lib.h"

#include "rpc.h"
#include "rpcClient.h"
#include "rpcServer.h"
#include "rpcTrace.h"


/*
 * Our sprite ID.  It is exported for general use by other modules.
 * This is set by a reverse arp transaction at boot time (see Rpc_Start),
 * or if that fails, by the rpc dispatcher who monitors the clientID field of
 * rpc reply messages (see RpcClientDispatch).  Finally, if neither of
 * those hooks work, a diskfull node will set its address by looking at
 * the disk header (see FsAttachDisk).
 *
 * Important: servers won't respond to requests until their rpc_SpriteID is set.
 */
int	rpc_SpriteID = 0;

/*
 * The packet as it looks sitting in the ethernet buffers.
 */
typedef struct RpcRawPacket {
    Net_EtherHdr	etherHdr;
    RpcHdr		rpcHdr;
    /*
     * data follows
     */
} RpcRawPacket;

/*
 * Occasionally the Intel ethernet interface wacks out and passes us garbage.
 * We count garbage packets and reset the interface after a series of junk.
 */
int badErrors = 0;

/*
 * Set to FALSE for debugging a few isolated machines without tons of msgs.
 * Since a few of the errors are printed out every once in a while anyway,
 * we may want to leave this FALSE.
 */
Boolean	rpc_PrintMismatch = FALSE;
/*
 * While testing there may be many version mismatch errors.  If
 * rpc_PrintMismatch is FALSE, then we only report a few of these errors
 * every once in a while.
 */
int mismatchErrors = 0;

/*
 * An array of bitmasks is kept for faster comparisions by the dispatcher.
 * Indexed by the total number of fragments in the packet, the array
 * contains a complete bitmask for that many fragments.
 */
unsigned int rpcCompleteMask[17] = {
	0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
		0x001F, 0x003F, 0x007F, 0x00FF,
		0x01FF, 0x03FF, 0x07FF, 0x0FFF,
		0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };
/*
 * Forward declarations.
 */
void RpcScatter();
int  RpcValidateClient();



/*
 *----------------------------------------------------------------------
 *
 * Rpc_Dispatch --
 *
 *      This does some consistency checks on an incoming packet, finds the
 *      protocol state for the packet (either client or server) and then
 *      calls either the server or client dispatch routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This may drop a packet if various consistency checks fail.
 *
 *----------------------------------------------------------------------
 */

Rpc_Dispatch(packetPtr, packetLength)
    Address	packetPtr;	/* pointer to the packet in the hardware
				 * receive buffers */
    int		packetLength;	/* its size */
{
    register RpcHdr	*rpcHdrPtr;
    register int expectedLength;
    Net_EtherHdr	*etherHdrPtr;

    etherHdrPtr = (Net_EtherHdr *)packetPtr;
    rpcHdrPtr = (RpcHdr *) (packetPtr + sizeof(Net_EtherHdr));

    if (rpcHdrPtr->version == RPC_SWAPPED_VERSION) {
	/*
	 * Byte swap the packet header and the parameter block.
	 */
	if (!RpcByteSwapInComing(rpcHdrPtr)) {
	    printf("Warning: Rpc_Dispatch failed byte-swap.");
	    return;
	}
    } else if (rpcHdrPtr->version != RPC_NATIVE_VERSION) {
	if (rpc_PrintMismatch) {
	    printf(
	    "Warning: Rpc_Dispatch version mismatch: %x not %x from client(?) %d\n",
		rpcHdrPtr->version, RPC_NATIVE_VERSION, rpcHdrPtr->clientID);
	} else {
	    if (mismatchErrors < 5) {
		printf("Warning: %s %x not %x from client(?) %d\n",
		    "Rpc_Dispatch version mismatch:",
		    rpcHdrPtr->version, RPC_NATIVE_VERSION,
		    rpcHdrPtr->clientID);
		mismatchErrors++;
	    } else if (mismatchErrors == 5) {
		printf("Warning: %s %x not %x from client(?) %d\n%s\n",
		    "Rpc_Dispatch version mismatch:",
		    rpcHdrPtr->version, RPC_NATIVE_VERSION,
		    rpcHdrPtr->clientID,
		    "I'll report no more version mismatches for a while.");
		mismatchErrors++;
	    } else {
		mismatchErrors++;
		if (mismatchErrors > 5000) {
		    mismatchErrors = 0;
		}
	    }
	}
	return;
    }
    expectedLength = sizeof(Net_EtherHdr) +
		     sizeof(RpcHdr) +
		     rpcHdrPtr->paramSize +
		     rpcHdrPtr->dataSize;
    if (packetLength < expectedLength) {
	rpcCltStat.shorts++;
	printf("Rpc_Dispatch: SHORT packet, (%d) not (%d) ",
				  packetLength, expectedLength);
	printf("srv %d clt %d rpc %d\n", rpcHdrPtr->serverID,
		    rpcHdrPtr->clientID, rpcHdrPtr->command);
	badErrors++;
	if (badErrors > 4) {
	    printf("Resetting network interface\n");
	    badErrors = 0;
	    Net_Reset();
	}
	return;
    } else if (packetLength > expectedLength &&
	       packetLength > (NET_ETHER_MIN_BYTES)) {
	/*
	 * Short messages (like acks and null replies)
	 * get padded to minimum ethernet length.
	 */
	rpcCltStat.longs++;
    }

    RPC_TRACE(rpcHdrPtr, RPC_INPUT, "Input");

    if (rpcHdrPtr->flags & RPC_SERVER) {
	register RpcServerState *srvPtr;
	/*
	 * Don't do anything if we don't know who we are.  This occurs
	 * at boot time if we get a request before doing reverse ARP.
	 */
	if (rpc_SpriteID == 0) {
	    return;
	}
	if (rpcHdrPtr->serverID != RPC_BROADCAST_SERVER_ID &&
	    rpcHdrPtr->serverID != rpc_SpriteID) {
	    /*
	     * A bug, or the Intel chip is wack-o
	     */
	    if (rpcHdrPtr->serverID > 0 &&
		rpcHdrPtr->serverID < NET_NUM_SPRITE_HOSTS) {
		Net_EtherAddress	dest;
		NET_ETHER_ADDR_COPY(NET_ETHER_HDR_DESTINATION(*etherHdrPtr),
				    dest);
		printf("Warning: Rpc_Dispatch, wrong server ID %d\n");
		printf("\tClient %d rpc %d ether dest %x:%x:%x:%x:%x:%x\n",
		       rpcHdrPtr->clientID, rpcHdrPtr->command,
		       NET_ETHER_ADDR_BYTE1(dest) & 0xff,
		       NET_ETHER_ADDR_BYTE2(dest) & 0xff,
		       NET_ETHER_ADDR_BYTE3(dest) & 0xff,
		       NET_ETHER_ADDR_BYTE4(dest) & 0xff,
		       NET_ETHER_ADDR_BYTE5(dest) & 0xff,
		       NET_ETHER_ADDR_BYTE6(dest) & 0xff);

	    } else {
		printf("Rpc_Dispatch: junk serverID %d from client %d\n",
			    rpcHdrPtr->serverID,
			    rpcHdrPtr->clientID);
		badErrors++;
		if (badErrors > 4) {
		    printf("Resetting network interface\n");
		    badErrors = 0;
		    Net_Reset();
		}
	    }
	    return;
	}

	rpcSrvStat.toServer++;
	/*
	 * Verify or initialize the sprite host id for the client
	 * (clientID) from the transport level source address.
	 * This doesn't usually kick in unless the client can't do reverse arp.
	 */
	if ( ! RpcValidateClient(etherHdrPtr, rpcHdrPtr)) {
	    rpcSrvStat.invClient++;
	    return;
	}
	/*
	 * Save sender's requested interfragment delay.
	 */
	rpcDelay[rpcHdrPtr->clientID] = rpcHdrPtr->delay;
	/*
	 * Match the input message to a server process.
	 */
	srvPtr = RpcServerAlloc(rpcHdrPtr);
	if (srvPtr == (RpcServerState *)NIL) {
	    rpcSrvStat.noAlloc++;
	} else {
	    RpcServerDispatch(srvPtr, rpcHdrPtr);
	}
    } else {
	/*
	 * Get the channel for the packet.
	 */
	register RpcClientChannel *chanPtr;

	rpcCltStat.toClient++;
	if (rpcHdrPtr->channel < 0 ||
	    rpcHdrPtr->channel >= rpcNumChannels) {
	    rpcCltStat.badChannel++;
	    printf("Rpc_Dispatch: bad channel %d from clt %d rpc %d",
	       rpcHdrPtr->channel, rpcHdrPtr->clientID, rpcHdrPtr->command);
	    badErrors++;
	    if (badErrors > 4) {
		printf("Resetting network interface\n");
		badErrors = 0;
		Net_Reset();
	    }
	} else {
	    /*
	     * Save sender's requested interfragment delay,
	     * then dispatch to client process.
	     */
	    if (rpcHdrPtr->serverID < NET_NUM_SPRITE_HOSTS &&
		rpcHdrPtr->serverID > 0) {
		rpcDelay[rpcHdrPtr->serverID] = rpcHdrPtr->delay;
	    }
	    chanPtr = rpcChannelPtrPtr[rpcHdrPtr->channel];
	    RpcClientDispatch(chanPtr, rpcHdrPtr);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * RpcScatter --
 *
 *	Copy the data in the network buffers into the buffers
 *	specified by the scatter vector.  This routine only works
 *	on RPC messages.  It knows the RPC packet format:
 *	  The Rpc header, which includes the sizes of the next two parts.
 *	  The parameter area.
 *	  The data area.
 *	The scatter vector lengths of the last two parts are used
 *	as maximum buffer sizes.  The actual sizes of the parts is
 *	taken from the rpc header.  This is done because the RPC system
 *	preallocates buffers which are large enough to handle any message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The copy.
 *
 *----------------------------------------------------------------------
 */
void
RpcScatter(rpcHdrPtr, bufferPtr)
    register RpcHdr *rpcHdrPtr;		/* The Rpc Header as it sits in the
					 * network's buffer.  The data follows
					 * the header directly. */
    RpcBufferSet *bufferPtr;		/* Specification of the buffers to
					 * copy the message to. */
{
    register Address netBufPtr;		/* A pointer in to network buffer */
    register int length;		/* Copying length */
    int destLength;			/* length of destination buffers */

    netBufPtr = (Address)rpcHdrPtr;

    /*
     * Copy the RPC header.
     */
    length = bufferPtr->rpcHdrBuffer.length;
    bcopy(netBufPtr, bufferPtr->rpcHdrBuffer.bufAddr, length);
    netBufPtr += length;

    /*
     * Copy the parameter and data areas.  Their sizes are in
     * the RPC header.  Complain if either area is too large.
     */
    length = rpcHdrPtr->paramSize;
    if (length != 0) {
	destLength = bufferPtr->paramBuffer.length;
	if (length + rpcHdrPtr->paramOffset > destLength) {
	    rpcCltStat.paramOverrun++;
	    printf("RpcScatter: rpc %d param size + off (%d + %d) > (%d)\n",
			   rpcHdrPtr->command, length, rpcHdrPtr->paramOffset,
			   destLength);
	    if (rpcHdrPtr->paramOffset < destLength) {
		length = destLength - rpcHdrPtr->paramOffset;
	    } else {
		length = 0;
	    }
	}
	bcopy(netBufPtr, bufferPtr->paramBuffer.bufAddr +
				     rpcHdrPtr->paramOffset, length);
	netBufPtr += rpcHdrPtr->paramSize;
    }

    length = rpcHdrPtr->dataSize;
    if (length != 0) {
	destLength = bufferPtr->dataBuffer.length;
	if (length + rpcHdrPtr->dataOffset > destLength) {
	/*
	 * The returned data is more than we expect.  One reason for
	 * this is that the Intel driver limits the size of the data area and
	 * parameter area to be 0, or to be greater than 12 bytes because
	 * of the Intel DMA hardware.
	 */
	    rpcCltStat.dataOverrun++;
	    if (rpcHdrPtr->dataOffset < destLength) {
		length = destLength - rpcHdrPtr->dataOffset;
	    } else {
		length = 0;
	    }
	}
	bcopy(netBufPtr, bufferPtr->dataBuffer.bufAddr + rpcHdrPtr->dataOffset,
		length);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpcValidateClient --
 *
 *      Check the clientID field of an incoming Rpc request.  Invalid ID's
 *      are screened out, and the special clientID of zero is overwritten
 *      with the client's true sprite ID.  This is done by looking the
 *      client's physical address up in a table with Net_AddrToID.  This
 *      true sprite ID is propogated back to clients so they can determine
 *      their own sprite ID.  This should be replaced by the use of
 *	Reverse Arp on clients at boot time.
 *
 * Results:
 *      FALSE if clientID is invalid or the physical address of the client
 *      isn't in the table.  TRUE otherwise - this gurantees a valid
 *      clientID in the RpcHdr.
 *
 * Side effects:
 *      Muck with the header so that subsequent users of the RpcHdr see a
 *      valid clientID field.
 *
 *----------------------------------------------------------------------
 */
Boolean
RpcValidateClient(etherHdrPtr, rpcHdrPtr)
    Net_EtherHdr *etherHdrPtr;
    RpcHdr 	 *rpcHdrPtr;
{
    register int		clientID;
    register Boolean		result = FALSE;

    clientID = rpcHdrPtr->clientID;

    if (clientID > 0 && clientID < NET_NUM_SPRITE_HOSTS) {
	/*
	 * A potentially valid clientID.  We don't need to
	 * save the client's transport address because the
	 * we can get that from the saved request message.
	 */
	result = TRUE;
    } else if (clientID == 0) {
	Net_EtherAddress	source;
	/*
	 * Look client's transport address up in our in core host table.
	 */
	NET_ETHER_ADDR_COPY(NET_ETHER_HDR_SOURCE(*etherHdrPtr), source);
	clientID = Net_AddrToID(0, NET_ROUTE_ETHER, (ClientData) &source);
        printf("Warning: RpcValidateClient had to set clientID %d\n", clientID);
	if (clientID < 0) {
	    /*
	     * Should invoke Reverse ARP to find out the Sprite ID.
	     */
	    printf("Client at unknown ethernet address %x:%x:%x:%x:%x:%x\n",
		       NET_ETHER_ADDR_BYTE1(source) & 0xff,
		       NET_ETHER_ADDR_BYTE2(source) & 0xff,
		       NET_ETHER_ADDR_BYTE3(source) & 0xff,
		       NET_ETHER_ADDR_BYTE4(source) & 0xff,
		       NET_ETHER_ADDR_BYTE5(source) & 0xff,
		       NET_ETHER_ADDR_BYTE6(source) & 0xff);
	    result = FALSE;
	} else {
	    rpcHdrPtr->clientID = clientID;
	    result = TRUE;
	}
    } else {
	result = FALSE;
	printf("Invalid Client Sprite ID (%d)\n", clientID);
    }
    return(result);
}
