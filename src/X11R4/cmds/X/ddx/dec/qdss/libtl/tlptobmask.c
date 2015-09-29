/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "X.h"
#include <sys/types.h>
#include "gcstruct.h"
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>

#include "qd.h"
#include "qdgc.h"

#include "tltemplabels.h"
#include "tl.h"


#ifdef BICHROME
# define PTOBOVERHEAD	 NMASKSHORTS + 12 + 8 + 2*NCOLORSHORTS
#else
# define PTOBOVERHEAD	 NMASKSHORTS + 12 + 7 + 1*NCOLORSHORTS
#endif

/*
 * Uses PTOBXY to broadcast a bitmap in the DMA queue to all planes
 *
 * Dragon clipping is set from a single box argument, not from the GC
 */
VOID
#ifdef BICHROME
 tlBitmapBichrome
#else
 tlBitmapStipple
#endif
	( pGC, qbit, fore, back, x0, y0, box)
    GCPtr	pGC;
    PixmapPtr	qbit;		/* x11 bitmap, with the mfb conventions */
    int		fore;
    int		back;		/* used in tlTileBichrome only */
    int		x0, y0;
    BoxPtr	box;		/* clipping */
 {
    unsigned short *	pshort = (unsigned short *) QD_PIX_DATA(qbit);
    int			slwidthshorts = qbit->devKind>>1;
    int			height = QDPIX_HEIGHT(qbit);
    int			width = QDPIX_WIDTH(qbit);
    int 		nscansperblock;
    unsigned short *	buf;

    extern int     req_buf_size;
    void doBitmap();

    SETTRANSLATEPOINT( 0, 0);

    /* Do clipping in y */
    if (box->y1 > y0) {
	int skip = box->y1 - y0;
	y0 += skip;
	pshort += skip * slwidthshorts;
	height -= skip;
    }
    if (y0 + height > box->y2)
	height = box->y2 - y0;
    if (height <= 0) return;

    if ((x0 & 0xF) != 0) {
	/* must shift */
	/* dest scanline may be one short wider than source  */
	buf = (unsigned short*)
	    ALLOCATE_LOCAL( (qbit->devKind+2) * height); /* upper bound */ 
	
	/* right-shift bitmap to same modulo-16 boundary as x0 */
	slwidthshorts = bitmapShiftRight( pshort, buf,
	    slwidthshorts, QDPIX_WIDTH(qbit), height, x0&0xf);
	pshort = buf;
    }
    else
	buf = NULL;
    nscansperblock = ( min( req_buf_size, MAXDMAPACKET / sizeof(short))
		     - 50 - PTOBOVERHEAD - slwidthshorts) / slwidthshorts;

    for (; ; ) {
	register unsigned short *p;
	int curHeight = min(nscansperblock, height);
	register unsigned	nshort = slwidthshorts * curHeight;
	height -= curHeight;

	/*
	 * I believe this may all have to fit in one DMA partition,
	 * because we program the dragon to do PTOB
	 */

#ifdef BICHROME
	if (pGC->alu == GXcopy) {
	    Need_dma( PTOBOVERHEAD + nshort);

	    *p++ = JMPT_SET_MASKED_ALU;
	    *p++ = pGC->planemask;
	    *p++ = LF_SOURCE|FULL_SRC_RESOLUTION;
	    *p++ = MAC_SETFOREBACKCOLOR;
	    SETCOLOR( p, pGC->fgPixel);
	    SETCOLOR( p, pGC->bgPixel);
	}
	else {
	    SetPlaneAlu(pGC);
	    Need_dma( PTOBOVERHEAD + nshort);
	}
#else
	Need_dma( PTOBOVERHEAD + nshort);

	*p++ = JMPT_SET_MASKED_ALU;
	*p++ = pGC->planemask;
	*p++ = umtable[ pGC->alu];
	*p++ = MAC_SETCOLOR;
	SETCOLOR( p, pGC->fgPixel);
#endif

	*p++ = JMPT_SETCLIP; /* clip to intersection of dest and arg clip */
	*p++ = max( box->x1, x0);
	*p++ = min( box->x2, x0+width);
	*p++ = y0; /* y is pre-clipped */
	*p++ = y0+curHeight;
#ifdef BICHROME
	*p++ = JMPT_PTOBXY;
#else
	*p++ = JMPT_PTOBXYMASK;
#endif
	*p++ = pGC->planemask;
	*p++ = x0 & 0x03fff;
	*p++ = y0 & 0x03fff;
	if ((x0 & 0xF) == 0)
	    /* We used the original 32-bit-aligned bitmap, so pretend
	     * it was the full 32-bit-aligned width.
	     * Clipping will trim the excess short, if any.
	     */
	    *p++ = (width + 31) & 0x3fe0;
	else
	    *p++ = width & 0x03fff;
	*p++ = curHeight & 0x03fff;
	/* DGA magic bit pattern for PTB, see VCB02 manual pp.3-117 */
	*p++ = 0x06000 | (0x01fff & -nshort);
	bcopy( pshort, p, nshort<<1);
	p += nshort;
	*p++ = JMPT_PTOBXYCLEAN;
#ifdef BICHROME
	if (pGC->alu == GXcopy)
	    *p++ = JMPT_RESET_FORE_BACK;
#else
	*p++ = JMPT_SETMASK;
	*p++ = 0xFFFF;
#endif
	Confirm_dma ();

	if (height <= 0) break;
	y0 += nscansperblock;
	pshort += nscansperblock*slwidthshorts;
    }
    if (buf) { DEALLOCATE_LOCAL(buf); }
}

#ifdef BICHROME		/* only one instance of bitmapShiftRight in library */
/*
 * Shift "right" in the frame buffer sense.  This is an algebraic left shift.
 * Pad each scan line of the destination to an integral number of shorts.
 * Source is known to be padded to an integral number of longwords.
 */
int
bitmapShiftRight( psrc, pdst, srcShorts, width, height, nbits)
    unsigned short *	psrc;
    unsigned short *	pdst;
    int			srcShorts;	/* source width in shorts */
    int			width;
    int			height;
    int			nbits;	/* number of bit places to shift: 0-15 */
{
    int		ir;	/* row index */
    int		ids;	/* index of destination short */
    int		dstShorts;

    dstShorts = (width + nbits + 15) >> 4;
    /*
     * for each scan line
     */
    for ( ir=0; ir<height; ir++, psrc+=srcShorts, pdst+=dstShorts)
    {
	/*
	 * for each short on the destination line,
	 * find the containing longword in the source and extract the bits
	 */
	pdst[ 0] = psrc[0] << nbits; 
	for ( ids=1; ids<dstShorts; ids++)
	    pdst[ ids] = *(unsigned *)&psrc[ids-1] >> 16-nbits; 
    }
    return dstShorts;
}
#endif
