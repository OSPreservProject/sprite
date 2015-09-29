/*****************************************************************************\
* 									      *
* 	File: sun1bw.c							      *
* 	Copyright (c) 1984 IBM						      *
* 	Date: Mon Mar 19 15:36:56 1984					      *
* 	Author: James Gosling						      *
* 									      *
* Window manager device driver for the SUN 1 black and white graphics board.  *
* 									      *
* HISTORY								      *
*	DSHR 16 July 1984 - Clipping of FillTrapezoid                         *
*		XXX - Need to propogate Bresenham error			      *
* 									      *
\*****************************************************************************/

#include "stdio.h"
/* #include <fcntl.h> */
#include "sys/ioctl.h"
#include "font.h"
#include "RasterFile.h"
#include "window.h"
#include "display.h"
#ifdef	notdef
#include "assert.h"
#else
#define	assert(X)
#endif

#define GXaddrRange	0x20000

/*
 * The low order 11 bits consist of the X or Y address times 2.
 * The lowest order bit is ignored, so word addressing works efficiently.
 */

# define GXselectX (0<<11)	/* the address is loaded into an X register */
# define GXselectx (0<<11)	/* the address is loaded into an X register */
# define GXselectY (1<<11)	/* the address is loaded into an Y register */
# define GXselecty (1<<11)	/* the address is loaded into an Y register */

/*
 * There are four sets of X and Y register pairs, selected by the following bits
 */

# define GXaddressSet0  (0<<12)
# define GXaddressSet1  (1<<12)
# define GXaddressSet2  (2<<12)
# define GXaddressSet3  (3<<12)
# define GXset0  (0<<12)
# define GXset1  (1<<12)
# define GXset2  (2<<12)
# define GXset3  (3<<12)

/*
 * The following bits indicate which registers are to be loaded
 */

# define GXnone    (0<<14)
# define GXothers  (1<<14)
# define GXsource  (2<<14)
# define GXmask    (3<<14)
# define GXpat     (3<<14)

# define GXupdate (1<<16)	/* actually update the frame buffer */


/*
 * These registers can appear on the left of an assignment statement.
 * Note they clobber X register 3.
 */

# define GXfunction	*(short *)(display.base + GXset3 + GXothers + (0<<1) )
# define GXwidth	*(short *)(display.base + GXset3 + GXothers + (1<<1) )
# define GXcontrol	*(short *)(display.base + GXset3 + GXothers + (2<<1) )
# define GXintClear	*(short *)(display.base + GXset3 + GXothers + (3<<1) )

# define GXsetMask	*(short *)(display.base + GXset3 + GXmask )
# define GXsetSource	*(short *)(display.base + GXset3 + GXsource )
# define GXpattern	*(short *)(display.base + GXset3 + GXpat )

/*
 * The following bits are written into the GX control register.
 * It is reset to zero on hardware reset and power-up.
 * The high order three bits determine the Interrupt level (0-7)
 */

# define GXintEnable   (1<<8)
# define GXvideoEnable (1<<9)
# define GXintLevel0	(0<<13)
# define GXintLevel1	(1<<13)
# define GXintLevel2	(2<<13)
# define GXintLevel3	(3<<13)
# define GXintLevel4	(4<<13)
# define GXintLevel5	(5<<13)
# define GXintLevel6	(6<<13)
# define GXintLevel7	(7<<13)

/*
 * The following are "function" encodings loaded into the function register
 */

# define GXnoop			0xAAAA
# define GXinvert		0x5555
# define GXcopy        		0xCCCC
# define GXcopyInverted 	0x3333
# define GXclear		0x0000
# define GXset			0xFFFF
# define GXpaint		0xEEEE
# define GXpaintInverted 	0x2222
# define GXxor			0x6666

/*
 * The following permit functions to be expressed as Boolean combinations
 * of the three primitive functions 'source', 'mask', and 'dest'.  Thus
 * GXpaint is equal to GXSOURCE|GXDEST, while GXxor is GXSOURCE^GXDEST.
 */

# define GXSOURCE		0xCCCC
# define GXMASK			0xF0F0
# define GXDEST			0xAAAA


/*
 * These may appear in statement contexts to just
 * set the X and Y registers of set number zero to the given values.
 */

# define GXsetX(X)	*(short *)(display.base + GXselectX + (X<<1)) = 1;
# define GXsetY(Y)	*(short *)(display.base + GXselectY + (Y<<1)) = 1;

/* Ultra tense line drawing routines.		Vaughn Pratt */

#define MOV(reg,sign) (0x3000|reg|sign)  /* builds a movw instruction */
#define RX 0xA00	/* a5 holds rx address */
#define RY 0x800	/* a4 holds ry address */
#define POS 0xc0	/* an@+ scans positively */
#define NEG 0x100	/* an@- scans negatively */

extern bres();

static
unsigned short mincmd[8] = {
	MOV(RY,POS), MOV(RX,POS),
	MOV(RY,POS), MOV(RX,NEG),
	MOV(RY,NEG), MOV(RX,POS),
	MOV(RY,NEG), MOV(RX,NEG)
};

static
unsigned short majcmd[8] = {
	MOV(RX,POS), MOV(RY,POS),
	MOV(RX,NEG), MOV(RY,POS),
	MOV(RX,POS), MOV(RY,NEG),
	MOV(RX,NEG), MOV(RY,NEG)
};

