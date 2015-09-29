/*
 * swapBuffer.c --
 *
 *	Byte-swapping routines.  Note that this may be clumsy and slow
 *	at first since data is copied twice in some instances.
 *
 *	Current bad assumption: I don't check whether I'm running off the
 *	end of a buffer during a particular copy (a byte copy, a half-word
 *	copy, word copy).  Fix this by checking right before the copy?.
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
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/swapBuffer.c,v 1.9 89/07/01 02:35:23 rab Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ctype.h"
#include "string.h"
#include "swapBuffer.h"

/*
 * On nasty errors we want to panic in a user process, but not let a
 * user process panic in the kernel.
 */
#ifdef KERNEL
#include "user/sys.h"
#define FakePanicVoid(errstring)	\
    Sys_Panic(SYS_WARNING, errstring);	\
    return;
#define	FakePanicValue(errstring)	\
    Sys_Panic(SYS_WARNING, errstring);	\
    return 0;
#else /* KERNEL */
#define FakePanicVoid(errstring)	\
    panic(errstring);
#define FakePanicValue(errstring)	\
    panic(errstring);
#endif /* KERNEL */

extern	long	strtol();
extern	void	panic();

/*
 * Forward declarations of procedures defined in this file.
 */
static  int     GetByte();
static  int     GetHalfWord();
static  int     GetWord();
static  int     GetDoubleWord();
static  void    CalcPutByteSize();
static  void    CalcPutHalfWordSize();
static  void    CalcPutWordSize();
static  void    CalcPutDoubleSize();
static  int     PutByte();
static  int     PutHalfWord();
static  int     PutWord();
static  int     PutDoubleWord();
static  int	CalcSize();
static  int     CopyData();



/*
 *----------------------------------------------------------------------
 *
 * Swap_Buffer -- 
 *
 *	Byte-swap incoming buffers from inType byte-order and alignment
 *	to outType byte-order and alignment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The data is copied from the inBuf to the outBuf,
 *	with byte-swapping or padding done as necessary.  *outSizePtr comes
 *	in with the size in bytes of the outBuf passed in.  As we return,
 *	*outSizePtr contains the actual size of swapped data.  If the number
 *	is larger than when it came in, then there wasn't enough space in
 *	outBuf, but we return how much space there should have been to
 *	swap the data correctly, so the calling routine could reallocate
 *	a larger buffer.   If *outSizePtr returns as 0, then something
 *	else went wrong, such as the format string contained garbage or
 *	contained a '*' that was not the last character, or the inBuf was
 *	too short for the format string, or some such other error.  A
 *	panic() in a user process or a Sys_Panic(SYS_WARNING, xxx) in the
 *	kernel will describe the error.
 *
 *----------------------------------------------------------------------
 */
