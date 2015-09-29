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

#include <errno.h>
#include <sys/types.h>
#include <stdio.h>		/* for debugging */

#include "X.h"
#include	"gcstruct.h"
#include "windowstr.h"

#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>
#include "qd.h"
#include "qdgc.h"
#include <vaxuba/qdioctl.h>  /* XXX - for bell-style debugging */
#include "tl.h"
#include "tlsg.h"
#include "tltemplabels.h"
#include "mi.h"

#define PIXELSIZE (NPLANES/8)	/* per-pixel image stepping */
# define JUSTGREEN(x) GREEN(x)

DDXPointRec ZeroPoint = {0, 0};
    static BoxRec NoClipBox = {0, 0, 1024, 2048};

extern int errno;
extern int Vaxstar;
extern	int	Nplanes;
extern	int	Nchannels;
extern unsigned int Allplanes;
static VOID shutengine(), preptb(), ptbpackets(), btpzblock(), btpspa();

static UsePtbPackets = 0;

/* ptbspa - processor to bitmap (aka set pixel array)             *
 *   Pre Inited:  source, planes, masks, r_mode, clip, alu, trans *
 * Dump pixels on the screen.  This either uses the PTBZ command  *
 *   and slow adder polling, or if possible, it does a direct     *
 *   dma-mapped write (for the gpx) or fifo-writes (for the star) *
 */

static VOID
ptbspa(pGC, dstx, dsty, dstw, dsth, realw, data, nclip, pclip, trans)
     GCPtr pGC;
     int dstx, dsty, dstw, dsth;
     int realw;
     unsigned char *data;
     register BoxPtr pclip;
     register int nclip;
     DDXPointRec trans;
{
    register unsigned char *ppixel;
    int blockmax = min(req_buf_size, MAXDMAPACKET/sizeof(short)) - 6;
    register unsigned short	*p;

    SETTRANSLATEPOINT(trans.x, trans.y);

    /*
     * system hangs in following WAITDMADONE, 
     * before first xterm comes up, if these three lines are
     * omitted: WHY?            XX
     */
    Need_dma(6);
    *p++ = JMPT_SET_MASKED_ALU;
    if (pGC) {
	*p++ = pGC->planemask;
	*p++ = umtable[pGC->alu];
    }
    else {
	*p++ = 0xFF;
	*p++ = LF_S | FULL_SRC_RESOLUTION;
    }
    *p++ = JMPT_RESETCLIP;
    *p++ = TEMPLATE_DONE;

    *p++ = JMPT_INITZBLOCKPTOB;	/* because this polls correctly. */
    Confirm_dma();

    for ( ; --nclip >= 0; pclip++)
    {
        register int tx, ty, tw, th, iy;
	int nscansperblock;
	
	tx = max(pclip->x1 - trans.x, dstx);
        tw = min(pclip->x2 - trans.x, dstx+dstw) - tx;
	ty = max(pclip->y1 - trans.y, dsty);
        th = min(pclip->y2 - trans.y, dsty+dsth) - ty;
	if (tw <= 0 || th <= 0)
	    continue;		/* nothing to draw in this clip rectangle */
	ppixel = data + (ty - dsty) * realw + tx - dstx;

	nscansperblock = blockmax / tw;
	if (UsePtbPackets ? nscansperblock < th : !nscansperblock) {
	    ptbpackets(tx, ty, tw, th, realw, (tw * th + 1) / 2, ppixel);
	    continue;
	}

	while (th > 0) {
	    register unsigned short *p;
	    register int scanpixels;
	    register int nscans = min(nscansperblock, th);
	    th -= nscans;

	    scanpixels = tw*nscans;
	    Need_dma( scanpixels + 6 );	/* better be <= req_buf_size */
	    *p++ = JMPT_ZBLOCKPTOB;
	    *p++ = tx;
	    *p++ = ty;
	    *p++ = tw;
	    *p++ = nscans;
	    ty += nscans;
	    *p++ = 0x6000 | (0x1fff & -scanpixels); /*must be same packet*/
	    while (--nscans >= 0) {
		scanpixels = tw;
		while (--scanpixels >= 0)
		    *p++ = *ppixel++;
		ppixel += realw - tw;
	    }
	    Confirm_dma();
	}
    }
}