static
BWvector(x0,y0,x1,y1)
int x0,y0,x1,y1;
{
    {
	register struct ViewPort   *v = CurrentViewPort;
	if (v) {

	/* The classic clipping algorithm */

#define TOP	001
#define BOTTOM	002
#define LEFT	004
#define RIGHT	010
#define crossings(x,y)	  (x<0 ? LEFT : x>=v->width ? RIGHT : 0) \
	    +(y < 0 ? TOP : y >= v -> height ? BOTTOM : 0)
		register    Cross0 = crossings (x0, y0);
	    register    Cross1 = crossings (x1, y1);
	    while (Cross0 || Cross1) {
		register    Cross,
		            x,
		            y;
		if (Cross0 & Cross1)
		    return;
		if ((Cross = Cross0) == 0)
		    Cross = Cross1;
		if (Cross & (LEFT | RIGHT)) {
		    register    edge = (Cross & LEFT) ? 0 : v -> width - 1;
		    y = y0 + (y1 - y0) * (edge - x0) / (x1 - x0);
		    x = edge;
		}
		else
		    if (Cross & (TOP | BOTTOM)) {
			register    edge = (Cross & TOP) ? 0 : v -> height - 1;
			x = x0 + (x1 - x0) * (edge - y0) / (y1 - y0);
			y = edge;
		    }
		if (Cross == Cross0) {
		    x0 = x;
		    y0 = y;
		    Cross0 = crossings (x, y);
		}
		else {
		    x1 = x;
		    y1 = y;
		    Cross1 = crossings (x, y);
		}
	    }
	    x0 += v -> left;
	    y0 += v -> top;
	    x1 += v -> left;
	    y1 += v -> top;
	}
    }
    {
	register    x,
	            y,
	            q = 0,
	            r;		/* d7,d6,d5,d4 */
	register unsigned   t = 0;/* d3 */
	register unsigned short *rx,
	                       *ry;/* a5,a4 */

	GXwidth = 1;

	x = x1 - x0;		/* x,y relative to x0,y0 */
	y = y1 - y0;

	GXsetX (x0);
	GXsetY (y0);
	rx = (unsigned short *) (display.base + GXsource + GXselectx) + x0;
	ry = (unsigned short *) (display.base + GXsource + GXselecty) + y0;


	if (x < 0) {
	    x = -x;
	    rx++;		/* since an@- predecrements */
	    q += 2;
	}
	if (y < 0) {
	    y = -y;
	    ry++;		/* since an@- predecrements */
	    q += 4;
	}
	if (x < y) {
	    ry += GXupdate / 2;
	    t = x;
	    x = y;
	    y = t;
	    q += 1;
	}
	else
	    rx += GXupdate / 2;
	((unsigned short *) bres)[0] = mincmd[q];
	((unsigned short *) bres)[1] = majcmd[q];
	q = x;
	r = -(x >> 1);
	bres ();		/* call Bresenham */
	return 0;
    }
}

/*
 *  These rasterop routines support the transfer of rasters between
 *  memory and framebuffer, memory and memory, and framebuffer and
 *  framebuffer.
 */

#define	XS	((short *)(display.base+GXset1+GXselectX))
#define	YS	((short *)(display.base+GXsource+GXset1+GXselecty))
#define	XD	((short *)(display.base+GXselectX))
#define	YD	((short *)(display.base+GXsource+GXupdate+GXselectY))


/*------------------------------------------------------------------*/
#define	touch(p)	((p)=0)
#define	loop(s)\
for(j=height;j>15;j-=16){\
	s;s;s;s;\
	s;s;s;s;\
	s;s;s;s;\
	s;s;s;s;\
}\
switch(j){\
case 15: s; case 14: s; case 13: s; case 12: s;\
case 11: s; case 10: s; case 9:  s; case 8:  s;\
case 7:  s; case 6:  s; case 5:  s; case 4:  s;\
case 3:  s; case 2:  s; case 1:  s;\
}



/*------------------------------------------------------------------*/
static
BWRasterOp (xs, ys, xd, yd, width, height) {
    if (width<0) {
	xs += width;
	xd += width;
	width = -width;
    }
    if (height<0) {
	ys += height;
	yd += height;
	height = -height;
    }
    {
	register struct ViewPort   *v = CurrentViewPort;
	if (v) {
	    register    t;
	    if (xs < 0) {
		width += xs;
		xd -= xs;
		xs = 0;
	    }
	    if (ys < 0) {
		height += ys;
		yd -= ys;
		ys = 0;
	    }
	    if (xd < 0) {
		width += xd;
		xs -= xd;
		xd = 0;
	    }
	    if (yd < 0) {
		height += yd;
		ys -= yd;
		yd = 0;
	    }
	    if ((t = v -> width - xs - width) < 0)
		width += t;
	    if ((t = v -> width - xd - width) < 0)
		width += t;
	    if ((t = v -> height - ys - height) < 0)
		height += t;
	    if ((t = v -> height - yd - height) < 0)
		height += t;
	    xs += v -> left;
	    xd += v -> left;
	    ys += v -> top;
	    yd += v -> top;
	}
    }
    if (width>0 && height>0)
    if (xs >= xd)		/* Moving right */
	if (ys >= yd) {		/* Moving up */
	    register short *yda = YD + yd,
	                   *xda = XD + xd;
	    register short *ysa = YS + ys,
	                   *xsa = XS + xs;
	    register short  i,
	                    j;
	    for (i = width; i > 0; i -= 16) {
		if (i >= 16)
		    GXwidth = 16;
		else
		    GXwidth = i;
		touch (*xda);
		touch (*xsa);
		j = *ysa++;	/* prime the pump */
		loop (*yda++ = *ysa++);
		yda -= height;
		xda += 16;
		ysa -= height + 1;/* 1 extra to prime the pump with */
		xsa += 16;
	    }
	}
	else {			/* right and down */
	    register short *yda = YD + yd + height,
	                   *xda = XD + xd;
	    register short *ysa = YS + ys + height,
	                   *xsa = XS + xs;
	    register short  i,
	                    j;
	    for (i = width; i > 0; i -= 16) {
		if (i >= 16)
		    GXwidth = 16;
		else
		    GXwidth = i;
		touch (*xda);
		touch (*xsa);
		j = *--ysa;	/* prime the pump */
		loop (*--yda = *--ysa);
		yda += height;
		xda += 16;
		ysa += height + 1;/* 1 extra to prime the pump with */
		xsa += 16;
	    }
	}
    else			/* left */
	if (ys >= yd) {		/* Moving left and up */
	    register short *yda = YD + yd,
	                   *xda = XD + xd + width;
	    register short *ysa = YS + ys,
	                   *xsa = XS + xs + width;
	    register short  i,
	                    j;
	    for (i = width; i > 0; i -= 16) {
		if (i >= 16) {
		    xda -= 16;
		    xsa -= 16;
		    GXwidth = 16;
		}
		else {
		    xda -= i;
		    xsa -= i;
		    GXwidth = i;
		}
		touch (*xda);
		touch (*xsa);
		j = *ysa++;	/* prime the pump */
		loop (*yda++ = *ysa++);
		yda -= height;
		ysa -= height + 1;/* 1 extra to prime the pump with */
	    }
	}
	else {			/* left and down */
	    register short *yda = YD + yd + height,
	                   *xda = XD + xd + width;
	    register short *ysa = YS + ys + height,
	                   *xsa = XS + xs + width;
	    register short  i,
	                    j;
	    for (i = width; i > 0; i -= 16) {
		if (i >= 16) {
		    xda -= 16;
		    xsa -= 16;
		    GXwidth = 16;
		}
		else {
		    xda -= i;
		    xsa -= i;
		    GXwidth = i;
		}
		touch (*xda);
		touch (*xsa);
		j = *--ysa;	/* prime the pump */
		loop (*--yda = *--ysa);
		yda += height;
		ysa += height + 1;/* 1 extra to prime the pump with */
	    }
	}
}

