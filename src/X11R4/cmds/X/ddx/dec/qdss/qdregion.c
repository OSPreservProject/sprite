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

#include "regionstr.h"

#ifndef X11R4
#define BOXBOUND( b1, b2)  \
	(b1)->x1 = min( (b1)->x1, (b2)->x1);  \
	(b1)->y1 = min( (b1)->y1, (b2)->y1);  \
	(b1)->x2 = max( (b1)->x2, (b2)->x2);  \
	(b1)->y2 = max( (b1)->y2, (b2)->y2);
#endif
/*
 * Does what you really want; creates a region and initializes it
 * to contain the argument boxes.
 *
 * Always creates at least one box, of zero size if necessary, although it is
 * hidden from the caller, as numRects is set to zero
 */
RegionPtr
qdRegionInit( boxes, nbox)
    register BoxPtr	boxes;	/* length must agree with nbox */
    register int	nbox;	/* */
{
#ifdef X11R4
/* The code is simplified from miRectsToRegion in miregion.c */
    extern RegionPtr miRegionCreate();
    register RegionPtr	pRgn;
    register BoxPtr	pBox;
    register int        i;
    Bool overlap; /* result ignored */

    pRgn = miRegionCreate(NullBox, 0);
    if (!nbox)
	return pRgn;
    if (nbox == 1)
    {
	pRgn->extents = *boxes;
	pRgn->data = (RegDataPtr)NULL;
	return pRgn;
    }
    pRgn->data = (RegDataPtr)Xalloc(REGION_SZOF(nbox));
    if (pRgn->data == NULL) {
	return pRgn;
    }
    pRgn->data->size = nbox;
    pRgn->data->numRects = nbox;
    for (i = nbox, pBox = REGION_BOXPTR(pRgn); --i >= 0; boxes++)
    {
	*pBox = *boxes;
	if ((pBox->x2 <= pBox->x1) || (pBox->y2 <= pBox->y1))
	    pRgn->data->numRects--;
	else
	    pBox++;
    }
    pRgn->extents.x1 = pRgn->extents.x2 = 0;
    miRegionValidate(pRgn, &overlap);
    return pRgn;
#else
    register RegionPtr	temp;    /* new region */
   
    temp = (RegionPtr) Xalloc (sizeof (RegionRec));
    temp->numRects = nbox;
    temp->size = max(1, nbox);
    temp->rects = (BoxPtr) Xalloc( temp->size * (sizeof(BoxRec)));
    if ( nbox == 0)
    {
        temp->extents.x1 = temp->rects[0].x1 = 0;
        temp->extents.y1 = temp->rects[0].y1 = 0;
        temp->extents.x2 = temp->rects[0].x2 = 0;
        temp->extents.y2 = temp->rects[0].y2 = 0;
    }
    else
    {
	int			ib;
	register BoxPtr		temprects;

        temp->extents.x1 = MAXSHORT;
        temp->extents.y1 = MAXSHORT;
        temp->extents.x2 = MINSHORT;
        temp->extents.y2 = MINSHORT;

	for (	ib=0, temprects=temp->rects;
		ib<nbox;
		ib++, boxes++, temprects++)
	{
	    BOXBOUND( &temp->extents, boxes);
	    *temprects = *boxes;
	}
    }
    miRegionValidate(temp);
    return(temp);
#endif
}