/*
 *  tlputimage
 *
 *  put a rectangle of Z-mode data into the framebuffer.
 */
int
tlputimage( pwin, pgc, x, y, w, h, pcolor)
    WindowPtr		pwin; /* not used */
    GCPtr		pgc;
    register int	x, y, w, h;
    register unsigned char *pcolor;
{
    RegionPtr pGCclip = QDGC_COMPOSITE_CLIP(pgc);

    ptbspa(pgc, x, y, w, h, w, pcolor,
	   REGION_NUM_RECTS(pGCclip), REGION_RECTS(pGCclip),
	   pgc->lastWinOrg);
}

tlSaveAreas (pPixmap, prgnSave, xorg, yorg)
    PixmapPtr	  	pPixmap;  	/* Backing pixmap */
    RegionPtr	  	prgnSave; 	/* Region to save (pixmap-relative) */
    int	    	  	xorg;	    	/* X origin of region */
    int	    	  	yorg;	    	/* Y origin of region */
{
    if (!QD_PIX_DATA(pPixmap)) { /* offscreen */
	int xSave = QDPIX_X((QDPixPtr)pPixmap);
	int ySave = QDPIX_Y((QDPixPtr)pPixmap);
	GC tmpGC;
#ifndef X11R4
	QDPrivGCRec tmpQDGC;
	tmpGC.devPriv = (pointer)&tmpQDGC;
#endif
	tmpGC.planemask = -1;
	tmpGC.alu = GXcopy;
#ifdef X11R4
	miTranslateRegion(prgnSave, xSave, ySave);
	tlbltregion(&tmpGC, prgnSave,
		    xSave - xorg,
		    ySave - yorg);
	miTranslateRegion(prgnSave, -xSave, -ySave); /* fix */
#else
	QDGC_COMPOSITE_CLIP(&tmpGC) = prgnSave;
	miTranslateRegion(prgnSave, xSave, ySave);
	tlbitblt(&tmpGC, xSave, ySave,
		 QDPIX_WIDTH(pPixmap), QDPIX_HEIGHT(pPixmap), xorg, yorg);
#endif
	miTranslateRegion(prgnSave, -xSave, -ySave); /* fix */
    } else {
	register VOLATILE struct adder *adder = Adder;	/* must be in a reg */
	register u_short		*p;
	register BoxPtr pclip = REGION_RECTS(prgnSave);
	register int nclip    = REGION_NUM_RECTS(prgnSave);

	SETTRANSLATEPOINT( 0, 0 );
	shutengine(Allplanes);
	write_ID( adder, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_ONES);

	for ( ; nclip > 0; nclip--, pclip++)
	    {
		register int tx, ty, tw, th, iy;
		register unsigned char *tcolor;	/* this stream of image data */
	
		tx = pclip->x1;
		tw = min(pclip->x2, 1024) - tx;
		ty = pclip->y1;
		th = min(pclip->y2, 864) - ty;
		if (tw <= 0 || th <= 0)
		    continue;	/* nothing to draw in this clip rectangle */
		tcolor = QD_PIX_DATA(pPixmap) + ty * QDPIX_WIDTH(pPixmap) + tx;
		btpspa (tx+xorg, ty+yorg, tw, th, QDPIX_WIDTH(pPixmap),tcolor);
	    }
	dmafxns.enable();
    }
}

tlRestoreAreas (pPixmap, prgnRestore, xorg, yorg)
    PixmapPtr	  	pPixmap;  	/* Backing pixmap */
    RegionPtr	  	prgnRestore; 	/* Region to restore (screen-relative)*/
    int	    	  	xorg;	    	/* X origin of window */
    int	    	  	yorg;	    	/* Y origin of window */
{
    if (!QD_PIX_DATA(pPixmap)) { /* offscreen */
	GC tmpGC;
#ifndef X11R4
	QDPrivGCRec tmpQDGC;
	tmpGC.devPriv = (pointer)&tmpQDGC;
#endif
	tmpGC.planemask = -1;
	tmpGC.alu = GXcopy;
#ifdef X11R4
	tlbltregion(tmpGC, prgnRestore,
		    xorg - QDPIX_X((QDPixPtr)pPixmap),
		    yorg - QDPIX_Y((QDPixPtr)pPixmap));
#else
	QDGC_COMPOSITE_CLIP(&tmpGC) = prgnRestore;
	tlbitblt(&tmpGC, xorg, yorg,
		 QDPIX_WIDTH(pPixmap), QDPIX_HEIGHT(pPixmap),
		 QDPIX_X((QDPixPtr)pPixmap), QDPIX_Y((QDPixPtr)pPixmap));
#endif
    } else {
	DDXPointRec trans;

	trans.x = xorg; trans.y = yorg;
	ptbspa(0, 0, 0, 1024, 864,
	       QDPIX_WIDTH(pPixmap) * (NPLANES/8),
	       QD_PIX_DATA(pPixmap),
	       REGION_NUM_RECTS(prgnRestore),
	       REGION_RECTS(prgnRestore),
	       trans);
    }
}

