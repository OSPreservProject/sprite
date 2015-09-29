/*
 *  These rasterop routines support the transfer of rasters between
 *  memory and framebuffer, memory and memory, and framebuffer and
 *  framebuffer.
 */
#include "framebuf.h"
#include "window.h"

#define	XS	((short *)(GXBase+GXset1+GXselectX))
#define	YS	((short *)(GXBase+GXsource+GXset1+GXselecty))
#define	XD	((short *)(GXBase+GXselectX))
#define	YD	((short *)(GXBase+GXsource+GXupdate+GXselectY))


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
RasterOp (xs, ys, xd, yd, width, height) {
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
ROPds (xd, yd, width, height) {
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
/*------------------------------------------------------------------*/
/*
 * operate memory-to-screen
 */
ROPms(xs, ys, rs, xd, yd, width, height)
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
ROPcopysm(xs, ys, xd, yd, rd, width, height)
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
ROPdminv(xd, yd, rd, width, height)
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
ROPdmset(xd, yd, rd, width, height)
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
ROPdmclr(xd, yd, rd, width, height)
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
