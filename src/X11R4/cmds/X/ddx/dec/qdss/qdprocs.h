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

/*
 * screen procs
 */
extern Bool qdCloseScreen();
extern void qdQueryBestSize();
extern Bool qdCreateWindow();
extern Bool qdDestroyWindow();
extern Bool qdPositionWindow();
extern Bool qdChangeWindowAttributes();
extern Bool qdMapWindow();
extern Bool qdUnmapWindow();
extern Bool qdRealizeFont();
extern Bool qdUnrealizeFont();
extern void qdPointerNonInterestBox();
extern void qdConstrainCursor();
extern void qdCursorLimits();
extern Bool qdDisplayCursor();
extern Bool qdRealizeCursor();
extern Bool qdUnrealizeCursor();
extern Bool qdSetCursorPosition();
extern Bool qdScreenSaver();
extern Bool qdCreateGC();
extern void qdDestroyGC();
extern void qdValidateGC();
extern PixmapPtr qdCreatePixmap();
extern PixmapPtr mfbCreatePixmap();
extern Bool qdDestroyPixmap();
#ifdef X11R4
extern void mfbGetSpans();
extern void qdGetSpans();
#else
extern unsigned int * mfbGetSpans();
extern unsigned int * qdGetSpans();
#endif
extern void qdGetImage();

#ifdef X11R4
Bool	qdCreateColormap();
#else
void	qdCreateColormap();
#endif
void	qdDestroyColormap();
void	qdInstallColormap();
void	qdUninstallColormap();
int	qdListInstalledColormaps();
void	qdStoreColors();
void	qdResolveColor();


/*
 * window procs
 */
extern void qdCopyWindow();
extern void qdSaveDoomedAreas();

/*
 * GC procs
 */
extern void qdValidateGC();

extern void qddopixel();
extern void qdFillSpans();
extern void qdSetSpansWin();
extern void qdSetSpansPix1();
extern void qdSetSpansPixN();
extern void qdPixPutImage();
extern void qdWinPutImage();

extern void tlPolylines();

extern void miWideLine();
extern void miWideDash();
extern void miMiter();
extern void miNotMiter();
extern void miGetImage();
extern void miRecolorCursor();

extern void qdPolySegment();

extern RegionPtr qdCopyArea();
extern RegionPtr qdCopyAreaWin();
extern RegionPtr qdCopyPlane();
extern RegionPtr qdCopyPlanePix();
extern void qdPolyFillRect();
extern void qdFillPolygon();

extern void qdImageTextPix();
extern int qdPolyTextPix();
extern void qdPolyGlyphBlt();

extern void qdTileBox();
extern void qdPaintBox();
extern void qdStippleBox();
extern PixmapPtr qdCopyPixmap();
extern RegionPtr qdpixtoreg();
extern RegionPtr qdRegionInit();
extern void  qdConvertRects();
extern void  qdPushPixels();
extern void  qdCopyGCDest();
extern void  qdChangeClip();
extern void  qdDestroyClip();
extern void  qdCopyClip();
extern void  qdPolyFillRectOddSize();
extern void  qdWinFSOddSize();
extern void  qdFSUndrawable();


/*
 * GCInterest procs
 */
extern void qdCopyGCDest();
