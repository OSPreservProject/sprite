/* 
 * rpcByteSwap.c --
 *
 *	The code to byte swap incoming rcp headers and parameter blocks.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"
#include "rpcInt.h"
#include "rpcPacket.h"

/*
 * Global var for testing byte-swapping.  Causes out-going messages to
 * be byte-swapped, so that the receiving machine must byte-swap them if
 * it is of the same sort.
 */
Boolean	rpcTestByteSwap = FALSE;
/*
 * Should move to rpcInt.h.
 */
extern	Boolean rpcPrintDebug;


/*
 *----------------------------------------------------------------------
 *
 * RpcByteSwapBuffer --
 *
 *	Byte swap the given buffer integer-by-integer.
 *	Byte swapping means taking an integer of the form 0x61626364 and
 *	turning it into 0x64636261.
 *
 * Results:
 *	None. 
 *
 * Side effects:
 *	The buffer is byte-swapped.
 *
 *----------------------------------------------------------------------
 */
void
RpcByteSwapBuffer(bufferPtr, numInts)
    int	*bufferPtr;
    int	numInts;
{
    int	i;
    int	offendingInt;
    int	fixedInt;
    int	shiftInt;
    int	*toSwap;
#   define	LOW_BYTE_MASK		0x000000ff
#   define	MIDDLE_LOW_BYTE_MASK	0x0000ff00
#   define	MIDDLE_HIGH_BYTE_MASK	0x00ff0000
#   define	HIGH_BYTE_MASK		0xff000000

    if (rpcTestByteSwap && rpcPrintDebug) {
	Sys_Printf("%s %x %s %d\n", "RpcByteSwapBuffer - called with addr(X) =",
	bufferPtr, "numInts(D) =", numInts);
    }
#ifdef NOTDEF
    if (((int) bufferPtr) % 4 != 0) {
	Sys_Panic(SYS_WARNING,
		"RpcByteSwapBuffer: Rpc header not long-word aligned!");
    }
#endif NOTDEF
    for (i = 0; i < numInts; i++) {
	/*
	 * First move the middle high byte to the middle low byte.
	 * Then move the middle low byte to the middle high byte.
	 * Then move the high byte to the low, and vice versa.
	 */
	offendingInt = ((int *) bufferPtr)[i];
	fixedInt = 0;

	shiftInt = offendingInt >> 8;
	shiftInt &= MIDDLE_LOW_BYTE_MASK;
	fixedInt |= shiftInt;

	shiftInt = offendingInt << 8;
	shiftInt &= MIDDLE_HIGH_BYTE_MASK;
	fixedInt |= shiftInt;

	shiftInt = offendingInt >> 24;
	shiftInt &= LOW_BYTE_MASK;
	fixedInt |= shiftInt;

	shiftInt = offendingInt << 24;
	shiftInt &= HIGH_BYTE_MASK;
	fixedInt |= shiftInt;

	((int *) bufferPtr)[i] = fixedInt;
    }
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * RpcByteSwapInComing --
 *
 *	Byte swap the in-coming rpc packet header and the following parameter
 *	block, integer-by-integer.  The data block is not byte swapped.
 *	It knows the RPC packet format:
 *	    The Rpc header, which includes the sizes of the next two parts.
 *	    The parameter area.
 *	    The data area.
 *	Byte swapping means taking an integer of the form 0x61626364 and turning
 *	it into 0x64636261.
 *
 * Results:
 *	TRUE if successful, FALSE if not.  Maybe this should require a more
 *	interesting set of return values, but Rpc_Dispatch just drops the
 *	packet if this routine returns FALSE.
 *
 * Side effects:
 *	The header packet structure and the parameter block are byte-swapped.
 *
 *----------------------------------------------------------------------
 */
Boolean
RpcByteSwapInComing(rpcHdrPtr)
    RpcHdr	*rpcHdrPtr;		/* The Rpc Header as it sits in the
					 * network's buffer.  The data follows
					 * the header directly. */
{
    int	numInts;
    int	paramSize;
	
    /*
     * First byte-swap the rpc header itself.  The header had better be an
     * integral number of integers long!
     */
    numInts = sizeof (RpcHdr) / sizeof (int);
    if ((sizeof (RpcHdr) % sizeof (int)) != 0) {
	return FALSE;
    }
    RpcByteSwapBuffer((int *) rpcHdrPtr, numInts);

    /* now that we've got the right values */
    paramSize = rpcHdrPtr->paramSize;

    /*
     * Now byte-swap the parameter block.  If it isn't an integral number of
     * integers long, something is wrong.
     */
    numInts = paramSize / sizeof (int);
    if ((paramSize % sizeof (int)) != 0) {
	return FALSE;
    }
    /*
     * We don't add on paramOffset here, since the parameter area, even if
     * it's a fragment, immediately follows the rpcHdr for the fragment.
     */
    RpcByteSwapBuffer((int *) (((char *) rpcHdrPtr) + sizeof (RpcHdr)),
	    numInts);

    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * RpcPrintHdr --
 *
 *	Print out fields from the given header.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sys_Printf is invoked.
 *
 *----------------------------------------------------------------------
 */
void
RpcPrintHdr(rpcHdrPtr)
    RpcHdr	*rpcHdrPtr;		/* The Rpc Header as it sits in the
					 * network's buffer.  The data follows
					 * the header directly. */
{

	Sys_Printf("\tversion(X): %x\n\tflags(X): %x\n\tclientID(D): %d\n",
	    rpcHdrPtr->version, rpcHdrPtr->flags, rpcHdrPtr->clientID);
        Sys_Printf("\tcommand(D): %d\n\tparamSize(X): %x\n", rpcHdrPtr->command,
	    rpcHdrPtr->paramSize);
        Sys_Printf("\tparamOffset(X): %x\n", rpcHdrPtr->paramOffset);
	return;
}

int
RpcSetPrintDebug()
{
    rpcPrintDebug = 1;
    return 0;
}

int
RpcUnsetPrintDebug()
{
    rpcPrintDebug = 0;
    return 0;
}

int
RpcSetTestByteSwap()
{
    rpcTestByteSwap = 1;
    return 0;
}

int
RpcUnsetTestByteSwap()
{
    rpcTestByteSwap = 0;
    return 0;
}
    
