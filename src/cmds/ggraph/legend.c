#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <math.h>
#include "ggraph.h"
#include "ggraphdefs.h"

/****************************************************************
 *								*
 *	drawlegend - draw a legend				*
 *								*
 ****************************************************************/
drawlegend()
{
    register int i;
    float legendlx, legendly;
    int legend_line_len;

    if(legendbox)
      drawlbox();
    legendlx = cg.lframe.frame1x + 10.0;
    legendly = cg.lframe.frame1y+10.0;
    for (curline = 0; ((curline != cg.maxlines) && (cl != NULL)); ++curline)
      if(cl->lonoff){
        drawctext(legendlx+10.0, legendly, cl->llelabel.t_font,
	  cl->llelabel.t_size, cl->llelabel.t_text, BOTLEFT_TEXT);
	legend_line_len = xcharsz[cl->llelabel.t_size] * 
	  strlen(cl->llelabel.t_text);
	drawline(cl->ltype, legendlx+10.0, legendly-3.0,
	  legendlx+legend_line_len+10, legendly-3.0);
	draw_symbol(cl->mtype, legendlx, legendly+5.0);
	legendly = legendly + cg.ylabel.t_size * 15.0 * 0.6;
      }
    if (cg.legend.t_text[0] == NULL)
      strcpy (cg.legend.t_text, "Legend");
    drawctext((cg.lframe.frame1x+cg.lframe.frame3x)/2.0, 
      cg.lframe.frame2y - 3.0, cg.legend.t_font, cg.legend.t_size,
      cg.legend.t_text, TOPCENTER_TEXT);
    return;
}

/****************************************************************
 *								*
 *	drawlbox - draw a frame around the legned		*
 *								*
 ****************************************************************/
drawlbox()
{
    drawline (cg.lframe.fsize, cg.lframe.frame1x, cg.lframe.frame1y, 
      cg.lframe.frame2x, cg.lframe.frame2y);
    drawline (cg.lframe.fsize, cg.lframe.frame2x, cg.lframe.frame2y, 
      cg.lframe.frame3x, cg.lframe.frame3y);
    drawline (cg.lframe.fsize, cg.lframe.frame3x, cg.lframe.frame3y, 
      cg.lframe.frame4x, cg.lframe.frame4y);
    drawline (cg.lframe.fsize, cg.lframe.frame4x, cg.lframe.frame4y, 
      cg.lframe.frame1x, cg.lframe.frame1y);
    return;
}
/****************************************************************
 *								*
 *	calc_legend - calculate legend parameters		*
 *								*
 ****************************************************************/
calc_legend(plotx, ploty)
float *plotx;
float *ploty;
{
    int lelen, maxlelen;
    float leg_width;

    maxlelen = 0;
    cg.lframe.fsize = BRUSH_THIN;	/* set brush size */
    for(curline=0;((curline!=MAXLINES) && (cl != NULL));++curline){
      if(cl->lonoff)
	if (cl->llelabel.t_text[0] == NULL)
	  strcpy (cl->llelabel.t_text, cl->lname); 
	  lelen = strlen (cl->llelabel.t_text); 
	  if(maxlelen < lelen)
	    maxlelen = lelen;
    }
    if(legendside){
/* should compute length of longest text string and size box */
/* accordingly */
	*plotx = XPLOTMAX - (maxlelen * 10.0) + 20.0;	/* figure out where is starts */
	if(!*ploty)
	  *ploty = YSCREENMIN+(ycharsz[cg.ylabel.t_size] * 3.0);
	cg.lframe.frame1x = *plotx+20.0;	/* set legend boundaries */
	cg.lframe.frame2x = cg.lframe.frame1x;
	cg.lframe.frame3x = XSCREENMAX - 5;
	cg.lframe.frame4x = cg.lframe.frame3x;
	cg.lframe.frame1y = cg.yorigin;
	cg.lframe.frame2y = ((cg.maxlines * ycharsz[cg.ylabel.t_size])+
	  (3.0*ycharsz[cg.legend.t_size]))+cg.lframe.frame1y;
	cg.lframe.frame3y = cg.lframe.frame2y;
	cg.lframe.frame4y = cg.lframe.frame1y;
    }else{
	*plotx = XPLOTMAX;		/* figure out where is starts */
	if(!*ploty)
	  *ploty = YSCREENMIN+((cg.maxlines * ycharsz[cg.ylabel.t_size] *2.0)+
	  (4.0*ycharsz[cg.legend.t_size]));

	leg_width = (maxlelen * xcharsz[cg.legend.t_size]) + 20.0;	/* figure out where is starts */

	cg.lframe.frame1x = ((XSCREENMAX - XSCREENMIN) - leg_width)/2.0
	  + XSCREENMIN - 10.0;	/* set legend boundaries */
	cg.lframe.frame2x = cg.lframe.frame1x;
	cg.lframe.frame3x = cg.lframe.frame1x + leg_width + 10.0;
	cg.lframe.frame4x = cg.lframe.frame3x;
	cg.lframe.frame1y = YSCREENMIN+5;
	cg.lframe.frame2y = ((cg.maxlines * ycharsz[cg.ylabel.t_size])+
	  (3.0*ycharsz[cg.legend.t_size]))+cg.lframe.frame1y;
	cg.lframe.frame3y = cg.lframe.frame2y;
	cg.lframe.frame4y = cg.lframe.frame1y;
    }
    return;
}
