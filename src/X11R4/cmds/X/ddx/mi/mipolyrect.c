/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
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
/* $XConsortium: mipolyrect.c,v 5.1 89/07/14 17:14:10 keith Exp $ */
#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmap.h"

void
miPolyRectangle(pDraw, pGC, nrects, pRects)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		nrects;
    xRectangle	*pRects;
{
    int i;
    xRectangle *pR = pRects;
    DDXPointRec rect[5];

    for (i=0; i<nrects; i++)
    {
	rect[0].x = pR->x;
	rect[0].y = pR->y;

	rect[1].x = pR->x + (int) pR->width;
	rect[1].y = rect[0].y;

	rect[2].x = rect[1].x;
	rect[2].y = pR->y + (int) pR->height;

	rect[3].x = rect[0].x;
	rect[3].y = rect[2].y;

	rect[4].x = rect[0].x;
	rect[4].y = rect[1].y;

        (*pGC->ops->Polylines)(pDraw, pGC, CoordModeOrigin, 5, rect);
	pR++;
    }
}
