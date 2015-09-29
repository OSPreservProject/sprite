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

#include "X.h"
#include "Xproto.h"
#include "Xmd.h"
#include "servermd.h"

#include "scrnintstr.h"
#include "input.h"
#include "cursor.h"
#include "misc.h"

#include "qd.h"
#include "qdprocs.h"

extern Bool qdScreenInit();
extern Bool qdScreenClose();		
extern void qdMouseProc();
extern void qdKeybdProc();

#define NUMSCREENS 1
#define NUMFORMATS 2

static PixmapFormatRec formats[NUMFORMATS] = {
    {1, 1, BITMAP_SCANLINE_PAD},
    {NPLANES, NPLANES, 8}
    };
extern	int	Nplanes;

Bool (*screenInitProcs[NUMSCREENS])() = {
    {qdScreenInit}
    };
Bool (*screenCloseProcs[NUMSCREENS])() = {
    {qdScreenClose}
    };

InitOutput( pScreenInfo, argc, argv)
    ScreenInfo *pScreenInfo;
    int		argc;
    char *	argv[];
{
    int i;

    /* This MUST be called before the Nplanes   *
     * global variable is used.  Gross, eh?	*/
    if ( tlInit() < 0)
    {
	ErrorF( "qdScreenInit: tlInit failed\n");
	return FALSE;
    }

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    pScreenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0; i< NUMFORMATS; i++)
    {
        if (Nplanes != 4) {
	    pScreenInfo->formats[i].depth = formats[i].depth;
        } else {
	    if (formats[i].depth == NPLANES)
		pScreenInfo->formats[i].depth = 4;	/* for screen format */
	    else  /* i.e.: 1 */
		pScreenInfo->formats[i].depth = formats[i].depth;  
        }
	pScreenInfo->formats[i].bitsPerPixel = formats[i].bitsPerPixel;
	pScreenInfo->formats[i].scanlinePad = formats[i].scanlinePad;

    }
	
    AddScreen( qdScreenInit, argc, argv);
}

InitInput( argc, argv)
    int		argc;
    char *	argv[];
{
    DevicePtr	p, k;
    
    p = AddInputDevice( qdMouseProc, TRUE);
    k = AddInputDevice( qdKeybdProc, TRUE);

#ifdef X11R4
    RegisterPointerDevice( p);
#else
    RegisterPointerDevice( p, 0);
#endif
    RegisterKeyboardDevice( k);
}
