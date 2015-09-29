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
#ifndef FONT_H
#define FONT_H 1

#include "fontos.h"

	/* data structures */
typedef struct _EncodedFont	*EncodedFontPtr;
typedef struct _FontProp	*FontPropPtr;
typedef struct _CharInfo	*CharInfoPtr;
typedef struct _CharSet		*CharSetPtr;
typedef struct _ExtentInfo	*ExtentInfoPtr;

#define NullCharInfo	((CharInfoPtr)0)
#define NullCharSet	((CharSetPtr)0)
#define NullEncodedFont	((EncodedFontPtr)0)

	/* draw direction */
#define LeftToRight 0
#define RightToLeft 1
#define BottomToTop 2
#define TopToBottom 3
typedef int DrawDirection;

#define NO_SUCH_CHAR	-1

#define FONT_PROPERTIES		(1<<0)
#define FONT_ACCELERATORS	(1<<1)
#define FONT_METRICS		(1<<2)
#define FONT_BITMAPS		(1<<3)
#define FONT_INK_METRICS	(1<<4)
#define	FONT_BDF_ENCODINGS	(1<<5)
#define FONT_SWIDTHS		(1<<6)
#define FONT_GLYPH_NAMES	(1<<7)

#define NUM_FONT_TABLES		8
/* maximum number of font tables that may be present in a file (arbitrary) */
#define MAX_FONT_TABLES		32

#define	FONT_NO_TABLES (0)

#define FONT_STANDARD_TABLES	( \
	FONT_METRICS | FONT_BITMAPS | FONT_BDF_ENCODINGS | \
	FONT_INK_METRICS | FONT_PROPERTIES | FONT_ACCELERATORS )

#define	FONT_ALL_METRICS	( \
	FONT_METRICS | FONT_INK_METRICS | FONT_ACCELERATORS|FONT_BDF_ENCODINGS)

#define FONT_LEGAL_TABLES	( \
	FONT_METRICS | FONT_BITMAPS | FONT_SWIDTHS | FONT_GLYPH_NAMES | \
	FONT_BDF_ENCODINGS | FONT_INK_METRICS | FONT_PROPERTIES | \
	FONT_ACCELERATORS )

#define FONT_EVERYTHING		(0x7FFFFFFF)

/*
 * and the public procedures
 * 7/12/89 (ef) -- should change soon.
 */

extern	EncodedFontPtr	 fontGetInfo(/*pPath,pathLen,pFont*/);
extern	EncodedFontPtr	 fontLoad(/*pPath,pathLen*/);
extern	void		 fontUnload(/*pFont*/);
extern	char		*fontTableName(/*table*/);

extern	void	GetGlyphs();
extern	void	QueryGlyphExtents();

#ifdef X11R4
extern	Bool	QueryTextExtents();
#else
extern	void	QueryTextExtents();
#endif

#endif /* FONT_H */
