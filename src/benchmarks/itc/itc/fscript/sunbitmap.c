/*
   Device dependent core graphics driver for SUN bitmap display
   15-Aug-82 by Mike Shantz.

*/
#include	"coretypes.h"
#include	"corevars.h"
#include	"framebuf.h"

	typedef unsigned char uchar;
	static short ddcp[2];		/* device coords current point */
	static short msklib[] = {
	    0x0000, 0x8000, 0x8080, 0x8410, 0x8888, 0x9124, 0x9494, 0xA552,
	    0xAAAA, 0xEB6E, 0xDDDD, 0xF7F7, 0xFFFF, 0xE3E3, 0xCE39, 0xF80F
	    };
	static short msklist[16];	/* 16x16 texture bit mask */
	static short TXfunc, RASfunc;	/* GXfunc for region fill, raster */
	static short LIintens;		/* GXfunc for lines */
	static short TEXfunc;		/* GXfunc for raster text */
	static int texture; 		/* set the region fill texture */
	static short amask, bmask, alin, blin, asht, bsht;
	static short a, b, cycl;   int i;
	static uchar red[256], green[256], blue[256];

/*------------------------------------*/
sunbitmap(ddstruct) register ddargtype *ddstruct;
{
    register short i1,i2,i3;
    rast_type *rptr;
    ipt_type *p1ptr, *p2ptr;
    int x,y,xs,ys,width,height;

	switch(ddstruct->opcode) {
	case INITIAL:
		if (GXopen()) exit(1);
		GXcontrol = GXvideoEnable;
		GXwidth = 1;
		TXfunc = GXMASK;		/* region fill function */
		RASfunc = GXSOURCE;		/* raster put function */
		TEXfunc = ~GXSOURCE & GXDEST;
		LIintens = GXclear;
		GXfunction = LIintens;
		for (i1=0; i1<256; i1++) {
		    red[i1] =   i1;
		    green[i1] = 255 - i1;
		    blue[i1] =  i1;
		    }
		break;
	case TERMINATE:
		GXclose();
		break;
	case CLEAR:
		GXfunction = GXset;
		ROPds( 0,0,1024,1024);		/* clear the screen to 1 */
		GXfunction = LIintens;
		break;
	case GETTAB:				/* Get color table  GAK! */
		for (i1=ddstruct->int1; i1<=ddstruct->int2; i1++) {
		    *((float*)ddstruct->ptr1) = ((float)red[i1]) / 255.;
		    *((float*)ddstruct->ptr2) = ((float)green[i1]) / 255.;
		    *((float*)ddstruct->ptr3) = ((float)blue[i1]) / 255.;
		    ddstruct->ptr1 += sizeof(float);
		    ddstruct->ptr2 += sizeof(float);
		    ddstruct->ptr3 += sizeof(float);
		    }
		break;
	case SETTAB:				/* Set color table  GAK! */
		for (i1=ddstruct->int1; i1<=ddstruct->int2; i1++) {
		    red[i1]  = (uchar)(*((float *)(ddstruct->ptr1)) * 255.);
		    green[i1]= (uchar)(*((float *)(ddstruct->ptr2)) * 255.);
		    blue[i1] = (uchar)(*((float *)(ddstruct->ptr3)) * 255.);
		    ddstruct->ptr1 += sizeof(float);
		    ddstruct->ptr2 += sizeof(float);
		    ddstruct->ptr3 += sizeof(float);
		    }
		break;
	case SETCOL:
		/* make textures for polygon fill */
		texture =  (int)(red[ddstruct->int1]) |
			  ((int)(green[ddstruct->int1]) << 8) |
			  ((int)(blue[ddstruct->int1]) << 16);
		if (ddstruct->logical) {
		    LIintens = GXinvert;
		    TXfunc =   GXMASK ^ GXDEST;
		    RASfunc =  ~GXSOURCE ^ GXDEST;
		    TEXfunc =  GXSOURCE ^ GXDEST;
		    }
		else {
		    LIintens = (ddstruct->int1 > 0) ? GXclear : GXset;
		    TXfunc = (texture > 0) ? ~GXMASK : GXset;
		    RASfunc = (ddstruct->int1 > 0) ? GXSOURCE & GXDEST
						   : ~GXSOURCE | GXDEST;
		    TEXfunc = (ddstruct->int1 > 0) ? ~GXSOURCE & GXDEST
						   : GXSOURCE | GXDEST;
		    }
		amask = msklib[ texture & 0xF];
		bmask = msklib[ (texture >> 4) & 0xF];
		alin  = (texture >> 8) & 0xF;
		blin  = (texture >> 12) & 0xF;
		cycl  = alin + blin;
		asht  = (texture >> 16) & 0xF;
		bsht  = (texture >> 20) & 0xF;
		if (cycl == 0) alin = 1;
		cycl = alin + blin;
		if (cycl > 16) {
		    blin = 16 - alin; cycl = alin + blin;
		    }
		a = amask;  b = amask;
		for (i=0; i<alin; i++) {
		    msklist[i] = a | b;
		    if (asht & 1) a = ((a >> 1) & 0x7FFF) | (a << 15);
		    if (asht & 2) a = (a << 1) | ((a >> 15) & 0x1);
		    if (asht & 4) b = ((b >> 1) & 0x7FFF) | (b << 15);
		    if (asht & 8) b = (b << 1) | ((b >> 15) & 0x1);
		    }
		a = bmask;  b = bmask;
		for (i=alin; i<cycl; i++) {
		    msklist[i] = a | b;
		    if (bsht & 1) a = ((a >> 1) & 0x7FFF) | (a << 15);
		    if (bsht & 2) a = (a << 1) | ((a >> 15) & 0x1);
		    if (bsht & 4) b = ((b >> 1) & 0x7FFF) | (b << 15);
		    if (bsht & 8) b = (b << 1) | ((b >> 15) & 0x1);
		    }
		i1 = 0;
		for (i=cycl; i<16; i++) {
		    msklist[i] = msklist[ i1++]; 
		    }
		break;
	case MOVE:
		ddcp[0]  = (ddstruct->int1 + 4) >> 3;
		ddcp[1]  = (ddstruct->int2 + 2048) >> 3;
		break;
	case LINE:
		i1 =  (ddstruct->int1 + 4) >> 3;
		i2 =  (ddstruct->int2 + 2048) >> 3;
		GXfunction = LIintens;
		vec(ddcp[0], ddcp[1], i1, i2);
		ddcp[0] = i1;
		ddcp[1] = i2;
		break;
	case TEXT:
		GXfunction = TEXfunc;
		drawstr( ddcp[0], 1023-ddcp[1], ddstruct->int1%3,
		ddstruct->ptr1);
		break;
	case POLYGN2:
		i1 =  ddstruct->int1;
		i2 =  ddstruct->int2;
		for (i=i1; i<i2+i1; i++) {
		    ddvtxlist[i].x = (vtxlist[i].x + 4) >> 3;
		    ddvtxlist[i].y = (vtxlist[i].y + 2048) >> 3;
		    }
		GXfunction = TXfunc;
		region2( &ddvtxlist[i1], i2, msklist);
		break;
	case RASPUT:
		rptr = (rast_type*)ddstruct->ptr1;
		p1ptr = (ipt_type*)ddstruct->ptr2;
		p2ptr = (ipt_type*)ddstruct->ptr3;
		i1 = p1ptr->x >> 3;			/* viewport xmin */
		i2 = p2ptr->x >> 3;			/* viewport xmax */
		width = rptr->width;
		xs = 0; x = ddcp[0];
		if (x<i1){ xs=i1-x; width-=xs; x+=xs;}
		if (x+width>i2) width=i2-x;
		if (width<=0) break;
		i1 = (p1ptr->y+2048) >> 3;		/* viewport ymin */
		i2 = (p2ptr->y+2048) >> 3;		/* viewport ymax */
		height = rptr->height;
		ys = 0; y = ddcp[1];
		if (y+height>i2) { ys=(y+height)-i2; height-=ys;}
		if (y<i1) { height-=i1-y; y=i1;}
		if (height<=0) break;
		GXfunction = RASfunc;
		ROPms( xs,ys,rptr,x, 1023-y-height, width, height);
		break;
	default:
		break;
	}
}