/*------------------------------------------------------------------*/
/*
 * operate don't-care-to-screen
 */
static
BWRasterSmash (xd, yd, width, height) {
    if (width<0) {
	xd += width;
	width = -width;
    }
    if (height<0) {
	yd += height;
	height = -height;
    }
    {
	register struct ViewPort   *v = CurrentViewPort;
	if (v) {
	    register    t;
	    if (xd < 0) {
		width += xd;
		xd = 0;
	    }
	    if (yd < 0) {
		height += yd;
		yd = 0;
	    }
	    if ((t = v -> width - xd - width) < 0)
		width += t;
	    if ((t = v -> height - yd - height) < 0)
		height += t;
	    xd += v -> left;
	    yd += v -> top;
	}
    }
    {
	register short *yda = YD + yd,
	               *xda = XD + xd;
	register short  i,
	                j;
	for (i = width; i > 0; i -= 16) {
	    if (i >= 16)
		GXwidth = 16;
	    else
		GXwidth = i;
	    touch (*xda);
	    loop (touch (*yda++));
	    yda -= height;
	    xda += 16;
	}
    }
}


static
BWFillTrapezoid  (x1, y1, w1, x2, y2, w2, c)
struct icon *c; {
    register    dy;
    int     dxl,
            dxr,
            fx;
    int     el,
            er;
    int     xl,
            xr;
    int     width,
            height;
    int     row;
    short  *bits;
    if (c -> OffsetToSpecific) {
	register struct BitmapIconSpecificPart *s =
	                                        (struct BitmapIconSpecificPart *) (((int) c) + c -> OffsetToSpecific);
	switch (s -> type) {
	    case BitmapIcon: {
		    bits = (short *) s -> bits;
		    height = s -> rows;
		    width = s -> cols;
		    if (width == 0 || height == 0)
			return;
		}
		break;
	    default: 
		return;
	}
    }
    else
	return;
    if (w1 < 0)
	x1 += w1, w1 = -w1;
    if (w2 < 0)
	x2 += w2, w2 = -w2;
    if (y1 > y2) {
	register    t;
	t = x1;
	x1 = x2;
	x2 = t;
	t = y1;
	y1 = y2;
	y2 = t;
	t = w1;
	w1 = w2;
	w2 = t;
    }

