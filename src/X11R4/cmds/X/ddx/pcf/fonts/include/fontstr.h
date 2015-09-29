/* $Header: fontstr.h,v 5.6 89/09/11 15:00:56 erik Exp $ */
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

#ifndef FONTSTR_H
#define FONTSTR_H

#include <X11/Xproto.h>
#include "font.h"

/*
 * This version of the server font data strucutre is only for describing
 * the in memory data structure. The file structure is not necessarily a
 * copy of this. That is up to the compiler and the OS layer font loading
 * machinery.
 */

#define GLYPHPADOPTIONS 4	/* 1, 2, 4, or 8 */

typedef enum {Linear8Bit, TwoD8Bit, Linear16Bit, TwoD16Bit} FontEncoding;

typedef struct _FontProp {
    ATOM name;
    INT32 value;	/* assumes ATOM is not larger than INT32 */
} FontPropRec;

typedef struct _ExtentInfo {
    DrawDirection	drawDirection;
    int			fontAscent;
    int			fontDescent;
    int			overallAscent;
    int			overallDescent;
    int			overallWidth;
    int			overallLeft;
    int			overallRight;
} ExtentInfoRec;

typedef struct _CharInfo {
    xCharInfo	metrics;	/* info preformatted for Queries */
    pointer	pPriv;		/* pointer to whatever other information */
				/* the application wants easily accessible */
} CharInfoRec;

/*
 * a single CharSet structure may be used by multiple EncodedFont records if 
 * there are multiple encodings using the same basic set of glyphs.
 */
typedef struct _CharSet {
        /* Information about Font File */
    Mask	tables;		/* which tables are currently loaded */
    int		fileType;	/* what format is the source file */
	/* Font accelerator information */
    Bool	noOverlap;	/* true if:
				 * max(rightSideBearing-characterWidth)
				 * <= minbounds->metrics.leftSideBearing
				 */
    Bool	constantMetrics;
    Bool	terminalFont;	/* Should be deprecated!  true if:
				 * constant metrics &&
				 * leftSideBearing == 0 &&
				 * rightSideBearing == characterWidth &&
				 * ascent == fontAscent &&
				 * descent == fontDescent
				 */
    Bool	constantWidth;/* true if minbounds->metrics.characterWidth
				 *      == maxbounds->metrics.characterWidth
				 */
    Bool	inkInside;	/* true if for all defined glyphs:
				 * leftSideBearing >= 0 &&
				 * leftSideBearing >= 0 &&
				 * rightSideBearing <= characterWidth &&
				 * -fontDescent <= ascent <= fontAscent &&
				 * -fontAscent <= descent <= fontDescent
				 */
    Bool	inkMetrics;	/* ink metrics != bitmap metrics */
					/* used with terminalFont */
    short	drawDirection;
    int		fontDescent;	/* minimum for quality typography */
    int		fontAscent;	/* minimum for quality typography */
    int		maxOverlap;	/* how much right side bearing is than width */
    xCharInfo	maxbounds;	/* for all characters in font -- used by DDX */
    xCharInfo	minbounds;

	/* property information */
    int		nProps;
    FontPropPtr	props;
    Bool	*isStringProp;

    int		nChars;

	/* metric information */
    Mask	metricFlags;	/* information about metrics */
    union {
	CharInfoPtr	 pCI;	/* all of the defined glyphs (1d),for DDX */
	CharInfoPtr	*ppCI;	/* all of the defined glyphs (2d), for DDX */
    } ci;
    union {
	CharInfoPtr	 pCI;	/* ink metrics, for passing to client (1d) */
	CharInfoPtr	*ppCI;	/* ink metrics, for passing to client (2d) */
    } inkci;

	/* scalable width information */
    int		*sWidth;	/* scalable width of char */

	/* glyph picture information */
    Mask	bitmapFlags;
    int		bitmapsSizes[GLYPHPADOPTIONS];
    int		pixDepth;	/* intensity bits per pixel */
    int		glyphSets;	/* number of sets of glyphs, for
				    sub-pixel positioning */
    char	**pBitOffsets;	/* pointer to bitmaps for each character */
    char	*pBitmaps;	/* pointer to buffer containing all bitmaps */

	/* glyph name information */
    Atom	*glyphNames;	/* references in order of metrics */

	/* private miscellania */
    int		refcnt;		/* # of encodings referencing these glyphs */
    pointer	osPrivate;
    pointer	*devPriv;	/* information private to screen */
/* 4/23/89 (ef) -- this has to get allocated somehow */
} CharSetRec;

/*
 * Font is created at font load time. It is specific to a single encoding.
 * e.g. not all of the glyphs in a font may be part of a single encoding.
 */
typedef struct _EncodedFont {
    char	*encoding;	/* name of this encoding */
    int		encIndex;	/* index of this encoding */
    CharSetPtr	pCS;		/* pointer to the shared data */
    int		firstCol;
    int		lastCol;
    int		firstRow;
    int		lastRow;
    Bool	linear;	/* true if firstRow == lastRow */
    Bool	allExist;	/* true if no holes in array */
    Mask	ciFlags;	/* info about CharInfo (1d vs. 2d) */
    CharInfoPtr *ppCI;		/* an array of pointers */
    CharInfoPtr *ppInkCI;	/*	may be copy of chars */
    xCharInfo	inkMin;		/* for this encoding's ink metrics */
    xCharInfo	inkMax;		/*	for reporting to clients */
    int		defaultCh;	/* for this encoding, may be NO_SUCH_CHAR */
    CharInfoPtr	pDefChar;	/* realized for this encoding */
    int		refcnt;
    pointer	osPrivate;
    pointer	*devPriv;	/* information private to screen */
} EncodedFontRec;

/*
 * and some access macros (moved here from font.h since they don't do any good
 * there anyway. The fields are not visible in that header file).
 */

/*
 * for linear char sets
 */
#define n1dChars(font) ((font)->lastCol - (font)->firstCol + 1)

/*
 * for 2D char sets
 */

#define	n2dCols(font)	((font)->lastCol - (font)->firstCol +1 )
#define	n2dRows(font)	((font)->lastRow - (font)->firstRow + 1)
#define n2dChars(font)	(n2dCols(font) * n2dRows(font))

#define	ADDRXTHISCHARINFO(pf, ch) \
	((pf)->ppCI[ (((ch)>>8)&0xff)*n2dCols(pf) + ((ch)&0xff)-(pf)->firstCol])

#define	GLYPHWIDTHPIXELS(pci) \
	((pci)->metrics.rightSideBearing - (pci)->metrics.leftSideBearing)
#define	GLYPHHEIGHTPIXELS(pci) \
 	((pci)->metrics.ascent + (pci)->metrics.descent)
#define	GLYPHWIDTHBYTES(pci)	(((GLYPHWIDTHPIXELS(pci))+7) >> 3)
#define GLYPHWIDTHPADDED(bc)	(((bc)+7) & ~0x7)

#if GLYPHPADBYTES == 0 || GLYPHPADBYTES == 1
#define	GLYPHWIDTHBYTESPADDED(pci)	(GLYPHWIDTHBYTES(pci))
#define	PADGLYPHWIDTHBYTES(w)		(((w)+7)>>3)
#endif

#if GLYPHPADBYTES == 2
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+1) & ~0x1)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+1) & ~0x1)
#endif

#if GLYPHPADBYTES == 4
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+3) & ~0x3)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+3) & ~0x3)
#endif

#if GLYPHPADBYTES == 8 /* for a cray? */
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+7) & ~0x7)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+7) & ~0x7)
#endif

#endif /* FONTSTR_H */
