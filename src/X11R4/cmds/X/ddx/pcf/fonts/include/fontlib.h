/***********************************************************
Copyright 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
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

#ifndef FONTLIB_H
#define FONTLIB_H

typedef struct _BuildParams {
    int		glyphPad;
    int		bitOrder;
    int		byteOrder;
    int		scanUnit;
    int		badbitsWarn;
    int		ignoredCharWarn;
    int		makeTEfonts;
    int		inhibitInk;
} BuildParamsRec, *BuildParamsPtr;

	/*
	 * encode bit/byte ordering, pad and scan unit info in a single
	 * byte.
	 */


#define	FMT_GLY_PAD_MASK	(3<<0)
#define	FMT_MS_BYTE		(1<<2)
#define	FMT_MS_BIT		(1<<3)
#define	FMT_SCAN_UNIT_MASK	(3<<4)

#define	FMT_BYTE_ORDER(f)	(((f)&FMT_MS_BYTE)?MSBFirst:LSBFirst)
#define FMT_BIT_ORDER(f)	(((f)&FMT_MS_BIT)?MSBFirst:LSBFirst)
#define	FMT_GLYPH_PAD(f)	(1<<FMT_GLYPH_PAD_INDEX(f))
#define	FMT_SCAN_UNIT(f)	(1<<FMT_SCAN_UNIT_INDEX(f))
#define	FMT_GLYPH_PAD_INDEX(f)	((f)&FMT_GLY_PAD_MASK)
#define	FMT_SCAN_UNIT_INDEX(f)	(((f)&FMT_SCAN_UNIT_MASK)>>4)

#define	FMT_SET_BYTE_ORDER(f,o)	((o)==MSBFirst?((f)|=FMT_MS_BYTE):\
						((f)&=(~FMT_MS_BYTE)))
#define	FMT_SET_BIT_ORDER(f,o)	((o)==MSBFirst?((f)|=FMT_MS_BIT):\
						((f)&=(~FMT_MS_BIT)))
#define	FMT_SET_GLYPH_PAD(f,p)	((f)=((f)&(~FMT_GLY_PAD_MASK))|\
					((ffs(p)-1)&FMT_GLY_PAD_MASK))
#define	FMT_SET_SCAN_UNIT(f,s)	((f)=((f)&(~FMT_SCAN_UNIT_MASK))|\
				   (((ffs(s)-1)<<4)&FMT_SCAN_UNIT_MASK))

	/*
	 * store as low byte
	 */
#define	FORMAT_MASK		(0xffffff00)
#define	ORDER_PAD_MASK		(0x000000ff)

/* 7/12/89 (ef) -- shouldn't these declarations really be somewhere else? */
extern void	ComputeFontAccelerators();	/* from fontaccel.c */
extern void	ComputeInfoAccelerators();

extern void	ComputeFontBounds();		/* from fontbounds.c */

extern void	BitOrderInvert();		/* from fontutil.c */
extern void	TwoByteInvert();
extern void	FourByteInvert();

extern void DumpFont();				/* from fontdump.c */

typedef enum {dumpFontInfo, dumpCharInfo, dumpPictures} DumpLevel;


/* 7/12/89 (ef) -- shouldn't these be in some "portability" file */
#define IntSize 4	/* how many bytes an int takes in the file */
#define ShortSize 2	/* how many bytes a short takes in the file */
#define ByteSize 1	/* how many bytes a byte takes in the file */

#ifdef vax

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER 	LSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#else
# ifdef sun

#  if (sun386 || sun5)
#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	LSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#  else
#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#  endif

# else
#  ifdef apollo

#	define DEFAULTGLPAD 	2		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#  else
#   ifdef ibm032

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#   else
#    ifdef hpux

#	define DEFAULTGLPAD 	2		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#    else
#     ifdef pegasus

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#     else
#      ifdef macII

#       define DEFAULTGLPAD     4               /* default padding for glyphs */
#       define DEFAULTBITORDER  MSBFirst        /* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#       else
#        ifdef mips

#	 define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	 define DEFAULTBITORDER	LSBFirst	/* default bitmap bit order */
# 	 define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	 define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#       else
#	 define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	 define DEFAULTBITORDER MSBFirst	/* default bitmap bit order */
#	 define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	 define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#       endif
#      endif
#     endif
#    endif
#   endif
#  endif
# endif
#endif

/*
 * BYTES_PER_ROW not the same as the GLYPHWIDTHBYTESPADDED in fontstr.h.
 * This one does the computation at runtime and not compile time.
 */

#define BYTES_PER_ROW(bits, nbytes) \
	((nbytes) == 1 ? (((bits)+7)>>3)	/* pad to 1 byte */ \
	:(nbytes) == 2 ? ((((bits)+15)>>3)&~1)	/* pad to 2 bytes */ \
	:(nbytes) == 4 ? ((((bits)+31)>>3)&~3)	/* pad to 4 bytes */ \
	:(nbytes) == 8 ? ((((bits)+63)>>3)&~7)	/* pad to 8 bytes */ \
	: 0)

/*
 * BYTES_FOR_GLYPH only works in a context where there is a 'params'
 * pointer to a BuildParamsRec.
 */

#define BYTES_FOR_GLYPH(ci,pad) (GLYPHHEIGHTPIXELS(ci) * \
            BYTES_PER_ROW(GLYPHWIDTHPIXELS(ci), pad))

#endif /* FONTLIB_H */
