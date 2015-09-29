/*
 *                      D R A W P S . C 
 * (minor mods by don on 5 Jan 90 to accommodate highres PostScript)
 *
 * $Revision: 1.4 $
 *
 * $Log:        drawPS.c,v $
 *
 * 89/04/24: decouty@irisa.fr, priol@irisa.fr
 *              added three shading levels for fig,
 *              removed '\n' in cmdout() calls
 *
 * Revision 1.4  87/05/07  15:06:24  dorab
 * relinked back hh to h and vv to v undoing a previous change.
 * this was causing subtle quantization problems. the main change
 * is in the definition of hconvPS and vconvPS.
 * 
 * changed the handling of the UC seal. the PS file should now be
 * sent via the -i option.
 * 
 * Revision 1.3  86/04/29  23:20:55  dorab
 * first general release
 * 
 * Revision 1.3  86/04/29  22:59:21  dorab
 * first general release
 * 
 * Revision 1.2  86/04/29  13:23:40  dorab
 * Added distinctive RCS header
 * 
 */
#ifndef lint
static char RCSid[] = 
  "@(#)$Header: drawPS.c,v 1.4 87/05/07 15:06:24 dorab Exp $ (UCLA)";
#endif

/*
 the driver for handling the \special commands put out by
 the tpic program as modified by Tim Morgan <morgan@uci.edu>
 the co-ordinate system is with the origin at the top left
 and the x-axis is to the right, and the y-axis is to the bottom.
 when these routines are called, the origin is set at the last dvi position,
 which has to be gotten from the dvi-driver (in this case, dvips) and will
 have to be converted to device co-ordinates (in this case, by [hv]convPS).
 all dimensions are given in milli-inches and must be converted to what
 dvips has set up (i.e. there are convRESOLUTION units per inch).

 it handles the following \special commands
    pn n                        set pen size to n
    pa x y                      add path segment to (x,y)
    fp                          flush path
    da l                        flush dashed - each dash is l (inches)
    dt l                        flush dotted - one dot per l (inches)
    sp                          flush spline
    ar x y xr yr sa ea          arc centered at (x,y) with x-radius xr
                                and y-radius yr, start angle sa (in rads),
                                end angle ea (in rads)
    sh                          shade last path (box, circle, ellipse)
    wh                          whiten last path (box, circle, ellipse)
    bk                          blacken last path (box,circle, ellipse)
    tx                          texture command - Added by T. Priol (priol@irisa.fr)

  this code is in the public domain

  written by dorab patel <dorab@cs.ucla.edu>
  december 1985
  released feb 1986
  changes for dvips july 1987

  */

#ifdef TPIC                     /* rest of the file !! */

#include "structures.h"

#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */


/*
 * external functions used here
 */
extern void cmdout();
extern void numout();
extern void error();

/*
 * external variables used here
 */
extern integer hh,vv;           /* the current x,y position in pixel units */
extern int actualdpi ;

#define convRESOLUTION DPI
#define TRUE 1
#define FALSE 0
#define tpicRESOLUTION 1000     /* number of tpic units per inch */

/* convert from tpic units to PS units */
#define PixRound(a,b) zPixRound((integer)(a),(integer)(b))
#define convPS(x) PixRound((x)*convRESOLUTION,tpicRESOLUTION)

/* convert from tpic locn to abs horiz PS locn */
#define hconvPS(x) (integer)(hh + convPS(x))
/* convert from tpic locn to abs vert PS locn */
#define vconvPS(x) (integer)(vv + convPS(x))
#define convDeg(x) (integer)(360*(x)/(2*3.14159265358)+0.5) /* convert to degs */

/* if PostScript had splines, i wouldnt need to store the path */
#define MAXPATHS 300            /* maximum number of path segments */
#define NONE 0                  /* for shading */
#define BLACK           1       /* for shading */
#define DARK_GRAY       2       /* for shading */
#define MEDIUM_GRAY     3       /* for shading */
#define LIGHT_GRAY      4       /* for shading */
#define WHITE           5       /* for shading */

/* the following defines are used to make the PostScript shorter;
  the corresponding defines are in the PostScript prolog special.lpro */
