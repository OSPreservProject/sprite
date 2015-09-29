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
 * This scheme seems way too complex; I think it should be replaced with
 * the one used for mfb.		dwm
 */
#ifdef QDVALIDATE
;-=+_*()))(({}{}}}}{{{{@#$@#$^%^*$%^&*!#$$^%&#($&%#(!@!!~```\';:",,":die!
#else
#define	QDVALIDATE
#endif

extern void tlSolidSpans(), tlTiledSpans(), tlStipSpans(), tlOpStipSpans();
extern void tlSolidRects(), tlTiledRects(), tlStipRects(), tlOpStipRects();
extern void tlImageText();
extern int tlPolyText(), tlPolyTextSolid();
extern void qdFSPixSolid(), qdFSPixTiled(), qdFSPixStippleorOpaqueStip(),
		qdFSPixStippleorOpaqueStip();
extern void miPolyFillRect();

/* qdvalidate.h
 *	validation lookup tables
 */

#define	VECFillSpans		0
#define	VECSetSpans		1
#define	VECPutImage		2
#define	VECCopyArea		3
#define	VECCopyPlane		4
#define	VECPolyPoint		5
#define	VECPolylines		6
#define	VECPolySegment		7
#define	VECPolyRectangle	8
#define	VECPolyArc		9
#define	VECFillPolygon		10
#define	VECPolyFillRect		11
#define	VECPolyFillArc		12
#define	VECPolyText8		13
#define	VECPolyText16		14
#define	VECImageText8		15
#define	VECImageText16		16
#define	VECImageGlyphBlt	17
#define	VECPolyGlyphBlt		18
#define	VECPushPixels		19
#define	VECLineHelper		20
/*	THESE MUST BE SET UP AT CREATE TIME
 * #define	VECChangeClip		21
 * #define	VECDestroyClip		22
 */
#define	VECMAX			21
#define	VECNULL			21

#define	VECMOST	10	/* most vectors indexes in one entry */

/* indexed by GC field-changed bits
 *	returns the index of the vector that may be changed.
 */
typedef struct _tlchangevecs
{
	char 	pivec[VECMAX+1];
}	tlchangevecs;
tlchangevecs tlChangeVecs[] =
{
/* GCFunction */		{VECNULL},
/* GCPlaneMask */		{VECNULL},
/* GCForeground */		{VECNULL},
/* GCBackground */		{VECNULL},
/* GCLineWidth */		{VECPolylines, VECPolySegment, VECNULL},
/* GCLineStyle */		{VECPolylines, VECPolySegment, VECNULL},
/* GCCapStyle */		{VECNULL},
/* GCJoinStyle */		{VECLineHelper, VECNULL},
/* GCFillStyle */		{VECPolyText8, VECFillSpans,
				 VECPolyFillRect, VECPolylines, VECPolySegment,
				 VECPushPixels, VECNULL},
/* GCFillRule */		{VECNULL},
/* GCTile */			{VECFillSpans, VECPolyFillRect, VECPolylines,
				 VECPolySegment, VECPolyText8, VECPushPixels,
				 VECNULL},
/* GCStipple */			{VECFillSpans, VECPolyFillRect, VECPolylines,
				 VECPolySegment, VECPolyText8, VECPushPixels,
				 VECNULL},
/* GCTileStipXOrigin */		{VECNULL},
/* GCTileStipYOrigin */		{VECNULL},
/* GCFont */			{VECImageText8, VECPolyText8, VECNULL},
/* GCSubwindowMode */		{VECNULL},
/* GCGraphicsExposures */	{VECNULL},
/* GCClipXOrigin */		{VECNULL},
/* GCClipYOrigin */		{VECNULL},
/* GCClipMask */		{VECNULL},
/* GCDashOffset */		{VECNULL},
/* GCDashList */		{VECPolylines, VECPolySegment, VECNULL},
/* GCArcMode */			{VECNULL},
	/* THESE ARE devPrivate->GCstate CHANGE BITS: */
/* VSFullReset */
{VECFillSpans, VECSetSpans, VECPutImage, VECCopyArea, VECCopyPlane,
 VECPolyPoint, VECPolylines, VECPolySegment, VECPolyRectangle, VECPolyArc,
 VECFillPolygon, VECPolyFillRect, VECPolyFillArc, VECPolyText8, VECPolyText16,
 VECImageText8, VECImageText16, VECImageGlyphBlt, VECPolyGlyphBlt,
 VECPushPixels, VECLineHelper, VECNULL},
/* VSDest  */
{VECPutImage, VECCopyArea, VECCopyPlane, VECFillPolygon, VECPolyFillRect,
 VECImageText8, VECPolyText8, VECPushPixels, VECPolylines, VECPolySegment,
 VECFillSpans, VECSetSpans, VECPutImage, VECNULL},
};

/* indexed by vector indexes (VEC*)
 *	returns a weird command language for validate
 */
typedef	void 	(* PFN)();

