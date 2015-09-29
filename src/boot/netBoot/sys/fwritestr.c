
/*
 * @(#)fwritestr.c 1.3 10/15/86
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Interpret a string of characters of length <len> into the frame buffer.
 *
 * Note that characters with the high bit set will not be recognized.
 * This is good, for it reserves them for ASCII-8 X3.64 implementation.
 * It just means all sources of chars which might come here must mask
 * parity if necessary.
 */

#include "../h/globram.h"
#include "../h/dpy.h"
#include "../h/video.h"
#include "../sun3/cpu.addrs.h"
#include "../h/enable.h"
#include "../h/keyboard.h"

#ifndef GRUMMAN

#define CTRL(c) ('c'-64 & 127)


fwritechar(c)
	unsigned char c;
{

	fwritestr(&c,1);
}


fwritestr(addr,len)
    register unsigned char *addr;
    register short len;		/* Declared short to get dbra - hah! */
{
    register char c;
    int lfs;			/* Lines to feed on a LF */
    
    cursorcomp();
    for (; --len != -1; ) {

	c = *addr++;		/* Fetch next char from string */

beginning:

	if (state & ESC) {
		switch (c) {
			case 0:		/*ignored*/
			case CTRL(J):	/*ignored*/
			case CTRL(M):	/*ignored*/
			case CTRL([):	/*ignored*/
			case CTRL(?):	/*ignored*/		continue;
			/* Begin X3.64 sequence; enter alpha mode */
			case '[':	state = ESCBRKT;
					cursor = BLOCKCURSOR;
								continue;
			/* Clear screen and enter alpha mode */
			case CTRL(L):	state = ALPHA;
					cursor = BLOCKCURSOR;
					pos(LEFT,TOP);
					rectfill(0,0,SCRWIDTH,SCRHEIGHT);
#ifdef PRISM
					rect_enable(0,0,SCRWIDTH,SCRHEIGHT,
						    0xFFFF);
						/* enable the mono FB */
#endif PRISM
								continue;
			/* Simulate DEL char for systems that can't xmit it */
			case '?':	c = CTRL(?);
					state &= ~ESC;		break;

			/* By default, ignore the character after the ESC */
			default:	state &= ~ESC;
					continue;
		}
	}

	switch (state) {

	case ALPHA:
		ac = 0;
		ac0 = -1;
		switch (c) {
			case CTRL(G):	blinkscreen();		break;
			case CTRL(H):	pos(ax-1,ay);	 	break;
			case CTRL(I):	pos((ax&-8)+8,ay); 	break;
			case CTRL(J):	/* linefeed */
	/* Linefeed is complicated, so it gets its own indentation */
FeedLine:
	if (ay < BOTTOM-1) {
		/* pos(ax,ay+1); */
		ay++;
		fbpos.pos.y += CHRHEIGHT;
		if (!scrlins) /* ...clear line */
			delchar(0,RIGHT,ay);
	} else {
		if (!scrlins) {  /* Just wrap to top of screen and clr line */
			pos (ax,0);
			delchar(0,RIGHT,ay);
		} else {
			lfs = scrlins;	/* We will scroll, but how much? */
			if (lfs == 1) {
				/* Find pending LF's and do them all now */
				unsigned char *cp = addr;
				short left = len;

				for (; --len != -1; addr++) {
					     if (*addr == CTRL(J)) lfs++;
					else if (*addr == CTRL(M)) ;
					else if (*addr >= ' ')     ;
					else if (*addr > CTRL(J)) break;
				}
				len = left; addr = cp;
			}
			if (lfs > BOTTOM) lfs = BOTTOM;
			delline (TOP, TOP+lfs);
			if (lfs != 1) /* avoid upsetting <dcok> for nothing */
			    pos (ax, ay+1-lfs);
		}
	}
	break;
			case CTRL(K):	pos(ax,ay-1); /* 4014 */	break;
			case CTRL(L):	pos(LEFT,TOP);
					rectfill(0,0,SCRWIDTH,SCRHEIGHT);
#ifdef PRISM
					rect_enable(0,0,SCRWIDTH,SCRHEIGHT,
						    0xFFFF);
						/* enable the mono FB */
#endif PRISM
			case CTRL(M):	/* pos(0,ay); */
					ax = 0;
					fbpos.pos.x = WINLEFT;
					break;
			case CTRL([):	state |= ESC; 		break;
			case CTRL(?):	/* ignored */		break;

			default:

			    c -= 32;
			    if (c >= 0)
			    {
				/* Write character on the screen. */
				drawchr (font[c], chrfunc);
				/* Update cursor position.  Inline for speed. */
				if (ax < RIGHT-1) {
					ax++;
					fbpos.pos.x += CHRWIDTH;
				} else {
					/* Wrap to col 1, pretend LF seen */
					ax = 0;
					fbpos.pos.x = WINLEFT;
					goto FeedLine;
				}
			    }
			    break;
		}  /* end of case ALPHA switch statement */
		break;

	case ESCBRKT:
		if ('0' <= c && c <= '9') {
			ac = ((char)ac)*10 + c - '0'; /* char for inline muls */
		} else if (c == ';') {
			ac0 = ac; ac = 0;
		} else {
		    acinit = ac;
		    if (ac == 0) ac = 1;	/* Default value is 1 */
		    switch ( c ) {
			case '@':	inschar(ax,ax+ac,ay);	break;
			case 'A':	pos(ax,ay-ac); 		break;
			case 'B':	pos(ax,ay+ac); 		break;
			case 'C':	pos(ax+ac,ay); 		break;
			case 'D':	pos(ax-ac,ay); 		break;
			case 'E':	pos(LEFT,ay+ac);		break;
			case 'f':
			case 'H':
					if (ac0 < 0) pos(0,ac-1);
					else         pos(ac-1,ac0-1);	break;
			case 'J':	delline(ay+1,BOTTOM); /* no break */
			case 'K':	delchar(ax,RIGHT,ay);		break;
			case 'L':	insline(ay,ay+ac); 		break;
			case 'M':	delline(ay,ay+ac);		break;
			case 'P':	delchar(ax,ax+ac,ay);		break;
			case 'm':	chrfunc = fillfunc^
					  ((acinit == 0)? 
						POX_SRC:
						POX_NOT(POX_SRC)
							      );	break;
			/* Sun-2 0-bits are white, 1-bits are black. */
			case 'p':	if (fillfunc != POX_CLR)
						screencomp();		break;
			case 'q':	if (fillfunc != POX_SET)
						screencomp();		break;
			case 'r':	scrlins = acinit;		break;
			case 's':	finit(ax, ay);			break;
			default:	/* X3.64 sez ignore if we don't know */
					if ((c < '@')) {
						state = SKIPPING;
						continue;
					}
		    }
		state = ALPHA;
		}
		break;

	case SKIPPING:	/* Waiting for char from cols 4-7 to end esc string */
		if (c < '@') break;
		state = ALPHA;
		break;

	default:
		/* Deal with graphics states if 2nd prom exists, else just
		   go to ALPHA mode and reinterpret the character. */
		state = ALPHA;
		cursor = BLOCKCURSOR;
		goto beginning;
	}
    }			/* End of for loop thru string of chars */

    cursorcomp();	/* Restore the cursor to the screen */
}


pos(x,y)
	int x, y;
{

	dcok = 0;	/* We've changed ax or ay and not dcax,dcay */

	ax = x < LEFT?		LEFT:
	     x >= RIGHT?	RIGHT-1:
				x;

	ay = y < TOP? 		TOP:
	     y >= BOTTOM?	BOTTOM-1:
				y;
}


cursorcomp()
{

	if (cursor != NOCURSOR)
/* The 2nd param (font) is used as the source for the character sent to the
   frame buffer.  However, the function we are using ignores the source,
   so we just punt with  font . 	*/
		drawchr(font, POX_NOT(POX_DST));
}

screencomp()
{

	invertwholescreen();
	chrfunc = POX_NOT(chrfunc);
	fillfunc = POX_NOT(fillfunc);
}

blinkscreen()
{
	struct videoctl copyvid;
	int i = 10000;

	if (gp->g_insource == INKEYB) {
		while (!sendtokbd(KBD_CMD_BELL)) if (0 == --i) return;
		while (--i);
		(void) sendtokbd(KBD_CMD_NOBELL);
		return;
	}

	set_enable(get_enable() & ~ENA_VIDEO);	/* Disable video output */
	while (i--);			/* Wait a moment... */
	set_enable(get_enable() |  ENA_VIDEO);	/* Enable  video output */
}

#ifndef PRISM

/*
 * Copy one rectangle in the frame buffer to another.
 * On the Sun-2, without RasterOp support, this only works word-aligned.
 */
rectcopy(fromxlo,fromylo,fromxhi,fromyhi,tox,toy)
	int fromxlo, fromylo, fromxhi, fromyhi, tox, toy;
{
	int y, yy, toyy;
	int width, height;
	
	if (fromxlo < 0) fromxlo = 0;
	if (fromylo < 0) fromylo = 0;
	if (fromxhi > SCRWIDTH) fromxhi = SCRWIDTH;
	if (fromyhi > SCRHEIGHT) fromyhi = SCRHEIGHT;
	if (tox < 0) tox = 0;
	if (toy < 0) toy = 0;

	yy =   GXBase + (fromxlo >> 3) +
		((unsigned short)fromylo * (unsigned short)(SCRWIDTH >> 3));
	toyy = GXBase + (tox     >> 3) +
		((unsigned short)toy     * (unsigned short)(SCRWIDTH >> 3));
	width = (fromxhi - fromxlo) >> 3;

	if (fromylo >= toy) {
		for (y = fromylo;
		     y < fromyhi;
		     y++, yy += SCRWIDTH>>3, toyy += SCRWIDTH>>3) {
			bltshort(yy, yy+width, toyy);
		}
	} else {
		height = ((unsigned short)(fromyhi - fromylo)) * 
			  (unsigned short)(SCRWIDTH >> 3);
		yy += height;
		toyy += height;
		for (y = fromyhi-1;
		     y >= fromylo;
		     y--, yy -= SCRWIDTH>>3, toyy -= SCRWIDTH>>3) {
			bltshort(yy, yy+width, toyy);
		}
	}	/* if (fromylo>=toy).. else */
}	/* end of rectcopy() */

#endif PRISM	/* end of ifndef PRISM */


#ifdef PRISM

/*
 *  rectcopy() for prism is similar to the nonprism rectcopy.  The difference
 *  is that we do the same thing to the enable plane as we do to the monochrome
 *  frame buffer.
 */
rectcopy(fromxlo,fromylo,fromxhi,fromyhi,tox,toy)
	int fromxlo, fromylo, fromxhi, fromyhi, tox, toy;
{
	int y, yy, toyy;
	int width, height;
	short	i;
	int Base;
	
	if (fromxlo < 0) fromxlo = 0;
	if (fromylo < 0) fromylo = 0;
	if (fromxhi > SCRWIDTH) fromxhi = SCRWIDTH;
	if (fromyhi > SCRHEIGHT) fromyhi = SCRHEIGHT;
	if (tox < 0) tox = 0;
	if (toy < 0) toy = 0;

   for (i=1, Base=GXBase; i < 3; i++, Base=(int)BW_ENABLE_MEM_BASE) {
	yy =   Base + (fromxlo >> 3) +
		((unsigned short)fromylo * (unsigned short)(SCRWIDTH >> 3));
	toyy = Base + (tox     >> 3) +
		((unsigned short)toy     * (unsigned short)(SCRWIDTH >> 3));
	width = (fromxhi - fromxlo) >> 3;

	if (fromylo >= toy) {
		for (y = fromylo;
		     y < fromyhi;
		     y++, yy += SCRWIDTH>>3, toyy += SCRWIDTH>>3) {
			bltshort(yy, yy+width, toyy);
		}
	} else {
		height = ((unsigned short)(fromyhi - fromylo)) * 
			  (unsigned short)(SCRWIDTH >> 3);
		yy += height;
		toyy += height;
		for (y = fromyhi-1;
		     y >= fromylo;
		     y--, yy -= SCRWIDTH>>3, toyy -= SCRWIDTH>>3) {
			bltshort(yy, yy+width, toyy);
		}
	}	/* if (fromylo>=toy).. else */
   }	/* for (i=1...) */
}	/* end of rectcopy() */

#endif PRISM


/*
 *	Draw the character pointed to (in the font) by  p at position  ax,
 *	ay  on the screen (global variables) using function f
 */

drawchr(p,f)
	short *p;
	int f;
{

	/* FIXME: maybe avoid multiplies in batchrop too, by fudging addr? */
	if (!dcok) {
		fbpos.pos.x = WINLEFT + ax * CHRWIDTH;
		fbpos.pos.y = WINTOP  + ay * CHRHEIGHT;
		dcok = 1;
	}
	chardata.md_image = p;

#ifdef PRISM
	rect_enable(fbpos.pos.x, fbpos.pos.y, fbpos.pos.x + 0x10,
		    fbpos.pos.y+CHRHEIGHT, 0xFFFF);
		    /*
		     * enable a block big enough for 1 character.
		     * A character is 12 wide by 22 high.  However, rect_enable
		     * fills in at least 16 bits (short word) at a time; so
		     * we're actually enabling a block 16 wide by 22 high.
		     */
#endif PRISM

	prom_mem_batchrop(fbpos, f, &charpos, 1);
}

/*
 *  FOR PRISM ONLY:
 *
 *  rect_enable enables or disables a rectangle in the enable plane.
 *
 *	rect_enable(xlo,ylo,xhi,yhi,0xFFFF)  enables a rect in the
 *		monochrome FB.
 *	rect_enable(xlo,ylo,xhi,yhi,0)  disables a rect in the monochrome FB.
 *
 *  The enable plane is originally setup by the bootprom as having enabled
 *  the BW frame buffer, but we can't guaranteed that'll be the case when
 *  other programs execute and since the kernel uses the bootprom monitor
 *  to display console messages, we must make sure that the locations where
 *  the message is to appear must be enabled.  Typically, this routine is
 *  called by draw_char to enable a block big enough for 1 character;
 *  rect_enable is also called in other places, usually to enable the
 *  entire monochrome plane.
 */

#ifdef PRISM
rect_enable(xlo,ylo,xhi,yhi,pattern)
	unsigned short pattern;
		/*
		 * xlo, ylo, xhi, and yhi are pixel positions; each pixel
		 * is a bit.
		 */
{
	int yy, width;
	unsigned short y;

	width = (xhi - xlo) >> 3;	/* change to bytes */
	y = ylo;
	yy = (int)BW_ENABLE_MEM_BASE + (y * (unsigned short)(SCRWIDTH >> 3)) +
	     (xlo >> 3);
	for (; y < yhi; y++, yy += SCRWIDTH >> 3) {
		setshort(yy, yy+width, pattern);
			/*
			 * a 1 bit enables each pixel to monochrome; so need to
			 * fill in 0xFFFF (for enabling) since setshort() uses
			 * the pattern as a 16 bit (short) word.
			 */
	}
}
#endif PRISM

/*
 * Fill a rectangle on the screen with black or white.
 * For the Sun-2 without RasterOp support, only works on word boundaries.
 */
rectfill(xlo,ylo,xhi,yhi)
{
	int yy, width, filler;
	unsigned short y;

	width = (xhi - xlo) >> 3;	/* Bytes of difference */
	filler = (fillfunc == POX_SET)? -1: 0;
	y = ylo;
	yy = GXBase + (y * (unsigned short)(SCRWIDTH >> 3)) + (xlo >> 3);
	for (;
	     y < yhi;
	     y++, yy += SCRWIDTH >> 3) {
		setshort(yy, yy+width, filler);
	}
}


/*
 * Invert the entire screen.
 */
invertwholescreen()
{
	register long *q, *end;
	register long invert = -1;

	q = (long *)GXBase;
#ifdef SIRIUS
	end = (long *) (GXBase + 256*1024);
#else
	end = (long *) (GXBase + 128*1024);
#endif SIRIUS

	for (; q < end; q++) *q ^= invert;
}

/* now its var gp->g_winbot  init'd in finit.c */
#ifndef SIRIUS
#define	WINBOT (WINTOP+BOTTOM*CHRHEIGHT) 
#endif SIRIUS
#define	WINRIGHT (WINLEFT+RIGHT*CHRWIDTH)

/* delete lines a (inclusive) to b (exclusive) */
delline(a,b)
	unsigned char a, b;
{
	register pixla = a*CHRHEIGHT, pixlb = b*CHRHEIGHT;

	rectcopy(0,WINTOP+pixlb,		SCRWIDTH,WINBOT,
		 0,WINTOP+pixla);
	rectfill(0,WINBOT-(pixlb-pixla),	SCRWIDTH,SCRHEIGHT);
#ifdef PRISM
	rect_enable(0,WINBOT-(pixlb-pixla),	SCRWIDTH,SCRHEIGHT, 0xffff);
#endif PRISM
}

/* insert (make room for) lines a (inclusive) to b (exclusive) */
insline(a,b)
	unsigned char a, b;
{
	register pixla = a*CHRHEIGHT, pixlb = b*CHRHEIGHT;

	rectcopy(0,WINTOP+pixla,    SCRWIDTH,WINBOT-(pixlb-pixla),
		 0,WINTOP+pixlb);
	rectfill(0,WINTOP+pixla,    SCRWIDTH,WINTOP+pixlb);
}

/*
 * delete chars a (inclusive) to b (exclusive) on line l
 *
 * One by one, we move any characters that need to slide.
 * We fill one character position by drawing a blank there,
 * then we know that rounding the cursor position down to a 16-bit boundary
 * is good enough to fill the rest of the line.
 */
delchar(a, b, l)
	unsigned char a, b, l;
{
	int savex = ax;
	int savey = ay;
	register int apos, bpos;
	short achar[CHRSHORTS];
	
	if (b < RIGHT) {
		/*
		 * We have to actually shift characters.
		 */
		fbpos.pos.y = WINTOP + l * CHRHEIGHT;	/* All on this line */
		apos = WINLEFT + a * CHRWIDTH;
		bpos = WINLEFT + b * CHRWIDTH;
		chardata.md_image = achar;	/* Here's temp char location */
		for (;bpos < WINRIGHT;
		      apos += CHRWIDTH, bpos += CHRWIDTH) {
			fbpos.pos.x = bpos;
			prom_mem_grab(fbpos, 0, &charpos, 0);
			fbpos.pos.x = apos;
			prom_mem_batchrop(fbpos, PIX_SRC, &charpos, 1);
		}
		ax = RIGHT-(b-a);	/* Position to fill from */
	} else {
		ax = a;			/* Position to fill from */
	}

	ay = l;
	dcok = 0;		/* We're doing severe play with dev. coords */
	drawchr(font, fillfunc);	/* Fill one char position there */
	
	/* OK, that character has been filled; now do the rest. */
	rectfill( (fbpos.pos.x + CHRWIDTH) &~ 0x000F,
		 fbpos.pos.y, 
		 SCRWIDTH,
		 fbpos.pos.y+CHRHEIGHT);
	
	pos(savex, savey);		/* Clean up our grunge */
}

/* insert (make room for) chars a (inclusive) to b (exclusive) on line l */
inschar(a, b, l)
	unsigned char a, b, l;
{
	int savex = ax;
	int savey = ay;
	register int apos, bpos;
	register short temp;
	short achar[CHRSHORTS];
	
	if (b > RIGHT) b = RIGHT;		/* Avoid problems */

	fbpos.pos.y = WINTOP + l * CHRHEIGHT;	/* All on this line */
	bpos = WINRIGHT - CHRWIDTH;		/* Start on last char */
	apos = bpos - (b - a) * CHRWIDTH;	/* Copy from N chars back */
	chardata.md_image = achar;	/* Here's temp char location */
	temp = RIGHT - b;			/* How many to copy */

	for (;temp-- != 0;
	      apos -= CHRWIDTH, bpos -= CHRWIDTH) {
		fbpos.pos.x = apos;
		prom_mem_grab(fbpos, 0, &charpos, 0);
		fbpos.pos.x = bpos;
		prom_mem_batchrop(fbpos, PIX_SRC, &charpos, 1);
	}

	ax = a; ay = l;		/* Position to fill from */
	dcok = 0;		/* We're doing severe play with dev. coords */
	for (; a < b; a++, fbpos.pos.x += CHRWIDTH) {
		drawchr(font, fillfunc);	/* Fill one char position */
	}

	pos(savex, savey);
}

static unsigned	logo_data[128] = {
			0x00000003, 0xC0000000, 0x0000000F, 0xF0000000,
			0x0000001F, 0xF8000000, 0x0000003F, 0xFC000000,
			0x0000003F, 0xFE000000, 0x0000007F, 0xFF000000,
			0x0000007E, 0xFF800000, 0x0000027F, 0x7FC00000,
			0x0000073F, 0xBFE00000, 0x00000FBF, 0xDFF00000,
			0x00001FDF, 0xEFF80000, 0x00002FEF, 0xF7FC0000,
			0x000077F7, 0xFBFE0000, 0x0000FBFB, 0xFDFF0000,
			0x0001FDFD, 0xFEFF0000, 0x0001FEFE, 0xFF7EC000,
			0x0006FF7F, 0x7FBDE000, 0x000F7FBF, 0xBFDBF000,
			0x001FBFDF, 0xDFE7F800, 0x003FDFEF, 0xEFEFF400,
			0x007FAFF7, 0xF7DFEE00, 0x00FF77FB, 0xF3BFDF00,
			0x01FEFBFD, 0xF97FBF80, 0x03FDFDFF, 0xF8FF7F00,
			0x07FBF8FF, 0xF9FEFE00, 0x0FF7F07F, 0xF3FDFCE0,
			0x1FEFE73F, 0xF7FBFBF8, 0x3FDFDFDF, 0xEFF7F7FC,
			0x7FBFBFE7, 0x9FEFEFFE, 0x7F7F7FE0, 0x1FDFDFFE,
			0xFEFEFFF0, 0x3FBFBFFF, 0xFDFDFFF0, 0x3F7F7FFF,
			0xFFFBFBF0, 0x3FFEFF7F, 0xFFF7F7F0, 0x3FFDFEFF,
			0x7FEFEFE0, 0x1FFBFDFE, 0x7FDFDFE7, 0x9FF7FBFE,
			0x3FBFBFDF, 0xEFEFF7FC, 0x0E7F7FBF, 0xF39FEFF8,
			0x00FEFF3F, 0xF83FDFF0, 0x01FDFE7F, 0xFC7FBFE0,
			0x03FBFC7F, 0xFEFF7FC0, 0x01F7FA7E, 0xFF7EFF80,
			0x00EFF73F, 0x7FBDFF00, 0x005FEFBF, 0xBFDBFE00,
			0x003FDFDF, 0xDFE7FC00, 0x001FAFEF, 0xEFF7F800,
			0x000F77F7, 0xF7FBF000, 0x0006FBFB, 0xFBFDE000,
			0x0001FDFD, 0xFDFEC000, 0x0001FEFE, 0xFEFF0000,
			0x0000FF7F, 0x7F7F0000, 0x00007FBF, 0xBFBE0000,
			0x00003FDF, 0xDFDC0000, 0x00001FEF, 0xEFE80000,
			0x00000FF7, 0xF7F00000, 0x000007FB, 0xFBE00000,
			0x000003FD, 0xF9C00000, 0x000001FE, 0xFC800000,
			0x000000FF, 0xFC000000, 0x0000007F, 0xFC000000,
			0x0000003F, 0xF8000000, 0x0000001F, 0xF8000000,
			0x0000000F, 0xF0000000, 0x00000003, 0xC0000000
		};

/*
 *	Draw the Sun logo, starting on line (y) on screen.
 *	No clipping, y must be valid.
 */

sunlogo(y)
	unsigned short y;
{
	register long *addr, *logo;
	register short i;
	
	addr = (long *) (GXBase +
		(unsigned short)((WINTOP + y * CHRHEIGHT)) * 
		(unsigned short)(SCRWIDTH/8)
		+ 2*(WINLEFT/16) );
	logo = (long *) logo_data;
	for (i = 64; i-- != 0;) {
#ifndef	SIRIUS
		if (chrfunc == POX_SRC) {
			*addr++ = *logo++;
			*addr++ = *logo++;
		} else {
			*addr++ = ~*logo++;
			*addr++ = ~*logo++;
		}
#else	SIRIUS
		*addr++ = *logo++;
	        *addr++ = *logo++;
#endif	SIRIUS
		addr += (SCRWIDTH/32) - 2;	/* -2 for the autoincs */
	}
}

#endif GRUMMAN	/* end of ifndef GRUMMAN */

#ifdef GRUMMAN

fwritechar(c)
	unsigned char c;
{
	return;
}

fwritestr(addr,len)
    register unsigned char *addr;
    register short len;
{
	return;
}

#endif GRUMMAN

