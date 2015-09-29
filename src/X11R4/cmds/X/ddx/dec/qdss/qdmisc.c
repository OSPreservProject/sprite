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

#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"

#include "qd.h"

void qdQueryBestSize(class, pwidth, pheight)
int class;
short *pwidth;
short *pheight;
{
    int given, test;

    switch(class)
    {
      case CursorShape:
	  *pwidth = 16;
	  *pheight = 16;
	  break;
      case TileShape:
      case StippleShape:
	  given = *pwidth;
	  if (given >= 0) {
	      /* Return closest power of two not less than width, but <= 32 */
	      test = 16;
	      while(test > given)
		 test >>= 1;
	      if (test != given) test <<= 1;
	      *pwidth = test;
	  }
	  given = *pheight;
	  if (given >= 0) {
	      /* Return closest power of two not less than height, but <= 32 */
	      test = 16;
	      while(test > given)
		 test >>= 1;
	      if (test != given) test <<= 1;
	      *pheight = test;
	  }
	  break;
    }
}

#if 0
/*
 * A more convenient interface to ChangeGC.  Takes on the burden of
 * knowing the required ordering of elements in the gc value argument
 * to ChangeGC.
 *
 * pGCchanges is very temporary.  It is probably just allocated on the
 * caller's stack.
 */
QDChangeGCHelper( prealGC, pGCchanges) 
    GCPtr	prealGC;	/* the GC to be changed */
    GCPtr	pGCchanges;	/* ephemeral GC struct */
{
    int		bit;
    int		mask;
    unsigned int * pval, * pv;

    pval = pv =
	(unsigned int *) ALLOCATE_LOCAL( GCLastBit * sizeof (unsigned int));
    
    
    for (   bit=0, mask=1;
	    bit<=GCLastBit;
	    bit++, mask<<=1)
    {
	switch( pGCchanges->stateChanges & mask)
	{
	  case GCFunction:
	    *pv++ = pGCchanges->alu; break;
	  case GCPlaneMask:
	    *pv++ = pGCchanges->planemask; break;
	  case GCForeground:
	    *pv++ = pGCchanges->fgPixel; break;
	  case GCBackground:
	    *pv++ = pGCchanges->bgPixel; break;
	  case GCLineWidth:
	    *pv++ = pGCchanges->lineWidth; break;
	  case GCLineStyle:
	    *pv++ = pGCchanges->lineStyle; break;
	  case GCCapStyle:
	    *pv++ = pGCchanges->capStyle; break;
	  case GCJoinStyle:
	    *pv++ = pGCchanges->joinStyle; break;
	  case GCFillStyle:
	    *pv++ = pGCchanges->fillStyle; break;
	  case GCFillRule:
	    *pv++ = pGCchanges->fillRule; break;
	  case GCTile:
	    *pv++ = (unsigned int)pGCchanges->tile; break;
	  case GCStipple:
	    *pv++ = (unsigned int)pGCchanges->stipple; break;
	  case GCTileStipXOrigin:
	    *pv++ = pGCchanges->patOrg.x; break;
	  case GCTileStipYOrigin:
	    *pv++ = pGCchanges->patOrg.y; break;
	  case GCFont:
	    *pv++ = (unsigned int)pGCchanges->font; break;
	  case GCSubwindowMode:
	    *pv++ = pGCchanges->subWindowMode; break;
	  case GCGraphicsExposures:
	    *pv++ = pGCchanges->graphicsExposures; break;
	  case GCClipXOrigin:
	    *pv++ = pGCchanges->clipOrg.x; break;
	  case GCClipYOrigin:
	    *pv++ = pGCchanges->clipOrg.y; break;
	  case GCClipMask:
	    *pv++ = (unsigned int)pGCchanges->clientClip; break;
	  case GCDashOffset:
	    *pv++ = pGCchanges->dashOffset; break;
	  case GCDashList:
	    *pv++ = pGCchanges->numInDashList; break;
	  case GCArcMode:
	    *pv++ = pGCchanges->arcMode; break;
	}
    }
    DoChangeGC( prealGC, pGCchanges->stateChanges, pval, TRUE);
    DEALLOCATE_LOCAL(pval);
}
#endif

#define MAXCOL 79
extern	int	Nchannels;

DumpPixmap( pDraw)
    DrawablePtr	pDraw; /* dbx insists on type agreement for 'call' command */
{
    PixmapPtr	pPix = (PixmapPtr)pDraw;
    int		rowct;
    int		b;		/* counts bytes across a scanline */
    int		bytesperblock;
    int		blockstart = 0;
    int		c;

   tlCancelPixmap(pDraw);
    ErrorF(
	    "%s: address == %x\n\tw==%d\n\th==%d\n\trefcnt==%d\n\tdepth==%d\n\tdevPrivate==%x\n",
	"DumpPixmap",
	pPix,
	QDPIX_WIDTH(pPix),
	QDPIX_HEIGHT(pPix),
	pPix->refcnt,
	pPix->drawable.depth,
	pPix->devPrivate );

    if ( pPix->drawable.depth == 1)
    {
	bytesperblock = MAXCOL/8;
	for (	blockstart = 0;
		blockstart < pPix->devKind;
		blockstart += bytesperblock)
	{
	    for ( rowct=0; rowct < QDPIX_HEIGHT(pPix); rowct++)
	    {
		char	line[256];

		for (	b=blockstart;
			b < min( bytesperblock+blockstart, pPix->devKind);
			b++)
		{
		    for (c=0; c<8; c++)
			 line[(b-blockstart)*8+c] = 
			     (*(QD_PIX_DATA(pPix) +
				rowct * pPix->devKind + b) >> c) & 0x1
			      ? '#' : '.';
		    line[(b-blockstart)*8+c]='\0';
		}
		ErrorF( "%s\n", line);
	    }
	    ErrorF( "\n");
	}
    }
    else	/* 8 or 24 plane pixmap */
    {
	bytesperblock = MAXCOL;
	for (	blockstart = 0;
		blockstart < QDPIX_WIDTH(pPix);
		blockstart += bytesperblock)
	{
	    for ( rowct=0; rowct < QDPIX_HEIGHT(pPix) * Nchannels; rowct++)
	    {
		char	line[256];

		for (	b=blockstart;
			b < min( bytesperblock+blockstart, QDPIX_WIDTH(pPix));
			b++)
		    line[(b-blockstart)] =   /* look at low bit */
			 *( QD_PIX_DATA(pPix)
			     + rowct * QDPIX_WIDTH(pPix) + b) & 1
			  ? '#' : '.';
		line[(b-blockstart)]='\0';
		ErrorF( "%s\n", line);
	    }
	    ErrorF( "\n");
	}
    }
}

unsigned
power2ceiling( x)
    register unsigned int x;
{
    register unsigned int z;

    for ( z = 1; z < -1; z<<=1)
	if ( x <= z)
	    break;
    return z;
}

/* return index (0..) of bit turned on (assuming pow2) */
int
power2bit( x)
    register unsigned int x;
{
    register int z;

    for ( z = 0; (unsigned) (1<<z) < (unsigned) (-1); z++)
	if ( x <= 1<<z)
	    break;
    return z;
}

