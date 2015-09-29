/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "miscstruct.h"

extern Bool cfbScreenInit();
extern void cfbQueryBestSize();
extern Bool cfbCreateWindow();
extern Bool cfbPositionWindow();
extern Bool cfbChangeWindowAttributes();
extern Bool cfbMapWindow();
extern Bool cfbUnmapWindow();
extern Bool cfbDestroyWindow();

extern Bool cfbRealizeFont();
extern Bool cfbUnrealizeFont();
extern Bool cfbRealizeCursor();
extern Bool cfbUnrealizeCursor();
extern Bool cfbScreenSaver();
extern Bool cfbCreateGC();

extern PixmapPtr cfbCreatePixmap();
extern Bool cfbDestroyPixmap();

extern void cfbCopyWindow();
extern void cfbPaintWindow();

extern void miPolyFillRect();
extern void cfbPolyFillRect();
extern void miPolyFillArc();
extern void cfbZeroPolyArcSS8Copy();
extern void cfbLineSS(), cfbLineSD(), cfbSegmentSS(), cfbSegmentSD();
extern RegionPtr cfbCopyPlane();
extern void cfbPolyFillArcSolidCopy();
extern RegionPtr cfbCopyArea();

extern void cfbPushPixels8();
extern void cfbSetSpans();
extern void cfbGetSpans();
extern void cfbSolidFS();
extern void cfbUnnaturalTileFS();
extern void cfbUnnaturalStippleFS();
extern void cfbFillBoxTileOdd();
extern void cfbFillBoxTile32();
extern void cfbFillBoxSolid();

extern void cfbTEGlyphBlt();
extern void cfbTEGlyphBlt8();
extern void cfbPolyGlyphBlt8();

extern void cfbSaveAreas();
extern void cfbRestoreAreas();

/* included from mfb.h; we can't include mfb.h directly because of other 
 * conflicts */
extern void mfbPushPixels();
extern void mfbSetSpans();
extern void mfbGetSpans();
extern void mfbUnnaturalTileFS();
extern void mfbUnnaturalStippleFS();
extern Bool mfbRealizeFont();
extern Bool mfbUnrealizeFont();
extern void mfbQueryBestSize();
extern RegionPtr mfbPixmapToRegion();
extern void mfbCopyRotatePixmap();

extern void miNotMiter();
extern void miMiter();
extern PixmapPtr cfbCopyPixmap();
extern void  cfbConvertRects();
extern void  miPolyArc();
extern void  miFillPolyArc();

extern void cfbPutImage();
extern void cfbGetImage();
extern RegionPtr miCopyArea();
extern RegionPtr miCopyPlane();
extern void cfbPolyPoint();
extern void miPushPixels();

#ifdef	STATIC_COLOR
extern void cfbInstallColormap();
extern void cfbUninstallColormap();
extern int cfbListInstalledColormaps();
#endif
extern void cfbResolveColor();
extern Bool cfbInitializeColormap();
extern Bool cfbCreateDefColormap();

extern void cfbCopyRotatePixmap();
extern void cfbYRotatePixmap();
extern void cfbXRotatePixmap();
extern void cfbPadPixmap();

/*
   private filed of pixmap
   pixmap.devPrivate = (unsigned int *)pointer_to_bits
   pixmap.devKind = width_of_pixmap_in_bytes
*/

extern int  cfbGCPrivateIndex;
extern int  cfbWindowPrivateIndex;

/* private field of GC */
typedef struct {
    unsigned char       rop;            /* reduction of rasterop to 1 of 3 */
    unsigned char       ropOpStip;      /* rop for opaque stipple */
    unsigned char       ropFillArea;    /*  == alu, rop, or ropOpStip */
    unsigned		fExpose:1;	/* callexposure handling ? */
    unsigned		freeCompClip:1;
    PixmapPtr		pRotatedPixmap;
    RegionPtr		pCompositeClip; /* FREE_CC or REPLACE_CC */
    } cfbPrivGC;

typedef cfbPrivGC	*cfbPrivGCPtr;

