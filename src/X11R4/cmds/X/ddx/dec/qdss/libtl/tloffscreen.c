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
 * qXss off-screen memory manager.  Treats off-screen memory as
 * a cache of pixmaps.  Imposes ruthless cache invalidation policies.
 *
 * syntax:
 * planemask =	tlConfirmPixmap( pPixmap);
 * 		tlCancelPixmap( pPixmap);
 *
 * tlConfirmPixmap verifies that a pixmap has been loaded.
 *
 * If this storage is deallocated be sure to call tlCancelPixmap.
 * (the name "FreePixmap" is taken by the ddx layer)
 */

extern int PixmapUseOffscreen;

#include <sys/types.h>
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>
#include "qd.h"
#include "tl.h"
#include "tltemplabels.h"
#include "gc.h"

extern	unsigned	int	Allplanes;

/*
 * GPX video memory is normally partioned into vertical sections as follows:
 *
 * 0-863: Visible
 * 864-927: Two rows of 32 "small slots". Each slot has room for a Pixmap
 *          upto 32*32 pixels (or you can Nplanes Bitmaps in the slot).
 * 928: One row for patterns of dashed lines.
 * 929-2047: Space for "large Pixmaps" and Bitmaps, including fonts.
 *
 * When debugging, it is sometimes useful to look at the non-visible
 * (offscreen) areas, to see what is going on. This is done by tricking
 * the server into thinking the visible areas is smaller than 864 lines,
 * and shifting the logically non-visible areas up into the visible screen.
 * If you want the logical screen height to be H (say 600), you must use
 * a server compiled with DEBUG defined, and at runtime set:
 * ScreenHeight=H, SlotHead.pos.y=H+65. You can do this in a debugger
 * by setting a breakpoint at main.
 */

#define SmallROWS 2
#define SmallPerRow 32
#define SmallHEIGHT 32 /* max height of "small" Pixmaps */
#define SmallWIDTH 32
#define SmallMAX (SmallPerRow*SmallROWS)

#define QZLOWY  2032
#define QZHIGHY (864+SmallHEIGHT*SmallROWS+1)
#define QZMAXALLOCY     (QZLOWY - QZHIGHY)
#define SmallPixY ScreenHeight

#define MAXIM		0x100
#define INDEXBITS	0xff	/* MAXIM-1 */
static int BADBITS =	8;

/* Note that SmallUsed[0] is reserved for the installed tile/stipple */
#if NPLANES==24
long SmallUsed[SmallMAX] = {0};
#else
unsigned char SmallUsed[SmallMAX] = {0};
#endif
int SmallUsedCountN = 0;

#if SmallWIDTH != 32 || SmallPerRow != 32
ERROR: Fix here (and elsewhere in this file)
#else
#define PixSmallIndex(p) (QDPIX_Y(p) - SmallPixY + (QDPIX_X(p)>>5))
#endif

typedef struct PixSlot {
    struct PixSlot *up, *down;
    DDXPointRec pos;
    short width, height;
    int planes;
} PixSlotRec, *PixSlotPtr;

extern int Nplanes;
extern int Nentries;
static VOID QDptbxy(), doptbxy();

/*
 * full-depth entries grow down from the top
 * single-plane entries grow up from the bottom
 */
static int	Lowclouds = QZHIGHY;	/* nothing up there yet */
static int	Highwater[] =		/* nothing down here yet */
	{QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,
	 QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,
	 QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY,QZLOWY};

#define QDHASH(lw)	( ((((lw)) >> BADBITS) ^ (lw)) & INDEXBITS)

/* Manage lists (to remember Least Recently Used) */
#define UNLINK(pPix) \
    (pPix)->prevLRU->nextLRU = (pPix)->nextLRU, \
    (pPix)->nextLRU->prevLRU = (pPix)->prevLRU
#define INSERT_AT_FRONT(pPix, listHead) \
    (pPix)->nextLRU = (listHead)->nextLRU, (pPix)->prevLRU = listHead, \
    (listHead)->nextLRU->prevLRU = (pPix), (listHead)->nextLRU = pPix

