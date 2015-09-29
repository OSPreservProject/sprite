#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <math.h>
#include "ggraph.h"
#include "ggraphdefs.h"

/****************************************************************
 *								*
 *	drawaxis - draw the axis for the graph			*
 *		xlabel - the label for the X axis		*
 *		ylabel - the label for the Y axis		*
 *		axistype - the line type to use			*
 *		xticks - number of ticks on X axis		*
 *		yticks - number of ticks on Y axis		*
 *		dxticks - delta for ticks on X axis		*
 *		dyticks - delta for ticks on Y axis		*
 *		sxtick - starting value for ticks on X axis	*
 *		sytick - starting value for ticks on Y axis	*
 *								*
 ****************************************************************/
drawaxis()
{
    register int i;
    char    s[20];

 /* draw X axis */
    if (xaxisf)
      if(crossxsw)
	drawline (cg.axline_type, cg.xorigin, 
	  (0.0 - cg.yoffset)*cg.scaley+cg.yorigin,
	   cg.xorigin + cg.numtickx * cg.dtickx * cg.scalex, 
	    (0.0 - cg.yoffset)*cg.scaley+cg.yorigin);
      else
	drawline (cg.axline_type, cg.xorigin, cg.yorigin,
	    cg.xorigin + cg.numtickx * cg.dtickx * cg.scalex, cg.yorigin);
 /* draw Y axis */
    if (yaxisf)
      if(crossysw)
	drawline (cg.axline_type, (0.0 - cg.xoffset)*cg.scalex+cg.xorigin, 
	  cg.yorigin, (0.0 - cg.xoffset)*cg.scalex+cg.xorigin,
          cg.yorigin + cg.numticky * cg.dticky * cg.scaley);
      else
	drawline (cg.axline_type, cg.xorigin, cg.yorigin,
	    cg.xorigin, cg.yorigin + cg.numticky * cg.dticky * cg.scaley);
    for (i = cg.firstxsw; i <= cg.numtickx; i++) {/* draw X grid lines */
    /* draw a tick mark */
	if (xtickf)
	  if(crossxsw)
	    drawline (cg.axline_type, cg.xorigin + (i * cg.dtickx * cg.scalex),
		    (0.0 - cg.yoffset)*cg.scaley+cg.yorigin, cg.xorigin +
		    (i * cg.dtickx * cg.scalex), (0.0 - cg.yoffset)*cg.scaley+cg.yorigin - 4.0);
	  else
	    drawline (cg.axline_type, cg.xorigin + (i * cg.dtickx * cg.scalex),
		    cg.yorigin, cg.xorigin +
		    (i * cg.dtickx * cg.scalex), cg.yorigin - 4.0);
    /* draw a dotted grid line */
	if (xgridf)
	    drawline (cg.xgrid_type, cg.xorigin + (i * cg.dtickx * cg.scalex),
		    cg.yorigin, cg.xorigin + (i * cg.dtickx * cg.scalex),
		    cg.yorigin + (cg.numticky * cg.dticky * cg.scaley));
    /* write tick label */
	if (xticklf) {
	    if (cg.xpreci2 != -1)
	      if(cg.logxsw){
		sprintf (s, "%*.*f",  cg.xpreci1, cg.xpreci2,
(pow((double)cg.logxtick, (double)(i+(int)(cg.xoffset)))));
if(debug) printf("xtick %s %f %d %d\n", s, cg.logxtick, i, (int)(cg.xoffset)); 
	      }else
		sprintf (s, "%*.*f",cg.xpreci1, cg.xpreci2,
		  (i * cg.dtickx) + cg.stickx);
	    else
	      if(cg.logxsw){
		sprintf (s, "%d", (int)(0.5 + 
		  pow((double)cg.logxtick, (double)(i+(int)cg.xoffset))));
if(debug) printf("xtick %s %f %d %d\n", s, cg.logxtick, i, (int)(cg.xoffset)); 
	      }else
		sprintf (s, "%d", (int) ((i * cg.dtickx) + cg.stickx));
	      if(crossxsw)
		drawctext (cg.xorigin + (i * cg.dtickx * cg.scalex),
		    (0.0 - cg.yoffset)*cg.scaley+cg.yorigin - 10.0,
		    cg.xtick.t_font, cg.xtick.t_size, s, TOPCENTER_TEXT);
	      else
		drawctext (cg.xorigin + (i * cg.dtickx * cg.scalex),
		    cg.yorigin - 10.0, cg.xtick.t_font, cg.xtick.t_size,
			 s, TOPCENTER_TEXT);

	}
    }
    for (i = cg.firstysw; i <= cg.numticky; i++) {/* draw Y grid lines */
    /* draw tick marks */
	if (ytickf)
	  if(crossysw)
	    drawline (cg.axline_type, (0.0 - cg.xoffset)*cg.scalex+cg.xorigin, cg.yorigin +
		    (i * cg.dticky * cg.scaley), (0.0 - cg.xoffset)*cg.scalex+cg.xorigin - 4.0,
		    cg.yorigin + (i * cg.dticky * cg.scaley));
	else
	    drawline (cg.axline_type, cg.xorigin, cg.yorigin +
		    (i * cg.dticky * cg.scaley), cg.xorigin - 4.0,
		    cg.yorigin + (i * cg.dticky * cg.scaley));

    /* draw grid */
	if (ygridf)
	    drawline (cg.ygrid_type, cg.xorigin,
		    cg.yorigin + (i * cg.dticky * cg.scaley),
		    cg.xorigin + cg.numtickx * cg.dtickx * cg.scalex,
		    cg.yorigin + (i * cg.dticky * cg.scaley));
    /* draw tick labels */
	if (yticklf) {
	    if (cg.ypreci2 != -1)
	      if(cg.logysw){
		sprintf (s, "%*.*f",  cg.ypreci1, cg.ypreci2,
(pow((double)cg.logytick, (double)(i+(int)cg.yoffset))));
if(debug) printf("ytick %s %f %d %d\n", s, cg.logytick, i, (int)(cg.yoffset)); 
	      }else
		sprintf (s, "%*.*f", cg.ypreci1, cg.ypreci2,
		          (i * cg.dticky) + cg.sticky);
	    else
	      if(cg.logysw){
		sprintf (s, "%d", (int)(0.5 + pow((double)cg.logytick,
		 (double)(i+(int)cg.yoffset))));
if(debug)printf("ytick %s %f %d %d\n", s, cg.logytick, i, (int)(cg.yoffset));
	      }else
		sprintf (s, "%d", (int) ((i * cg.dticky) + cg.sticky));
	      if(crossysw)
		drawctext ((0.0 - cg.xoffset)*cg.scalex+cg.xorigin - 8.0,
		  cg.yorigin + ((i * cg.dticky * cg.scaley) + 8.0),
		    cg.ytick.t_font, cg.ytick.t_size, s, TOPRIGHT_TEXT);
	      else
		drawctext (cg.xorigin - 8.0,
		    cg.yorigin + ((i * cg.dticky * cg.scaley) + 8.0),
		    cg.ytick.t_font, cg.ytick.t_size, s, TOPRIGHT_TEXT);

	}
    }

 /* draw X label */
    drawctext (cg.xlabel.t_xpos, cg.xlabel.t_ypos, cg.xlabel.t_font,
      cg.xlabel.t_size, cg.xlabel.t_text, TOPCENTER_TEXT);
 /* draw Y label */
    if(!cg.yvert)
      drawctext (cg.ylabel.t_xpos, cg.ylabel.t_ypos,
	cg.ylabel.t_font, cg.ylabel.t_size, cg.ylabel.t_text, TOPCENTER_TEXT);
    else{			/* compute spacing for text */
	drawvtext (cg.ylabel.t_xpos, cg.ylabel.t_ypos, cg.ylabel.t_font, cg.ylabel.t_size,
	  cg.ylabel.t_text, ycharsz[cg.ylabel.t_size]);
    }
}

/****************************************************************
 *								*
 *	drawframe - draw a frame around the graph		*
 *								*
 ****************************************************************/
drawframe()
{
    drawline (cg.gframe.fsize, cg.gframe.frame1x, cg.gframe.frame1y,
      cg.gframe.frame2x, cg.gframe.frame2y);
    drawline (cg.gframe.fsize, cg.gframe.frame2x, cg.gframe.frame2y,
      cg.gframe.frame3x, cg.gframe.frame3y);
    drawline (cg.gframe.fsize, cg.gframe.frame3x, cg.gframe.frame3y,
      cg.gframe.frame4x, cg.gframe.frame4y);
    drawline (cg.gframe.fsize, cg.gframe.frame4x, cg.gframe.frame4y,
      cg.gframe.frame1x, cg.gframe.frame1y);
    return;
}