void
Swap_Buffer(inBuf, inSize, inType, outType, format, outBuf, outSizePtr)
    char	*inBuf;		/* in-coming buffer with data to swap */
    int		inSize;		/* in BYTES - size of in-coming data */
    int		inType;		/* byte-order/padding of in-coming buffer */
    int		outType;	/* byte-order/padding of out-going buffer */
    char	*format;	/* format string of in-coming data */
    char	*outBuf;	/* where to put byte-swapped data */
    int		*outSizePtr;	/* in-out param: size of outBuf passed in,
				 * we calculate actual size of swapped data,
				 * in BYTES and return that.  See procedure
				 * comment for more details.
				 */
{
    char	*inPtr;
    char	*outPtr;
    char	*formatPtr;
    char	byteBuf[1];
    char	halfWordBuf[2];
    char	wordBuf[4];
    char	doubleBuf[8];
    int		outSize;
    int		okayP = 1;
    char	*cPtr;


#ifdef NOTDEF
    if (((unsigned int) inBuf) % 4  != 0) {
	*outSizePtr = 0;
	FakePanicVoid("Swap_Buffer: incoming buffer not long-word aligned.\n");
    }
#endif NOTDEF

    /*
     * '*' can only be the last character in a format string.
     */
    cPtr = strchr(format, '*');
    if (cPtr != NULL && cPtr != &(format[strlen(format) - 1])) {
	*outSizePtr = 0;
	FakePanicVoid("Swap_Buffer: the format string can contain a '*' only in the last character.\n");
    }

    formatPtr = format;
    inPtr = inBuf;
    outPtr = outBuf;
    outSize = *outSizePtr;

#ifdef NOTDEF
    if (((unsigned int) outBuf) % 4  != 0) {
	*outSizePtr = 0;
	FakePanicVoid("Swap_Buffer: outgoing buffer not long-word aligned.\n");
    }
#endif NOTDEF

    /*
     * while there's still more in inbuf, and we haven't finished with
     * the format string...
     */
    while (*formatPtr != '\0' && okayP) {
	switch (*formatPtr) {
	case 'b':
	    /* increments inPtr and outPtr and formatPtr */
	    okayP = CopyData(inType, &inPtr, inSize, inBuf, byteBuf, outType,
		    &outPtr, outSize, outBuf, &formatPtr, GetByte, PutByte);
	    break;
	case 'h':
	    okayP = CopyData(inType, &inPtr, inSize, inBuf, halfWordBuf,
		    outType, &outPtr, outSize, outBuf, &formatPtr, GetHalfWord,
		    PutHalfWord);
	    break;
	case 'w':
	    okayP = CopyData(inType, &inPtr, inSize, inBuf, wordBuf, outType,
		    &outPtr, outSize, outBuf, &formatPtr, GetWord, PutWord);
	    break;
	case 'd':
	    okayP = CopyData(inType, &inPtr, inSize, inBuf, doubleBuf, outType,
		    &outPtr, outSize, outBuf, &formatPtr, GetDoubleWord,
		    PutDoubleWord);
	    break;
	default:
	    *outSizePtr = 0;
	    FakePanicVoid(
		    "Swap_Buffer: unrecognized character in format string.\n");
	}
    }

    *outSizePtr = (unsigned int) outPtr - (unsigned int) outBuf;
    /*
     * Time for error checking now, in the following order.
     * 1) Check to see if we ran off of outBuf, if so, return the necessary
     *		size of outBuf in order to swap correctly.
     * 2) Check to see if we're not at the end of the format string.  If we
     *		are not, then we ran off the end of inBuf (or ourBuf, but
     *		we checked that in #1) and we panic (saying inBuf too short).
     * 3) Check to see if we didn't finish inBuf.  If so, then we ran off the
     *		end of the format string (or outBuf, but we checked than in
     *		#1) and we panic (saying format string not long enough).
     * 4) There's no need to check to see if we didn't finish outBuf.
     *		If we didn't, then either we ran off the end of inBuf
     *		or we ran off the end of the format string, or both.
     *		If we ran off the end of inBuf, but not format,
     *		then we checked this in #2.  If we ran off the end
     *		of format, but not inBuf, then we checked this in #3.
     *		So we must have run off the end of both - and this is okay,
     *		since it just means that they gave us an output buffer that
     *		was too large, so return no error!  Just keep the actual
     *		size of the output data in the *outSizePtr.
     */

    /* #1 Did we run off the end of outBuf? */
    if (*outSizePtr > outSize) {
	/* return necessary size of output buffer */
	(void) Swap_BufSize(inBuf, inSize, inType, outType, format, outSizePtr);
	return;
    }
    /* #2 Did we not finish format? */
    if (*formatPtr != '\0') {
	*outSizePtr = 0;
	FakePanicVoid("Swap_Buffer: ran out of input data before reaching the end of the format string.\n");
    }
    if (((unsigned int) inPtr - (unsigned int) inBuf) < inSize) {
	*outSizePtr = 0;
	FakePanicVoid("Swap_Buffer: format string was not long enough to account for the size of the input data.\n");
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Swap_BufSize -- 
 *
 *	Calculate the necessary size of an output buffer to swap the incoming
 *	data into.  This takes care of padding on the different machines
 *	that would affect the size of the swapped data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	*outSizePtr is set to the size necessary to hold the byte-swapped
 *	input data.  This takes into account differences of padding on
 *	the different machines.  If the output size can't be correctly
 *	calculated, then *outSizePtr returns a 0.  This can happen if
 *	something went wrong, such as the format string contained garbage or
 *	contained a '*' that was not the last character, or the inBuf was
 *	too short for the format string, or some such other error.  Note that
 *	less error checking is done in this routine than in Swap_Buffer(), since
 *	the size of the inBuf relative to the format string isn't checked
 *	unless it's crucial to calculating *outSizePtr.  (This occurs if the
 *	format string contains a '*'.) 	If such an error occurs,
 *	panic() in a user process or a Sys_Panic(SYS_WARNING, xxx) in the
 *	kernel will describe the error.
 *
 *----------------------------------------------------------------------
 */
void
Swap_BufSize(inBuf, inSize, inType, outType, format, outSizePtr)
    char	*inBuf;		/* in-coming buffer with data to swap */
    int		inSize;		/* in BYTES - size of in-coming data */
    int		inType;		/* byte-order/padding of in-coming buffer */
    int		outType;	/* byte-order/padding of out-going buffer */
    char	*format;	/* format string of in-coming data */
    int		*outSizePtr;	/* We calculate the necessary size of swapped
				 * data, in BYTES and return that.
				 */
{
    char	*inPtr;
    int		okayP = 1;
    char	*formatPtr;
    char	*cPtr;

    /*
     * '*' can only be the last character in a format string.
     */
    cPtr = strchr(format, '*');
    if (cPtr != NULL && cPtr != &(format[strlen(format) - 1])) {
	*outSizePtr = 0;
	FakePanicVoid("Swap_BufSize: the format string can contain a '*' only in the last character.\n");
    }

    inPtr = inBuf;
    formatPtr = format;
    *outSizePtr = 0;

    while (*formatPtr != '\0' && okayP) {
	switch (*formatPtr) {
	case 'b':
	    /*
	     * Increments inPtr, outSize and formatPtr
	     */
	    okayP = CalcSize(inType, &inPtr, inSize, inBuf, outType, outSizePtr,
		    &formatPtr, GetByte, CalcPutByteSize);
	    break;
	case 'h':
	    okayP = CalcSize(inType, &inPtr, inSize, inBuf, outType, outSizePtr,
		    &formatPtr, GetHalfWord, CalcPutHalfWordSize);
	    break;
	case 'w':
	    okayP = CalcSize(inType, &inPtr, inSize, inBuf, outType, outSizePtr,
		    &formatPtr, GetWord, CalcPutWordSize);
	    break;
	case 'd':
	    okayP = CalcSize(inType, &inPtr, inSize, inBuf, outType, outSizePtr,
		    &formatPtr, GetDoubleWord, CalcPutDoubleSize);
	    break;
	default:
	    /* I should say what character it is... */
	    *outSizePtr = 0;
	    FakePanicVoid("Swap_BufSize: unrecognized character in format string.\n");
	}
    }
    if (!okayP) {
	*outSizePtr = 0;
	FakePanicVoid("Swap_BufSize: the size of the format string and inBuf were not compatible.\n");
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * CalcSize -- 
 *
 *	Calculate needed size for output byte-swapped buffer.
 *
 * Results:
 *	True (1) if everything went okay and we were able to calculate
 *	the output buffer size.  False (0) if we weren't able to calculate
 *	it.  This occurs when the format string contains a '*' and we run
 *	out of the in-coming buffer space before we even get to the '*', or
 *	when the format string contains garbage.
 *
 * Side effects:
 *	*outSizePtr gets filled in with the needed size of the output buffer.
 *
 *----------------------------------------------------------------------
 */
static int
CalcSize(inType, inPtrPtr, inSize, inBuf, outType, outSizePtr, formatPtrPtr,
							    GetFunc, SizeFunc)
    int		inType;
    char	**inPtrPtr;
    int		inSize;
    char	*inBuf;
    int		outType;
    int		*outSizePtr;
    char	**formatPtrPtr;
    int		(*GetFunc)();
    void	(*SizeFunc)();
{
    int		repCount;
    int		an_int;
    char	*inPtr;
    char	*formatPtr;
    int		okayP = 1;
    int		checkInSizeP = 0;
    int		inSizeLeft;
 
    inPtr = *inPtrPtr;
    formatPtr = *formatPtrPtr;

    /*
     * We need to check the size remaining in the inBuf only if there's
     * a '*' in the format string.   This is because we cannot calculate
     * the outSize correctly if there's a '*' except by seeing how much
     * data there really is in the inBuf.
     */
    if (strchr(formatPtr, '*') != NULL) {
	checkInSizeP = 1;
    }
    formatPtr++;
    repCount = 0;
    while (*formatPtr != '\0' && isdigit(*formatPtr)) {
	char	*testStr;
	repCount *= 10;
	an_int = strtol(formatPtr, &testStr, 10);
	if (testStr == formatPtr) {
	    FakePanicValue("Bad format string.\n");
	}
	repCount += an_int;
	formatPtr++;
    }
    /*
     * If there were no digits following the type character, then set repCount
     * to 1 so that we do 1 character.
     */
    if (repCount == 0) {
	repCount = 1;
    }
    if (*formatPtr == '*') {
	formatPtr++;
	/*
	 * Infinity, since we count down until zero.
	 */
	repCount = -1;
    }

    while (repCount != 0 && (((unsigned int) inPtr) -
	    ((unsigned int) inBuf)) < inSize) {
	/* increments inPtr */
	if (checkInSizeP) {
	    inSizeLeft = inSize - ((unsigned int) inPtr - (unsigned int) inBuf);
	    okayP = (*GetFunc)(inType, &inPtr, NULL, inSizeLeft);
	    if (!okayP) {
		return 0;
	    }
	}
	/* increments outSize */
	(*SizeFunc)(outType, outSizePtr);
	repCount--;
    }
    *inPtrPtr = inPtr;
    *formatPtrPtr = formatPtr;
    return 1;
}


/*
 *----------------------------------------------------------------------
 *
 * CopyData -- 
 *
 *	Byte-swap and copy a piece of data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The data is copied from the inBuf to the outBuf using given
 *	retrieval and copy functions.
 *
 *----------------------------------------------------------------------
 */
static int
CopyData(inType, inPtrPtr, inSize, inBuf, buf, outType, outPtrPtr,
						outSize, outBuf, formatPtrPtr,
						GetFunc, PutFunc)
    int		inType;
    char	**inPtrPtr;
    int		inSize;
    char	*inBuf;
    char	*buf;
    int		outType;
    char	**outPtrPtr;
    int		outSize;
    char	*outBuf;
    char	**formatPtrPtr;
    int		(*GetFunc)();
    int		(*PutFunc)();
{
    char	*inPtr;
    char	*outPtr;
    char	*formatPtr;
    int		an_int;
    int		repCount;
    int		inSizeLeft;
    int		outSizeLeft;
    int		okayP = 1;

    inPtr = *inPtrPtr;
    outPtr = *outPtrPtr;
    formatPtr = *formatPtrPtr;

    formatPtr++;
    repCount = 0;
    while (*formatPtr != '\0' && isdigit(*formatPtr)) {
	char	*testStr;
	repCount *= 10;
	an_int = strtol(formatPtr, &testStr, 10);
	if (testStr == formatPtr) {
	    FakePanicValue("Ioctl format string bad.\n");
	}
	repCount += an_int;
	formatPtr++;
    }
    /*
     * If there were no digits following the type character, then set repCount
     * to 1 so that we do 1 character.
     */
    if (repCount == 0) {
	repCount = 1;
    }
    if (*formatPtr != '\0' && *formatPtr == '*') {
	formatPtr++;
	/*
	 * Infinity, since we count down until zero.
	 */
	repCount = -1;
    }
    while (repCount != 0 && okayP) {
	inSizeLeft = inSize - ((unsigned int) inPtr - (unsigned int) inBuf);
	outSizeLeft = outSize - ((unsigned int) outPtr - (unsigned int) outBuf);
	/* increments inPtr */
	okayP = (*GetFunc)(inType, &inPtr, buf, inSizeLeft);
	/* increments outPtr */
	if (okayP) {
	    okayP = (*PutFunc)(outType, &outPtr, buf, outSizeLeft);
	}
	repCount--;
    }

    *inPtrPtr = inPtr;
    *outPtrPtr = outPtr;
    *formatPtrPtr = formatPtr;

    return okayP;
}


/*
 *----------------------------------------------------------------------
 *
 * GetByte, GetHalfWord, GetWord, GetDoubleWord -- 
 *
 *	Using the given byte-alignment type, grab the next bytes in the buffer.
 *	If the buf param isn't NULL, the grabbed bytes will be put into it.
 *
 *	In the case of halfwords, words, and doubles, some byte-swapping may
 *	be necessary.  If the inType of the buffer is of a different
 *	alignment, some skipping of byte padding may be necessary before
 *	grabbing the desired bytes.
 *
 *	Regardless, buf will be filled with bytes in the following order,
 *	with no padding:
 *		byte:	buf[0] is the byte.
 *		halfword:	buf[0] is low byte, buf[1] is high byte.
 *		word:	buf[0] is low byte, ..., buf[3] is high byte.
 *		double:	buf[0] is low byte, ..., buf[7] is high byte.
 *
 * Results:
 *	True (1) if everyting goes okay, and false (0) if there isn't enough
 *	space left in inBuf.
 *
 * Side effects:
 *	The byte grabbed is put into buf, if it isn't NULL.  The ptr into
 *	the input buffer is incremented.  Even if there wasn't enough buffer
 *	space, the routine increments the ptrs to where they would be if there
 *	had been, so the calling routine can see what should have happened.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetByte(inType, inPtrPtr, buf, inSizeLeft)
    int		inType;
    char	**inPtrPtr;
    char	*buf;
    int		inSizeLeft;
{
    if (buf != NULL && inSizeLeft >= 1) {
	buf[0] = **inPtrPtr;
    }
    (*inPtrPtr)++;

    return (inSizeLeft >= 1);
}

static int
GetHalfWord(inType, inPtrPtr, buf, inSizeLeft)
    int		inType;
    char	**inPtrPtr;
    char	*buf;
    int		inSizeLeft;
{
    if (inType == SWAP_SUN_TYPE || inType == SWAP_SPARC_TYPE) {
	/* one-byte padding if necessary */
	if ((((unsigned int) *inPtrPtr) % 2) != 0) {
	    if (buf != NULL && inSizeLeft >= 3) {
		buf[0] = *((*inPtrPtr) + 1);
		buf[1] = *((*inPtrPtr) + 2);
	    }
	    *inPtrPtr += 3;
	    return (inSizeLeft >= 3);
	} else {
	    if (buf != NULL && inSizeLeft >= 2) {
		buf[0] = **inPtrPtr;
		buf[1] = *((*inPtrPtr) + 1);
	    }
	    *inPtrPtr += 2;
	    return (inSizeLeft >= 2);
	}
    } else if (inType == SWAP_VAX_TYPE || inType == SWAP_SPUR_TYPE) {
	/* one-byte padding if necessary */
	if ((((unsigned int) *inPtrPtr) % 2) != 0) {
	    if (buf != NULL && inSizeLeft >= 3) {
		buf[0] = *((*inPtrPtr) + 2);
		buf[1] = *((*inPtrPtr) + 1);
	    }
	    *inPtrPtr += 3;
	    return (inSizeLeft >= 3);
	} else {
	    if (buf != NULL && inSizeLeft >= 2) {
		buf[0] = *((*inPtrPtr) + 1);
		buf[1] = **inPtrPtr;
	    }
	    *inPtrPtr += 2;
	    return (inSizeLeft >= 2);
	}
    }
    /* for lint */
    return 0;
}

static int
GetWord(inType, inPtrPtr, buf, inSizeLeft)
    int		inType;
    char	**inPtrPtr;
    char	*buf;
    int		inSizeLeft;
{
    int		fix_it;

    if (inType == SWAP_SUN_TYPE) {
	/* one-byte padding if necessary */
	fix_it = (2 - (((unsigned int) *inPtrPtr) % 2)) % 2;
	if (buf != NULL && inSizeLeft >= 4 + fix_it) {
	    buf[0] = *((*inPtrPtr) + fix_it);
	    buf[1] = *((*inPtrPtr) + fix_it + 1);
	    buf[2] = *((*inPtrPtr) + fix_it + 2);
	    buf[3] = *((*inPtrPtr) + fix_it + 3);
	}
	(*inPtrPtr) += 4 + fix_it;
	return (inSizeLeft >= 4 + fix_it);
    } else if (inType == SWAP_VAX_TYPE) {
	fix_it = (2 - (((unsigned int) *inPtrPtr) % 2)) % 2;
	if (buf != NULL && inSizeLeft >= 4 + fix_it) {
	    buf[0] = *((*inPtrPtr) + fix_it + 3);
	    buf[1] = *((*inPtrPtr) + fix_it + 2);
	    buf[2] = *((*inPtrPtr) + fix_it + 1);
	    buf[3] = *((*inPtrPtr) + fix_it);
	}
	(*inPtrPtr) += 4 + fix_it;
	return (inSizeLeft >= 4 + fix_it);
    } else if (inType == SWAP_SPUR_TYPE) {
	fix_it = (4 - (((unsigned int) *inPtrPtr) % 4)) % 4;
	if (buf != NULL && inSizeLeft >= 4 + fix_it) {
	    buf[0] = *(*inPtrPtr + fix_it + 3);
	    buf[1] = *(*inPtrPtr + fix_it + 2);
	    buf[2] = *(*inPtrPtr + fix_it + 1);
	    buf[3] = *(*inPtrPtr + fix_it);
	}
	(*inPtrPtr) += 4 + fix_it;
	return (inSizeLeft >= 4 + fix_it);
    } else if (inType == SWAP_SPARC_TYPE) {
	fix_it = (4 - (((unsigned int) *inPtrPtr) % 4)) % 4;
	if (buf != NULL && inSizeLeft >= 4 + fix_it) {
	    buf[0] = *(*inPtrPtr + fix_it);
	    buf[1] = *(*inPtrPtr + fix_it + 1);
	    buf[2] = *(*inPtrPtr + fix_it + 2);
	    buf[3] = *(*inPtrPtr + fix_it + 3);
	}
	(*inPtrPtr) += 4 + fix_it;
	return (inSizeLeft >= 4 + fix_it);

    }
    /* for lint */
    return 0;
}

/*ARGSUSED*/
static int
GetDoubleWord(inType, inPtrPtr, buf, inSizeLeft)
    int		inType;
    char	**inPtrPtr;
    char	*buf;
    int		inSizeLeft;
{
    /* what does this mean and where? */
    FakePanicValue("GetDoubleWord:  don't know what a double means yet.\n");
}


/*
 *----------------------------------------------------------------------
 *
 * CalcPutByteSize, CalcPutHalfWordSize, CalcPutWordSize, CalcPutDoubleSize -- 
 *
 *	Using the given byte-alignment type, figure out how much space is
 *	required in the output buffer to add the next bytes.  This means
 *	figuring out if padding is required on the machine type of the output
 *	buffer.
 *
 *	We assume that outSizePtr is the current offset from the beginning
 *	of a long-word aligned buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	outSize is incremented to record the amount of space required.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
CalcPutByteSize(outType, outSizePtr)
    int		outType;
    int		*outSizePtr;
{
    (*outSizePtr)++;
    return;
}

static void
CalcPutHalfWordSize(outType, outSizePtr)
    int		outType;
    int		*outSizePtr;
{
    if (outType == SWAP_SUN_TYPE || outType == SWAP_VAX_TYPE ||
	    outType == SWAP_SPUR_TYPE || outType == SWAP_SPARC_TYPE) {
	if ((((unsigned int) *outSizePtr) % 2) != 0) {
	    /* one byte padding to short-word align the thing */
	    (*outSizePtr) += 3;
	} else {
	    (*outSizePtr) += 2;
	}
    }
    return;
}