#ifdef X11R4
static QDPixRec SmallHeadN =
    {{{0}}, 0, &SmallHeadN, &SmallHeadN};
static QDPixRec SmallHead1 =
    {{{0}}, 0, &SmallHead1, &SmallHead1};
static QDPixRec BigHead =
    {{{0}}, 0, &BigHead, &BigHead};
#else
static QDPixRec SmallHeadN =
    {{{0}}, {0,0}, 0, &SmallHeadN, &SmallHeadN};
static QDPixRec SmallHead1 =
    {{{0}}, {0,0}, 0, &SmallHead1, &SmallHead1};
static QDPixRec BigHead =
    {{{0}}, {0,0}, 0, &BigHead, &BigHead};
#endif
static PixSlotRec SlotHead = { &SlotHead, &SlotHead, {0, QZHIGHY},
    1024, QZMAXALLOCY, 0};
static PixSlotPtr SearchStart = &SlotHead;

int PixHSplit = 1;
/*
 * Confirm that an off-screen pixel map is loaded.
 *
 * returns maskplane, or 0 if Pixmap could not be loaded
 */
int
tlConfirmPixmap( pPix)
    register QDPixPtr	pPix;
{
    register PixSlotPtr ptr;
    register int i;
    register int plane;

    if (PixmapIsLocked(pPix)) return pPix->planes;

    if (QDPIX_WIDTH(&pPix->pixmap) <= SmallWIDTH
     && QDPIX_HEIGHT(&pPix->pixmap) <= SmallHEIGHT) {
	if (QDPIX_Y(pPix) != NOTOFFSCREEN) {
	    UNLINK(pPix);
	    if (pPix->pixmap.drawable.depth > 1)
		INSERT_AT_FRONT(pPix, &SmallHeadN);
	    else
		INSERT_AT_FRONT(pPix, &SmallHead1);
	    return pPix->planes;
	}

	if (pPix->pixmap.drawable.depth > 1) {
	    for (i = SmallMAX; ; )
		if (--i <= 0) {
		    /* throw out least recently used pixmap */
		    if (SmallUsedCountN < 8) {
#define PixAt(pix, pt) (QDPIX_X(pix) == (pt).x && QDPIX_Y(pix) == (pt).y)
			/* remove bitmaps */
			QDPixPtr ptr = SmallHead1.nextLRU;
			DDXPointRec recent1, recent2, victim;
			recent1.x = QDPIX_X(ptr);
			recent1.y = QDPIX_Y(ptr);
			recent2.x = QDPIX_X(ptr->nextLRU);
			recent2.y = QDPIX_Y(ptr->nextLRU);
			ptr = SmallHead1.prevLRU;
			while (PixAt(ptr, recent1)
			       || PixAt(ptr, recent2))
			    ptr = ptr->prevLRU;
			victim.x = QDPIX_X(ptr);
			victim.y = QDPIX_Y(ptr);
			i = PixSmallIndex(ptr);
			for (ptr = SmallHead1.nextLRU; ptr != &SmallHead1; ) {
			    QDPixPtr next = ptr->nextLRU;
			    if (PixAt(ptr, victim))
				tlCancelPixmap(ptr);
			    ptr = next;
			}
		    } else {
			i = PixSmallIndex(SmallHeadN.prevLRU);
			tlCancelPixmap(SmallHeadN.prevLRU);
		    }
		    break;
		} 
		else if (SmallUsed[i] == 0) break;
	    SmallUsed[i] = Allplanes;
	    QDPIX_X(pPix) = (i & 31) << 5;
	    QDPIX_Y(pPix) = SmallPixY + (i & ~31);
	    INSERT_AT_FRONT(pPix, &SmallHeadN);
	    if (QD_PIX_DATA(&pPix->pixmap))
		tlspadump(pPix);
	    SmallUsedCountN++;
	    pPix->planes = Allplanes;
	} else {
	    for (i = SmallMAX; ; )
		if (--i <= 0) {
		    if (SmallUsedCountN >= SmallMAX-4) {
			/* throw out least recently used pixmap */
			i = PixSmallIndex(SmallHeadN.prevLRU);
			tlCancelPixmap(SmallHeadN.prevLRU);
		    } else {
			/* throw out least recently used bitmap */
			i = PixSmallIndex(SmallHead1.prevLRU);
			tlCancelPixmap(SmallHead1.prevLRU);
		    }
		    break;
		} 
		else if (SmallUsed[i] != Allplanes) break;

	    for (plane = 1; SmallUsed[i] & plane; plane <<= 1) ;
	    pPix->planes = plane;
	    SmallUsed[i] |= pPix->planes;
	    QDPIX_X(pPix) = (i & 31) << 5;
	    QDPIX_Y(pPix) = SmallPixY + (i & ~31);
	    INSERT_AT_FRONT(pPix, &SmallHead1);
	    if (QD_PIX_DATA(&pPix->pixmap))
		QDptbxy(QDPIX_X(pPix), QDPIX_Y(pPix), pPix->planes,
			QDPIX_WIDTH(&pPix->pixmap),QDPIX_HEIGHT(&pPix->pixmap),
			(long *)QD_PIX_DATA(&pPix->pixmap));
	}
	return pPix->planes;
    }

    if (QDPIX_Y(pPix) != NOTOFFSCREEN) {
	UNLINK(pPix);
	INSERT_AT_FRONT(pPix, &BigHead);
	return pPix->planes;
    }
    if (QDPIX_WIDTH(&pPix->pixmap) > 1024
     || QDPIX_HEIGHT(&pPix->pixmap) > QZMAXALLOCY)
	return 0;
  retry:
    /* plane is used as a magic "maximum mask" below */
    if (pPix->pixmap.drawable.depth > 1) plane = 1;
    else plane = Allplanes;
    for (ptr = SearchStart; ; ptr = ptr->down) {
	if (ptr->height < QDPIX_HEIGHT(&pPix->pixmap) ||
	    ptr->width < QDPIX_WIDTH(&pPix->pixmap) || ptr->planes >= plane)
	    if (ptr->down == SearchStart) break;
	    else continue;
	QDPIX_X(pPix) = ptr->pos.x;
	QDPIX_Y(pPix) = ptr->pos.y;

	if (ptr->planes == 0
	 && ptr->height > QDPIX_HEIGHT(&pPix->pixmap) + 32) {
	    PixSlotPtr tmp = (PixSlotPtr)Xalloc(sizeof(PixSlotRec));
	    /* Split ptr into two parts. Use upper part. */
	    tmp->width = ptr->width;
	    tmp->pos.x = ptr->pos.x;
	    tmp->pos.y = ptr->pos.y + QDPIX_HEIGHT(&pPix->pixmap);
	    tmp->height = ptr->height - QDPIX_HEIGHT(&pPix->pixmap);
	    ptr->height = QDPIX_HEIGHT(&pPix->pixmap);
	    tmp->planes = 0;
	    
	    /* link tmp in below ptr */
	    tmp->down = ptr->down;
	    tmp->up = ptr;
	    ptr->down = tmp;
	    tmp->down->up = tmp;
	}
if (PixHSplit)
	if (ptr->planes == 0 && ptr->width > QDPIX_WIDTH(&pPix->pixmap) + 64) {
	    PixSlotPtr tmp = (PixSlotPtr)Xalloc(sizeof(PixSlotRec));
	    /* Split ptr into two parts. Use left part. */
	    tmp->height = ptr->height;
	    tmp->pos.y = ptr->pos.y;
	    i = (QDPIX_WIDTH(&pPix->pixmap) + 15) & ~15;
	    tmp->pos.x = ptr->pos.x + i;
	    tmp->width = ptr->width - i;
	    ptr->width = i;
	    tmp->planes = 0;
	    
	    /* link tmp in below ptr */
	    tmp->down = ptr->down;
	    tmp->up = ptr;
	    ptr->down = tmp;
	    tmp->down->up = tmp;
	}

	pPix->slot = ptr;
	if (pPix->pixmap.drawable.depth == 1) {
	    for (plane = 1; ptr->planes & plane; plane <<= 1) ;
	    if (QD_PIX_DATA(&pPix->pixmap))
		QDptbxy(QDPIX_X(pPix), QDPIX_Y(pPix), plane,
			QDPIX_WIDTH(&pPix->pixmap),QDPIX_HEIGHT(&pPix->pixmap),
			(long *)QD_PIX_DATA(&pPix->pixmap));
	    SearchStart = ptr;
	} else {
	    plane = Allplanes;
	    if (QD_PIX_DATA(&pPix->pixmap))
		tlspadump(pPix);
	    SearchStart = ptr->down;
	}
	pPix->planes = plane;
	ptr->planes |= plane;
	INSERT_AT_FRONT(pPix, &BigHead);
	return pPix->planes;
    }

    /* Full. Throw something out, as long as the queue isn't empty. */
    i = 0;
    for ( ; (i < 64) && (BigHead.prevLRU != &BigHead); ) {
	i += QDPIX_HEIGHT(&BigHead.prevLRU->pixmap);
	tlCancelPixmap(BigHead.prevLRU);
    }
#ifdef DEBUG
    ErrorF("threw out some offscreen pixmaps\n");
    if (i < 64)
	ErrorF("queue got empty\n");
#endif
    goto retry;
}