/* write offscreen-allocated full-depth Pixmap. no clipping, no trans */
tlspadump(pPix)
     register QDPixPtr pPix;
{
    ptbspa(0, QDPIX_X(pPix), QDPIX_Y(pPix),
	   QDPIX_WIDTH(&pPix->pixmap), QDPIX_HEIGHT(&pPix->pixmap),
	   QDPIX_WIDTH(&pPix->pixmap),
	   QD_PIX_DATA(&pPix->pixmap),
	   1, &NoClipBox, ZeroPoint);
}

/* XXX - is src_1 supposed to be translated? */

RegionPtr
tlspaca( ppix, pwin, pgc, srcx, srcy, w, h, dstx, dsty )
QDPixPtr	ppix;
WindowPtr	pwin; /* not used: pGC->lastWinOrg used instead */
GCPtr		pgc;
int		srcx, srcy, w, h, dstx, dsty;
{
    int dstw, dsth, winx, winy;
    int x1, x2, y1, y2;
    RegionPtr prgnExposed = NULL;

    if (QDPIX_Y(ppix) != NOTOFFSCREEN
      || (QDPIX_WIDTH(&ppix->pixmap) <= 1024 &&
          QDPIX_HEIGHT(&ppix->pixmap) <= DragonPix))
    {
	if (! tlConfirmPixmap( ppix ))
	    FatalError( "tlspaca: could not store pixmap off-screen\n");
	tlSinglePixmap(ppix);
	x1 = srcx;
	y1 = srcy;

        /* Shrink as necessary to fit pixmap boundaries */
        if (x1 < 0)
            x1 = 0;
        if (y1 < 0)
            y1 = 0;
	
        winx = dstx + pgc->lastWinOrg.x;
        winy = dsty + pgc->lastWinOrg.y;
	if (x1 + w > QDPIX_WIDTH(&ppix->pixmap))
	    dstw = QDPIX_WIDTH(&ppix->pixmap) - x1;
        else
	    dstw = w;
	if (y1 + h > QDPIX_HEIGHT(&ppix->pixmap))
	    dsth = QDPIX_HEIGHT(&ppix->pixmap) - y1;
        else
	    dsth = h;
	tlbitblt( pgc, winx, winy,
		dstw, dsth,
		x1+QDPIX_X(ppix), y1+QDPIX_Y(ppix));
    }
    else {
	RegionPtr pClipRegion = QDGC_COMPOSITE_CLIP(pgc);
	ptbspa(pgc, dstx, dsty, w, h, QDPIX_WIDTH(&ppix->pixmap),
	       QD_PIX_DATA(&ppix->pixmap) +
	           srcy * QDPIX_WIDTH(&ppix->pixmap) + srcx,
	       REGION_NUM_RECTS(pClipRegion), REGION_RECTS(pClipRegion),
	       pgc->lastWinOrg);
    }

    if (pgc->graphicsExposures)
        prgnExposed = miHandleExposures(ppix, pwin, pgc,
                          srcx, srcy,
                          w, h, 
                          dstx, dsty, 0);
    return prgnExposed;
}