static void
CalcPutWordSize(outType, outSizePtr)
    int		outType;
    int		*outSizePtr;
{
    int		fix_it;

    if (outType == SWAP_SUN_TYPE || outType == SWAP_VAX_TYPE) {
	if ((((unsigned int) *outSizePtr) % 2) != 0) {
	    /* one byte padding to short-word align the thing */
	    (*outSizePtr) += 5;
	} else {
	    (*outSizePtr) += 4;
	}
    } else if (outType == SWAP_SPUR_TYPE || outType == SWAP_SPARC_TYPE) {
	fix_it = (4 - (((unsigned int) *outSizePtr) % 4)) % 4;
	/* byte padding to long-word align the thing */
	(*outSizePtr) += 4 + fix_it;
    }
    return;
}

/*ARGSUSED*/
static void
CalcPutDoubleSize(outType, outSizePtr)
    int		outType;
    int		*outSizePtr;
{
    FakePanicVoid("CalcPutDoubleSize: Don't know what this means yet.\n");
}


/*
 *----------------------------------------------------------------------
 *
 * PutByte, PutHalfWord, PutWord, PutDoubleWord -- 
 *
 *	Using the given byte-alignment type, byte-swap and pad the data in
 *	the buffer as necessary and copy it to the outBuf.
 *
 *	We assume that outSizePtr is the current offset from the beginning
 *	of a long-word aligned buffer.
 *
 *	The data in buf is ordered with buf[0] as low byte and buf[n] as
 *	high byte, with no padding.  So if the outType machine type requires
 *	byte-swapping or padding from the current outPtr location, this is
 *	done here.
 *
 * Results:
 *	True (1) if everything goes okay, false (0) if there isn't enough
 *	space left in outBuf.
 *
 * Side effects:
 *	outSize is incremented to record the amount of space required.
 *	outPtr is incremented to its new position.
 *	Data is copied into outPtr's buffer.  Even if there wasn't enough buffer
 *	space, the routine increments the ptrs to where they would be if there
 *	had been, so the calling routine can see what should have happened.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PutByte(outType, outPtrPtr, buf, outSizeLeft)
    int		outType;
    char	**outPtrPtr;
    char	*buf;
    int		outSizeLeft;
{
    if (outSizeLeft >= 1) {
	**outPtrPtr = buf[0];
    }
    (*outPtrPtr)++;
    return (outSizeLeft >= 1);
 
}

static int
PutHalfWord(outType, outPtrPtr, buf, outSizeLeft)
    int		outType;
    char	**outPtrPtr;
    char	*buf;
    int		outSizeLeft;
{
    if (outType == SWAP_SUN_TYPE || outType == SWAP_SPARC_TYPE) {
	if ((((unsigned int) *outPtrPtr) % 2) != 0) {
	    if (outSizeLeft >= 3) {
		**outPtrPtr = 0;
		*(*outPtrPtr + 1) = buf[0];
		*(*outPtrPtr + 2) = buf[1];
	    }
	    (*outPtrPtr) += 3;
	    return (outSizeLeft >= 3);
	} else {
	    if (outSizeLeft >= 2) {
		**outPtrPtr = buf[0];
		*(*outPtrPtr + 1) = buf[1];
	    }
	    (*outPtrPtr) += 2;
	    return (outSizeLeft >= 2);
	}
    } else if (outType == SWAP_VAX_TYPE || outType == SWAP_SPUR_TYPE) {
	if ((((unsigned int) *outPtrPtr) % 2) != 0) {
	    if (outSizeLeft >= 3) {
		**outPtrPtr = 0;
		*(*outPtrPtr + 1) = buf[1];
		*(*outPtrPtr + 2) = buf[0];
	    }
	    (*outPtrPtr) += 3;
	    return (outSizeLeft >= 3);
	} else {
	    if (outSizeLeft >= 2) {
		**outPtrPtr = buf[1];
		*(*outPtrPtr + 1) = buf[0];
	    }
	    (*outPtrPtr) += 2;
	    return (outSizeLeft >= 2);
	}
    }
    /* for lint */
    return 0;
}