int
tlCancelPixmap( pix)
    QDPixPtr  pix;
{
    if (QDPIX_Y(pix) == NOTOFFSCREEN || PixmapIsLocked(pix))
	return;
    if (QDPIX_Y(pix) < SmallPixY + SmallROWS * SmallHEIGHT) {
	if (pix->pixmap.drawable.depth > 1)
	    SmallUsedCountN--;
	UNLINK(pix);
	SmallUsed[PixSmallIndex(pix)] -= pix->planes;
    } else {
	register PixSlotPtr slot = pix->slot;
	int changes = 1;
	slot->planes -= pix->planes;
	while (slot->planes == 0 && changes) {
	    register PixSlotPtr other;
	    changes = 0;
if (PixHSplit) {
	    /* Try to merge with region right of slot */
	    if ((other = slot->down)->planes == 0 && other != &SlotHead
	      && slot->pos.y == other->pos.y && slot->height == other->height){
		other->down->up = slot;
		slot->down = other->down;
		slot->width += other->width;
		Xfree(other);
		changes++;
	    }
	    /* Try to merge with region left of slot */
	    other = slot->up;
	    if (other->planes == 0 && slot != &SlotHead
	      && slot->pos.y == other->pos.y && slot->height == other->height){
		slot->down->up = other;
		other->down = slot->down;
		other->width += slot->width;
		Xfree(slot);
		slot = other;
		changes++;
	    }
}
	    /* Try to merge with region below chunk */
	    if ((other = slot->down)->planes == 0 && other != &SlotHead
	      && slot->pos.x == other->pos.x && slot->width == other->width) {
		other->down->up = slot;
		slot->down = other->down;
		slot->height += other->height;
		Xfree(other);
		changes++;
	    }
	    /* Try to merge with region above chunk */
	    other = slot->up;
	    if (other->planes == 0 && slot != &SlotHead
	      && slot->pos.x == other->pos.x && slot->width == other->width) {
		slot->down->up = other;
		other->down = slot->down;
		other->height += slot->height;
		Xfree(slot);
		slot = other;
		changes++;
	    }
	    SearchStart = slot;
	}
	pix->slot = NULL;
	UNLINK(pix);
    }
    if (QD_PIX_DATA(&pix->pixmap) == NULL) {
	int size = pix->pixmap.devKind * QDPIX_HEIGHT(&pix->pixmap);
	QD_PIX_DATA(&pix->pixmap) = (pointer)Xalloc(size);
#ifdef DEBUG
	if (DebugDma > 0)
	    fprintf(stderr, "[Copy pixmap 0x%x from offscreen]\n", pix);
#endif
	CopyPixmapFromOffscreen(pix, QD_PIX_DATA(&pix->pixmap));
    }
    pix->planes = 0;
    QDPIX_X(pix) = 0;
    QDPIX_Y(pix) = NOTOFFSCREEN;
    pix->pixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
}