/* private field of window */
typedef struct {
    unsigned	char fastBorder; /* non-zero if border is 32 bits wide */
    unsigned	char fastBackground;
    unsigned short unused; /* pad for alignment with Sun compiler */
    DDXPointRec	oldRotate;
    PixmapPtr	pRotatedBackground;
    PixmapPtr	pRotatedBorder;
    } cfbPrivWin;

/* precomputed information about each glyph for GlyphBlt code.
   this saves recalculating the per glyph information for each
box.
*/
typedef struct _pos{
    int xpos;		/* xposition of glyph's origin */
    int xchar;		/* x position mod 32 */
    int leftEdge;
    int rightEdge;
    int topEdge;
    int bottomEdge;
    int *pdstBase;	/* longword with character origin */
    int widthGlyph;	/* width in bytes of this glyph */
} TEXTPOS;

/* reduced raster ops for cfb */
#define RROP_BLACK	GXclear
#define RROP_WHITE	GXset
#define RROP_NOP	GXnoop
#define RROP_INVERT	GXinvert

/* out of clip region codes */
#define OUT_LEFT 0x08
#define OUT_RIGHT 0x04
#define OUT_ABOVE 0x02
#define OUT_BELOW 0x01

/* major axis for bresenham's line */
#define X_AXIS	0
#define Y_AXIS	1

/* optimization codes for FONT's devPrivate field */
#define FT_VARPITCH	0
#define FT_SMALLPITCH	1
#define FT_FIXPITCH	2

/* macros for cfbbitblt.c, cfbfillsp.c
   these let the code do one switch on the rop per call, rather
than a switch on the rop per item (span or rectangle.)
*/

#define fnCLEAR(src, dst)	(0)
#define fnAND(src, dst) 	(src & dst)
#define fnANDREVERSE(src, dst)	(src & ~dst)
#define fnCOPY(src, dst)	(src)
#define fnANDINVERTED(src, dst)	(~src & dst)
#define fnNOOP(src, dst)	(dst)
#define fnXOR(src, dst)		(src ^ dst)
#define fnOR(src, dst)		(src | dst)
#define fnNOR(src, dst)		(~(src | dst))
#define fnEQUIV(src, dst)	(~src ^ dst)
#define fnINVERT(src, dst)	(~dst)
#define fnORREVERSE(src, dst)	(src | ~dst)
#define fnCOPYINVERTED(src, dst)(~src)
#define fnORINVERTED(src, dst)	(~src | dst)
#define fnNAND(src, dst)	(~(src & dst))
#define fnSET(src, dst)		(~0)

/* Binary search to figure out what to do for the raster op.  It may
 * do 5 comparisons, but at least it does no function calls 
 * Special cases copy because it's so frequent 
 * XXX - can't use this in many cases because it has no plane mask.
 */
#define DoRop(alu, src, dst) \
( ((alu) == GXcopy) ? (src) : \
    (((alu) >= GXnor) ? \
     (((alu) >= GXcopyInverted) ? \
       (((alu) >= GXnand) ? \
         (((alu) == GXnand) ? ~((src) & (dst)) : ~0) : \
         (((alu) == GXcopyInverted) ? ~(src) : (~(src) | (dst)))) : \
       (((alu) >= GXinvert) ? \
	 (((alu) == GXinvert) ? ~(dst) : ((src) | ~(dst))) : \
	 (((alu) == GXnor) ? ~((src) | (dst)) : (~(src) ^ (dst)))) ) : \
     (((alu) >= GXandInverted) ? \
       (((alu) >= GXxor) ? \
	 (((alu) == GXxor) ? ((src) ^ (dst)) : ((src) | (dst))) : \
	 (((alu) == GXnoop) ? (dst) : (~(src) & (dst)))) : \
       (((alu) >= GXandReverse) ? \
	 (((alu) == GXandReverse) ? ((src) & ~(dst)) : (src)) : \
	 (((alu) == GXand) ? ((src) & (dst)) : 0)))  ) )
