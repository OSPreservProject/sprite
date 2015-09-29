/*
   Device dependent core graphics driver for SUN color display
   4-Nov-82 by Mike Shantz.

*/
#include	"coretypes.h"
#include	"corevars.h"
#include "colorbuf.h"

	static short ddcp[2];		/* device coords current point */
	static short LIintens, TEXfunc;	/* GXfunc for lines, raster text */
	static short TXfunc, RASfunc;	/* GXfunc for region fill, raster */
	static uchar red[256], green[256], blue[256];
	static short colorindex;

/*------------------------------------*/
suncolor(ddstruct) register ddargtype *ddstruct;
{
    register short i1,i2,i3;
	int i, d;
	rast_type *rptr;
	ipt_type *p1ptr, *p2ptr;
        int x,y,xs,ys,width,height;

	switch(ddstruct->opcode) {
	case INITIAL:
		if ( cgxopen()) exit(1);
		*GR_sreg = GR_disp_on;
		TXfunc = LIintens = GR_Mask;	/* region fill function */
		TEXfunc = ~GR_Source;		/* raster text function */
		RASfunc = GR_Source;		/* raster put function */
		for (i1=0; i1<256; i1++) {	/* load color map 0 */
			red[i1] = 0; green[i1] = 0; blue[i1] = 0;
			}
		red[0] = 150; green[0] = 150;	/* grey backgrnd */
		blue[0] = 150;
		for (i1=2; i1<64; i1++) red[i1] = (i1<<2);
		for (i1=64; i1<128; i1++) green[i1] = (i1-64)<<2;
		for (i1=128; i1<192; i1++) blue[i1] = (i1-128)<<2;
		for (i1=192; i1<256; i1++) {
			red[i1] = (i1-192)<<2; green[i1] = (i1-192)<<2;
			}
		write_cmap( red,green,blue,0);
		Set_Video_Cmap( 0);
		colorindex = 1;
		break;
	case TERMINATE:
		cgxclose( CGXFile);
		break;
	case CLEAR:
		*GR_freg = GR_clear;	
		COPds( 0,0,640,480);		/* clear the screen to 0 */
		break;
	case GETTAB:				/* Get color table GAK! */
		for (i1=ddstruct->int1; i1<=ddstruct->int2; i1++) {
		    *((float*)ddstruct->ptr1) = ((float)red[i1]) / 255.;
		    *((float*)ddstruct->ptr2) = ((float)green[i1]) / 255.;
		    *((float*)ddstruct->ptr3) = ((float)blue[i1]) / 255.;
		    ddstruct->ptr1 += sizeof(float);
		    ddstruct->ptr2 += sizeof(float);
		    ddstruct->ptr3 += sizeof(float);
		    }
		write_cmap( red,green,blue,0);
		break;
	case SETTAB:				/* Set color table GAK! */
		for (i1=ddstruct->int1; i1<=ddstruct->int2; i1++) {
		    red[i1] =  (uchar)*(float *)(ddstruct->ptr1) * 255;
		    green[i1]= (uchar)*(float *)(ddstruct->ptr2) * 255;
		    blue[i1] = (uchar)*(float *)(ddstruct->ptr3) * 255;
		    ddstruct->ptr1 += sizeof(float);
		    ddstruct->ptr2 += sizeof(float);
		    ddstruct->ptr3 += sizeof(float);
		    }
		write_cmap( red,green,blue,0);
		break;
	case SETCOL:
		/* select colors for polygon fill */
		colorindex = ddstruct->int1;
		if (ddstruct->logical) {
		    LIintens = TXfunc =   GR_Mask ^ GR_Dest;
		    RASfunc =  (GR_Source & GR_Mask) ^ GR_Dest;
		    TEXfunc =  GR_Source ^ GR_Dest;
		    }
		else {
		    if (ddstruct->int1) {
			LIintens = TXfunc = GR_Mask;
			TEXfunc = (GR_Source & GR_Mask)|(~GR_Source & GR_Dest);
			RASfunc = (~GR_Source & GR_Mask)|(GR_Source & GR_Dest);
			}
		    else {
			LIintens = TXfunc = GR_clear;
			TEXfunc = (~GR_Source & GR_Dest);
			RASfunc = GR_clear;
			}
		    }
		break;
	case MOVE:
		/* color frame buffer is 640 by 480 */
		ddcp[0] = (ddstruct->int1 + (ddstruct->int1 >> 2)) >> 4;
		ddcp[1] = (ddstruct->int2 + (ddstruct->int2 >> 2)) >> 4;
		break;
	case LINE:
		i1 = (ddstruct->int1 + (ddstruct->int1 >> 2)) >> 4;
		i2 = (ddstruct->int2 + (ddstruct->int2 >> 2)) >> 4;
		*GR_freg = LIintens;	
		*GR_creg = colorindex;	
		cvec(ddcp[0], 479-ddcp[1], i1, 479-i2);
		ddcp[0] = i1;
		ddcp[1] = i2;
		break;
	case TEXT:
		*GR_freg = TEXfunc;
		*GR_creg = colorindex;	
		colorstr( ddcp[0], 479-ddcp[1], ddstruct->int1%3,
		ddstruct->ptr1);
		break;
	case POLYGN2:
		i1 =  ddstruct->int1;
		i2 =  ddstruct->int2;
		for (i=i1; i<i2+i1; i++) {
		    d = vtxlist[i].x;
		    ddvtxlist[i].x = (d + (d >> 2)) >> 4;
		    d = vtxlist[i].y;
		    ddvtxlist[i].y = (d + (d >> 2)) >> 4;
		    }
		*GR_freg = TXfunc;
		*GR_creg = colorindex;
		cregion2( &ddvtxlist[i1], i2);
		break;
	case RASPUT:
		rptr = (rast_type*)ddstruct->ptr1;
		p1ptr = (ipt_type*)ddstruct->ptr2;
		p2ptr = (ipt_type*)ddstruct->ptr3;
		i1 = (p1ptr->x + (p1ptr->x >> 2)) >> 4;	/* viewport xmin */
		i2 = (p2ptr->x + (p2ptr->x >> 2)) >> 4;	/* viewport xmax */
		width = rptr->width;
		xs = 0; x = ddcp[0];
		if (x<i1){ xs=i1-x; width-=xs; x+=xs;}
		if (x+width>i2) width=i2-x;
		if (width<=0) break;
		i1 = (p1ptr->y + (p1ptr->y >> 2)) >> 4;	/* viewport ymin */
		i2 = (p2ptr->y + (p2ptr->y >> 2)) >> 4;	/* viewport ymax */
		height = rptr->height;
		ys = 0; y = ddcp[1];
		if (y+height>i2) { ys=(y+height)-i2; height-=ys;}
		if (y<i1) { height-=i1-y; y=i1;}
		if (height<=0) break;
		*GR_freg = RASfunc;
		*GR_creg = colorindex;
		COPms( xs,ys,rptr,x, 479-y-height, width, height);
		break;
	default:
		break;
	}
}
