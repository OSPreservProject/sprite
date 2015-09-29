/*-
 * buf.c --
 *	Functions for automatically-expanded buffers.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: buf.c,v 1.2 88/09/08 18:16:18 ouster Exp $ SPRITE (Berkeley)";
#endif lint

#include <bstring.h>
#include <stdlib.h>
#include    "buf.h"

typedef struct {
    int	    size; 	/* Current size of the buffer */
    Byte    *buffer;	/* The buffer itself */
    Byte    *inPtr;	/* Place to write to */
    Byte    *outPtr;	/* Place to read from */
} Buf, *BufPtr;

#ifndef max
#define max(a,b)  ((a) > (b) ? (a) : (b))
#endif

/*
 * BufExpand --
 * 	Expand the given buffer to hold the given number of additional
 *	bytes.
 */
#define BufExpand(bp,nb) \
 	if (((bp)->size - ((bp)->inPtr - (bp)->buffer)) < (nb)) {\
	    int newSize = (bp)->size + max((nb),BUF_ADD_INC); \
	    Byte  *newBuf = (Byte *) malloc ((unsigned) newSize); \
	    \
	    bcopy ((bp)->outPtr, newBuf, (bp)->inPtr - (bp)->outPtr); \
	    (bp)->inPtr = newBuf + ((bp)->inPtr - (bp)->outPtr); \
	    (bp)->outPtr = newBuf;\
	    free ((char *) (bp)->buffer);\
	    (bp)->buffer = newBuf;\
	    (bp)->size = newSize;\
	}

#define BUF_DEF_SIZE	256 	/* Default buffer size */
#define BUF_ADD_INC	256 	/* Expansion increment when Adding */
#define BUF_UNGET_INC	16  	/* Expansion increment when Ungetting */

/*-
 *-----------------------------------------------------------------------
 * Buf_AddByte --
 *	Add a single byte to the buffer.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The buffer may be expanded.
 *
 *-----------------------------------------------------------------------
 */
void
Buf_AddByte (buf, byte)
    Buffer  buf;
    Byte    byte;
{
    register BufPtr  bp = (BufPtr) buf;

    BufExpand (bp, 1);

    *bp->inPtr = byte;
    bp->inPtr += 1;
}

/*-
 *-----------------------------------------------------------------------
 * Buf_AddBytes --
 *	Add a number of bytes to the buffer.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Guess what?
 *
 *-----------------------------------------------------------------------
 */
void
Buf_AddBytes (buf, numBytes, bytesPtr)
    Buffer  buf;
    int	    numBytes;
    Byte    *bytesPtr;
{
    register BufPtr  bp = (BufPtr) buf;

    BufExpand (bp, numBytes);

    bcopy (bytesPtr, bp->inPtr, numBytes);
    bp->inPtr += numBytes;
}

/*-
 *-----------------------------------------------------------------------
 * Buf_UngetByte --
 *	Place the byte back at the beginning of the buffer.
 *
 * Results:
 *	SUCCESS if the byte was added ok. FAILURE if not.
 *
 * Side Effects:
 *	The byte is stuffed in the buffer and outPtr is decremented.
 *
 *-----------------------------------------------------------------------
 */
void
Buf_UngetByte (buf, byte)
    Buffer  buf;
    Byte    byte;
{
    register BufPtr	bp = (BufPtr) buf;

    if (bp->outPtr != bp->buffer) {
	bp->outPtr -= 1;
	*bp->outPtr = byte;
    } else if (bp->outPtr == bp->inPtr) {
	*bp->inPtr = byte;
	bp->inPtr += 1;
    } else {
	/*
	 * Yech. have to expand the buffer to stuff this thing in.
	 * We use a different expansion constant because people don't
	 * usually push back many bytes when they're doing it a byte at
	 * a time...
	 */
	int 	  numBytes = bp->inPtr - bp->outPtr;
	Byte	  *newBuf;

	newBuf = (Byte *) malloc ((unsigned) (bp->size + BUF_UNGET_INC));
	bcopy ((char *) bp->outPtr, (char *) (newBuf+BUF_UNGET_INC), numBytes);
	bp->outPtr = newBuf + BUF_UNGET_INC;
	bp->inPtr = bp->outPtr + numBytes;
	free ((char *)bp->buffer);
	bp->buffer = newBuf;
	bp->size += BUF_UNGET_INC;
	bp->outPtr -= 1;
	*bp->outPtr = byte;
    }
}

/*-
 *-----------------------------------------------------------------------
 * Buf_UngetBytes --
 *	Push back a series of bytes at the beginning of the buffer.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	outPtr is decremented and the bytes copied into the buffer.
 *
 *-----------------------------------------------------------------------
 */