    {
	register struct ViewPort   *v = CurrentViewPort;
	if (v) {
#define	XIntersect(x1, y1, x2, y2, xt) \
((y1) + ((y2) - (y1)) * ((xt) - (x1)) / ((x2) - (x1)))
#define YIntersect(x1, y1, x2, y2, yt) \
((x1) + ((x2) - (x1)) * ((yt) - (y1)) / ((y2) - (y1)))
#define X0Intersect(x1, y1, x2, y2) \
((y1) - (x1) * ((y2) - (y1)) / ((x2) - (x1)))
	/* Gross Y box clip */
	if ((y1 >= v -> height /* && y2 >= v->height */ )
	    || ( /* y1 < 0 && */ y2 < 0)) {
		/*  Nothing visible */
		return;
	}
	/* Reject zero-area fills */
	if (w1 == 0 && w2 == 0)
		return;
	if (y1 == y2)
		return;
	/* Clip against top/bottom of window */
	if (y1 < 0 /* && y2 > 0 */ ) {
		int     x3, x4;

		x3 = YIntersect (x1, y1, x2, y2, 0);
		x4 = YIntersect (x1 + w1, y1, x2 + w2, y2, 0);
		x1 = x3;
		y1 = 0;
		w1 = x4 - x3;
	} 
	if (y2 >= v -> height /* && y1 <= v->height */) {
		int     x3,
		x4;
		x3 = YIntersect (x1, y1, x2, y2, v -> height-1);
		x4 = YIntersect (x1 + w1, y1, x2 + w2, y2, v -> height-1);
		x2 = x3;
		y2 = v -> height-1;
		w2 = x4 - x3;
	}
	assert (y1 >= 0 && y2 < v -> height);
	/* Gross X box clip */
	if ((x1 >= v -> width && x2 >= v -> width) || (x1 + w1 < 0 && x2 + w2 < 0))
		/*  Nothing visible */
		return;
	/* Clip against left of window */
	if (x1 + w1 < 0 && x2 + w2 >= 0) {
		/*  Lower part of right edge inside */
		int     y3;

		y3 = X0Intersect (x1 + w1, y1, x2 + w2, y2);
		assert ((y3 >= y1 && y3 <= y2));
		if (x2 > 0) {
			/* Left edge crosses too */
			int     y4,
			w4;
			y4 = X0Intersect (x1, y1, x2, y2);
			assert ((y4 >= y3 && y4 <= y2));  /* XXX - failed */
			w4 = YIntersect (x1 + w1, y1, x2 + w2, y2, y4);
			BWFillTrapezoid (0, y4, w4, x2, y2, w2, c);
			y1 = y3;
			x1 = w1 = 0;
			y2 = y4;
			x2 = 0;
			w2 = w4;
		} 
		else {
			y1 = y3;
			x1 = w1 = 0;
			w2 += x2;
			x2 = 0;
		}
	} 
	else if (x1 + w1 >= 0 && x2 + w2 < 0) {
		/*  Upper part of right edge inside */
		int     y3;
		y3 = X0Intersect (x1 + w1, y1, x2 + w2, y2);
		assert ((y3 >= y1 && y3 <= y2));
		if (x1 > 0) {
			/* Left edge crosses too */
			int     y4,
			w4;
			y4 = X0Intersect (x1, y1, x2, y2);
			assert ((y4 >= y1 && y4 <= y3));
			w4 = YIntersect (x1 + w1, y1, x2 + w2, y2, y4);
			BWFillTrapezoid (x1, y1, w1, 0, y4, w4, c);
			y1 = y4;
			x1 = 0;
			w1 = w4;
			y2 = y3;
			x2 = 0;
			w2 = 0;
		} 
		else {
			w1 += x1;
			x1 = 0;
			x2 = w2 = 0;
			y2 = y3;
		}
	}
	/* assert ((x1+w1)>=0 && (x2+w2)>=0) */
	else if (x1 < 0 && x2 < 0) {
		/*  Left of window cuts top and bottom edges */
		w1 += x1;
		x1 = 0;
		w2 += x2;
		x2 = 0;
	} 
	else if (x1 < 0 /* && x2 >= 0 */ ) {
		/*  Lower part of left edge inside */
		int     y3,
		w3;
		assert (x2 >= 0);
		y3 = X0Intersect (x1, y1, x2, y2);
		assert ((y3 >= y1 && y3 <= y2));
		w3 = YIntersect (x1 + w1, y1, x2 + w2, y2, y3);
		BWFillTrapezoid (0, y1, w1 + x1, 0, y3, w3, c);
		x1 = 0;
		y1 = y3;
		w1 = w3;
	} 
	else if (x2 < 0 /* && x1 >= 0 */ ) {
		/*  Upper part of left edge inside */
		int     y3,
		w3;
		assert (x1 >= 0);
		y3 = X0Intersect (x1, y1, x2, y2);
		assert ((y3 >= y1 && y3 <= y2));
		w3 = YIntersect (x1 + w1, y1, x2 + w2, y2, y3);
		BWFillTrapezoid (0, y3, w3, 0, y2, w2 + x2, c);
		x2 = 0;
		y2 = y3;
		w2 = w3;
	} /* else x1 >= 0 && x2 >= 0 and thus no clip */

	/* Clip against right edge of window */
	if (x1 >= v -> width && x2 < v -> width) {
		/* Lower part of left edge inside */
		int     y3;
		y3 = XIntersect (x1, y1, x2, y2, v -> width-1);
		assert ((y3 >= y1 && y3 <= y2));
		if ((x2 + w2) < v -> width) {
			/*  Right edge crosses too */
			int     y4,
			x4;
			y4 = XIntersect (x1 + w1, y1, x2 + w2, y2, v -> width-1);
			assert ((y4 >= y3) && (y4 <= y2));
			x4 = YIntersect (x1, y1, x2, y2, y4);
			BWFillTrapezoid (x4, y4, v -> width - x4, x2, y2, w2, c);
			x1 = v -> width - 1;
			y1 = y3;
			w1 = 0;
			x2 = x4;
			y2 = y4;
			w2 = v -> width - x4;
		} 
		else {
			w2 = v->width - x2;
			x1 = v->width - 1;
			y1 = y3;
			w1 = 0;
		}
	} 
	else if (x1 < v -> width && x2 >= v -> width) {
		/* Upper part of left edge inside */
		int     y3;
		y3 = XIntersect (x1, y1, x2, y2, v -> width - 1);
		assert ((y3 >= y1 && y3 <= y2));
		if ((x1 + w1) < (v -> width - 1)) {
			/*  Right edge crosses too */
			int     y4,
			x4;
			y4 = XIntersect (x1 + w1, y1, x2 + w2, y2, v -> width - 1);
			assert ((y4 >= y1) && (y4 <= y3));
			x4 = YIntersect (x1, y1, x2, y2, y4);
			BWFillTrapezoid (x1, y1, w1, x4, y4,  v -> width - x4, c);
			x1 = x4;
			y1 = y4;
			w1 = v -> width - x4;
			x2 = v -> width - 1;
			y2 = y3;
			w2 = 0;
		} 
		else {
			w1 = v->width - x1;
			x2 = v -> width - 1;
			y2 = y3;
			w2 = 0;
		}
	}
	/* assert(x1 < v->width && x2 < v->width) */
	else if ((x1 + w1) >= v->width && (x2 + w2) >= v->width) {
		/* Right of window cuts top & bottom edge */
		w1 = v -> width - x1;
		w2 = v -> width - x2;
	} 
	else if ((x1 + w1) >= v -> width && (x2+w2) < v->width) {
		/* Lower part of right edge inside */
		int     y3,
		w3;
		assert ((x2 + w2) < v -> width);
		y3 = XIntersect (x1 + w1, y1, x2 + w2, y2, v -> width - 1);
		assert ((y3 >= y1) && (y3 <= y2));
		w3 = YIntersect (x1, y1, x2, y2, y3);
		BWFillTrapezoid (x1, y1, v -> width - x1, w3, y3, v -> width - w3, c);
		x1 = w3;
		y1 = y3;
		w1 = v -> width - w3;
	} 
	else if ((x2 + w2) >= v -> width && (x1+w1) < v->width) {
		/* Upper part of right edge inside */
		int     y3,
		w3;
		assert ((x1 + w1) < v -> width);
		y3 = XIntersect (x1 + w1, y1, x2 + w2, y2, v -> width - 1);
		assert ((y3 >= y1) && (y3 <= y2));
		w3 = YIntersect (x1, y1, x2, y2, y3);
		BWFillTrapezoid (w3, y3, v -> width - w3, x2, y2, v -> width - x2, c);
		x2 = w3;
		y2 = y3;
		w2 = v -> width - w3;
	} /* else (x1+w1) < v->width && (x2+w2) < v->width and thus no clip */

	assert ((x1 >= 0 && y1 >= 0 && (x1 + w1) <= v -> width && y1 < v -> height));
	assert ((x2 >= 0 && y2 >= 0 && (x2 + w2) <= v -> width && y2 < v -> height));
#undef	YIntersect
#undef	XIntersect
#undef	X0Intersect
	    xr = (xl = x1) + w1;
	    xl += v -> left;
	    y1 += v -> top;
	    xr += v -> left;
	    y2 += v -> top;
	} else {
	    xr = (xl = x1) + w1;
	}
    }
    dxl = x2 - x1;
    dy = y2 - y1;
    dxr = (x2 + w2) - (x1 + w1);
#define	TXD	((short *)(display.base+GXsource+GXupdate+GXselectX))
#define	TYD	((short *)(display.base+GXselectY))
    el = er = 0;
    row = y1 % height;
    fx = (xl + width - 1) / width * width;
    if (dxl == 0 && w1 == w2) {
	register    skew = fx - xl;
	if (skew == 0)
	    skew = 16, fx += 16;
	if (xl <= xr)
	    if (fx >= xr) {
		register short *xda = TXD + xl;;
		GXwidth = xr - xl;
		skew = 16 - skew;
		while (1) {
		    register    w = bits[row];
		    int     x;
		    touch (*(TYD + y1));
		    *xda = w << skew;
		    if (y1 >= y2)
			break;
		    y1++;
		    row++;
		    if (row >= height)
			row = 0;
		}
	    }
	    else {
		register short *xlim = TXD + (xr - width);
		register    twidth;
		if ((twidth = xr - fx) > 0)
		if (width == 16)
		    twidth &= 017;
		else
		    twidth %= width;
		else twidth = 0;
		while (1) {
		    register short *xda;
		    register    w = bits[row];
		    int     x;
		    touch (*(TYD + y1));
		    xda = TXD + xl;
		    GXwidth = skew;
		    *xda = w << (16 - skew);
		    xda += fx - xl;
		    w <<= 16 - width;
		    GXwidth = width;
		    while (xda <= xlim) {
			*xda = w;
			xda += width;
		    }
		    if (twidth > 0) {
			GXwidth = twidth;
			*xda = w;
		    }
		    if (y1 >= y2)
			break;
		    y1++;
		    row++;
		    if (row >= height)
			row = 0;
		}
	    }
    }
    else {	/* Not a rectangle */
	while (1) {
	    register short *xda;
	    register    w = bits[row];
	    int     x;

	    /* Eliminate zero-width ROPs */
	    if (xl == xr)
	        goto dodda;
	    touch (*(TYD + y1));
	    xda = TXD + xl;
	    if (fx >= xr) {
		register    skew;
		if (skew = fx - xl) {
		    GXwidth = xr - xl;
		    *xda = w << (16 - skew);
		}
	    }
	    else {
		register short *xlim;
		{
		    register    skew;
		    if (skew = fx - xl) {
			GXwidth = skew;
			*xda = w << (16 - skew);
			xda += fx - xl;
		    }
		}
		xlim = TXD + (xr - width);
		w <<= 16 - width;
		GXwidth = width;
		while (xda <= xlim) {
		    *xda = w;
		    xda += width;
		}
		{
		    register    t = (xlim - xda) + width;
		    if (t > 0) {
			GXwidth = t;
			*xda = w;
		    }
		}
	    }

dodda:	    if (y1 >= y2)
		break;
	    el += dxl;
	    while (el >= dy)
		el -= dy, xl++;
	    while (-el >= dy)
		el += dy, xl--;
	    while (xl > fx)
		fx += width;
	    while (xl <= fx - width)
		fx -= width;
	    er += dxr;
	    while (er >= dy)
		er -= dy, xr++;
	    while (-er >= dy)
		er += dy, xr--;
	    y1++;
	    row++;
	    if (row >= height)
		row = 0;
	}
    }
}