static int
PutWord(outType, outPtrPtr, buf, outSizeLeft)
    int		outType;
    char	**outPtrPtr;
    char	*buf;
    int		outSizeLeft;
{
    int		fix_it;
    int		i;

    if (outType == SWAP_SUN_TYPE) {
	/* fix_it = 0 or 1 */
	fix_it = (2 - (((unsigned int) *outPtrPtr) % 2)) % 2;
	if (outSizeLeft >= 4 + fix_it) {
	    **outPtrPtr = 0;	/* Takes care of padding if fix_it = 1 */
	    *(*outPtrPtr + fix_it) = buf[0];
	    *(*outPtrPtr + fix_it + 1) = buf[1];
	    *(*outPtrPtr + fix_it + 2) = buf[2];
	    *(*outPtrPtr + fix_it + 3) = buf[3];
	}
	(*outPtrPtr) += 4 + fix_it;
	return (outSizeLeft >= 4 + fix_it);
    } else if (outType == SWAP_VAX_TYPE) {
	/* fix_it = 0 or 1 */
	fix_it = (2 - (((unsigned int) *outPtrPtr) % 2)) % 2;
	if (outSizeLeft >= 4 + fix_it) {
	    **outPtrPtr = 0;	/* Takes care of padding if fix_it = 1 */
	    *(*outPtrPtr + fix_it) = buf[3];
	    *(*outPtrPtr + fix_it + 1) = buf[2];
	    *(*outPtrPtr + fix_it + 2) = buf[1];
	    *(*outPtrPtr + fix_it + 3) = buf[0];
	}
	(*outPtrPtr) += 4 + fix_it;
	return (outSizeLeft >= 4 + fix_it);
    } else if (outType == SWAP_SPUR_TYPE) {
	fix_it = (4 - (((unsigned int) *outPtrPtr) % 4)) % 4;
	/* padding */
	if (outSizeLeft >= 4 + fix_it) {
	    for (i = 0; i < fix_it; i++) {
		*(*outPtrPtr + i) = 0;
	    }
	    *(*outPtrPtr + fix_it) = buf[3];
	    *(*outPtrPtr + fix_it + 1) = buf[2];
	    *(*outPtrPtr + fix_it + 2) = buf[1];
	    *(*outPtrPtr + fix_it + 3) = buf[0];
	}
	(*outPtrPtr) += 4 + fix_it;
	return (outSizeLeft >= 4 + fix_it);
    } else if (outType == SWAP_SPARC_TYPE) {
	fix_it = (4 - (((unsigned int) *outPtrPtr) % 4)) % 4;
	/* padding */
	if (outSizeLeft >= 4 + fix_it) {
	    for (i = 0; i < fix_it; i++) {
		*(*outPtrPtr + i) = 0;
	    }
	    *(*outPtrPtr + fix_it) = buf[0];
	    *(*outPtrPtr + fix_it + 1) = buf[1];
	    *(*outPtrPtr + fix_it + 2) = buf[2];
	    *(*outPtrPtr + fix_it + 3) = buf[3];
	}
	(*outPtrPtr) += 4 + fix_it;
	return (outSizeLeft >= 4 + fix_it);
    }
    /* for lint */
    return 0;
}

/*ARGSUSED*/
static int
PutDoubleWord(outType, outPtrPtr, buf, outSizeLeft)
    int		outType;
    char	**outPtrPtr;
    char	*buf;
    int		outSizeLeft;
{
    FakePanicValue("PutDoubleWord: Don't know what this means yet.\n");
}
