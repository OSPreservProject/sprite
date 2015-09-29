#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <math.h>
#include "ggraph.h"
#include "ggraphdefs.h"

/****************************************************************
 *								*
 *	putcirclepoint - draw a circle around the point		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putcirclepoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "ARC\n");
    else
      fprintf (outfile, "%d\n", CIRCLE);
    fprintf(outfile, "%4.1f %4.1f\n", x, y);
    fprintf(outfile, "%4.1f %4.1f\n", x + (SQRT2 * cg.symbolsz), 
				 y + (SQRT2 * cg.symbolsz));
    fprintf(outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf(outfile, "%4.1f %4.1f\n", x, y - cg.symbolsz);
    fprintf(outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf(outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y);

    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	putboxpoint - draw a box around the point		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putboxpoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	putcrosspoint - draw a cross on the point		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putcrosspoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	putxpoint - draw a star on the point			*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putxpoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y - cg.symbolsz);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	puttripoint - draw a triangle on the point		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
puttripoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	pututripoint - draw an upside down triangle		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
pututripoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y - cg.symbolsz);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	putxboxpoint - draw a crossed box on the point		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putxboxpoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y - cg.symbolsz);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	putcboxpoint - draw a starred box on the point		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putcboxpoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x, y);
    fprintf (outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y - cg.symbolsz);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	putdiapoint - draw a diamond on the point		*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putdiapoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x , y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

/****************************************************************
 *								*
 *	putcdiapoint - draw a crossed diamond on the point	*
 *			    x, y - point to use as center	*
 *								*
 ****************************************************************/
putcdiapoint (x, y)
float   x,
        y;
{
    if(version == SUN_GREMLIN)
      fprintf (outfile, "VECTOR\n");
    else
      fprintf (outfile, "%d\n", LINE);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x , y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x, y + cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y - cg.symbolsz);
    fprintf (outfile, "%4.1f %4.1f\n", x, y);
    fprintf (outfile, "%4.1f %4.1f\n", x + cg.symbolsz, y);
    fprintf (outfile, "%4.1f %4.1f\n", x - cg.symbolsz, y);
    fprintf(outfile, (version == SUN_GREMLIN) ? "*\n" : "-1.00 -1.00\n");
    fprintf (outfile, "%d %d\n%d\n", BRUSH_NORMAL, 0, 0);
}

draw_symbol(symtype, symx, symy)
int symtype;
float symx, symy;
{
    switch (symtype) {
	case BOX: 
	    putboxpoint (symx, symy);
	    break;
	case RING: 
	    putcirclepoint (symx, symy);
	    break;
	case CROSS: 
	    putcrosspoint (symx, symy);
	    break;
	case STAR: 
	    putxpoint (symx, symy);
	    break;
	case TRIANGLE: 
	    puttripoint (symx, symy);
	    break;
	case UTRIANGLE: 
	    pututripoint (symx, symy);
	    break;
	case CROSSBOX: 
	    putcboxpoint (symx, symy);
	    break;
	case STARBOX: 
	    putxboxpoint (symx, symy);
	    break;
	case DIAMOND: 
	    putdiapoint (symx, symy);
	    break;
	case CROSSDIAMOND: 
	    putcdiapoint (symx, symy);
	    break;
	case NOSYMBOL: 
	default: 
	    break;
    }
}
