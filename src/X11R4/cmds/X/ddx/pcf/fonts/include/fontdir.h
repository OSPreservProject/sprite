#ifndef FONTDIR_H
#define	FONTDIR_H 1
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

/* $XConsortium: fontdir.h,v 1.6 88/10/10 18:21:12 rws Exp $ */

#include "fontos.h"
#include "fontfile.h"

#define FontDirFile	"fonts.dir"
#define AliasFile	"fonts.alias"
#define NoMatch		-1

typedef struct _FontTable *FontTablePtr;
#define NullFontTable ((FontTablePtr) NULL)

/* from fontdir.c */

extern	FontTablePtr		fdCreateFontTable(/* directory, size */);
extern	void			fdFreeFontTable(/* table */);
extern	TypedFontFilePtr	fdFindFontFile(/* table, fontName */);
extern	Bool			fdAddFont(/* table, fontName, pTFF */);
extern	Bool			fdAddFileNameAliases(/* table */);

/* 8/15/89 (ef) -- should this be in utils somewhere? */
extern	Bool			fdMatch( /* pattern, string */ );

#endif /* FONTDIR_H */