/* This is extremely gross.  Why is this necessary?  */
static VOID
shutengine(planemask)
unsigned long   planemask;
{
    register unsigned short	*p;
    register VOLATILE struct adder *adder = Adder;

    /*
     * system hangs in following WAITDMADONE, 
     * before first xterm comes up, if these three lines are
     * omitted: WHY?            XX
     */
    Need_dma(3);
    *p++ = JMPT_SETPLANEMASK;
    *p++ = GREEN(planemask);
    *p++ = TEMPLATE_DONE;
    Confirm_dma ();

    dmafxns.flush ( TRUE);      /* spin until all dragon FIFOs are empty */
#if 0
    write_ID(adder, CS_UPDATE_MASK, GREEN(planemask));
    write_ID( adder, FOREGROUND_COLOR, 0xffff );
    write_ID( adder, BACKGROUND_COLOR, 0x0000 );
    write_ID( adder, MASK_1, 0xffff );    /* necessary??? */
#endif
}

/***************************************************************
 * setuptransfer - this does some basic setup for ptb or btp's. *****
 * This is designed to work for stepping or contiguous transfers.    *****
 * It is hoped that in the future this will honor the soft dragon shadow. *
 * Pre Setup:  clear running fifo or dma packets                         *
 * Setup:  setup x, y, w, h in dragon source and destination registers  *
 *         planemasks as given                                         *
 *         initialize dragon fg, bg, and mask registers to sane values *
 * Not Setup:  rasterop_mode                                        ***
 *             -> if hardware-translating then                ******
 *              * hardware translate point              ******
 *             -> if writing-enabled then         ******
 *              * clipping                  ******
 *              * alu                 ******
 *             command register ******
 *******************************/
static
setuptransfer(x, y, w, h, mode)
    int		x, y, w, h, mode;
{
    register VOLATILE struct adder	*adder = Adder;

    poll_status( adder, RASTEROP_COMPLETE );
    poll_status( adder, ADDRESS_COMPLETE );
    poll_status( adder, TX_READY );

    adder->source_1_dx = 1;	/* positive horizontal scanning */
    adder->source_1_dy = 1;	/* positive vertical scanning */
    adder->destination_x = x;
    adder->destination_y = y;
    adder->fast_dest_dx = w;
    adder->fast_dest_dy = 0;
    adder->slow_dest_dx = 0;
    adder->slow_dest_dy = h;

    /* Set up viper registers for PTBZ transfer.
     * Actual data flow is observed to be very different from that
     * described in manual:  bytes from the DMA chip go into the viper SOURCE
     * register rather than FOREGROUND or BACKGROUND.
     */
}

static VOID
ptbpackets( x, y, w, h, realw, nwords, ppixel )
    int unsigned x, y, w, h, realw;
    register int nwords;
    unsigned char *ppixel;
{
    register unsigned short *p;
    register int cwords;	/* total words in this packet */
    register int scaninc = 0;	/* current offset from start of this scan */
    register int bytesleft;	/* bytes left in this packet (<= cwords*2) */
    register int scanbytes;	/* number of bytes to copy off this scan */
    unsigned char lastbyte;
    register unsigned char *dma_byte;
    extern struct DMAreq *DMArequest;
    extern short sg_fifo_func;

    Need_dma( 6 );
    *p++ = JMPT_PTOB;	/* this does not 'loadd' the command reg. */
    *p++ = x;
    *p++ = y;
    *p++ = w;
    *p++ = h;
    *p++ = TEMPLATE_DONE;
    Confirm_dma();
    dmafxns.flush( TRUE );	/* just to ream out the template packets */
    Adder->command = PTBZ;	/* must be here or a_comp never flushed */
    while (nwords > 0)
    {
	/* We must now assemble the packets.  Internally, the packets must
	 * be accurately copied to the precision of one byte.  The final byte
	 * must be word-padded (the fifo dma engine only recognizes lengths of
	 * shorts -- sorry).  The restrictions are:  packets cannot exceeed
	 * req_buf_size in length, scanlines may be discontiguous, and of
	 * course we eventually hit a tricky final packet length.  The basic
	 * model is to ask for req_buf_size words, fill it up scanline at a
	 * time or in one big chunk, and ask for more when we run out, making
	 * sure to ask for a minimal amount on the last packet.  */
	bytesleft = 2 * (cwords = min( nwords, req_buf_size ));
	General_dma( cwords, FIFO_EMPTY | BYTE_PACK );
	dma_byte = (unsigned char *) dma_word;
	if (!Vaxstar)
	    DMArequest->DMAtype = PTOB;
	else {
	    sg_fifo_type = PTOB;
	    sg_fifo_func = PTB_UNPACK_ENB;
	}
	if (w == realw) {
	    bcopy( ppixel, dma_byte, bytesleft ); /* movec the data */
	    ppixel += bytesleft;
	} else
	    while (bytesleft > 0) {
		scanbytes = min( w - scaninc, bytesleft );
		if (((int) dma_byte) & 1) {
		    bcopy( ppixel + 1, dma_byte + 1, scanbytes - 1 );
		    *((unsigned short *) (dma_byte - 1)) =
			lastbyte + (((unsigned short) *ppixel) << 8);
		} else {
		    if (scanbytes & ((int) 1)) {
			bcopy( ppixel, dma_byte, scanbytes - 1 );
			lastbyte = ppixel[scanbytes - 1];
		    } else
			bcopy( ppixel, dma_byte, scanbytes );
		}
		scaninc = (scaninc + scanbytes) % w;
		bytesleft -= scanbytes;
		dma_byte += scanbytes;
		if (scaninc)
		    ppixel += scanbytes;
		else
		    ppixel += scanbytes + realw - w;
	    }
	if (scanbytes & ((int) 1) && ((int) dma_byte) & 1)
	    *((unsigned short *) (dma_byte - 1)) = lastbyte;
	dma_word += cwords;
	nwords -= cwords;
    }
    dmafxns.flush( TRUE );	/* get rid of ptb packets */
    Need_dma(1);
    *p++ = JMPT_RESET_FORE_BACK;
    Confirm_dma();
}

