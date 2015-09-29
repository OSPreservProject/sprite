/*******************************************************************\
* 								    *
* 	File: sun1color.c					    *
* 	Copyright (c) 1984 IBM					    *
* 	Date: Sat Mar 17 15:50:31 1984				    *
* 	Author: James Gosling					    *
* 								    *
* Window manager device driver for the SUN 1 color graphics board.  *
* 								    *
* HISTORY							    *
* 								    *
\*******************************************************************/


#include "stdio.h"
#include "font.h"
#include "window.h"
#include "display.h"
/* #include <fcntl.h> */
#include "sys/ioctl.h"


#define ColorStatus	(* (unsigned char *) (display.base + 0x1800))
#define		LoadColorMap(n)	(n)
#define		DisplayMap(n)	((n)<<2)
#define		ColorInterruptEnable	0x10
#define 	ColorPaintMode	0x20
#define		ColorDisplayOn	0x40
#define	InVerticalRetrace	(ColorStatus&0x80)
#define ColorMask	(* (unsigned char *) (display.base + 0x1900))
#define ColorFunction	(* (unsigned char *) (display.base + 0x1a00))
#define		CopyColorFromMask	0xF0
#define		InvertUnderMask		0x5A
#define		CopyColor		0xCC
#define ColorX(op,x)	(* (unsigned char *) (display.base + 0x0800 + (op) + (x)))
#define ColorY(op,y)	(* (unsigned char *) (display.base + 0x0000 + (op) + (y)))
#define		Set0		0x000
#define		Set1		0x400
#define		Update		0x2000
#define RedMap(i)	(* (unsigned char *) (display.base + 0x1000 + (i)))
#define GreenMap(i)	(* (unsigned char *) (display.base + 0x1100 + (i)))
#define BlueMap(i)	(* (unsigned char *) (display.base + 0x1200 + (i)))






#define MOV(reg,sign) (0x1000|reg|sign)  /* builds a movb instruction */
#define RX 0xA00	/* a5 holds rx address */
#define RY 0x800	/* a4 holds ry address */
#define POS 0xc0	/* an@+ scans positively */
#define NEG 0x100	/* an@- scans negatively */

extern bres();

static unsigned short mincmd[8] = {
	MOV(RY,POS), MOV(RX,POS),
	MOV(RY,POS), MOV(RX,NEG),
	MOV(RY,NEG), MOV(RX,POS),
	MOV(RY,NEG), MOV(RX,NEG)
};

static unsigned short majcmd[8] = {
	MOV(RX,POS), MOV(RY,POS),
	MOV(RX,NEG), MOV(RY,POS),
	MOV(RX,POS), MOV(RY,NEG),
	MOV(RX,NEG), MOV(RY,NEG)
};


ColorVector(x0,y0,x1,y1)
unsigned x0,y0,x1,y1;
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
	            r;		/* d7,d6,d5,d4  */
	register unsigned   t = 0;/* d3 */
	register unsigned char *rx,
	                       *ry;/* a5, a4 */

	x = x1 - x0;		/* x,y relative to x0,y0 */
	y = -(y0 - y1);

	rx = &ColorX (Set0, x0);
	ry = &ColorY (Set0, y0);
	*rx = 0;
	*ry = 0;
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
	    ry += Update;
	    t = x;
	    x = y;
	    y = t;
	    q += 1;
	}
	else
	    rx += Update;

	((unsigned short *) bres)[0] = mincmd[q];
	((unsigned short *) bres)[1] = majcmd[q];
	q = x;
	r = -(x >> 1);
	bres ();		/* call Bresenham */
	return 0;
    }
}

struct ColorMap {
    char    red,
            green,
            blue,
            flags;
};

/* Color map flags */
#define CMchanged	0200
#define CMrefcnt	0037

#define NColors 256

struct ColorMap ColorMap[NColors];

