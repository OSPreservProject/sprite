/* Font reading and writing and string drawing */

/*********************************\
* 				  *
* 	James Gosling, 1983	  *
* 	Copyright (c) 1983 IBM	  *
* 				  *
\*********************************/


#include "font.h"
#include "window.h"
#include "framebuf.h"


/* Draw a string using fancy font tables */

#define	XS	((short *)(GXBase+GXset1+GXselectX))
#define	YS	((short *)(GXBase+GXsource+GXset1+GXselecty))
#define	XD	((short *)(GXBase+GXselectX))
#define	YD	((short *)(GXBase+GXsource+GXupdate+GXselectY))
#define	touch(p)	((p)=0)
#define	loop(s)\
for (j = height; j > 15; j -= 16) { \
	s; s; s; s; \
	s; s; s; s; \
	s; s; s; s; \
	s; s; s; s; \
} \
switch (j) { \
    case 15: s; case 14: s; case 13: s; case 12: s; \
    case 11: s; case 10: s; case 9: s; case 8: s; \
    case 7: s; case 6: s; case 5: s; case 4: s; \
    case 3: s; case 2: s; case 1: s; \
}

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
DrawString (x, y, font, s, n, CharShim, SpaceShim)
register    x,
            y;
register struct font   *font;
unsigned char   *s;
{
    struct ViewPort *v;
    int     MaxX;
/*  debug (("%c%c.. n=%d; CharShim=%d\n", s[0], s[1], n, CharShim)); */
    if ((v = CurrentViewPort) == 0)
	v = &DefaultViewPort;
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
		ROPms (0, 0, &r, x - s -> ocol, y - s -> orow, s -> cols, s -> rows);
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
		DrawIcon (x, y, c);
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

DrawIcon (x, y, c)
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
		    ROPms (0, 0, &r, x - s -> ocol, y - s -> orow, s -> cols, s -> rows);
		}
	}
    }
}
