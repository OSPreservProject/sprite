/************************************************************************
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

************************************************************************/

/* $XConsortium: fonttype.c,v 1.4 88/10/10 18:22:45 rws Exp $ */

#include <X11/X.h>
#include "fontstr.h"
#include "fonttype.h"

#ifndef UNCOMPRESSFILT
#define UNCOMPRESSFILT "/usr/ucb/uncompress"
#endif
#ifndef FCFLAGS
#define FCFLAGS "-t"
#endif
#ifndef BDFTOSNFFILT
#define BDFTOSNFFILT "/usr/bin/X11/bdftosnf"
#endif
#ifndef SHELLPATH
#define SHELLPATH "/bin/sh"
#endif
#ifndef ZBDFTOSNFFILT
#define ZBDFTOSNFFILT "/usr/ucb/uncompress | /usr/bin/X11/bdftosnf -t"
#endif

#ifdef FONT_BDF
#include "bdf.h"
#endif /* FONT_BDF */

#ifdef FONT_PCF
#include "pcf.h"
#endif /* FONT_PCF */

#ifdef COMPRESSED_FONTS
static char *
simpleZFilter[] = {UNCOMPRESSFILT, NULL};
#endif

#ifdef FONT_BDF
static char *
bdfFilter[] = {BDFTOSNFFILT, FCFLAGS, NULL};
#ifdef COMPRESSED_FONTS
static char *
bdfZFilter[] = {SHELLPATH, "-c", ZBDFTOSNFFILT, NULL};
#endif
#endif

FontFileReaderRec fontFileReaders[] = {
#ifdef FONT_PCF
    {".pcf", pcfReadFont,  NullFreeFontProc, (char **)NULL},
#ifdef COMPRESSED_FONTS
    {".pcf.Z", pcfReadFont, NullFreeFontProc, simpleZFilter},
#endif /* COMPRESSED_FONTS */
#endif /* FONT_PCF */

#ifdef FONT_SNF
    {".snf", snfReadFont, NullFreeFontProc, (char **)NULL},
#ifdef COMPRESSED_FONTS
    {".snf.Z", snfReadFont, NullFreeFontProc, simpleZFilter},
#endif /* COMPRESSED_FONTS */
#endif /* FONT_SNF */

#ifdef FONT_BDF
    {".bdf", bdfReadFont, NullFreeFontProc, (char **)NULL},
#ifdef COMPRESSED_FONTS
    {".bdf.Z", bdfReadFont, NullFreeFontProc, simpleZFilter},
#endif /* COMPRESSED_FONTS */
#endif /* FONT_BDF */
    NULL
};

