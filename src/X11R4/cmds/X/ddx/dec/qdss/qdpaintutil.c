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

/* #include "X3lib.h" */
#include "miscstruct.h"		/* required for misc.h */
#include "misc.h"		/* includes BoxPtr */
#include "pixmapstr.h"		/* includes BoxPtr */

#include "qdlib.h"

x11clipTopaintclip( pX, pP, n)
    BoxPtr		pX;
    X3ScreenRect *	pP;
    int			n;
{
    while (n--)
    {
	pP->x = pX->x1;
	pP->y = pX->y1;
	pP->w = pX->x2 - pX->x1;
	pP->h = pX->y2 - pX->y1;
	pX++; pP++;
    }
}

/*
 * allocates NO storage, takes a copy of source's data pointer
 */
x11PixmapToQPIXMAP( xPix, qPix)
    PixmapPtr	xPix;
    QPIXMAP *	qPix;
{
    qPix->w = xPix->width;
    qPix->h = xPix->height;
    if ( xPix->drawable.depth > 1)
	qPix->widthBytes = xPix->width;
    else	/* single plane */
	qPix->widthBytes = QPPADBYTES( xPix->width);
    qPix->bits = QD_PIX_DATA(xPix);
    qPix->numPlanes = xPix->drawable.depth;
    qPix->tileMagic = 0;		/* Is this looked at? XXX */
}
