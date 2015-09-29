/* $XConsortium: dixfontstr.h,v 1.6 89/03/11 15:20:13 rws Exp $ */
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

#ifndef DIXFONTSTRUCT_H
#define DIXFONTSTRUCT_H

#include "dixfont.h"
#include "font.h"
#include "misc.h"

extern FontPtr	FontFileLoad(/* name, length */); /* implemented in OS layer */
extern Bool	FontFilePropLoad(/* name, length, *font, fi, *props */);
extern void	FontUnload(/* font */);

typedef struct _DIXFontProp {
    ATOM	name;
    INT32	value;	/* assumes ATOM is not larger than INT32 */
} DIXFontProp;

/*
 * FONT is created at font load time; it is not part of the
 * font file format
 */
typedef struct _Font {
    FontInfoPtr	pFI;
    DIXFontProp	*pFP;
    CharInfoPtr	pCI;			/* bitmap metrics and offset */
    char	*pGlyphs;
    pointer	osPrivate;
    int		fileType;		/* tag for OS layer */
    int		refcnt;			/* free storage when this goes to 0 */
    pointer	devPriv[MAXSCREENS];	/* information private to screen */
    CharInfoPtr	pInkCI;			/* ink metrics */
    CharInfoPtr	pInkMin;		/* ink metrics */
    CharInfoPtr	pInkMax;		/* ink metrics */
} FontRec;


/* in dixfont.c */
extern FontPtr	OpenFont();
extern Bool	SetDefaultFont();
extern int	CloseFont();
extern Bool	DescribeFont();
extern void	ServerBitmapFromGlyph();
extern Bool	CursorMetricsFromGlyph();
extern void	GetGlyphs();
extern Bool	QueryTextExtents();

#endif /* DIXFONTSTRUCT_H */