void
tlSinglePixmap(pPix)
    QDPixPtr pPix;
{
    if (!(PixmapUseOffscreen & (pPix->pixmap.drawable.depth > 1 ? 4 : 2)))
	tlCancelPixmap( pPix );
    else if (QD_PIX_DATA(&pPix->pixmap) && QDPIX_Y(pPix) != NOTOFFSCREEN) {
	Xfree(QD_PIX_DATA(&pPix->pixmap));
	QD_PIX_DATA(&pPix->pixmap) = NULL;
    }
}

/*
 * End of interface routines
 */

/*
 * Start of internal routines.
 */



#define LONGSPADDED( x, w)	((((x) & 31) + (w)+31) >> 5)

/*
 * Copy a bitmap from main memory into one plane of the frame buffer.
 *
 * This routine requires the leftmost bit of the pixel map to be aligned
 * with bit (x % 32) in the first long word of each scanline,
 * and the right end of each scanline to be padded to a
 * long word boundary.
 *
 *  the dma buffer has
 *  JMPT_SETVIPER24
 *  rop | FULL_SRC_RESOLUTION
 *  JMPT_RESETCLIP
 *  JMPT_PTOBXY
 *  maskplane
 *  x, y, width, height
 *  PTB | count
 *  pixvals
 *  JMPT_PTOBXYCLEAN
 */
