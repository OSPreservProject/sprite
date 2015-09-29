#include "framebuf.h"
#include "window.h"

/* Ultra tense line drawing routines.		Vaughn Pratt */

#define MOV(reg,sign) (0x3000|reg|sign)  /* builds a movw instruction */
#define RX 0xA00	/* a5 holds rx address */
#define RY 0x800	/* a4 holds ry address */
#define POS 0xc0	/* an@+ scans positively */
#define NEG 0x100	/* an@- scans negatively */

extern bres();

unsigned short mincmd[8] = {
	MOV(RY,POS), MOV(RX,POS),
	MOV(RY,POS), MOV(RX,NEG),
	MOV(RY,NEG), MOV(RX,POS),
	MOV(RY,NEG), MOV(RX,NEG)
};

unsigned short majcmd[8] = {
	MOV(RX,POS), MOV(RY,POS),
	MOV(RX,NEG), MOV(RY,POS),
	MOV(RX,POS), MOV(RY,NEG),
	MOV(RX,NEG), MOV(RY,NEG)
};

vec(x0,y0,x1,y1)
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
	rx = (unsigned short *) (GXBase + GXsource + GXselectx) + x0;
	ry = (unsigned short *) (GXBase + GXsource + GXselecty) + y0;


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