/*------------------------------------------------------------------*/
/*
 * operate memory-to-screen
 */
static
BWCopyMemoryToScreen (xs, ys, rs, xd, yd, width, height)
struct raster *rs;
{
    {
	register struct ViewPort   *v = CurrentViewPort;
	if (v) {
	    register    t;
	    if (xd < 0) {
		width += xd;
		xs -= xd;
		xd = 0;
	    }
	    if (yd < 0) {
		height += yd;
		ys -= yd;
		yd = 0;
	    }
	    if ((t = v -> width - xd - width) < 0)
		width += t;
	    if ((t = v -> height - yd - height) < 0)
		height += t;
	    xd += v -> left;
	    yd += v -> top;
	    if (width<=0 || height<=0) return;
	}
    }
    {
	register short *yda = YD + yd,
	               *xda = XD + xd;
	register short *sa = rs -> bits + xs / 16 * rs -> height + ys;
	register short  i,
	                j,
	                skew;
	if ((skew = xs & 15) != 0) {
	    if (width < 16 - skew) {
		GXwidth = width;
		width = 0;
	    }
	    else {
		GXwidth = 16 - skew;
		width -= 16 - skew;
	    }
	    touch (*xda);
	/* 
	 * shifting can likely be avoided by creative use
	 * of the pattern register
	 */
	    loop (*yda++ = *sa++ << skew);
	    yda -= height;
	    xda += 16 - skew;
	    sa += rs -> height - height;
	}
	for (i = width; i > 0; i -= 16) {
	    if (i >= 16)
		GXwidth = 16;
	    else
		GXwidth = i;
	    touch (*xda);
	    loop (*yda++ = *sa++);
	    yda -= height;
	    xda += 16;
	    sa += rs -> height - height;
	}
    }
}


/*------------------------------------------------------------------*/
/*
 * copy screen-to-memory
 */