#define MOVETO "a"
#define LINETO "li"
#define RCURVETO "rc"
#define RLINETO "rl"
#define STROKE "st"
#define FILL "fil"
#define NEWPATH "np"
#define CLOSEPATH "closepath"
/*
 * STROKE and FILL must restore the current point to that
 * saved by NEWPATH
 */

static integer xx[MAXPATHS], yy[MAXPATHS]; /* the current path in milli-inches */
static integer pathLen = 0;             /* the current path length */
static integer shading = NONE;  /* what to shade the last figure */
static integer penSize = 2;             /* pen size in PS units */

/* forward declarations */
static void doShading();
static integer zPixRound();             /* (int)(x/y)PixRound((int)x,(int)y) */
static double  shadetp;                 /* shading level        */

void
setPenSize(cp)
     char *cp;
{
  long ps;

  if (sscanf(cp, " %ld ", &ps) != 1) 
    {
      error("Illegal .ps command format");
      return;
    }

  penSize = convPS(ps);
  numout((integer)penSize);
  cmdout("setlinewidth");
}                               /* end setPenSize */

void
addPath(cp)
     char *cp;
{
  long x,y;

  if (++pathLen >= MAXPATHS) error("! Too many points");
  if (sscanf(cp, " %ld %ld ", &x, &y) != 2)
    error("! Malformed path expression");
  xx[pathLen] = x;
  yy[pathLen] = y;
}                               /* end of addPath */

void
arc(cp)
     char *cp;
{
  long xc, yc, xrad, yrad;
  float startAngle, endAngle;

  if (sscanf(cp, " %ld %ld %ld %ld %f %f ", &xc, &yc, &xrad, &yrad,
             &startAngle, &endAngle) != 6)
    {
      error("Illegal arc specification");
      return;
    }

/* we need the newpath since STROKE doesnt do a newpath */

  cmdout(NEWPATH);
  numout((integer)hconvPS(xc));
  numout((integer)vconvPS(yc));
  numout((integer)convPS(xrad));
  if (xrad != yrad)
    numout((integer)convPS(yrad));
  numout((integer)convDeg(startAngle));
  numout((integer)convDeg(endAngle));

  if (xrad == yrad)             /* for arcs and circles */
    cmdout("arc");
  else
    cmdout("ellipse");

  doShading();
}                               /* end of arc */

void
flushPath()
{
  register int i;

  if (pathLen < 2) 
    {
      error("Path less than 2 points - ignored");
      return;
    }
  
#ifdef DEBUG
    if (dd(D_SPECIAL))
        (void)fprintf(stderr,
#ifdef SHORTINT
            "flushpath(1): hh=%ld, vv=%ld, x=%ld, y=%ld, xPS=%ld, yPS=%ld\n",
#else   /* ~SHORTINT */
            "flushpath(1): hh=%d, vv=%d, x=%d, y=%d, xPS=%d, yPS=%d\n",
#endif  /* ~SHORTINT */
                    hh, vv, xx[1], yy[1], hconvPS(xx[1]), vconvPS(yy[1]));
#endif /* DEBUG */
  cmdout(NEWPATH); /* to save the current point */
  numout((integer)hconvPS(xx[1]));
  numout((integer)vconvPS(yy[1]));
  cmdout(MOVETO);
  for (i=2; i < pathLen; i++) {
#ifdef DEBUG
    if (dd(D_SPECIAL))
        (void)fprintf(stderr,
#ifdef SHORTINT
            "flushpath(%ld): hh=%ld, vv=%ld, x=%ld, y=%ld, xPS=%ld, yPS=%ld\n",
#else   /* ~SHORTINT */
            "flushpath(%d): hh=%d, vv=%d, x=%d, y=%d, xPS=%d, yPS=%d\n",
#endif  /* ~SHORTINT */
                    i, hh, vv, xx[i], yy[i], hconvPS(xx[i]), vconvPS(yy[i]));
#endif /* DEBUG */
    numout((integer)hconvPS(xx[i]));
    numout((integer)vconvPS(yy[i]));
    cmdout(LINETO);
    }
  if (xx[1] == xx[pathLen] && yy[1] == yy[pathLen])
    cmdout(CLOSEPATH);
  else {
    numout((integer)hconvPS(xx[pathLen]));
    numout((integer)vconvPS(yy[pathLen]));
    cmdout(LINETO);
  }
  doShading();
  pathLen = 0;
}                               /* end of flushPath */

