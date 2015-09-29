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

/* tldstate.h - masks, indexes, illegals for dragon state tracking */

#define	i_TEMPLATE	(		(u_char) 0)
#define	i_BVIPERS	(i_TEMPLATE +	(u_char) 1)
#define	i_ALU		i_BVIPERS			/* first viper:	*/
#define	i_MASK		(i_ALU +	(u_char) 1)
#define	i_SOURCE	(i_MASK +	(u_char) 1)
#define	i_BGPIXEL	(i_SOURCE +	(u_char) 1)
#define	i_FGPIXEL	(i_BGPIXEL +	(u_char) 1)
#define	i_OCR		(i_FGPIXEL +	(u_char) 1)
#define	i_SRC1OCRA	i_OCR
#define	i_SRC2OCRA	(i_OCR +	(u_char) 1)
#define	i_DSTOCRA	(i_OCR +	(u_char) 2)
#define	i_SRC1OCRB	(i_OCR +	(u_char) 3)
#define	i_SRC2OCRB	(i_OCR +	(u_char) 4)
#define	i_DSTOCRB	(i_OCR +	(u_char) 5)	/* last viper.	*/
#define	i_EVIPERS	(i_DSTOCRB +	(u_char) 1)
#define	i_PLANEMASK	i_EVIPERS			/* planemask	*/
#define	i_CLIP		(i_PLANEMASK +	(u_char) 1)	/* adder:	*/
#define	i_TRANSLATE	(i_CLIP +	(u_char) 1)
#define	i_RASTERMODE	(i_TRANSLATE +	(u_char) 1)
#define	i_FGMASK	(i_RASTERMODE +	(u_char) 1)	/* stiple loads	*/
#define	i_FGBGMASK	(i_FGMASK +	(u_char) 1)
#define	i_NULL		(i_FGBGMASK +	(u_char) 1)

/* converts i_*OCR? value to (0-5) index into array */
#define	OCRINDEX(i)	((i) - i_OCR)
#define	PLANEINDEX(i)	((i) - i_BVIPERS)

#define	m_TEMPLATE	(1L <<	i_TEMPLATE	)
#define	m_ALU		(1L <<	i_ALU		)
#define	m_MASK		(1L <<	i_MASK		)
#define	m_SOURCE	(1L <<	i_SOURCE	)
#define	m_BGPIXEL	(1L <<	i_BGPIXEL	)
#define	m_FGPIXEL	(1L <<	i_FGPIXEL	)
#define	m_SRC1OCRA	(1L <<	i_SRC1OCRA	)
#define	m_SRC2OCRA	(1L <<	i_SRC2OCRA	)
#define	m_DSTOCRA	(1L <<	i_DSTOCRA	)
#define	m_SRC1OCRB	(1L <<	i_SRC1OCRB	)
#define	m_SRC2OCRB	(1L <<	i_SRC2OCRB	)
#define	m_DSTOCRB	(1L <<	i_DSTOCRB	)
#define	m_PLANEMASK	(1L <<	i_PLANEMASK	)
#define	m_CLIP		(1L <<	i_CLIP		)
#define	m_TRANSLATE	(1L <<	i_TRANSLATE	)
#define	m_RASTERMODE	(1L <<	i_RASTERMODE	)
#define	m_FGMASK	(1L <<	i_FGMASK	)
#define	m_FGBGMASK	(1L <<	i_FGBGMASK	)
#define	m_NOOP		(1L <<	i_NULL		)
#define	m_NULL		(0L)

/* the rest of this stuff is depth-dependent - yuk! */

typedef	u_short		dTemplateRec, *dTemplatePtr;
#define	BITS_TEMPLATE	((dTemplateRec) 0x3fff)
#define	ILL_TEMPLATE	(~(BITS_TEMPLATE))

typedef	u_short		dAluRec, *dAluPtr;
#define	BITS_ALU	((dAluRec) 0xff)
#define	ILL_ALU		(~(BITS_ALU))

