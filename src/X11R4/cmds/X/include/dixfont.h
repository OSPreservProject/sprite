/* $XConsortium: dixfont.h,v 1.2 88/09/06 15:48:30 jim Exp $ */
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
#ifndef DIXFONT_H
#define DIXFONT_H 1
#define NullDIXFontProp ((DIXFontPropPtr)0)
#define NullFont ((FontPtr)0)

typedef struct _DIXFontProp *DIXFontPropPtr;
typedef struct _Font *FontPtr;
/*
 * this type is for people who want to talk about the font encoding
 */

typedef enum {Linear8Bit, TwoD8Bit, Linear16Bit, TwoD16Bit} FontEncoding;
typedef struct _FontData *FontDataPtr;

extern FontPtr OpenFont();
#endif /* DIXFONT_H */