void
Buf_UngetBytes (buf, numBytes, bytesPtr)
    Buffer  buf;
    int	    numBytes;
    Byte    *bytesPtr;
{
    register BufPtr	bp = (BufPtr) buf;

    if (bp->outPtr - bp->buffer >= numBytes) {
	bp->outPtr -= numBytes;
	bcopy ((char *) bytesPtr, (char *) bp->outPtr, numBytes);
    } else if (bp->outPtr == bp->inPtr) {
	Buf_AddBytes (buf, numBytes, bytesPtr);
    } else {
	int 	  curNumBytes = bp->inPtr - bp->outPtr;
	Byte	  *newBuf;
	int 	  newBytes = max(numBytes,BUF_UNGET_INC);

	newBuf = (Byte *) malloc ((unsigned) (bp->size + newBytes));
	bcopy((char *) bp->outPtr, (char *) (newBuf+newBytes), curNumBytes);
	bp->outPtr = newBuf + newBytes;
	bp->inPtr = bp->outPtr + curNumBytes;
	free ((char *)bp->buffer);
	bp->buffer = newBuf;
	bp->size += newBytes;
	bp->outPtr -= numBytes;
	bcopy((char *) bytesPtr, (char *) bp->outPtr, numBytes);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Buf_GetByte --
 *	Return the next byte from the buffer. Actually returns an integer.
 *
 * Results:
 *	Returns BUF_ERROR if there's no byte in the buffer, or the byte
 *	itself if there is one.
 *
 * Side Effects:
 *	outPtr is incremented and both outPtr and inPtr will be reset if
 *	the buffer is emptied.
 *
 *-----------------------------------------------------------------------
 */
int
Buf_GetByte (buf)
    Buffer  buf;
{
    BufPtr  bp = (BufPtr) buf;
    int	    res;

    if (bp->inPtr == bp->outPtr) {
	return (BUF_ERROR);
    } else {
	res = (int) *bp->outPtr;
	bp->outPtr += 1;
	if (bp->outPtr == bp->inPtr) {
	    bp->outPtr = bp->inPtr = bp->buffer;
	}
	return (res);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Buf_GetBytes --
 *	Extract a number of bytes from the buffer.
 *
 * Results:
 *	The number of bytes gotten.
 *
 * Side Effects:
 *	The passed array is overwritten.
 *
 *-----------------------------------------------------------------------
 */
int
Buf_GetBytes (buf, numBytes, bytesPtr)
    Buffer  buf;
    int	    numBytes;
    Byte    *bytesPtr;
{
    BufPtr  bp = (BufPtr) buf;
    
    if (bp->inPtr - bp->outPtr < numBytes) {
	numBytes = bp->inPtr - bp->outPtr;
    }
    bcopy ((char *) bp->outPtr, (char *) bytesPtr, numBytes);
    bp->outPtr += numBytes;

    if (bp->outPtr == bp->inPtr) {
	bp->outPtr = bp->inPtr = bp->buffer;
    }
    return (numBytes);
}

/*-
 *-----------------------------------------------------------------------
 * Buf_GetAll --
 *	Get all the available data at once.
 *
 * Results:
 *	A pointer to the data and the number of bytes available.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Byte *
Buf_GetAll (buf, numBytesPtr)
    Buffer  buf;
    int	    *numBytesPtr;
{
    BufPtr  bp = (BufPtr)buf;

    if (numBytesPtr != (int *)NULL) {
	*numBytesPtr = bp->inPtr - bp->outPtr;
    }
    
    return (bp->outPtr);
}

/*-
 *-----------------------------------------------------------------------
 * Buf_Discard --
 *	Throw away bytes in a buffer.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The bytes are discarded. 
 *
 *-----------------------------------------------------------------------
 */
void
Buf_Discard (buf, numBytes)
    Buffer  buf;
    int	    numBytes;
{
    register BufPtr	bp = (BufPtr) buf;

    if (bp->inPtr - bp->outPtr <= numBytes) {
	bp->inPtr = bp->outPtr = bp->buffer;
    } else {
	bp->outPtr += numBytes;
    }
}

/*-
 *-----------------------------------------------------------------------
 * Buf_Size --
 *	Returns the number of bytes in the given buffer.
 *
 * Results:
 *	The number of bytes.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
Buf_Size (buf)
    Buffer  buf;
{
    return (((BufPtr)buf)->inPtr - ((BufPtr)buf)->outPtr);
}

/*-
 *-----------------------------------------------------------------------
 * Buf_Init --
 *	Initialize a buffer. If no initial size is given, a reasonable
 *	default is used.
 *
 * Results:
 *	A buffer to be given to other functions in this library.
 *
 * Side Effects:
 *	The buffer is created, the space allocated and pointers
 *	initialized.
 *
 *-----------------------------------------------------------------------
 */
Buffer
Buf_Init (size)
    int	    size; 	/* Initial size for the buffer */
{
    BufPtr  bp;	  	/* New Buffer */

    bp = (Buf *) malloc(sizeof(Buf));

    if (size <= 0) {
	size = BUF_DEF_SIZE;
    }
    bp->size = size;
    bp->buffer = (Byte *) malloc ((unsigned) size);
    bp->inPtr = bp->outPtr = bp->buffer;

    return ((Buffer) bp);
}

/*-
 *-----------------------------------------------------------------------
 * Buf_Destroy --
 *	Nuke a buffer and all its resources.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The buffer is freed.
 *
 *-----------------------------------------------------------------------
 */
void
Buf_Destroy (buf, freeData)
    Buffer  buf;  	/* Buffer to destroy */
    Boolean freeData;	/* TRUE if the data should be destroyed as well */
{
    BufPtr  bp = (BufPtr) buf;
    
    if (freeData) {
	free ((char *)bp->buffer);
    }
    free ((char *)bp);
}