/*
 *  tlgetspan
 *
 *  written by Brian Kelleher; June 1986
 *  severely hacked by matt.
 *
 *  Get a span of Z-mode data from the framebuffer.
 */
int
tlgetspan( pwin, x, y, w, pcolor)
    WindowPtr		pwin;
    register int	x, y, w;
    register unsigned char *pcolor;
{
    register VOLATILE struct adder *adder = Adder; /* must be in a register */
    register u_short		*p;
 
    /*
     *  Wait for the DMA buffer to empty, and then for the fifo
     *  maintained by the gate array to empty.
     */
    dmafxns.flush ( TRUE);

    write_ID( adder, CS_UPDATE_MASK, 0xffff);
    write_ID( adder, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_ONES);

    adder->source_1_dy = 1;
    adder->slow_dest_dx = 0;
    adder->slow_dest_dy = 1;
    adder->fast_dest_dx = w;
    adder->fast_dest_dy = 0;
    adder->error_1 = 0;
    adder->error_2 = 0;

    btpzblock( x, y, w, 1, 0xff, pcolor, ZGREEN);
    dmafxns.enable();

/* shouldn't be needed, but somebody is trashing FG */
    Need_dma(1);
    *p++ = JMPT_RESET_FORE_BACK;
    Confirm_dma();
}

/*
 *  tlgetimage
 *
 *  Get a rectangle of Z-mode data from the framebuffer.
 */
int
tlgetimage( pwin, x, y, w, h, planemask, pcolor)
    WindowPtr		pwin;
    register int	x, y, w, h;
    unsigned long	planemask;
    register unsigned char *pcolor;
{
    register VOLATILE struct adder *adder = Adder; /* must be in a register */
    register u_short		*p;

    shutengine(planemask);
    write_ID( adder, DST_OCR_A, EXT_NONE|INT_NONE|NO_ID|NO_WAIT); 
    write_ID( adder, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_ONES);

#ifdef X11R4
    x += pwin->drawable.x;
    y += pwin->drawable.y;
#else
    x += pwin->absCorner.x;
    y += pwin->absCorner.y;
#endif

    setuptransfer(x, y, w, h, NORMAL); /* no source indexing, (negatives) */
    btpzblock( x, y, w, h, GREEN(planemask), pcolor, ZGREEN);
    dmafxns.enable();
    Need_dma(2);
    *p++ = JMPT_SETPLANEMASK;
    *p++ = 0xFF;
    Confirm_dma();
}

/*
 *  Get RGB data from the framebuffer.
 */
