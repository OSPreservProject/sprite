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

#include "X.h"
#include "windowstr.h"
#include "gcstruct.h"

#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>

#include "qd.h"
#include "qdgc.h"
#include "tl.h"
#include "tltemplabels.h"

#include "tldstate.h"

/* tldrawshapes - create various packets using tlprimitive
 *	set some flags and use conditional code in tlprimitive
 *	the flags correspond to dragon setup and output primitives
 *	set these flags with care!
 * flags:
 *	PROCNAME	procedure name
 *	    conventions:
 *		tl + (Oddities) + (Style) + (Shape) + s
 *	D_TILE	dragon tile (style = tiled or stip or opstip)
 *	D_MASK	(for tiles) dragon pattern (style = stip or opstip)
 *	D_FG	(for masks) use only foreground
 *	D_FGBG	(for masks) use foreground and background
 *	D_ODD	(for tiles) odd sized dragon tile (not power of 2 in [4,512])
 *	D_SPANS	draw spans (otherwise, rectangles)
 * n.b.:
 *	D_MASK assumes D_TILE
 *	D_MASK assumes D_FG or D_FGBG
 *	D_ODD assumes D_TILE
 *	D_FG and D_FGBG are mutually exclusive (and may be fatal)
 *	D_TILE without D_MASK does not require D_FG or D_FGBG
 */

#define	PROCNAME	tlSolidRects
#define	D_FG
#include	"tlprimitive.c"

#define	PROCNAME	tlSolidSpans
#define	D_SPANS
#include	"tlprimitive.c"

#define	PROCNAME	tlTiledRects
#define	D_TILE
#undef	D_FG
#undef	D_SPANS
#include	"tlprimitive.c"

#define	PROCNAME	tlTiledSpans
#define	D_SPANS
#include	"tlprimitive.c"

#define	PROCNAME	tlStipRects
#define	D_MASK
#define	D_FG
#undef	D_SPANS
#include	"tlprimitive.c"

#define	PROCNAME	tlStipSpans
#define	D_SPANS
#include	"tlprimitive.c"

#define	PROCNAME	tlOpStipRects
#undef	D_FG
#define	D_FGBG
#undef	D_SPANS
#include	"tlprimitive.c"

#define	PROCNAME	tlOpStipSpans
#define	D_SPANS
#include	"tlprimitive.c"
