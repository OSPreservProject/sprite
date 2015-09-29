/* 
 * sockBuf.c --
 *
 *	This file contains routines to manage the send and receive buffers
 *	of a socket. The basic buffer operations are alloc, append, fetch, 
 *	copy and remove. A buffer consists of a linked list of Sock_BufDataInfo
 *	elements. Each element contains an area of memory with data. 
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/sockBuf.c,v 1.6 91/04/23 23:55:35 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ipServer.h"
#include "stat.h"
#include "socket.h"
#include "sockInt.h"

#include "list.h"

/*
 * CheckList is a debugging routine to make sure the 
 */
static void CheckList();



/*
 *----------------------------------------------------------------------
 *
 * Sock_BufFetch --
 *
 *	Copies data from the front of a socket's receive buffer to a user's 
 *	buffer. Depending on the flags, the data may or may not be 
 *	removed from the buffer. The address of the sender of the data
 *	is also available.
 *
 * Results:
 *	SUCCESS		- the data was obtained from the buffer.
 *	FS_WOULD_BLOCK	- there's no data in the buffer.
 *
 * Side effects:
 *	Memory in a buffer element may be freed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sock_BufFetch(sockPtr, flags, bufSize, buffer, amtCopiedPtr, addressPtr)
    Sock_SharedInfo	*sockPtr;	/* Socket of interest. */
    int			flags;		/* Type of operation: OR of
					 * SOCK_BUF_STREAM, SOCK_BUF_PEEK. */
    register int	bufSize;	/* How much data to get. */
    Address		buffer;		/* Place to copy it to. */
    int			*amtCopiedPtr;	/* Actual amount copied. */
    Net_InetSocketAddr	*addressPtr;	/* Socket address of sender of the
					 * data. */
{
    register Sock_BufInfo	*bufInfoPtr;
    register Sock_BufDataInfo   *dataPtr;
    register int		dataLen;
    List_Links			*nextPtr;
    int				amtCopied;

    bufInfoPtr = &sockPtr->recvBuf;

    *amtCopiedPtr = 0;

    if (bufSize == 0) {
	return(SUCCESS);
    }

    stats.sock.buffer.fetch++;

    /*
     * Check if there's any data to return to the client.
     */
    if (List_IsEmpty(&bufInfoPtr->links)) {
	if (Sock_IsRecvStopped(sockPtr)) {
	    return(SUCCESS);
	} else {
	    return(FS_WOULD_BLOCK);
	}
    }

    amtCopied = 0;
    if (flags & SOCK_BUF_STREAM) {

	/*
	 * Go through the list of buffers and copy the data to the
	 * output area. If the PEEK flag is not set, remove the
	 * buffer from the list.
	 */
	(List_Links *) dataPtr = List_First(&bufInfoPtr->links);
	while (!List_IsAtEnd(&bufInfoPtr->links, (List_Links *) dataPtr)) {

	    dataLen = dataPtr->len;
	    if (dataLen <= bufSize) {

		bcopy(dataPtr->bufPtr, buffer, dataLen);
		amtCopied	+= dataLen;
		buffer		+= dataLen;
		bufSize		-= dataLen;

		nextPtr = List_Next((List_Links *) dataPtr);
		if (!(flags & SOCK_BUF_PEEK)) {
		    bufInfoPtr->size -= dataLen;
		    List_Remove((List_Links *) dataPtr);
		    free(dataPtr->base);
		    free((char *) dataPtr);
		}
		(List_Links *) dataPtr = nextPtr;

		if (bufSize == 0) {
		    break;
		}
	    } else {
		/*
		 * Take some of the data from this buffer. If the
		 * PEEK flag is not set, adjust the size and start
		 * to reflect the amount of data read.
		 */
		bcopy(dataPtr->bufPtr, buffer, bufSize);
		amtCopied	+= bufSize;
		if (!(flags & SOCK_BUF_PEEK)) {
		    bufInfoPtr->size	-= bufSize;
		    dataPtr->len	-= bufSize;
		    dataPtr->bufPtr	+= bufSize;
		}
		break;
	    }
	}
    } else {

	/*
	 * Record-oriented read. 
	 */

	(List_Links *) dataPtr = List_First(&bufInfoPtr->links);

	/*
	 * If the client's buffer is too small, they won't get the whole
	 * packet.
	 */
	if (bufSize < dataPtr->len) {
	    amtCopied = bufSize;
	} else {
	    amtCopied = dataPtr->len;
	}

	bcopy(dataPtr->bufPtr, buffer, amtCopied);

	/*
	 * Save the address of the sender in case the client wants to
	 * get it later.
	 */
	if (addressPtr != (Net_InetSocketAddr *) NULL) {
	    *addressPtr = dataPtr->address;
	}

	/*
	 * If the PEEK flag is not set, remove the packet from the list.
	 */
	if (!(flags & SOCK_BUF_PEEK)) {
	    bufInfoPtr->size -= dataPtr->len;
	    List_Remove((List_Links *) dataPtr);
	    free(dataPtr->base);
	    free((char *) dataPtr);
	}
    }
    *amtCopiedPtr = amtCopied;
    CheckList(bufInfoPtr);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_BufAppend --
 *
 *	Appends data to the end of a socket's send or receive buffer.
 *
 * Results:
 *	SUCCESS		- all of the data were appended to the buffer.
 *	FS_WOULD_BLOCK	- none or some of the data were appended.
 *
 * Side effects:
 *	Memory for the list element is allocated.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sock_BufAppend(sockPtr, type, atomic, dataLen, data, base, addrPtr, 
	amtAppendedPtr)
    Sock_SharedInfo	*sockPtr;	/* Socket of interest. */
    Sock_BufType	type;		/* Indicates RECV or SEND buffer. */
    register int	dataLen;	/* Amount to append. */
    Boolean		atomic;		/* If TRUE, don't append if can't fit
					 * the whole packet. */
    Address		data;		/* Buffer holding data. */
    Address		base;		/* Base address of data. */
    Net_InetSocketAddr	*addrPtr;	/* Sender of the data. */
    int			*amtAppendedPtr; /* Actual amount of data appended to
					  * the buffer. */
{
    register Sock_BufDataInfo	*dataPtr;
    register Sock_BufInfo	*bufInfoPtr;
    ReturnStatus		status;
    int				freeSpace;

    if (type == SOCK_RECV_BUF) {
	bufInfoPtr = &sockPtr->recvBuf;
    } else {
	bufInfoPtr = &sockPtr->sendBuf;
    }

    stats.sock.buffer.append++;

    /*
     * If there's not enough room in the buffer to hold all of the data,
     * take as much as possible.  FS_WOULD_BLOCK is returned if we don't
     * accept all the data so the Sprite kernel can make our client
     * wait properly.
     */

    freeSpace = bufInfoPtr->maxSize - bufInfoPtr->size;
    if (dataLen > freeSpace) {
	if ((freeSpace == 0) || atomic) {
	    if (amtAppendedPtr != (int *) NULL) {
		*amtAppendedPtr = 0;
	    }
	    stats.sock.buffer.appendFail++;
	    return(FS_WOULD_BLOCK);
	} else {
	    stats.sock.buffer.appendPartial++;
	    stats.sock.buffer.appPartBytes += dataLen;
	    dataLen = freeSpace;
	    status  = FS_WOULD_BLOCK;
	}
    } else {
	status = SUCCESS;
    }

    dataPtr = (Sock_BufDataInfo *) malloc(sizeof(Sock_BufDataInfo));
    dataPtr->bufPtr	= data;
    dataPtr->len	= dataLen;
    dataPtr->base	= base;
    if (addrPtr != (Net_InetSocketAddr *) NULL) {
	dataPtr->address = *addrPtr;
    }
    List_InitElement((List_Links *) dataPtr);
    List_Insert((List_Links *) dataPtr, LIST_ATREAR(&bufInfoPtr->links));

    bufInfoPtr->size += dataLen;
    if (amtAppendedPtr != (int *) NULL) {
	*amtAppendedPtr = dataLen;
    }

    /*
     * Make sure the list is not corrupted.
     */
    CheckList(bufInfoPtr);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_BufRemove --
 *
 *	Removes a specific amount of data from the front of a buffer.
 *	A special value for the amount removes all data from the buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory for the data and list elements is freed.
 *
 *----------------------------------------------------------------------
 */

void
Sock_BufRemove(sockPtr, type, amount)
    Sock_SharedInfo	*sockPtr;	/* Socket of interest. */
    Sock_BufType	type;		/* Indicates RECV or SEND buffer. */
    register int	amount;		/* Amount to remove. -1 means all. */
{
    register Sock_BufDataInfo	*dataPtr;
    register Sock_BufInfo	*bufInfoPtr;
    List_Links			*nextPtr;

    if (type == SOCK_RECV_BUF) {
	bufInfoPtr = &sockPtr->recvBuf;
    } else {
	bufInfoPtr = &sockPtr->sendBuf;
    }

    stats.sock.buffer.remove++;

    /*
     * An amount of -1 means to remove all data from the buffer.
     */
    if (amount == -1) {
	amount = bufInfoPtr->size;
	bufInfoPtr->size = 0;
    } else if (amount < 0) {
	panic("Sock_BufRemove: amount < 0 (%d)\n", amount);
	return;
    } else {
	bufInfoPtr->size -= amount;
	if (bufInfoPtr->size < 0) {
	    panic(
		"Sock_BufRemove: tried to remove too much (%d)\n", amount);
	    return;
	}
    }

    dataPtr = (Sock_BufDataInfo *) List_First(&bufInfoPtr->links);
    while (!List_IsAtEnd(&bufInfoPtr->links, (List_Links *) dataPtr)) {

	if (amount >= dataPtr->len) {
	    amount -= dataPtr->len;
	    nextPtr = List_Next((List_Links *) dataPtr);
	    List_Remove((List_Links *) dataPtr);
	    free(dataPtr->base);
	    free((char *) dataPtr);
	    if (amount == 0) {
		break;
	    }
	    dataPtr = (Sock_BufDataInfo *) nextPtr;
	} else {
	    /*
	     * The buffer is larger than the amount to be removed.
	     * Just adjust the size and start to reflect the amount removed.
	     */
	    dataPtr->len    -= amount;
	    dataPtr->bufPtr += amount;
	    amount = 0;
	    break;
	}
    }
    if (amount > 0) {
	panic("Sock_BufRemove: amount > 0 (%d)\n", amount);
    }
    CheckList(bufInfoPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_BufCopy --
 *
 *	Copies data from a socket buffer starting from a given offset to
 *	user's buffer.
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
Sock_BufCopy(sockPtr, type, offset, bufSize, buffer)
    Sock_SharedInfo	*sockPtr;	/* Socket of interest. */
    Sock_BufType	type;		/* Indicates RECV or SEND buffer. */
    register int	offset;		/* Offset from beginning of buffer
					 * to start copying. */
    int			bufSize;	/* How much to copy. */
    Address		buffer;		/* Buffer to hold the data. */
{
    register Sock_BufDataInfo	*dataPtr;
    register Sock_BufInfo	*bufInfoPtr;
    register int		dataLen;
    Address			data;

    if (offset < 0) {
	panic("Sock_BufCopy: negative offset: %d\n", offset);
	return;
    }

    stats.sock.buffer.copy++;

    if (type == SOCK_RECV_BUF) {
	bufInfoPtr = &sockPtr->recvBuf;
    } else {
	bufInfoPtr = &sockPtr->sendBuf;
    }

    LIST_FORALL(&bufInfoPtr->links, (List_Links *) dataPtr) {
	data    = dataPtr->bufPtr;
	dataLen = dataPtr->len;

	if (offset > 0) {
	    if (offset >= dataLen) {
		/*
		 * The data segment ends before the offset.
		 */
		offset -= dataLen;
		continue;
	    } else {
		/*
		 * We want to start copying inside this data segment. 
		 */
		data 	+= offset;
		dataLen	-= offset;
		offset = 0;
	    }
	} 
	if (dataLen > bufSize) {
	    dataLen = bufSize;
	} 
	stats.sock.buffer.copyBytes += dataLen;
	bcopy(data, buffer, dataLen );
	buffer	+= dataLen;
	bufSize	-= dataLen;
	if (bufSize == 0) {
	    break;
	}
    }
    CheckList(bufInfoPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_BufAlloc --
 *
 *	Sets the maximum number of bytes that a buffer can hold
 *	initializes the data list. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The data list is initialized.
 *
 *----------------------------------------------------------------------
 */

void
Sock_BufAlloc(sockPtr, type, maxSize)
    Sock_SharedInfo	*sockPtr;	/* Socket of interest. */
    Sock_BufType	type;		/* Indicates RECV or SEND buffer. */
    int			maxSize;	/* Maximum # of bytes allowed. */
{
    if (type == SOCK_RECV_BUF) {
	sockPtr->recvBuf.maxSize = maxSize;
	List_Init(&sockPtr->recvBuf.links);
    } else {
	sockPtr->sendBuf.maxSize = maxSize;
	List_Init(&sockPtr->sendBuf.links);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_BufSize --
 *
 *	Returns the the values of the various size parameters of a buffer.
 *
 * Results:
 *	The size value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_BufSize(sockPtr, type, request)
    Sock_SharedInfo	*sockPtr;	/* Socket of interest. */
    Sock_BufType	type;		/* Indicates RECV or SEND buffer. */
    Sock_BufRequest	request;	/* Type of value wanted. */
{
    register Sock_BufInfo	*bufInfoPtr;
    int size = 0;

    if (type == SOCK_RECV_BUF) {
	bufInfoPtr = &sockPtr->recvBuf;
    } else {
	bufInfoPtr = &sockPtr->sendBuf;
    }

    switch (request) {
	case SOCK_BUF_USED:
	    size = bufInfoPtr->size;
	    break;

	case SOCK_BUF_FREE:
	    size = bufInfoPtr->maxSize - bufInfoPtr->size;
	    break;

	case SOCK_BUF_MAX_SIZE:
	    size = bufInfoPtr->maxSize;
	    break;

	default:
	    panic("Sock_BufSize: unknown request %d\n", request);
	    break;
    }
    return(size);
}


/*
 *----------------------------------------------------------------------
 *
 * CheckList --
 *
 *	A debugging routine to make sure the buffer size agrees with
 *	the sum of the data lengths in the buffer's queue.
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
CheckList(bufInfoPtr)
    register Sock_BufInfo	*bufInfoPtr;
{
    register Sock_BufDataInfo	*dataPtr;
    register int size = 0;

    LIST_FORALL(&bufInfoPtr->links, (List_Links *) dataPtr) {
	size += dataPtr->len;
    }

    if (size != bufInfoPtr->size) {
	(void) panic("CheckList: size mismatch: sum = %d, list = %d\n",
		size, bufInfoPtr->size);
    }
}
