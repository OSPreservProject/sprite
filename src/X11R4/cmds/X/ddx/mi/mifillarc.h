/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of MIT not be used in
advertising or publicity pertaining to distribution of the
software without specific prior written permission.
M.I.T. makes no representation about the suitability of
this software for any purpose. It is provided "as is"
without any express or implied warranty.

********************************************************/

/* $XConsortium: mifillarc.h,v 5.2 89/11/05 15:16:21 rws Exp $ */

#define FULLCIRCLE (360 * 64)

typedef struct _miFillArc {
    int xorg, yorg;
    int y;
    int dx, dy;
    int e, ex;
    int ym, yk, xm, xk;
} miFillArcRec;

#define miFillArcEmpty(arc) (!(arc)->angle2 || \
			     !(arc)->width || !(arc)->height || \
			     (((arc)->width == 1) && ((arc)->height & 1)))

#define miCanFillArc(arc) (((arc)->width == (arc)->height) || \
			   (((arc)->width <= 800) && ((arc)->height <= 800)))

extern void miFillArcSetup(), miFillArcSliceSetup();

#define MIFILLARCSETUP() \
    x = 0; \
    y = info.y; \
    e = info.e; \
    ex = info.ex; \
    xk = info.xk; \
    xm = info.xm; \
    yk = info.yk; \
    ym = info.ym; \
    dx = info.dx; \
    dy = info.dy; \
    xorg = info.xorg; \
    yorg = info.yorg

#define MIFILLCIRCSTEP(slw) \
    e += (y << 3) - xk; \
    while (e >= 0) \
    { \
	x++; \
	e += (ex = -((x << 3) + xk)); \
    } \
    y--; \
    slw = (x << 1) + dx; \
    if ((e == ex) && (slw > 1)) \
	slw--

#define MIFILLELLSTEP(slw) \
    e += (y * ym) - yk; \
    while (e >= 0) \
    { \
	x++; \
	e += (ex = -((x * xm) + xk)); \
    } \
    y--; \
    slw = (x << 1) + dx; \
    if ((e == ex) && (slw > 1)) \
	slw--

#define miFillArcLower(slw) (((y + dy) != 0) && ((slw > 1) || (e != ex)))

typedef struct _miSliceEdge {
    int	    x;
    int     stepx;
    int	    deltax;
    int	    e;
    int	    dy;
    int	    dx;
} miSliceEdgeRec, *miSliceEdgePtr;

typedef struct _miArcSlice {
    miSliceEdgeRec edge1, edge2;
    int min_top_y, max_top_y;
    int min_bot_y, max_bot_y;
    Bool edge1_top, edge2_top;
    Bool flip_top, flip_bot;
} miArcSliceRec;

#define MIARCSLICESTEP(edge) \
    edge.x -= edge.stepx; \
    edge.e -= edge.dx; \
    if (edge.e <= 0) \
    { \
	edge.x -= edge.deltax; \
	edge.e += edge.dy; \
    }

#define miFillSliceUpper(slice) \
		((y >= slice.min_top_y) && (y <= slice.max_top_y))

#define miFillSliceLower(slice) \
		((y >= slice.min_bot_y) && (y <= slice.max_bot_y))

#define MIARCSLICEUPPER(xl,xr,slice,slw) \
    xl = xorg - x; \
    xr = xl + slw - 1; \
    if (slice.edge1_top && (slice.edge1.x < xr)) \
	xr = slice.edge1.x; \
    if (slice.edge2_top && (slice.edge2.x > xl)) \
	xl = slice.edge2.x;

#define MIARCSLICELOWER(xl,xr,slice,slw) \
    xl = xorg - x; \
    xr = xl + slw - 1; \
    if (!slice.edge1_top && (slice.edge1.x > xl)) \
	xl = slice.edge1.x; \
    if (!slice.edge2_top && (slice.edge2.x < xr)) \
	xr = slice.edge2.x;