static VOID
btpzblock( x, y, w, h, planebyte, pcolor, zblock)
    int				x, y;
    register int		w, h;
    register unsigned char	planebyte;
    register unsigned char *	pcolor;
    int				zblock;
{
    register int		width;
    register VOLATILE struct adder *adder = Adder;  /* must be in a register */

    adder->source_1_x = x;
    adder->source_1_y = y;
    adder->source_1_dx = 1;
    adder->command = (BTPZ | zblock);
    adder->request_enable = RX_READY;
    while (h--)
    {
	for (width = w; width > 0; width--)
	{
	    POLL_STATUS( adder, RX_READY);
	    *pcolor++ = (u_char)adder->id_data & planebyte;
	}
    }
    poll_status( adder, ADDRESS_COMPLETE); 
}

static VOID
btpspa ( x, y, w, h, realw, ppixel)
    int x, y, w, h;
    int realw;
    unsigned char *ppixel;
{
    register int		width;
    register VOLATILE struct adder *adder = Adder; /* must be in a register */
    register unsigned char	*pcolor;

    adder->source_1_x = x;
    adder->source_1_y = y;
    adder->source_1_dx = 1;
    adder->source_1_dy = 1;
    adder->fast_dest_dx = w;
    adder->fast_dest_dy = 0;
    adder->slow_dest_dx = 0;
    adder->slow_dest_dy = h;
    adder->command = (BTPZ);
    adder->request_enable = RX_READY;
    while (h--)
    {
	pcolor = ppixel;
	for (width = w; width > 0; width--)
	{
	    POLL_STATUS( adder, RX_READY);
	    *pcolor = (u_char)adder->id_data;
	    pcolor = pcolor + PIXELSIZE;
	}
	ppixel += realw * PIXELSIZE;
    }
    poll_status( adder, ADDRESS_COMPLETE); 
}

#ifdef BTP_PACKETS
/*
 * this code is incomplete -- it is supposed to
 * transfer data rapidly from dragon to vax.
 */
#if 0
static
btpspa ( x, y, w, h, realw, ppixel)
    int x, y, w, h;
    int realw;
    unsigned char *ppixel;
{
    Need_dma( 1);
    *p++ = JMPT_INITZBLOCKPTOB;	/* because this polls correctly. */
    Confirm_dma();
    btppackets( x, y, w, h, realw, (w * h + 1) / 2, ppixel);
    Need_dma( 1 );
    *p++ = TEMPLATE_DONE;
    Confirm_dma();
}
#endif
static
btppackets( x, y, w, h, realw, nwords, ppixel )
    int unsigned x, y, w, h, realw;
    register int nwords;
    unsigned char *ppixel;
{
    register unsigned short *p;
    register int cwords;	/* total words in this packet */
    register int scaninc = 0;	/* current offset from start of this scan */
    register int bytesleft;	/* bytes left in this packet (<= cwords*2) */
    register int scanbytes;	/* number of bytes to copy off this scan */
    unsigned char lastbyte;
    register unsigned char *dma_byte;
    extern struct DMAreq *DMArequest;
    extern short sg_fifo_func;

    Need_dma( 6 );
    *p++ = JMPT_INITRAWBTOP;	/* this does not 'loadd' the command reg. */
    *p++ = x;
    *p++ = y;
    *p++ = w;
    *p++ = h;
    *p++ = TEMPLATE_DONE;
    Confirm_dma();
    dmafxns.flush( TRUE );	/* just to ream out the template packets */
    write_ID( Adder, GREEN_UPDATE, Allplanes);
    Adder->command = BTPZ;	/* must be here or a_comp never flushed */
    while (nwords > 0)
    {
	/* We must now assemble the packets.  Internally, the packets must
	 * be accurately copied to the precision of one byte.  The final byte
	 * must be word-padded (the fifo dma engine only recognizes lengths of
	 * shorts -- sorry).  The restrictions are:  packets cannot exceeed
	 * req_buf_size in length, scanlines may be discontiguous, and of
	 * course we eventually hit a tricky final packet length.  The basic
	 * model is to ask for req_buf_size words, fill it up scanline at a
	 * time or in one big chunk, and ask for more when we run out, making
	 * sure to ask for a minimal amount on the last packet.  */
	bytesleft = 2 * (cwords = min( nwords, req_buf_size ));
	General_dma( cwords, FIFO_EMPTY | BYTE_PACK );
	dma_byte = (unsigned char *) dma_word;
	if (!Vaxstar)
	    DMArequest->DMAtype = BTOP;
	else {
	    sg_fifo_type = BTOP;
	    sg_fifo_func = BTP_PACK_ENB;
	}
	POLL_STATUS(Adder, ADDRESS_COMPLETE);
	/*	dmafxns.flush( TRUE ); XXX almost... */
	if (w == realw) {
	    bcopy( dma_byte, ppixel, bytesleft ); /* movec the data */
	    ppixel += bytesleft;
	} else
	    while (bytesleft > 0) {
		scanbytes = min( w - scaninc, bytesleft );
		if (((int) dma_byte) & 1) {
		    bcopy( dma_byte + 1, ppixel + 1, scanbytes - 1 );
		    *((unsigned short *) (ppixel - 1)) =
			lastbyte + (*((unsigned short *) dma_byte) << 8);
		} else {
		    if (scanbytes & ((int) 1)) {
			bcopy( dma_byte, ppixel, scanbytes - 1 );
			lastbyte = (unsigned char)
			 (*((unsigned short *) (dma_byte + (scanbytes - 1))));
		    } else
			bcopy( dma_byte, ppixel, scanbytes );
		}
		scaninc = (scaninc + scanbytes) % w;
		bytesleft -= scanbytes;
		dma_byte += scanbytes;
		if (scaninc)
		    ppixel += scanbytes;
		else
		    ppixel += scanbytes + realw - w;
	    }
	if (scanbytes & ((int) 1) && ((int) dma_byte) & 1)
	    *((unsigned short *) (ppixel - 1)) = lastbyte;
	dma_word += cwords;
	nwords -= cwords;
    }
    dmafxns.flush( TRUE );	/* just to ream out the template packets */
}
static int NewBtop = 1;
#endif /*BTP_PACKETS*/

