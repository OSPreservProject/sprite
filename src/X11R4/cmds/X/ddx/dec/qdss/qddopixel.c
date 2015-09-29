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

#include <sys/types.h>
#include <stdio.h>	/* debug */

#include "X.h"
#include "windowstr.h"
#include "regionstr.h"
#include "pixmap.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include "Xproto.h"
#include "Xprotostr.h"
#include "mi.h"
#include "Xmd.h"

/*
 * driver headers
 */
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdioctl.h>
#include <vaxuba/qdreg.h>

#include "qd.h"
#include "qdgc.h"

#include "scrnintstr.h"

/* qddopixel(psrc, pdst, pGC)
 *    does output to a (full-depth) pixel pointed to by pdst
 *	  dst byte from pPoint in pPixmap
 *        alu (with dst pixel), planemask, {fg, bg} on fillStyle
 *   NOT: tiling, stippling, opaquestippling, clipping
 */


unsigned char
qdlogic(src, dst, alu, planebyte)
    unsigned char	src, dst;
    int			alu;
    unsigned char	planebyte;
{
    unsigned char	aluout;

    switch (alu)
    {
      case GXclear:
	aluout = 0;
	break;
      case GXand:
	aluout = src & dst;
	break;
      case GXandReverse:
	aluout = src & ~dst;
	break;
      case GXcopy:
	aluout = src;
	break;
      case GXandInverted:
	aluout = ~src & dst;
	break;
      case GXnoop:
	aluout = dst;
	break;
      case GXxor:
	aluout = src ^ dst;
	break;
      case GXor:
	aluout = src | dst;
	break;
      case GXnor:
	aluout = ~(src | dst);
	break;
      case GXequiv:
	aluout = ~src ^ dst;
	break;
      case GXinvert:
	aluout = ~dst;
	break;
      case GXorReverse:
	aluout = src | ~dst;
	break;
      case GXcopyInverted:
	aluout = ~src;
	break;
      case GXorInverted:
	aluout = ~src | dst;
	break;
      case GXnand:
	aluout = ~(src & dst);
	break;
      case GXset:
	aluout = ~0;
	break;
    }
    dst = (aluout & planebyte) | (dst & ~planebyte);
    return(dst);
}

extern	int	Nentries;
extern	int	Nplanes;
extern unsigned int Allplanes;
#if NPLANES==24
extern	int	Nchannels;
#define FOR_EACH_CHANNEL for (icolor = 0; icolor < Nchannels; icolor++)
#else
#define Nchannels 1
#define FOR_EACH_CHANNEL /* no loop */
#define icolor 0
#endif

void
#if NPLANES==24
qddopixel(psrc, pdst, pGC, igreen)
#else
qddopixel(psrc, pdst, pGC)
#endif
    unsigned char *	psrc;
    register unsigned char *	pdst;
#if 0
    PixmapPtr		pPixmap;
    DDXPointPtr		pPoint;