static
UpdateColorMap () {
    register struct ColorMap   *p;
    register struct ColorMap   *lim = ColorMap + NColors;
    register struct ColorMap   *restart = 0;
    register    NDumped = 0;
    register    index = 0;
    for (p = ColorMap; p < lim; p++, index++)
	if (p -> flags & CMchanged) {
	    if (NDumped == 0)
		while (!InVerticalRetrace);
	    do {
		RedMap (index) = p -> red;
		GreenMap (index) = p -> green;
		BlueMap (index) = p -> blue;
	    } while (!InVerticalRetrace);
	    NDumped++;
	    p -> flags &= ~CMchanged;
	}
}

static
SetColorMap (i, red, blue, green) {
    register struct ColorMap *p = &ColorMap[i];
    p -> red = red;
    p -> blue = blue;
    p -> green = green;
    p -> flags |= CMchanged;
}

/*
 *  These rasterop routines support the transfer of rasters between
 *  memory and colorbuffer, memory and memory, and colorbuffer and
 *  colorbuffer.  Also for raster text strings.
 */

#define	touch(p)	((p)=0)
#define	loop(ctr, s)\
for(j=ctr;j>15;j-=16){\
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

ColorRasterOp (xs, ys, xd, yd, width, height) {
    register short  i,
                    j;
    if (width < 0) {
	xs += width;
	xd += width;
	width = -width;
    }
    if (height < 0) {
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
    if (width > 0 && height > 0)
	if (yd <= ys && (xd < xs || yd < ys)) {
	    register unsigned char *yda = &ColorY (Set0 | Update, yd),
	                           *xda = &ColorX (Set0, xd),
	                           *ysa = &ColorY (Set1 | Update, ys),
	                           *xsa = &ColorX (Set1, xs);
	    for (i = width; i > 0; i--) {
		touch (*xda);
		touch (*xsa);
		j = *ysa++;	/* prime the pump */
		loop (height, *yda++ = *ysa++);
		yda -= height;
		xda++;
		ysa -= height + 1;/* 1 extra to prime the pump with */
		xsa++;
	    }
	}
	else {
	    register unsigned char *xda = &ColorX (Set0 | Update, xd + width),
	                           *yda = &ColorY (Set0, yd + height),
	                           *xsa = &ColorX (Set1 | Update, xs + width),
	                           *ysa = &ColorY (Set1, ys + height);
	    for (i = height; i > 0; i--) {
		yda--;
		ysa--;
		touch (*yda);
		touch (*ysa);
		j = *--xsa;	/* prime the pump */
		loop (width, *--xda = *--xsa);
		xda += width;
		xsa += width + 1;/* 1 extra to prime the pump with */
	    }

	}
}

static
ColorRasterSmash (xd, yd, width, height)
register width; {
    register unsigned char *yda,
                           *xda;
    register short  i,
                    j,
		    a;

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
    yda = &ColorY (Set0 | Update, yd),
    xda = &ColorX (Set0, xd);
    if (ColorFunction == InvertUnderMask || width < 6) {		/* for narrow regions */
	for (i = width; i > 0; i--) {
	    touch (*xda);
	    loop (height, touch (*yda++));
	    yda -= height;
	    xda++;
	}
	return (0);
    }
    a = xd % 5;
    if (a) {			/* fill to first 5 pixel boundary */
	for (i = a; i < 5; i++) {
	    touch (*xda);
	    loop (height, touch (*yda++));
	    yda -= height;
	    xda++;
	}
	width -= 5 - a;
    }
    ColorStatus |= ColorPaintMode;/* fill thru all 5 pixel wide colums */
    for (i = width; i > 4; i -= 5) {
	touch (*xda);
	loop (height, touch (*yda++));
	yda -= height;
	xda += 5;
    }
    ColorStatus &= ~ColorPaintMode;/* fill any remaining width */
    width = i;
    for (i = width; i > 0; i--) {
	touch (*xda);
	loop (height, touch (*yda++));
	yda -= height;
	xda++;
    }
}

static
ColorDrawIcon (x, y, c)
struct icon *c; {
    if (c -> OffsetToSpecific) {
	register struct BitmapIconSpecificPart *s =
	                                        (struct BitmapIconSpecificPart *) (((int) c) + c -> OffsetToSpecific);
	switch (s -> type) {
	    case BitmapIcon: 
		x -= s -> ocol;
		y -= s -> orow;
		{
		    register struct ViewPort   *v = CurrentViewPort;
		    if (v) {
			register    t;
			if (x < 0) {
			    return;/* 
				 width += x;
				 xs -= x;
				 x = 0; */
			}
			if (y < 0) {
			    return;/* 
				 height += y;
				 ys -= y;
				 y = 0; */
			}
			if ((t = v -> width - x - s -> cols) < 0) {
			    return;/* width += t; */
			}
			if ((t = v -> height - y - s -> rows) < 0) {
			    return;/* height += t; */
			}
			x += v -> left;
			y += v -> top;
/*	    if (width <= 0 || height <= 0)
		return; */
		    }
		}
		{
		    register unsigned char *yda = &ColorY (Set0 | Update, y),
		                           *xda = &ColorX (Set0, x);
		    register short  i,
		                    j;
		    register unsigned short mask;
		    register unsigned short *bits = s -> bits;
		    mask = 1 << 15;
		    for (i = s -> cols; i > 0; i--) {
			touch (*xda);
			loop (s -> rows, (((*bits++ & mask) ? touch (*yda) : 0), yda++));
#ifdef BSD41c
			mask = mask >> 1;	/* SUN C compiler bug */
#else
			mask >>= 1;
#endif
			yda -= s -> rows;
			xda++;
			if (mask == 0)
			    mask = 1 << 15;
			else
			    bits -= s -> rows;
		    }
		}
	}
    }
}


/************************************************************************\
* 									 *
* Routine to draw strings on the color display.				 *
* 									 *
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
ColorDrawString (x, y, font, s, n, CharShim, SpaceShim)
register    x,
            y;
register struct font   *font;
unsigned char   *s;
{
    while (*s) {
	register struct icon   *c = &font -> chars[*s++];
	if (c -> OffsetToGeneric) {
	    register struct IconGenericPart *g =
	                                    (struct IconGenericPart *) (((int) c) + c -> OffsetToGeneric);
	    ColorDrawIcon (x, y, c);
	    x += g -> Spacing.x + CharShim;
	    y += g -> Spacing.y;
	    if (s[-1] == ' ')
		x += SpaceShim;
	}
	if (--n == 0) break;
    }
    LastY = y;
    return LastX = x;
}


ColorCopyMemoryToScreen (xs, ys, rs, xd, yd, width, height)
struct raster  *rs; {
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
	    if (width <= 0 || height <= 0)
		return;
	}
    }
    {
	register unsigned char *yda = &ColorY (Set0 | Update, yd),
	                       *xda = &ColorX (Set0, xd);
	register short  i,
	                j;
	register char *bits = ((char *) rs -> bits) + xs * rs -> height + ys;
	for (i = width; --i >= 0; ) {
	    touch (*xda);
	    loop (height, *yda++ = *bits++);
	    yda -= height;
	    xda++;
	    bits += rs -> height - height;
	}
    }
}

ColorCopyScreenToMemory (xs, ys, xd, yd, rd, width, height)
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
	register char *da = ((char *) rd -> bits) + xd * rd -> height + yd;
	register unsigned char *ysa = &ColorY (Set1 | Update, ys),
	                       *xsa = &ColorX (Set1, xs);
	register short  i,
	                j;
	for (i = width; --i >= 0; ) {
	    touch (*xsa);
	    j = *ysa++;		/* prime the pump */
	    loop (height, *da++ = *ysa++);
	    da += rd -> height - height;
	    ysa -= height + 1;	/* 1 extra to prime the pump with */
	    xsa++;
	}
    }
}


