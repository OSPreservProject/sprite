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

#include "X.h"			/* for CoordModeOrigin */

#include "windowstr.h"
#include "gcstruct.h"
#include "qd.h"
#include "qdgc.h"

#define MININT	0x80000000
#define MAXINT	0x7fffffff
#define NOINTERSECTION( pb1, pb2)  \
	(    (pb1)->x1 >= (pb2)->x2  \
	  || (pb1)->x2 <= (pb2)->x1  \
	  || (pb1)->y1 >= (pb2)->y2  \
	  || (pb1)->y2 <= (pb2)->y1)

/*
 * does the thin lines case only
 */
void
qdPolylines( pWin, pGC, mode, npt, pptInit)
    WindowPtr		pWin;
    GCPtr		pGC;
    int			mode;           /* Origin or Previous */
    int 		npt;            /* number of points */
    register DDXPointPtr pptInit;
{
    register DDXPointPtr abspts;  	/* actually, window-relative */
    register int	ip;		/* index into polygon list */
    register int	ir;		/* index into rectangle list */
    BoxRec		ptbounds;	/* used for trivial reject */
    BoxPtr		pclip, pclipi;	/* used for trivial reject */
    int			nclip;		/* used for trivial reject */
    RegionPtr		pdrawreg = QDGC_COMPOSITE_CLIP(pGC);
    Bool		allocated = FALSE;

    ptbounds.x1	= MAXINT;
    ptbounds.y1	= MAXINT;
    ptbounds.x2	= MININT;
    ptbounds.y2	= MININT;

    if ( npt == 0)		/* make sure abspts[0] is valid */
	return;
    if ( mode == CoordModeOrigin)
	abspts = pptInit;
    else	/* CoordModePrevious */
    {
	abspts = (DDXPointPtr) ALLOCATE_LOCAL( npt * sizeof( DDXPointRec));
        allocated = TRUE;
	abspts[ 0].x = pptInit[ 0].x;
	abspts[ 0].y = pptInit[ 0].y;
	for ( ip=1; ip<npt; ip++)
	{
	    abspts[ ip].x = abspts[ ip-1].x + pptInit[ ip].x;
	    abspts[ ip].y = abspts[ ip-1].y + pptInit[ ip].y;
	}
    }

    /*
     * Prune list of clip rectangles by trivial rejection of entire polyline.
     * Note that we have to add one to the right and bottom bounds, because
     * lines are non-zero width.
     */
    for ( ip=0; ip<npt; ip++)
    {
	ptbounds.x1 = min( ptbounds.x1, abspts[ ip].x);
	ptbounds.y1 = min( ptbounds.y1, abspts[ ip].y);
	ptbounds.x2 = max( ptbounds.x2, abspts[ ip].x+1);
	ptbounds.y2 = max( ptbounds.y2, abspts[ ip].y+1);
    }
    /*
     * translate ptbounds to absolute screen coordinates
     */
    ptbounds.x1 += pWin->absCorner.x;
    ptbounds.y1 += pWin->absCorner.y;
    ptbounds.x2 += pWin->absCorner.x;
    ptbounds.y2 += pWin->absCorner.y;

    nclip = 0;
    ir = REGION_NUM_RECTS(pdrawreg);
    pclip = (BoxPtr)ALLOCATE_LOCAL( ir * sizeof(BoxRec));
    pclipi = REGION_RECTS(pdrawreg);
    for (; --ir >= 0; pclipi++)
	if ( ! NOINTERSECTION( &ptbounds, pclipi)
	    pclip[ nclip++] = *pclipi;

    /*
     * draw the polyline
     */
    tlzlines( pWin, pGC, nclip, pclip, npt, abspts);

    /*
     * X11 wants the last pointed drawn, if not coincident with the first.
     */
    if (   abspts[ 0].x == abspts[ npt-1].x
	&& abspts[ 0].y == abspts[ npt-1].y)
	return;
    abspts[ 1] = abspts[ 0] = abspts[ npt-1];
    abspts[ 1].x++;	/* a length 1 line, direction shouldn't matter */

    tlzlines( pWin, pGC, nclip, pclip, 2, abspts);

    DEALLOCATE_LOCAL(pclip);
    if (allocated) DEALLOCATE_LOCAL(abspts);
        
}