#endif
    GCPtr		pGC;
{
    QDPrivGCPtr		pPriv;
    register unsigned char *	ptable;
#if NPLANES==24
    int			igreen = QDPIX_WIDTH(pPixmap) * QDPIX_HEIGHT(pPixmap);
    int			icolor;
#endif
#if 0
    register unsigned char *	pdst;
    pdst = QD_PIX_DATA(pPixmap) + pPoint->x + pPoint->y * QDPIX_WIDTH(pPixmap);
#endif
#ifdef X11R4
    pPriv = (QDPrivGCPtr) (pGC->devPrivates[qdGCPrivateIndex].ptr);
#else
    pPriv = (QDPrivGCPtr) (pGC->devPriv);
#endif
    ptable = pPriv->ptresult;
#ifdef DEBUGDOPIXEL
    if (!pPriv)
        FatalError("GC devprivate == NULL in qddopixel\n");
    if (*psrc > 1 && pGC->fillStyle & (FillStippled|FillOpaqueStippled))
        FatalError("src byte > 1 for stippling case in qddopixel\n");
#endif
    if (pPriv->mask & QD_NEWLOGIC)
    {
	int	idst;

	switch (pGC->alu)
	{
	  case GXclear:
	  case GXset:
	    pPriv->mask = QD_LOOKUP|QD_DSTBYTE;
	    if (pPriv->ptresult)
		Xfree(pPriv->ptresult);
	    pPriv->igreen = 256;
	    pPriv->ptresult = (unsigned char *)
		Xalloc(NPLANES*32*sizeof(unsigned char));
	    ptable = pPriv->ptresult;
	    FOR_EACH_CHANNEL {
	        for (idst = 0; idst < 256; idst++, ptable++)
	            *ptable = qdlogic(0, (unsigned char) idst, pGC->alu,
			*((unsigned char *)&(pGC->planemask) + icolor));
	    }
	    break;
	  case GXnoop:
	  case GXinvert:
	    pPriv->mask = QD_LOOKUP|QD_DSTBYTE;
	    if (pPriv->ptresult)
		Xfree(pPriv->ptresult);
	    pPriv->igreen = 256;
	    pPriv->ptresult = (unsigned char *)
		Xalloc(NPLANES*32*sizeof(unsigned char));
	    ptable = pPriv->ptresult;
	    FOR_EACH_CHANNEL {
	        for (idst = 0; idst < 256; idst++, ptable++)
	            *ptable = qdlogic(0, (unsigned char) idst, pGC->alu,
			*((unsigned char *)&(pGC->planemask) + icolor));
	    }
	    break;
	  case GXcopy:
	  case GXcopyInverted:
	    if ((pGC->planemask & Allplanes) == Allplanes)
	    {
	        pPriv->mask = QD_LOOKUP;
	        if (pPriv->ptresult)
		    Xfree(pPriv->ptresult);
	        pPriv->igreen = 256;
	        pPriv->ptresult = (unsigned char *)
		    Xalloc(NPLANES*32*sizeof(unsigned char));
	        ptable = pPriv->ptresult;
		FOR_EACH_CHANNEL {
	            for (idst = 0; idst < 256; idst++, ptable++)
	                *ptable = qdlogic((unsigned char) idst, 0,
			    pGC->alu, 255);
	        }
		break;
	    }
	  default:
#ifndef FASTSTIP
	    pPriv->mask = 0;
	    if (pPriv->ptresult)
		Xfree(pPriv->ptresult);
	    pPriv->ptresult = (unsigned char *) NULL;
#else
            switch (pGC->fillStyle)
	    {
              case FillSolid:
	      case FillTiled:
	        pPriv->mask = 0;
	        if (pPriv->ptresult)
	     	  Xfree(pPriv->ptresult);
	 	pPriv->ptresult = (unsigned char *) NULL;
	        break;
	      case FillOpaqueStippled:
	        pPriv->mask = QD_LOOKUP|QD_SRCBIT|QD_DSTBYTE;
	        if (pPriv->ptresult)
		    Xfree(pPriv->ptresult);
	        pPriv->igreen = 512;
	        pPriv->ptresult = (unsigned char *)
		    Xalloc(NPLANES*64*sizeof(unsigned char));
	        ptable = pPriv->ptresult;
		FOR_EACH_CHANNEL {
	            for (idst = 0; idst < 256; idst++, ptable++)
	                *ptable = qdlogic((unsigned char *) &pGC->bgPixel,
			    (unsigned char) idst, pGC->alu, 255);
	            for (idst = 0; idst < 256; idst++, ptable++)
	                *ptable = qdlogic((unsigned char *) &pGC->fgPixel,
			    (unsigned char) idst, pGC->alu, 255);
	        }
	        break;
	      case FillStippled:
	        pPriv->mask = QD_LOOKUP|QD_SRCBIT|QD_DSTBYTE;
	        if (pPriv->ptresult)
	 	    Xfree(pPriv->ptresult);
	        pPriv->igreen = 512;
	        pPriv->ptresult = (unsigned char *)
		    Xalloc(NPLANES*64*sizeof(unsigned char));
	        ptable = pPriv->ptresult;
	        FOR_EACH_CHANNEL {
	            for (idst = 0; idst < 256; idst++, ptable++)
	                *ptable = qdlogic(0, (unsigned char) idst, GXnoop, 255);
	            for (idst = 0; idst < 256; idst++, ptable++)
	                *ptable = qdlogic((unsigned char *) &pGC->fgPixel,
			    (unsigned char) idst, pGC->alu, 255);
	        }
	        break;
	    }
#endif
	    break;
	}
	ptable = pPriv->ptresult;
    }
#if NPLANES == 24
    for (icolor = 0; icolor < Nchannels; icolor++, pdst += igreen, psrc++,
	ptable += pPriv->igreen)
#endif
    {
        switch (pPriv->mask)
	{
          case QD_LOOKUP:
          case QD_LOOKUP|QD_SRCBIT:
	    *pdst = *(ptable + *psrc);
	    break;
          case QD_LOOKUP|QD_DSTBYTE:
	    *pdst = *(ptable + *pdst);
	    break;
    /*    case QD_LOOKUP|QD_SRCBIT|QD_DSTBYTE:
     *	    *pdst = *(ptable + *pdst + (*psrc << 8));
     *	    break;
     */
          case 0:
            *pdst = qdlogic(*psrc, *pdst, pGC->alu,
			    *((unsigned char *)&(pGC->planemask) + icolor));
	    break;
#ifdef DEBUGDOPIXEL
          default:
	    FatalError("illegal lookup type in GC private structure\n");
	    break;
#endif
        }
    }
}
