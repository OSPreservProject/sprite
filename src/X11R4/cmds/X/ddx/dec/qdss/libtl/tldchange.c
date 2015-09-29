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

#include	<sys/types.h>
#include	"Ultrix2.0inc.h"
#include	"miscstruct.h"
#include	"tldstate.h"
#include	"tltemplabels.h"

#include	"qd.h"
#include	<vaxuba/qduser.h>
#include	<vaxuba/qdreg.h>
#include	"tl.h"
#include "X.h"
#include "gcstruct.h"
#include "qdgc.h"

#define SETALU(planes, alu) \
{ int _planes_ = planes;\
  if (_planes_) {\
    *p++ = JMPT_SETPLANEMASK;\
    *p++ = _planes_;\
    *p++ = JMPT_SETALU;\
    *p++ = alu;\
}}

/* Used for CopyPlane and similar operations.
 * Compensates for protocol/GPX mismatch:
 * The Protocol requires the source to be expanded *before* being passed
 * to the ALU, while the GPX FG/BG MUX is *after* the ALU.
 * The trick is to ignore the MUX (let FG=ones and GB=zeros),
 * but set the ALU function to a different value for each plane,
 * depending on a combination of the alu with the fgPixel and bgPixel
 * for that plane.
 */
SetPlaneAlu(pGC)
     GCPtr	pGC;
{
    /* For planes where fgPixel==bgPixel, the result is independent of
     * the source, so the effective ALU reduces to one of the following. */
    static char map_alu[4] = {
	LF_ZEROS|FULL_SRC_RESOLUTION,	/* D = 0 */
	0xC|FULL_SRC_RESOLUTION,	/* D = D -- i.e. no-op */
	0x3|FULL_SRC_RESOLUTION,	/* D = NOT D */
	LF_ONES|FULL_SRC_RESOLUTION, };	/* D = 1 */

    register unsigned short     *p;
    int planemask = pGC->planemask & Allplanes;
    int fgPixel = pGC->fgPixel;
    int bgPixel = pGC->bgPixel;
    Need_dma(6 * NMASKSHORTS + 5*3 + 1);
    SETALU(Allplanes & ~planemask, 0xC|FULL_SRC_RESOLUTION); /* no-op */
    SETALU( fgPixel & ~bgPixel & planemask, umtable[pGC->alu]);
    SETALU(~fgPixel &  bgPixel & planemask, umtable[pGC->alu] & ~0x0040);
    SETALU(~fgPixel & ~bgPixel & planemask, map_alu[pGC->alu >> 2]);
    SETALU( fgPixel &  bgPixel & planemask, map_alu[pGC->alu & 3]);
    *p++ = JMPT_SETPLANEMASK;
    *p++ = Allplanes;
    Confirm_dma();
}

#define TILE_X 0
#define TILE_Y ScreenHeight

InstallState(pGC, pDraw)
    GCPtr pGC;
     DrawablePtr pDraw;
{
    register unsigned short     *p;

 /*
  * Uses the following pGC fields:
  * - fillStyle
  * - Depending on fillStyle:
  * - fgPixel, bgPixel, tile or stipple, patOrg
  * to set (depending on fillStyle):
  * - SOURCE
  * - SRC2_OCRB
  * - SOURCE2_X, SOURCE2_Y, SOURCE2_MAGIC
  */

    DDXPointRec		tileOrg;	/* off-screen rotated tile address */
    int			magic;
    int		mask;	/* planes used by offscreen dragon tile */
    int fore, back;

    switch(pGC->fillStyle) {
      case FillSolid:
	Need_dma(4+NCOLORSHORTS);
#if 0
	*p++ = MAC_SETCOLOR;
	SETCOLOR(p, fore);
#endif
	*p++ = JMPT_SETSRC2OCRB;
	*p++ = 0;
	Confirm_dma();
	break;
      case FillTiled:
	tlrotile(pGC->tile, pGC->patOrg, &magic);
	Need_dma(6);
	*p++ = JMPT_SETSRC2OCRB;
	*p++ = EXT_NONE|INT_SOURCE|NO_ID|BAR_SHIFT_DELAY;
        *p++ = JMPT_INIT2TILE;
	*p++ = TILE_X;
	*p++ = TILE_Y;
	*p++ = magic;
	Confirm_dma();
	break;
      case FillStippled:
	tlrotile(pGC->stipple, pGC->patOrg, &magic);
	Need_dma(7+NCOLORSHORTS);
#if 0
	*p++ = MAC_SETCOLOR;
	SETCOLOR(p, fore);
#endif
	*p++ = JMPT_SETSRC2OCRB;
	*p++ = EXT_NONE|12|NO_ID|BAR_SHIFT_DELAY; /* 12 == INT_M2 */
        *p++ = JMPT_INIT2TILE;
	*p++ = TILE_X;
	*p++ = TILE_Y;
	*p++ = magic;
	Confirm_dma();
	break;
      case FillOpaqueStippled:
	/* Confirm stipple in advance (instead of letting tlrotile
	 * do it) since Confirm assumes default values in fg/bg registers. */
	if (!tlConfirmPixmap(pGC->stipple))
	    FatalError("InstallState: cannot Confirm stipple");
	Need_dma(1+2*NCOLORSHORTS);
	*p++ = MAC_SETFOREBACKCOLOR;
	if (pDraw && pDraw->depth == 1) {
	    fore = pGC->fgPixel ? Allplanes : 0;
	    back = pGC->bgPixel ? Allplanes : 0;
	}
	else {
	    fore = pGC->fgPixel;
	    back = pGC->bgPixel;
	}
	SETCOLOR(p, fore);
	SETCOLOR(p, back);
	Confirm_dma();
	tlrotile(pGC->stipple, pGC->patOrg, &magic);
	Need_dma(7);
	*p++ = JMPT_SETSRC2OCRB;
	*p++ = EXT_NONE|INT_SOURCE|NO_ID|BAR_SHIFT_DELAY;
        *p++ = JMPT_INIT2TILE;
	*p++ = TILE_X;
	*p++ = TILE_Y;
	*p++ = magic;
	*p++ = JMPT_RESET_FORE_BACK;
	Confirm_dma();
	break;
    }
    InstalledGC = pGC;
}