static
ColorSelectColor (c)
unsigned short c; {
    switch (c) {
	case f_invert | f_CharacterContext:
	case f_invert:
	case 0x5555:
	    ColorMask = 1;
	    ColorFunction = InvertUnderMask;
	    break;
	case f_copy:
	case 0xCCCC:
	    ColorFunction = CopyColor;
	    break;
	case f_black:
	default: 
	    ColorMask = 0;
	    ColorFunction = CopyColorFromMask;
	    break;
	case f_black | f_CharacterContext: 
	    ColorMask = 5;
	    ColorFunction = CopyColorFromMask;
	    break;
	case f_white:
	case 0xFFFF:
	    ColorMask = 1;
	    ColorFunction = CopyColorFromMask;
	    break;
	case f_white | f_CharacterContext: 
	    ColorMask = 1;
	    ColorFunction = CopyColorFromMask;
	    break;
    }
    display.d_CurrentColor = c;
    CurrentDisplay -> d_CurrentColor = c;
}

static
ColorSizeofRaster (w, h) {
    return w*h;
}

#ifdef UNDEF

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
    dxl = x2 - x1;
    dy = y2 - y1;
    dxr = (x2 + w2) - (xr = (xl = x1) + w1);
    {
	register struct ViewPort   *v = CurrentViewPort;
	if (v) {
/*	    register    t;
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
		height += t; */
	    xl += v -> left;
	    y1 += v -> top;
	    xr += v -> left;
	    y2 += v -> top;
	}
    }