#ifdef undef			/* random bits of old code */
    register int		width;
    
    while (h--)
    {
	for (width = w; width > 0; width--)
	{
	    POLL_STATUS( adder, TX_READY);
	    adder->id_data = *ppixel;
	    ppixel += step;
	}
    }
    while (dga->bytcnt_hi & 0xff || dga->bytcnt_lo)
	;
    /* WAITBYTCNTZERO(dga); */
    WAITDMADONE(dga);       /* mode bit 1 is NOT TO BE TOUCHED  until
			       fifo is empty */
} else {			/* either vaxStar or non-contiguous gpx */
    register struct fcc *sgfcc = fcc_cbcsr;
    register short nwords = (w * h + 1) / 2;
    register short cwords;
    extern short *sg_int_flag;
    extern short *change_section;
    while (nwords > 0)
      {
	  cwords = min( nwords, req_buf_size );
	  bcopy( ppixel, SG_Fifo, cwords * 2 ); /* movec, preferably */
	  *(unsigned long *) &sgfcc->cbcsr = (unsigned long) 0L; /* halt */
	  *(unsigned long *) &sgfcc->put = (unsigned long) cwords;
	  /* launch this puppy.. */
	  *(unsigned long *) &sgfcc->cbcsr =
	      (unsigned long)((ENTHRSH<<16)|PTB_UNPACK_ENB); /* & block */
	  /* should wait nicely here -- XXX... */
	  nwords -= req_buf_size;
	  ppixel += cwords * 2;
	  while (!(sgfcc->icsr & ITHRESH)) ; /* wait for thresh int */
	  while (*sg_int_flag != -1) ;
	  while ((sgfcc->fwused) || (*change_section == 1)) ;
      }
}
#endif	/* undef */

#define MAX_XFR_SIZE (1<<15)

static int SlowBtpKludge = 0;

