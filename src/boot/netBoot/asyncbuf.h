
/*	@(#)asyncbuf.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* These definitions and structures permit the creation of 
 * asynchronously accessed buffers in shared memory.  The
 * writer and the reader of the buffer have separate pointers
 * into it, which they themselves manage.
 *
 * -------------------------------------- John Gilmore, 21Apr82
 *
 * Generalized to buffers of any type...JCGilmore 21May82
 *
 * Example:
 *	DEFBUF (bytebuf, char, 300);
 *	struct bytebuf OurBuffer[1];
 *	...
 *	initbuf (OurBuffer, 300);
 *	...
 *	OurChar = getchar();
 *	bput (OurBuffer, OurChar);
 *
 * Note that OurBuffer is declared as an array of one bytebuf.  This is
 * so that "OurBuffer" will act as a pointer constant.  The macros are set up to
 * take pointers rather than structure names for reasons that will be
 * obvious two paragraphs down.
 *
 * This means that if you reference the members of OurBuffer, you must
 * say  OurBuffer->getptr  or  (*OurBuffer).getptr  or  OurBuffer[0].getptr
 * instead of  OurBuffer.getptr .
 *
 * Meanwhile somebody else with access to the same memory locations is
 * quietly doing
 *	DEFBUF (bytebuf, char, 300);
 *	struct bytebuf *TheirBuffer = GetTheirBufferAddr();
 * 	...
 *	if (bgetp (TheirBuffer)) bget (TheirBuffer, TheirChar);
 *	...
 *
 * It is also possible to pull out multiple datums at once.  The construct
 *	bgets (TheirBuffer, addr, count);
 * will return the address of a series of datums, and how many there are.
 * This count might be zero if no data is waiting.  If the buffer has 
 * wrapped around from end to start, the first call will return a count
 * from the "get" pointer thru the end of the buffer.  After these datums
 * are accepted (with baccs), the next bgets will return an addr and count
 * that describes the 2nd half of the data.  NOTE that data returned by
 * bgets is still sitting in the buffer area.  It MUST be accepted by
 * calling
 *	baccs (TheirBuffer, count);
 * before attempting to do another bget, bgetp, or bgets.  It can be accepted
 * in chunks, tho, as in:
 *	while (count--) {do_something (addr++); baccs(TheirBuffer, 1);}
 *
 * bgets declares one register pointer variable in an internal block.  This
 * will cause one more register to be saved and restored on procedure entry
 * and exit.  If no register is available, I believe the compiler will just
 * use a temp.
 *
 * OVERRUN HANDLING: Overrun is set by bputo() when it puts a datum which
 * overfills the buffer.  That bputo() causes the "get" side to believe that
 * there is no data in the buffer.  However it can test the overrun flag
 * with boverp() and report the error, then clear the flag with boverclr().
 * bput(), bgetp(), and bget() do not set or check overrun.  The unimplemented
 * bputm() would return FALSE and avoid putting the datum if overrun would
 * occur; since nobody needs it I'm leaving it as an exercise for the first
 * person who does.
 *
 * MODIFIERS NOTE: exactly when each of getptr and putptr wrap from the
 * end to the beginning is VITAL.  
 *
 * When putting data, putptr wraps just before placing some data in the buffer.
 *
 * When getting data, getptr wraps AFTER we have determined that
 * at least one datum exists in the buffer, but BEFORE we reference that
 * datum.  This is because, if we wrapped it first thing, putptr might
 * still be un-wrapped, and we'd think we had a whole bufferfull of data.
 *
 * MODIFIERS ALSO NOTE: great pain was taken to avoid referencing the 
 * "other" pointer more than once in a get or put.  This avoids problems
 * when interrupt routines are updating the pointers (which is what
 * these macros were designed for).  NO macros set the other guy's pointer.
 *
 */

#ifndef DEFBUF

/* This macro defines a buffer type of your choice.  Parameters are the
   name of the buffer structure, type of element, and number of elements.
 *
 */