#define	mask(nbit)	((1<<(nbit))-1)	/* the n rightmost bits of a word */
static
BWCopyScreenToMemory (xs, ys, xd, yd, rd, width, height)
struct raster *rd;
{
    {
	register struct ViewPort   *v = CurrentViewPort;
	if (v) {
	    register    t;
	    if (xs < 0) {
		width += xs;
		xd -= xs;
		xs = 0;
	    }
	    if (ys < 0) {
		height += ys;
		yd -= ys;
		ys = 0;
	    }
	    if ((t = v -> width - xs - width) < 0)
		width += t;
	    if ((t = v -> height - ys - height) < 0)
		height += t;
	    xs += v -> left;
	    ys += v -> top;
	}
    }
    {
	register short *da = rd -> bits + (xd >> 4) * rd -> height + yd;
	register short *ysa = YS + ys,
	               *xsa;
	register short  i,
	                j,
	                sm,
	                dm,
	                skew;
	GXwidth = 16;
	if ((skew = xd & 15) != 0) {
	    sm = mask (16 - skew);
	    if (width < 16 - skew) {
		sm &= ~mask (16 - skew - width);
		width = 0;
	    }
	    else
		width -= 16 - skew;
	    dm = ~sm;
	    xsa = XS + ((xs - skew) & 1023);
	    touch (*xsa);
	    j = *ysa++;		/* prime the pump */
	    loop (*da = (*ysa++ & sm) | (*da & dm);
		  da++);
	    da += rd -> height - height;
	    ysa -= height + 1;	/* 1 extra to prime the pump with */
	    xsa = XS + xs + 16 - skew;
	}
	else
	    xsa = XS + xs;
	for (i = width; i > 15; i -= 16) {
	    touch (*xsa);
	    j = *ysa++;		/* prime the pump */
	    loop (*da++ = *ysa++);
	    da += rd -> height - height;
	    ysa -= height + 1;	/* 1 extra to prime the pump with */
	    xsa += 16;
	}
	if (i != 0) {
	    dm = mask (16 - i);
	    sm = ~dm;
	    touch (*xsa);
	    j = *ysa++;		/* prime the pump */
	    loop (*da = (*ysa++ & sm) | (*da & dm);
	    da++);
	}
    }
}


/*------------------------------------------------------------------*/
/*
 * operate don't cares to memory, operation is GXinvert
 */
static
BWROPdminv(xd, yd, rd, width, height)
struct raster *rd;
{
    register short *da = rd -> bits + xd / 16 * rd -> height + yd;
    register short  i,
                    j,
                    sm,
                    skew;
    if ((skew = xd & 15) != 0) {
	sm = mask (16 - skew);
	if (width < 16 - skew) {
	    sm &= ~mask (16 - skew - width);
	    width = 0;
	}
	else
	    width -= 16 - skew;
	loop (*da++ ^= sm);
	da += rd -> height - height;
    }
    for (i = width; i > 15; i -= 16) {
	loop (*da++ ^= -1);
	da += rd -> height - height;
    }
    if (i != 0) {
	sm =~ mask (16 - i);
	loop (*da++ ^= sm);
    }
}


/*------------------------------------------------------------------*/
/*
 * operate don't cares to memory, operation is GXset
 */
static
BWROPdmset(xd, yd, rd, width, height)
struct raster *rd;
{
    register short *da = rd -> bits + xd / 16 * rd -> height + yd;
    register short  i,
                    j,
                    sm,
                    skew;
    if ((skew = xd & 15) != 0) {
	sm = mask (16 - skew);
	if (width < 16 - skew) {
	    sm &= ~mask (16 - skew - width);
	    width = 0;
	}
	else
	    width -= 16 - skew;
	loop (*da++ |= sm);
	da += rd -> height - height;
    }
    for (i = width; i > 15; i -= 16) {
	loop (*da++ = -1);
	da += rd -> height - height;
    }
    if (i != 0) {
	sm =~ mask (16 - i);
	loop (*da++ |= sm);
    }
}

/*------------------------------------------------------------------*/
/*
 * operate don't cares to memory, operation is GXclear
 */
static
BWROPdmclr(xd, yd, rd, width, height)
struct raster *rd;
{
	register short *da=rd->bits+xd/16*rd->height+yd;
	register short i, j, sm, skew;
	if((skew=xd&15)!=0){
		sm=mask(16-skew);
		if(width<16-skew){
			sm &= ~mask(16-skew-width);
			width=0;
		}
		else
			width-=16-skew;
		sm = ~sm;
		loop(*da++ &= sm);
		da += rd->height-height;
	}
	for(i=width;i>15;i-=16){
		loop(*da++=0);
		da += rd->height-height;
	}
	if(i!=0){
		sm=mask(16-i);
		loop(*da++&=sm);
	}
}

/* Draw a string using fancy font tables */

#define short_loop(j, s)\
switch (j) { \
    case 27: s; case 26: s; case 25: s; case 24: s; \
    case 23: s; case 22: s; case 21: s; case 20: s; \
    case 19: s; case 18: s; case 17: s; case 16: s; \
    case 15: s; case 14: s; case 13: s; case 12: s; \
    case 11: s; case 10: s; case 9: s; case 8: s; \
    case 7: s; case 6: s; case 5: s; case 4: s; \
    case 3: s; case 2: s; case 1: s; \
}


/************************************************************************\
* 									 *
*  This routine is optimized for simple horizontal strings.		 *
*   (x,y)		is the upper left corner of the first character. *
*   font		is the font to be used for drawing the string.	 *
*   s		is the string.						 *
*   n		is the number of characters in the string.		 *
*   CharShim	is the amount by which to increase every character's	 *
* 		width.  (including space)				 *
*   SpaceShim	is the amount by which to increase the width of space	 *
* 		character.						 *
* 									 *
\************************************************************************/
static
BWDrawString (x, y, font, s, n, CharShim, SpaceShim)
register    x,
            y;