typedef	u_long		dMaskRec, *dMaskPtr;
#define	BITS_MASK	((dMaskRec) 0xffff)
#define	ILL_MASK	(~(BITS_MASK))

#if	NPLANES==24
  typedef	u_long		dSourceRec, *dSourcePtr;
# define	BITS_SOURCE	((dSourceRec) 0xffffff)
#else	/* NPLANES==8 */
  typedef	u_short		dSourceRec, *dSourcePtr;
# define	BITS_SOURCE	((dSourceRec) 0xff)
#endif
#define	ILL_SOURCE	(~(BITS_SOURCE))

typedef	dSourceRec	dPixelRec, *dPixelPtr,
			dFgpixelRec, *dFgpixelPtr, dBgpixelRec, *dBgpixelptr;
#define	BITS_PIXEL	BITS_SOURCE
#define	ILL_PIXEL	(~(BITS_PIXEL))
#define	BITS_BGPIXEL	BITS_PIXEL
#define	ILL_BGPIXEL	ILL_PIXEL
#define	BITS_FGPIXEL	BITS_PIXEL
#define	ILL_FGPIXEL	ILL_PIXEL

typedef	u_char		dOcrRec, *dOcrPtr;
#define	BITS_OCR	((dOcrRec) 0x3f)
#define	ILL_OCR		(~(BITS_OCR))

typedef	u_long		dPlanemaskRec, *dPlanemaskPtr;
#define	BITS_PLANEMASK	((dPlanemaskRec) 0xff)
#define	ILL_PLANEMASK	(~(BITS_PLANEMASK))

typedef	u_char		dStipmaskRec, *dStipmaskPtr;
#define	BITS_STIPMASK	((dStipmaskRec) 0xff)
/* can't think of any use for an ill_stipmask. */

typedef	u_short		dClipRec, *dClipPtr;
#define	BITS_CLIP	((dClipRec) 0xfff)
#define	ILL_CLIP	(~(BITS_CLIP))

typedef	dClipRec	dTranslateRec, *dTranslatePtr;
#define	BITS_TRANSLATE	BITS_CLIP
#define	ILL_TRANSLATE	ILL_CLIP

typedef	u_short		dRastermodeRec, *dRastermodePtr;
#define	BITS_RASTERMODE	((dRastermodeRec) 0xff)
#define	ILL_RASTERMODE	(~(BITS_RASTERMODE))

/* These are the structures themselves */

typedef struct	dViperStr
{
	dAluRec		alu;		/* r_3			*/
	dMaskRec	mask;		/* wide load of m_1 & 2	*/
	dSourceRec	source;		/* z-mode load	*/
	dBgpixelRec	bgpixel;	/* z-mode load	*/
	dFgpixelRec	fgpixel;	/* z-mode load	*/
	dOcrRec		ocr[6];		/* use OCRINDEX(i_*OCR?)	*/
}	dViperRec, *dViperPtr;

typedef struct	dAdderStr
{
	BoxRec		clip;
	DDXPointRec	translate;
	dRastermodeRec	rastermode;
}	dAdderRec, *dAdderPtr;

/* planes[PLANEINDEX] indicated the last planemask used for updating this *
 * field.  If no new planes were added--no work.			  */
typedef struct	dStateStr
{
	dPlanemaskRec	planemask;
	dTemplateRec	template;	/* last template jmpt_ */
	dPlanemaskRec	planes[i_EVIPERS - i_BVIPERS];

	dViperRec	common;		/* in planes true in planes[i_*] */
	dViperRec	remain;		/* in the other planes */

	dAdderRec	adder;
}	dStateRec, *dStatePtr;

/* The Shadow lurks... */
extern dStateRec	dState;

typedef u_long
	dQueueRec, *dQueuePtr;

typedef struct	dUpdateStr
{
	dPlanemaskRec	planemask;
	dTemplateRec	template;	/* next template jmpt_ */
	dStipmaskRec	stipmask;

	dViperRec	common;
	dAdderRec	adder;
}	dUpdateRec, *dUpdatePtr;