#define	DEFBUF(NAME,TYPE,HOWMANY) \
struct NAME { \
	TYPE *	getptr; \
	TYPE *	putptr; \
	TYPE *	endptr;		/* always contains &blocks[HOWMANY] */ \
	char	overrun;	/* set TRUE when an overrun occurs */ \
	TYPE	block [HOWMANY]; \
};

/*
 * Initialize a buffer before its first use.  Parameters are buffer name
 * and number of elements.
 */
#define	initbuf(buffer, HOWMANY) \
    { \
	(buffer)-> getptr = (buffer)-> putptr = &(buffer)-> block[0]; \
	(buffer)-> endptr = &(buffer)-> block[HOWMANY]; \
	(buffer)-> overrun = 0; \
    }
	

/*
 * bputo (buffer, datum);	puts datum into buffer, setting overrun
 *				if reader is too far behind.
 * bputm (buffer, datum)	puts datum into buffer if there's room,
 *	^^unimplemented^^	returning FALSE; else returns TRUE.
 * bput  (buffer, datum);	puts datum into buffer, ignoring overrun.
 * bgetp (buffer)		returns TRUE if there is data to be read.
 * bget  (buffer, datum)	returns the next datum.
 * bgets (buffer, addr, count)	returns addr&count of contiguous data in buf
 * baccs (buffer, count)	accepts count datums after using bgets
 * bputclr (buffer)		clears the buffer -- for "put" task
 * bgetclr (buffer)		clears the buffer -- for "get" task
 * boverp (buffer)		returns TRUE if overrun is set
 * boverclr (buffer)		clears overrun to FALSE
 * bsize (buffer, count)	returns how many datums are there, in count
 *
 */

#define	bput(buffer, datum) \
    { \
	if ( (buffer)->putptr >= (buffer)->endptr ) \
		(buffer)->putptr = &((buffer)->block[0]); \
	( * ( (buffer)->putptr )++ ) = datum; \
    }

#define	bputo(buffer, datum) \
    { \
	bput(buffer, datum); \
	if ( (buffer)->putptr == (buffer)->getptr ) \
		(buffer)->overrun = 1; \
    }

#define	bgetp(buffer) \
	( (buffer)->getptr != (buffer)->putptr )
/* This depends on TRUE and FALSE being returned by != */
/* If that doesn't work, use:
  (    ( (buffer)->getptr != (buffer)->putptr ) ? 1 : 0   )
								    */	

#define	bget(buffer, datum) \
    { \
	if ( (buffer)->getptr >= (buffer)->endptr ) \
		(buffer)->getptr = &(buffer)->block[0]; \
        datum = *(buffer)->getptr++; \
    }

#define	bgets(buffer, addr, count) \
    { \
	register unsigned char* putptr = (buffer)->putptr;\
	/* This avoids double ref to (buffer)->putptr - it might change. */\
	/* Note the above assumes datatype of buffer is "u char" - BUG */\
	if ((buffer)->getptr > putptr) {\
		/* Buffer has wrapped. See if we've gotten last chunk \
		   at end.  If so, go to front; otherwise treat endptr \
		   as if it was putptr, to return trailing chunk only. */\
		if ( (buffer)->getptr >= (buffer)->endptr ) \
			(buffer)->getptr = &(buffer)->block[0]; \
		else						\
			putptr = (buffer)->endptr; \
	} \
	addr = (buffer)->getptr; \
	count = putptr - (buffer)->getptr; \
    }

#define	baccs(buffer, count) \
	(buffer)->getptr += count

#define	bputclr(buffer) \
	(buffer)->putptr = (buffer)->getptr

#define	bgetclr(buffer) \
	(buffer)->getptr = (buffer)->putptr

#define	boverp(buffer) \
	(0 != (buffer)->overrun)

#define	boverclr(buffer) \
	(buffer)->overrun = 0

#define	bsize(buffer, count) \
    { \
	count = (buffer)->putptr - (buffer)->getptr; \
	/* If putptr<getptr, adjust for wrap. */\
	if ((int)count < 0) count += \
		sizeof((buffer)->block) / sizeof((buffer)->block[0]); \
    }

#endif DEFBUF