CopyPixmapFromOffscreen(pPix, cur_bits)
     QDPixPtr pPix;
     unsigned char *cur_bits;
{
    short int	x = QDPIX_X(pPix),
		y = QDPIX_Y(pPix),
		w = QDPIX_WIDTH((PixmapPtr)pPix),
	        h = QDPIX_HEIGHT((PixmapPtr)pPix);
    int		bytesPerW = pPix->pixmap.devKind;
    short int	wperxfr = (pPix->pixmap.drawable.depth == 1) ? (bytesPerW*8)
	: bytesPerW;
    unsigned short int	hperxfr = MAX_XFR_SIZE/bytesPerW;
    short int	plane_mask = pPix->planes;
    int		size;
    register unsigned short *pData;
    register VOLATILE struct adder *adder = Adder;
    register unsigned short *p;

#ifdef BTP_PACKETS
    if (NewBtop && pPix->pixmap.drawable.depth > 1) {
	int i;
	
	for (i = 0; i < h; i++)
	    btppackets(x, y+i, bytesPerW, 1, bytesPerW, (bytesPerW+1)/2,
		       cur_bits + i * bytesPerW);
	return;
    }
#endif

    size = bytesPerW * hperxfr;

if (SlowBtpKludge) {
    /* GROSS, DESPERATION-TIME HACK!
     * BTPXY doesn't seem to work (hangs, waiting for driver).
     * Use BTBZ instead...
     */
    if (pPix->pixmap.drawable.depth == 1) {
	/* Move this code into a getspan routine! */
	char *buf = (char*)ALLOCATE_LOCAL(w);
	int i;
	bzero(cur_bits, bytesPerW * h);
	for (i = 0; i < h; i++) {
	    register int j;
	    tlgetspan((WindowPtr)0, x, y + i, w, buf);
	    for (j = w; --j >= 0; )
		if (buf[j] & plane_mask) {
		    cur_bits[i * bytesPerW + (j>>3)] |= 1 << (j & 7);
		}
	}
	DEALLOCATE_LOCAL(buf);
	return;
    }
}

    if (pPix->pixmap.drawable.depth == 1) {
	Need_dma(11);
	*p++ = JMPT_SETSRC1OCRB;
	*p++ = EXT_SOURCE|INT_NONE|ID|WAIT;
	*p++ = JMPT_SETRASTERMODE;
	*p++ = NORMAL;
	*p++ = JMPT_SETPLANEMASK;
	*p++ = plane_mask;
    }
    else {
	Need_dma(5);
    }
    *p++ = JMPT_INITGET;
    *p++ = x;		/* source_1_x */
    *p++ = wperxfr;	/* source_1_dx */
    *p++ = wperxfr;	/* fast_dest_dx */
    *p++ = JMPT_RESETCLIP;
    Confirm_dma();

    /*
     *  Wait for the DMA buffer to empty, and then for the fifo
     *  maintained by the gate array to empty.
     */
    dmafxns.flush (TRUE);
    adder->request_enable = 0;

    do
	{
	    if (h < hperxfr)
		{
		hperxfr = h;
		size = bytesPerW * hperxfr;
		}
                                  
	    poll_status(adder, ADDRESS_COMPLETE); 
	    adder->source_1_y = y;
	    adder->slow_dest_dy = hperxfr;
	    adder->source_1_dy = hperxfr;

	    if (pPix->pixmap.drawable.depth == 1) { /* handle XY mode */
		register int n = size>>1;     /* number of words to transfer */
		register unsigned short *pdata = (unsigned short *) cur_bits;
		register unsigned int dummy;
		adder->command = (PBT | OCRB | S1E | 0);

		while (n--) {
		    POLL_STATUS(adder, RX_READY);
		    dummy = (unsigned short)adder->id_data;
		    *pdata++ = dummy;
		}
	    } else { /* handle Z mode */
		register int n = size;	/* number of bytes to transfer */
		register unsigned char *pdata = (unsigned char *) cur_bits;
		adder->command = (BTPZ);

		while (n--) {
		    POLL_STATUS(adder, RX_READY);
		    *pdata++ = (unsigned char)(adder->id_data & Allplanes);
		}
	    }

/*+							    */
/*  at this point we setup for the next transfer.  This	    */
/*  includes updating the y value for the pixamp the	    */
/*  remaining height, and the buffer pointer		    */
/*-							    */

	    h -= hperxfr;
	    y += hperxfr;
	    cur_bits += size;
	}
    while (h);

    dmafxns.enable();
    if (pPix->pixmap.drawable.depth == 1) {
	Need_dma(3);
	*p++ = JMPT_SETPLANEMASK;
	*p++ = Allplanes;
	*p++ = JMPT_RESETRASTERMODE;
	Confirm_dma();
    }
}