register struct font   *font;
unsigned char   *s;
{
    struct ViewPort *v;
    int     MaxX;
/*  debug (("%c%c.. n=%d; CharShim=%d\n", s[0], s[1], n, CharShim)); */
    v = CurrentViewPort;
    if (font -> type == BitmapIcon && font -> fn.rotation == 0) {
	while (x < font -> NWtoOrigin.x && *s) {
	    register struct icon   *c = &font -> chars[*s];
	    if (x + font -> WtoE.x >= 0 && c -> OffsetToSpecific) {
	register struct BitmapIconSpecificPart   *s =
		(struct BitmapIconSpecificPart   *) (((int) c) + c -> OffsetToSpecific);
		struct raster   r;
		r.bits = (short *) s -> bits;
		r.height = s -> rows;
		r.width = s -> cols;
		BWCopyMemoryToScreen (0, 0, &r, x - s -> ocol, y - s -> orow, s -> cols, s -> rows);
	    }
	    if (c -> OffsetToSpecific) {
		register struct IconGenericPart *gp = (struct IconGenericPart  *) (((int) c) + c -> OffsetToGeneric);
		x += gp -> Spacing.x + CharShim;
		if (*s == ' ')
		    x += SpaceShim;
	    }
	    s++;
	    if (--n == 0)
		goto out;
	}
	MaxX = v -> width;
	if (y >= font -> NWtoOrigin.y && y - font -> NWtoOrigin.y + font -> NtoS.y <= v -> height) {
	    if (font -> WtoE.x > 16 || font -> NtoS.y > 27)
		while (*s != '\0') {
		    struct icon *c = &font -> chars[*s];
		    if (c -> OffsetToSpecific) {
			struct IconGenericPart *gp = (struct IconGenericPart   *) (((int) c) + c -> OffsetToGeneric);
			struct BitmapIconSpecificPart  *sp = (struct BitmapIconSpecificPart *) (((int) c) + c -> OffsetToSpecific);
			register    height = sp -> rows;
			register    i = sp -> cols,
			            j;
			if (x + i <= MaxX) {
			    register short *yda = YD + (y - sp -> orow + v -> top),
			                   *xda = XD + (x + v -> left - sp -> ocol);
			    register short *sa = (short *) sp -> bits;
			    for (; i > 0; i -= 16) {
				if (i >= 16)
				    GXwidth = 16;
				else
				    GXwidth = i;
				touch (*xda);
				loop (*yda++ = *sa++);
				yda -= height;
				xda += 16;
			    }
			} else goto ClipOut;
			if (*s == ' ')
			    x += SpaceShim;
			x += gp -> Spacing.x + CharShim;
		    }
		    if (--n == 0)
			goto out;
		    s++;
		}
	    else
		while (*s != '\0') {
		    struct icon *c = &font -> chars[*s];
		    if (c -> OffsetToSpecific) {
			struct IconGenericPart *gp = (struct IconGenericPart   *)
			                                                        (((int) c) + c -> OffsetToGeneric);
			register
			struct BitmapIconSpecificPart  *sp = (struct BitmapIconSpecificPart *)
			                                                                    (((int) c) + c -> OffsetToSpecific);
			register    i = sp -> cols;
			if (x + i <= MaxX) {
			    register short *yda = YD + (y - sp -> orow + v -> top);
			    register short *sa = (short *) sp -> bits;
			    GXwidth = i;
			    touch (*(XD + (x + v -> left - sp -> ocol)));
			    short_loop (sp -> rows, *yda++ = *sa++);
			} else goto ClipOut;
			if (*s == ' ')
			    x += SpaceShim;
			x += gp -> Spacing.x + CharShim;
		    }
		    if (--n == 0)
			goto out;
		    s++;
		}
		goto out;
	}
    }
    ClipOut:
	while (*s) {		/* Bug: doesnt handle space and char shims
				   */
	    register struct icon   *c = &font -> chars[*s++];
	    if (c -> OffsetToGeneric) {
		register struct IconGenericPart *g =
		                                (struct IconGenericPart *) (((int) c) + c -> OffsetToGeneric);
		BWDrawIcon (x, y, c);
		x += g -> Spacing.x;
		y += g -> Spacing.y;
	    }
	    if (--n == 0)
		goto out;
	}
out: 
    LastY = y;
    return LastX = x;
}

static
BWDrawIcon (x, y, c)
register struct icon   *c; {
    if (c -> OffsetToSpecific) {
	register struct BitmapIconSpecificPart   *s =
		(struct BitmapIconSpecificPart   *) (((int) c) + c -> OffsetToSpecific);
	switch (s -> type) {
	    case BitmapIcon: {
		    struct raster   r;
		    r.bits = (short *) s -> bits;
		    r.height = s -> rows;
		    r.width = s -> cols;
		    BWCopyMemoryToScreen (0, 0, &r, x - s -> ocol, y - s -> orow, s -> cols, s -> rows);
		}
	}
    }
}

static
BWSizeofRaster (w, h) {
    return (((w+15)>>4)*h)<<1;
}

static
BWSelectColor (c)
unsigned short  c; {
    switch (c) {
	case f_invert | f_CharacterContext: 
	    GXfunction = GXSOURCE ^ GXDEST;
	    break;
	case f_invert: 
	case GXinvert: 
	    GXfunction = ~GXDEST;
	    break;
	case f_copy: 
	case GXSOURCE: 
	    GXfunction = GXSOURCE;
	    break;
	case f_BlackOnWhite | f_CharacterContext: 
	    GXfunction = ~GXSOURCE;
	    break;
	case f_WhiteOnBlack | f_CharacterContext: 
	    GXfunction = GXSOURCE;
	    break;
	case f_black: 
	default: 
	    GXfunction = 0;
	    break;
	case f_black | f_CharacterContext: 
	    GXfunction = ~GXSOURCE & GXDEST;
	    break;
	case f_white: 
	case 0xFFFF: 
	    GXfunction = -1;
	    break;
	case f_white | f_CharacterContext: 
	    GXfunction = GXSOURCE | GXDEST;
	    break;
    }
    display.d_CurrentColor = c;
    CurrentDisplay -> d_CurrentColor = c;
}

