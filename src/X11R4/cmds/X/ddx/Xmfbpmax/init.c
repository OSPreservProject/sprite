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

/* $XConsortium: init.c,v 1.4 89/10/10 10:16:30 rws Exp $ */

#include "X.h"
#include "Xproto.h"
#include <sys/types.h>
#include <machine/pmreg.h>

#include "scrnintstr.h"
#include "servermd.h"
#include "input.h"

/*
 * The contents of this file are not just dependent upon the pmax b/w display,
 * but describe the configuration of the entire server.
 *
 * There is an init file for each possible configuration of a server. This
 * is the one for a server that has nothing but an pm hooked to it.
 */

extern Bool pmScreenInit();
extern void pmMouseProc();
extern void pmKeybdProc();

#define NUMFORMATS 1
static	PixmapFormatRec formats[] = {{1, 1, BITMAP_SCANLINE_PAD}};

InitOutput(screenInfo, argc, argv)
    ScreenInfo *screenInfo;
    int argc;
    char **argv;
{
    int i;

    int		imageByteOrder;
    int		bitmapScanlineUnit;
    int		bitmapScanlinePad;
    int		bitmapBitOrder;
    int		numPixmapFormats;

    screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    screenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0; i< NUMFORMATS; i++)
    {
	screenInfo->formats[i].depth = formats[i].depth;
	screenInfo->formats[i].bitsPerPixel = formats[i].bitsPerPixel;
	screenInfo->formats[i].scanlinePad = formats[i].scanlinePad;
    }

    AddScreen(pmScreenInit, argc, argv);

}

void
InitInput(argc, argv)
    int argc;
    char *argv[];
{
    DevicePtr p, k;
    
    p = AddInputDevice(pmMouseProc, TRUE);

    k = AddInputDevice(pmKeybdProc, TRUE);

    RegisterPointerDevice(p);
    RegisterKeyboardDevice(k);
}
