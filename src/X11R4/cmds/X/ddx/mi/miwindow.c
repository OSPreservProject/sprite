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
/* $XConsortium: miwindow.c,v 5.5 89/10/06 17:28:09 keith Exp $ */
#include "X.h"
#include "miscstruct.h"
#include "region.h"
#include "mi.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"

void 
miClearToBackground(pWin, x, y, w, h, generateExposures)
    WindowPtr pWin;
    short x,y;
    unsigned short w,h;
    Bool generateExposures;
{
    BoxRec box;
    RegionPtr pReg, pBSReg = NullRegion;

    box.x1 = pWin->drawable.x + x;
    box.y1 = pWin->drawable.y + y;
    if (w)
        box.x2 = box.x1 + w;
    else
        box.x2 = box.x1 + (int) pWin->drawable.width - x;
    if (h)
        box.y2 = box.y1 + h;	
    else
        box.y2 = box.y1 + (int) pWin->drawable.height - y;

    pReg = (* pWin->drawable.pScreen->RegionCreate)(&box, 1);
    if (pWin->backStorage)
    {
	/*
	 * If the window has backing-store on, call through the
	 * ClearToBackground vector to handle the special semantics
	 * (i.e. things backing store is to be cleared out and
	 * an Expose event is to be generated for those areas in backing
	 * store if generateExposures is TRUE).
	 */
	pBSReg = (* pWin->drawable.pScreen->ClearBackingStore)(pWin, x, y, w, h,
						 generateExposures);
    }

    (* pWin->drawable.pScreen->Intersect)(pReg, pReg, &pWin->clipList);
    if (generateExposures)
	(*pWin->drawable.pScreen->WindowExposures)(pWin, pReg, pBSReg);
    else if (pWin->backgroundState != None)
        (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pReg, PW_BACKGROUND);
    (* pWin->drawable.pScreen->RegionDestroy)(pReg);
    if (pBSReg)
	(* pWin->drawable.pScreen->RegionDestroy)(pBSReg);
}
