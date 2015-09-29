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
#include "miscstruct.h"
#include "pixmapstr.h"
#include "region.h"
#include "mistruct.h"
#include "gc.h"		/* for NEXT_SERIAL_NUMBER */

#include "Xmd.h"
#include "servermd.h"

#include "qd.h"

extern	int	Nchannels;

int PixmapUseOffscreen = 7;
/* masks:
 * 1 - use new gc code (instea of mfbCreateGC)
 * 2 - use offscreen for bitmaps
 * 4 - use offscreen for pixmaps
 */

PixmapPtr
qdCreatePixmap( pScreen, width, height, depth)
    ScreenPtr pScreen;
    int width;
    int height;
    int depth;
{
    register QDPixPtr qdpix;
    int		nbytes;

 /* NOTE: the resulting PixMap must be compatible with mfb pixmaps! */

    /*
     * allocate the generic and the private pixmap structs in one chunk
     */
    qdpix = (QDPixPtr) Xalloc( sizeof(QDPixRec));
    if ( !qdpix)
	return (PixmapPtr)NULL;

    qdpix->pixmap.drawable.type = DRAWABLE_PIXMAP;
    qdpix->pixmap.drawable.pScreen = pScreen;
    qdpix->pixmap.drawable.depth = depth;
#ifdef X11R4
    qdpix->pixmap.drawable.class = 0;
    qdpix->pixmap.drawable.bitsPerPixel = depth == 4 ? 8 : depth;
    qdpix->pixmap.drawable.id = 0;
#endif
    qdpix->pixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
    QDPIX_WIDTH(&qdpix->pixmap) = width;
    QDPIX_HEIGHT(&qdpix->pixmap) = height;
    qdpix->pixmap.refcnt = 1;

    QDPIX_X(qdpix) = 0;
    QDPIX_Y(qdpix) = 0;

    nbytes = depth > 1 ? width * Nchannels : PixmapBytePad(width, 1);
    qdpix->pixmap.devKind = nbytes;
    if (PixmapUseOffscreen & (depth > 1 ? 4 : 2)) {
	QD_PIX_DATA(&qdpix->pixmap) = NULL;
	if (tlConfirmPixmap(qdpix))
	    return (PixmapPtr)qdpix;
    }
    qdpix->planes = 0;
    nbytes *= height;
    if (   nbytes <= 0
	|| (QD_PIX_DATA(&qdpix->pixmap) = (unsigned char *) Xalloc( nbytes)) == NULL)
    {
    	Xfree( qdpix);
	return (PixmapPtr)NULL;
    }
    return (PixmapPtr)qdpix;
}

/*
 * DIX assumes DestroyPixmap may be called with NULL arg
 */
Bool
qdDestroyPixmap( pPixmap)
    PixmapPtr pPixmap;
{
#ifndef X11R4
    if ( ! IS_VALID_PIXMAP(pPixmap))
	return TRUE;
#endif
    if ( --pPixmap->refcnt != 0)
	return TRUE;

    if (QD_PIX_DATA(pPixmap) != NULL)
	Xfree(QD_PIX_DATA(pPixmap));
    QD_PIX_DATA(pPixmap) = (unsigned char*)1; /* hack to prevent copying */
    tlCancelPixmap((QDPixPtr)pPixmap);
    Xfree(pPixmap);
    return TRUE;
}


static int
sizeQDdata( pPix)
    PixmapPtr	pPix;
{
    switch ( pPix->drawable.depth)
    {
      case 1:
	return PixmapWidthInPadUnits( QDPIX_WIDTH(pPix), 1);
      case NPLANES:
      default:
	return QDPIX_WIDTH(pPix) * QDPIX_HEIGHT(pPix) * Nchannels;
    }
}

static
PixmapPtr
QD1toNplanes( pBit, fgpixel, bgpixel)
    PixmapPtr   pBit;
    int		fgpixel, bgpixel;
{
    register unsigned char *	newbytes;
    register unsigned char *	oldbits = QD_PIX_DATA(pBit);
    register int	r, c;
    register int	pixel;
    

    newbytes = (unsigned char *) Xalloc( sizeQDdata( pBit));
    /*
     * Now expand the bitmap and two colors into the full-depth pixmap
     */
    for ( r=0; r < QDPIX_HEIGHT(pBit); r++)
	for ( c=0; c < QDPIX_WIDTH(pBit); c++)
	{
	    pixel = oldbits[ r*QPPADBYTES( QDPIX_WIDTH(pBit)) + (c>>3)] & 1<<(c&0x7)
		? fgpixel
		: bgpixel;
	    newbytes[ r * QDPIX_WIDTH(pBit) + c] = pixel;
#if NPLANES==24
	    newbytes[ (QDPIX_HEIGHT(pBit)+r)*QDPIX_WIDTH(pBit) + c] = pixel >> 8;
	    newbytes[ ((QDPIX_HEIGHT(pBit)<<1)+r)*QDPIX_WIDTH(pBit) + c] = pixel >> 16;
#endif
	}

    Xfree( QD_PIX_DATA(pBit));
    QD_PIX_DATA(pBit) = newbytes;
    return pBit;	/* now a PixMap */
}

/*
* This macro evaluates to TRUE if i is an integer power of 2; FALSE otherwise.
* It uses this trick from Kernighan and Ritchie:  in a 2's complement number
* system, (i & (i-1)) clears the rightmost 1-bit in i.
*/

#define POWER_OF_2(i) \
	(((i) & ((i) - 1)) == 0)

/*  Return TRUE if this pixmap is natural size:
 *      its width and height must be integer powers of 2;
 *      the hardware requires that the width be at least 16 and the
 *      height be at least 4, but tlrotile duplicates the pixmap if needed;
 *      the hardware allows width and height upto 512, however,
 *      for software convenience, we only allow upto 32.
 */

Bool
qdNaturalSizePixmap(pPixmap)
	PixmapPtr pPixmap;
{
	int w;
	int h;

	if (pPixmap == (PixmapPtr) NULL)
	{
		return(FALSE);
	}

	w = QDPIX_WIDTH(pPixmap);
	h = QDPIX_HEIGHT(pPixmap);

	return POWER_OF_2(w) && w <= 32 && POWER_OF_2(h) && h <= 32;
}