#define	TXD	((short *)(display.base+GXsource+GXupdate+GXselectX))
#define	TYD	((short *)(display.base+GXselectY))
    el = er = 0;
    row = y1 % height;
    fx = (xl + width - 1) / width * width;
    while (1) {
	register short *xda;
	register    w = bits[row];
	int     x;
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
	if (y1 >= y2)
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

#endif

static
ColorTry (name)
char   *name; {
    register    ps = getpagesize ();
    int     fd = open (name, 1);
    if (fd >= 0) {
	register struct display *p = &displays[NDisplays++];
/*	fcntl (fd, F_SETFD, 1); */
	ioctl (fd, FIOCLEX, 0);
	debug (("  Found sun1color %s\n", name));
	p -> screen.top = 0;
	p -> screen.left = 0;
	p -> screen.width = 640-1;	/* The last pixel in each row smudges badly */
	p -> screen.height = 475;
	p -> DeviceFileName = name;
	p -> base = malloc (16 * 1024 + ps);
	p -> base = (p -> base + ps - 1) & ~(ps - 1);
	mmap (p -> base, 16 * 1024, 2, 1, fd, 0);
	p -> d_RasterOp = ColorRasterOp;
	p -> d_RasterSmash = ColorRasterSmash;
	p -> d_vector = ColorVector;
	p -> d_DrawString = ColorDrawString;
	p -> d_DrawIcon = ColorDrawIcon;
	p -> d_SizeofRaster = ColorSizeofRaster;
	p -> d_CopyScreenToMemory = ColorCopyScreenToMemory;
	p -> d_CopyMemoryToScreen = ColorCopyMemoryToScreen;
	p -> d_DefineColor = NOP /* ColorDefineColor */;
	p -> d_SelectColor = ColorSelectColor;
	p -> d_FillTrapezoid = NOP;
	display.base = p -> base;
	CurrentDisplay = 0;
	ColorStatus = ColorDisplayOn;
	SetColorMap (0, 0, 0, 0);
	SetColorMap (1, 255, 255, 255);
	SetColorMap (2, 255, 0, 0);
	SetColorMap (3, 0, 255, 0);
	SetColorMap (4, 0, 0, 255);
	SetColorMap (5, 200, 0, 100);
	UpdateColorMap ();
    }
}

InitializeSun1Color () {
    ColorTry ("/dev/cg0");
}
