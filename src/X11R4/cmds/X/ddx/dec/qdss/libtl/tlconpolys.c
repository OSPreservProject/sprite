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

/*
 *  convex polygons on the qdss
 *  written by drewry, february 1986
 *  hacked for qzss, 30 may 1986
 *
 *  edited by kelleher; sept 1986 -- changed stopping condition from
 *	if (iA == iMax)		to	if (yA == yMax)
 *	    Adone = TRUE;		    Adone = TRUE;
 *    Also changed the starting condition to deal with colinear
 *    vertices correctly -- see comments in code.
 */

#include <sys/types.h>

#include "X.h"
#include "windowstr.h"
#include "gcstruct.h"

/*
 * driver headers
 */
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>

#include	"tl.h"

#include "qd.h"
#include "qdgc.h"

#include "tltemplabels.h"

tlconpoly(pWin, pGC, npts, ppt)
    WindowPtr	pWin;
    GCPtr	pGC;
    int		npts;		/* one vertex count per polygon */
    DDXPointRec	*ppt;		/* vertices for all the polygons */
{
    RegionPtr	pSaveGCclip = QDGC_COMPOSITE_CLIP(pGC);
    int		nrects = REGION_NUM_RECTS(pSaveGCclip);
				/* number of rectangles to try to draw in */
    BoxPtr	prect = REGION_RECTS(pSaveGCclip);  /* clipping mask */
    register unsigned short     *p;
    int			ct;
    int iMin, iMax;      /* indices of min and max points */
    int yMin =  0x07fffffff;  /* minimum (first) y */
    int yMax = -0x07fffffff;  /* maximum (last) y */
    int ip;

#ifdef X11R4
    SETTRANSLATEPOINT(pWin->drawable.x, pWin->drawable.y);
#else
    SETTRANSLATEPOINT(pWin->absCorner.x, pWin->absCorner.y);
#endif
    
/* *was called via QZdopolys( pgc, count, npts, pPts, NULL, pclip) in PAINT*
 *
 *  A and B edges as in VCB02 video subsystem technical manual section 4.2.6
 *
 *  the dma buffer has
 *      while (count--)
 *      {
 *      	initialization
 *              JMPT_INITCONPOLY
 *              SOURCE_1_X
 *              DESTINATION_X
 *              DESTINATION_Y
 *      }
 *      TEMPLATE_DONE;
 *
 *  for each edge that runs out,
 *  if A<=B
 *     slow_dest_dx, slow_dest_dy
 *  else
 *     JMPT B_LE_A
 *  if B<=A
 *     source_1_dx, source_1_dy
 *  else
 *     JMPT CONPOLY
 *     ...
 *     DONE
 */
	/*
	 *  find lowest point in the list
	 */
	for (ip=0; ip < npts; ip++) {
	    if (ppt[ip].y < yMin) {
		iMin = ip;
		yMin = ppt[ip].y;
	    }
	    if (ppt[ip].y > yMax) {
		iMax = ip;
		yMax = ppt[ip].y;
	    }
	}

    if (yMax == yMin)
	return;

    for ( ; nrects > 0; nrects--, prect++) {
    register int iA;     /* index for current A edge */  
    register int iB;     /* index for current B edge */  
    register int iAnew;  /* index for next A edge */
    register int iBnew;  /* index for next B edge */
    int yA, yB;          /* current y value for each edge */
    int yAold;           /* temporary for left edge */
    int Adone, Bdone;    /* flags for checking doneness of each edge */

    Need_dma(11);
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = pGC->planemask;
    *p++ = umtable[pGC->alu];
    *p++ = JMPT_SETCOLOR;
    *p++ = pGC->fgPixel;
    *p++ = JMPT_SETCLIP;
    *p++ = prect->x1;
    *p++ = prect->x2;
    *p++ = prect->y1;
    *p++ = prect->y2;
    *p++ = JMPT_INITCONPOLY;
    Confirm_dma();

	/* output template data for running the edges */

	iA = iB = iMin;
	iBnew = ((iB == 0) ? npts-1 : iB-1);
	iAnew = (iA+1) % npts;
	Adone = Bdone = FALSE;

	/*
	 *  Make sure we don't expire both edges of the polygon
	 *  immediately.
	 */
	while (ppt[iA].y == ppt[iAnew].y) {
	    iA = iAnew;
	    iAnew = (iA+1) % npts;
	}
	yA = ppt[iA].y;
	yB = ppt[iB].y;

	/* template data for first vertex */
	Need_dma(3);
	*p++ = ppt[iB].x & 0x3fff; /* SOURCE_1_X */
	*p++ = ppt[iA].x & 0x3fff; /* DESTINATION_X */
	*p++ = ppt[iA].y & 0x3fff; /* DESTINATION_Y */
	Confirm_dma();

	while (!Adone || !Bdone) {
	    yAold = yA;
	    if ((yA <= yB) && !Adone) {
		Need_dma(2);
		*p++ = (ppt[iAnew].x - ppt[iA].x) & 0x3fff;
		*p++ = (ppt[iAnew].y - ppt[iA].y) & 0x3fff;
		Confirm_dma();
		yA = ppt[iAnew].y;
		iA = iAnew;
		iAnew = (iA+1) % npts;
		if (yA == yMax)
		    Adone = TRUE;
	    }
	    else
	    {
		Need_dma(1);
		*p++ = JMPT_B_LE_A;
		Confirm_dma();
	    }

	    /*
	     * while (ppt[iB].y == ppt[iBnew].y)
	     * {
	         * iB = iBnew;
	         * iBnew = ((iB == 0) ? npts-1 : iB-1);
	     * }
	     */
	    if ((yB <= yAold) && !Bdone) {
		Need_dma(2);
		*p++ = (ppt[iBnew].x - ppt[iB].x) & 0x3fff;
		*p++ = (ppt[iBnew].y - ppt[iB].y) & 0x3fff;
    		Confirm_dma();
		yB = ppt[iBnew].y;
		iB = iBnew;
		iBnew = ((iB == 0) ? npts-1 : iB-1);
		if (yB == yMax)
		    Bdone = TRUE;
	    }
	    else
	    {
		Need_dma(1);
		*p++ = JMPT_CONPOLY;
    		Confirm_dma();
	    }
	}
    }
    Need_dma(2);
    *p++ = JMPT_RESETRASTERMODE;
    *p++ = JMPT_RESET_FAST_DY_SLOW_DX;
    Confirm_dma();
}