static
BWSnapShot () {
    static sequence = 0;
    register FILE *f;
    register height;
    struct RasterHeader header;
    char fname[100];
    static char *SnapShotDir;
    if (SnapShotDir == 0) {
	SnapShotDir = getprofile ("snapshotdir");
	if (SnapShotDir == 0) SnapShotDir = "/tmp";
    }
    sprintf (fname, "%s/snapshot-%d", SnapShotDir, ++sequence);
    if ((f = fopen (fname, "w")) == 0) return -1;
    GXwidth = 16;
    header.Magic=RasterMagic;
    header.width = display.screen.width;
    header.height = display.screen.height;
    header.depth = 1;
    fwrite (&header, sizeof header, 1, f);
    for (height = 0; height < display.screen.height; height++) {
	register short *xsa, *xlim;
	register short t;
	touch (* (short *) (display.base+GXselectY+(height<<1)));
	xsa = (short *) (display.base+GXsource+GXselectX);
	xlim = xsa+display.screen.width;
	t = *xsa;
	while (xsa < xlim) {
	    xsa += 16;
	    t = *xsa;
	    putc (t>>8, f);
	    putc (t, f);
	}
    }
    fclose(f);
}

static struct display *rjumpdisplay;
extern int	SmallScreen;

static
BWtry (name)
char   *name; {
    register    ps = getpagesize ();
    int     fd = open (name, 1);
    if (fd >= 0) {
	register struct display *p = &displays[NDisplays++];
/*	fcntl (fd, F_SETFD, 1); */
	ioctl (fd, FIOCLEX, 0);
	debug (("  Found sun1bw %s\n", name));
	rjumpdisplay = p;
 	if (SmallScreen) {
 	p -> screen.top = 144;
 	p -> screen.left = 152;
 	p -> screen.width = 720;
 	p -> screen.height = 512;
 	} else {
	p -> screen.top = 0;
	p -> screen.left = 0;
	p -> screen.width = 1024;
	p -> screen.height = 800;
	}
	p -> DeviceFileName = name;
	p -> base = malloc (0x20000 + ps);
	p -> base = (p -> base + ps - 1) & ~(ps - 1);
	mmap (p -> base, 0x20000, 2, 1, fd, 0);
	p -> d_RasterOp = BWRasterOp;
	p -> d_RasterSmash = BWRasterSmash;
	p -> d_vector = BWvector;
	p -> d_DrawString = BWDrawString;
	p -> d_DrawIcon = BWDrawIcon;
	p -> d_CurrentColor = 9999;
	p -> d_SizeofRaster = BWSizeofRaster;
	p -> d_CopyScreenToMemory = BWCopyScreenToMemory;
	p -> d_CopyMemoryToScreen = BWCopyMemoryToScreen;
	p -> d_DefineColor = NOP;
	p -> d_FillTrapezoid = BWFillTrapezoid;
	p -> d_SelectColor = BWSelectColor;
	p -> d_SnapShot = BWSnapShot;
	display.base = p -> base;
	CurrentDisplay = 0;
	GXcontrol = GXvideoEnable;
    }
}

InitializeSun1bw () {
    int     stn = NDisplays;
    BWtry ("/dev/bw0");
    if (stn == NDisplays)
	BWtry ("/dev/console");
}

NOP () {}


#define MaxStars 1000
#define denom 128
#define ldenom 7
#define lfix 16

static
struct star {
    long    x,
            y,
            xc,
            yc;
    long    rx,
            ry;
    char    visible;
}           stars[MaxStars];

rjump () {
    if (rjumpdisplay) {
	register    i;
	register struct star   *p;
	int     cx,
	        cy,
		rx,
		ry;
	register    nstars;
	int     function = f_white;
	int     laps;

#define	ds	display.screen

	if (CurrentDisplay != rjumpdisplay) {
	    display = *rjumpdisplay;
	    CurrentDisplay = rjumpdisplay;
	}
	CurrentViewPort = 0;
	if (SmallScreen) {
	rx = 1024;
	ry = 800;
	} else {
	rx = ds.width;
	ry = ds.height;
	}
	cx = rx / 2;
	cy = ry / 2;
	HW_SelectColor (f_black);
	HW_RasterSmash (0, 0, rx, ry);
	CurrentViewPort = &ds;
	nstars = rx * ry / (72 * 72 * 2);	/* One star for every two
						   square inches */
	if (nstars > MaxStars)
	    nstars = MaxStars;
	for (p = &stars[nstars]; --p >= stars;) {
	    register    h;
	    p -> xc = (random () % rx) << lfix;
	    p -> yc = (random () % ry) << lfix;
	    p -> x = p -> xc;
	    p -> y = p -> yc;
	    h = vlen (p -> x - cx, p -> y - cy);
	    p -> rx = (p -> x - (cx << lfix)) * 3 / h;
	    p -> ry = (p -> y - (cy << lfix)) * 3 / h;
	    p -> visible = 1;
	}
	GXwidth = 2;
	while (1) {
	    HW_SelectColor (function);
	    for (laps = 0; laps < 150; laps++) {
		for (p = &stars[nstars]; --p >= stars;)
		    if (p -> visible) {
			register    sx = p -> x >> lfix;
			register    sy = p -> y >> lfix;
			p -> x += p -> rx;
			p -> y += p -> ry;
			if (sx < ds.left || sy < ds.top || sx >= ds.width || sy >= ds.height)
			    p -> visible = 0;
			else {
			    GXsetX (sx);
			    *(short *) (display.base + GXselectY + GXsource + GXupdate + (sy << 1)) = 3;
			    *(short *) (display.base + GXselectY + GXsource + GXupdate + 2 + (sy << 1)) = 3;
			}
		    }
		if (laps == 0 && function==f_white)
		    sleep (2);
	    }
	    if (function == f_white) {
		for (p = &stars[nstars]; --p >= stars;) {
		    p -> x = p -> xc;
		    p -> y = p -> yc;
		    p -> visible = 1;
		}
		function = f_black;
	    }
	    else
		break;
	}
    }
#undef	ds

}

static vlen (xd, yd)
short xd, yd; {
    register    x = xd * xd + yd * yd;
    register    t,
                v,
                i;
    v = 0;
    i = 1024;
    while (i != 0) {
	t = v + i;
	if (t * t <= x)
	    v = t;
	i >>= 1;
    }
    return (v);
}
