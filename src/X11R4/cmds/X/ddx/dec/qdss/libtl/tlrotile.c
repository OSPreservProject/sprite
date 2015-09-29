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

#include	"qdprocs.h"

#include "qd.h"
#include "tltemplabels.h"
#include "tl.h"

#define TILE_X 0
#define TILE_Y ScreenHeight

/* we don't care about the translate point.  rotile template code doesn't
 *	use it.  Confirm sets it itself.
 *
 * If pmagic is non-NULL, then ppixmap must be a "natural" Pixmap
 * (i.e. both size are powers of two --- and currently less than 32).
 * In that case. tlrotile repeats (and rotates) the ppixmap so that the
 * size is greater than the minimum (width 16, height 4), and installs it
 * in offscreen memory. Also, *pmagic is set to the value to use for the
 * source-2-height-and-width register [2E] of the Addr.
 * If pmagic is NULL, the tlrotile is used to repeat small non-natural tiles
 * to a larger size, so the caller (qdPolyFillBoxesOddSize) can be faster.
 */
VOID
tlrotile( ppixmap, patOrg, pmagic )
register QDPixPtr	ppixmap;
DDXPointRec	patOrg;
int		*pmagic;
{
    register unsigned short *p;
    int		tilexoff;	/* from tile origin to current point */
    int		tileyoff;	/* from tile origin to current point */
    int		nby = 0;	/* short count for rotile packs */
    int		xextra;	/* number of extra tiles on xaxis of rotated copy */
    int		yextra;	/* number of extra tiles on yaxis of rotated copy */
    QDPixPtr	dstPixmap;

    patOrg.x = UMOD( patOrg.x, QDPIX_WIDTH(&ppixmap->pixmap));
    patOrg.y = UMOD( patOrg.y, QDPIX_HEIGHT(&ppixmap->pixmap));
    /* round to dragon word boundary */
    if (pmagic == NULL) { /* called from qdPolyFillBoxesOddSize */
	xextra = 32 / QDPIX_WIDTH(&ppixmap->pixmap) - 1;
	yextra = 32 / QDPIX_HEIGHT(&ppixmap->pixmap) - 1;
    }
    else {
	if (QDPIX_WIDTH(&ppixmap->pixmap) < QD_MINXTILE) {
	    xextra = QD_MINXTILE / QDPIX_WIDTH(&ppixmap->pixmap) - 1;
#if QD_MINXTILE == 16
	    *pmagic = 2;
#else
	    *pmagic = power2bit(QD_MINXTILE) - 2;
#endif
	} else {
	    xextra = 0;
	    *pmagic = power2bit(QDPIX_WIDTH(&ppixmap->pixmap)) - 2;
	}
	if (QDPIX_HEIGHT(&ppixmap->pixmap) < QD_MINYTILE) {
	    yextra = QD_MINYTILE / QDPIX_HEIGHT(&ppixmap->pixmap) - 1;
#if QD_MINYTILE != 4
	    *pmagic |= (power2bit(QD_MINYTILE) - 2) << 4;
#endif
	} else {
	    yextra = 0;
	    *pmagic |= (power2bit(QDPIX_HEIGHT(&ppixmap->pixmap)) - 2) << 4;
	}
	*pmagic &= 0x3fff;
    }

    if (!tlConfirmPixmap( ppixmap ))
	FatalError( "tlrotile: could not store tile off-screen\n");

    /* planes+setvip+rotile_base + rotile_packs */
    for (tileyoff = patOrg.y + (yextra+1) * QDPIX_HEIGHT(&ppixmap->pixmap);
    	tileyoff > 0; tileyoff -= QDPIX_HEIGHT(&ppixmap->pixmap))
    {
        for (tilexoff = patOrg.x + (xextra+1) * QDPIX_WIDTH(&ppixmap->pixmap);
    	    tilexoff > 0; tilexoff -= QDPIX_WIDTH(&ppixmap->pixmap))
        {
    	    nby += 2;		/* rotile_packs */
	}
    }

    Need_dma(13+nby+1);	/* planes+vipers+rotile_base */
    *p++ = JMPT_SETALU;
    *p++ = LF_SOURCE|FULL_SRC_RESOLUTION; /*==umtable[GXcopy]*/
	/*
	 * RoTile
	 *	this rotates the offscreen tile.
	 *	the result is placed at tilex, tiley.
	 * DMA packet (RoTile)
	 *       srcx,srcy,dx,dy
	 *       clipx1,clipx2,clipy1.clipy2  (single clip rect)
	 *        {dstx,dsty}*     (new dest; same size, clip)
	 */
    if (ppixmap->pixmap.drawable.depth == 1) {
	*p++ = JMPT_INIT2COLORBITMAP;
        *p++ = ppixmap->planes;
    }
    else {
	*p++ = JMPT_SETSRC1OCRB;
	*p++ = EXT_NONE|INT_SOURCE|NO_ID|BAR_SHIFT_DELAY;
    }
    *p++ = JMPT_INITROTILE;
    *p++ = QDPIX_X(ppixmap);			/* src x */
    *p++ = QDPIX_Y(ppixmap);			/* src y */
    *p++ = QDPIX_WIDTH(&ppixmap->pixmap);	/* width */
    *p++ = QDPIX_HEIGHT(&ppixmap->pixmap);	/* height */
    *p++ = TILE_X;		/* bitblt clipping */
    *p++ = TILE_X + (xextra+1) * QDPIX_WIDTH(&ppixmap->pixmap);
    *p++ = TILE_Y;
    *p++ = TILE_Y + (yextra+1) * QDPIX_HEIGHT(&ppixmap->pixmap);
	/*
	 * four bitblt's (a la drewry)
	 */
    for (tileyoff = patOrg.y + (yextra+1)* QDPIX_HEIGHT(&ppixmap->pixmap);
      tileyoff > 0;
      tileyoff -= QDPIX_HEIGHT(&ppixmap->pixmap))
    {
	unsigned short dstY =
	    (TILE_Y + tileyoff - QDPIX_HEIGHT(&ppixmap->pixmap)) & 0x3fff;
        for (tilexoff =  patOrg.x + (xextra+1) * QDPIX_WIDTH(&ppixmap->pixmap);
    	  tilexoff > 0;
	  tilexoff -= QDPIX_WIDTH(&ppixmap->pixmap))
        {
	    *p++ = (TILE_X+tilexoff-QDPIX_WIDTH(&ppixmap->pixmap)) & 0x3fff;
	    *p++ = dstY;
        }
    }

    *p++ = JMPT_RESETRASTERMODE;
    /*----> Confirm that end of buffer is not exceeded*/
    Confirm_dma();
}