void
flushDashed(cp, dotted)
     char *cp;
     int dotted;
{
  float inchesPerDash;

  if (sscanf(cp, " %f ", &inchesPerDash) != 1) 
    {
      error ("Illegal format for dotted/dashed line");
      return;
    }
  
  if (inchesPerDash <= 0.0)
    {
      error ("Length of dash/dot cannot be negative");
      return;
    }

  inchesPerDash = 1000 * inchesPerDash; /* to get milli-inches */
  
  cmdout("[");
  if (dotted) {
    numout((integer)penSize);
    numout((integer)convPS((int)inchesPerDash) - penSize);
  } else                                /* if dashed */
    numout((integer)convPS((int)inchesPerDash));

  cmdout("]");
  numout((integer)0);
  cmdout("setdash");

  flushPath();

  cmdout("[] 0 setdash");
}                               /* end of flushDashed */

void
flushSpline()
{                               /* as exact as psdit!!! */
  register long i, dxi, dyi, dxi1, dyi1;

  if (pathLen < 2)
    {
      error("Spline less than two points - ignored");
      return;
    }
  
  cmdout(NEWPATH);      /* to save the current point */
  numout((integer)hconvPS(xx[1]));
  numout((integer)vconvPS(yy[1]));
  cmdout(MOVETO);
  numout((integer)convPS((xx[2]-xx[1])/2));
  numout((integer)convPS((yy[2]-yy[1])/2));
  cmdout(RLINETO);

  for (i=2; i < pathLen; i++)
    {
      dxi = convPS(xx[i] - xx[i-1]);
      dyi = convPS(yy[i] - yy[i-1]);
      dxi1 = convPS(xx[i+1] - xx[i]);
      dyi1 = convPS(yy[i+1] - yy[i]);

      numout((integer)dxi/3);
      numout((integer)dyi/3);
      numout((integer)(3*dxi+dxi1)/6);
      numout((integer)(3*dyi+dyi1)/6);
      numout((integer)(dxi+dxi1)/2);
      numout((integer)(dyi+dyi1)/2);
      cmdout(RCURVETO);
    }

  numout((integer)hconvPS(xx[pathLen]));
  numout((integer)vconvPS(yy[pathLen]));
  cmdout(LINETO);

  doShading();
  pathLen = 0;
         
}                               /* end of flushSpline */

/* set shading level (used with  Fig 1.4-TFX). priol@irisa.fr, 89/04 */
void
SetShade(cp)
char *cp;
{
        
 if (strncmp(cp,"55",2) == 0){
         shadetp = 0.25; shading = LIGHT_GRAY;
 }
  else if (strncmp(cp,"cc",2) == 0){
          shadetp = 0.50; shading = MEDIUM_GRAY;
  }
   else if (strncmp(cp,"c0",2) == 0){
           shadetp = 0.75; shading = DARK_GRAY;
   }       
}                               /* end of SetShade       */

void
shadeLast()
{
  char tpout[20];
  shading = MEDIUM_GRAY;
  sprintf(tpout,"%1.2f setgray",shadetp); /* priol@irisa.fr */
  cmdout(tpout);
}                               /* end of shadeLast */

void
whitenLast()
{
  shading = WHITE;
  cmdout("1 setgray");
}                               /* end of whitenLast */

void
blackenLast()
{
  shading = BLACK;
  cmdout("0 setgray");          /* actually this aint needed */
}                               /* end of whitenLast */

static void
doShading()
{
  if (shading) 
    {
      cmdout(FILL);
      shading = NONE;
      cmdout("0 setgray");      /* default of black */
    }
  else
    cmdout(STROKE);
}                               /* end of doShading */

static integer
zPixRound(x, conv)      /* return rounded number of pixels */
        register integer x;             /* in DVI units */
        register integer conv;          /* conversion factor */
{
    return((integer)((x + (conv >> 1)) / conv));
}

#endif /* TPIC */

