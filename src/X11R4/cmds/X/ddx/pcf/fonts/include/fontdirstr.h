#ifndef FONTDIRSTR_H
#define	FONTDIRSTR_H 1
/***********************************************************
Copyright 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
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

/* $XConsortium: fontdirstr.h,v 1.6 88/10/10 18:21:12 rws Exp $ */

#include "fontdir.h"

typedef struct _FontName {
    char		*fontName;
    TypedFontFilePtr	 pTFF;
} FontNameRec, *FontNamePtr;

#define NullFontName ((FontNamePtr) NULL)


typedef struct _FontTable {
    int			used;
    int			size;
    FontNamePtr		fonts;		/* always sorted */
    pointer		osContext;	/* usually a directory name */
    pointer		pPriv;
} FontTableRec;

#define	fdPrivate(ft)	((ft)->pPriv)
#define	fdOSContext(ft)	((ft)->osContext)

#endif /* FONTDIRSTR_H */