static VOID
QDptbxy( x0, y0, maskplane, width, height, plong)
    int                 x0, y0, maskplane;
    register int        width;
    register int        height;
    register long      *plong;
{
    int	nlongsscanline;
    int	nscansperblock;
    int	nscansdone;
    int blockstride;

    extern int     req_buf_size;

    SETTRANSLATEPOINT( 0, 0);
    /* 
     * pad width to short word bounds on both ends
     */
    nlongsscanline = LONGSPADDED( x0, width);
    /*
     * save 15 words for setup display list
     */
    nscansperblock = ( min( req_buf_size, MAXDMAPACKET/sizeof(short)) - 15 )
						/ (nlongsscanline<<1);
    blockstride = nscansperblock*nlongsscanline;
    for (nscansdone=0;
	 height-nscansdone > nscansperblock;
	    nscansdone+=nscansperblock,
	    y0+=nscansperblock,
	    plong+=blockstride)
	doptbxy( x0, y0, maskplane, width, nscansperblock, plong);
    doptbxy( x0, y0, maskplane, width, height-nscansdone, plong);
}

static VOID doptbxy( x0, y0, maskplane, width, height, plong)
     int	x0, y0, maskplane;
     int	width;
     int	height;
     register long *plong;
{
    register unsigned short *p;
    register long *pl;
    register int nlong = LONGSPADDED( x0, width) * height;

    Need_dma( 11 + (nlong<<1));
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = maskplane;
    *p++ = LF_SOURCE | FULL_SRC_RESOLUTION;
    *p++ = JMPT_RESETCLIP;
    *p++ = JMPT_PTOBXY;
    *p++ = Allplanes;
    *p++ = x0; /* assumed to be within screen */
    *p++ = y0;
    *p++ = (width + 31 ) & 0x3fe0;
    *p++ = height & 0x3fff;
    /*
     * DGA magic bit pattern for PTB, see VCB02 manual
     */
    *p++ = 0x6000 |(0x1fff & -(nlong<<1));
    pl = (long*)p;
    while (nlong--)
        *pl++ = *plong++;
    p = (u_short*)pl;

    Confirm_dma();
}
