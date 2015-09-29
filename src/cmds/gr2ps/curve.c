/* 
 * This program is part of gr2ps.  It converts Gremlin's curve output to
 * control vertices of Bezier Cubics, as supported by PostScript.
 * Gremlin currently supports three kinds of curves:
 *	(1) cubic interpolated spline with
 *	     i) periodic end condition, if two end points coincide
 *	    ii) natural end condition, otherwise
 *	(2) uniform cubic B-spline with 
 *	     i) closed curve (no vertex interpolated), if end vertices coincide
 *	    ii) end vertex interpolation, otherwise
 *	(3) Bezier cubics
 *
 * The basic idea of the conversion algorithm for the first two is
 *	(1) take each curve segment's two end points as Bezier end vertices.
 *	(2) find two intermediate points in the orginal curve segment
 *	    (with u=1/4 and u=1/2, for example).
 *	(3) solve for the two intermediate control vertices.
 * The conversion between Bezier Cubics of Gremlin and that of PostScript
 * is straightforward.
 *
 * Author: Peehong Chen (phc@renoir.berkeley.edu)
 * Date: 9/17/1986
 */

#include <math.h>
#include <stdio.h>

#define MAXPOINTS	200
#define BezierMax	5
#define BC1		1.0/9		/* coefficient of Bezier conversion */
#define BC2		4*BC1
#define BC3		3*BC2
#define BC4		8*BC2

static double	Qx, Qy,
		x[MAXPOINTS],
		y[MAXPOINTS],
		h[MAXPOINTS],
		dx[MAXPOINTS],
		dy[MAXPOINTS],
		d2x[MAXPOINTS],
		d2y[MAXPOINTS],
		d3x[MAXPOINTS],
		d3y[MAXPOINTS];
static int	numpoints = 0;

struct point {
	double	p_x, p_y;
};


/* 
 * This routine copies the list of points into an array.
 */
static
MakePoints(count, list, output)
    struct point	*list;
    FILE		*output;
{
    register int	i;

    if (count > MAXPOINTS - 1) {
	error("warning: Too many points given, only first %d used.",
	      MAXPOINTS - 1);
	count = MAXPOINTS - 1;
    }

    /* Assign points from list to array for convenience of processing */
    for (i = 1; i <= count; i++) {
	x[i] = list[i - 1].p_x;
	y[i] = list[i - 1].p_y;
    }
    numpoints = count;
} /* end MakePoints */


/*
 * This routine converts each segment of a curve, P1, P2, P3, and P4
 * to a set of two intermediate control vertices, V2 and V3, in a Bezier
 * segment, plus a third vertex of the end point P4 (assuming the current
 * position is P1), and then writes a PostScript command "V2 V3 V4 curveto"
 * to the output file.
 * The two intermediate vertices are obtained using
 *    Q(u) = V1 * (1-u)^3 + V2 * 3u(1-u)^2 + V3 * 3(1-u)u^2 + V4 * u^3
 * with u=1/4, and u=1/2,
 *	Q(1/4) = Q2 = (x2, y2)
 *	Q(1/2) = Q3 = (x3, y3)
 *	V1 = P1
 *	V4 = P4
 * and
 *	V2 = (32/9)*Q2 - (4/3)*(Q3 + V1) + (1/9)*V4
 *	V3 = -(32/9)*Q2 + 4*Q3 + V1 - (4/9)*V4 
 */
static
BezierSegment(output, x1, y1, x2, y2, x3, y3, x4, y4)
    FILE	*output;
    double 	x1, y1, x2, y2, x3, y3, x4, y4;
{
    double	V2x, V2y, V3x, V3y;

    V2x = BC4 * x2 - BC3 * (x3 + x1) + BC1 * x4;
    V2y = BC4 * y2 - BC3 * (y3 + y1) + BC1 * y4;
    V3x = -BC4 * x2 + 4 * x3 +  x1 - BC2 * x4;
    V3y = -BC4 * y2 + 4 * y3 +  y1 - BC2 * y4;
    
    fprintf(output,
	"  %lg %lg %lg %lg %lg %lg curveto\n", V2x, V2y, V3x, V3y, x4, y4);
} /* end BezierSegment */


/* 
 * This routine calculates parameteric values for use in calculating
 * curves.  The values are an approximation of cumulative arc lengths 
 * of the curve (uses cord * length).  For additional information, 
 * see paper cited below.
 *
 * This is from Gremlin (called Paramaterize in gremlin),
 * with minor modifications (elimination of param list)
 *
 */
static
IS_Parameterize()
{
    register i, j;
    double t1, t2;
    double u[MAXPOINTS];

    for (i=1; i<=numpoints; ++i) {
	u[i] = 0.0;
	for (j=1; j<i; ++j) {
	    t1 = x[j+1] - x[j];
	    t2 = y[j+1] - y[j];
	    u[i] += (double) sqrt((double) ((t1 * t1) + (t2 * t2)));
	}
    }

    for (i=1; i<numpoints; ++i)
	h[i] = u[i+1] - u[i];
}  /* end IS_Parameterize */


/*
 * This routine solves for the cubic polynomial to fit a spline
 * curve to the the points  specified by the list of values.
 * The curve generated is periodic.  The alogrithms for this 
 * curve are from the "Spline Curve Techniques" paper cited below.
 *
 * This is from Gremlin (called PeriodicSpline in gremlin)
 *
 */
static
IS_PeriodicEnd(h, z, dz, d2z, d3z)
double h[MAXPOINTS];		/* Parameterizeaterization */
double z[MAXPOINTS];		/* point list */
double dz[MAXPOINTS];		/* to return the 1st derivative */
double d2z[MAXPOINTS];		/* 2nd derivative */
double d3z[MAXPOINTS];		/* and 3rd derivative */
{
    double a[MAXPOINTS]; 
    double b[MAXPOINTS]; 
    double c[MAXPOINTS]; 
    double d[MAXPOINTS]; 
    double deltaz[MAXPOINTS];
    double r[MAXPOINTS];
    double s[MAXPOINTS];
    double ftmp;
    register i;

    /* step 1 */
    for (i=1; i<numpoints; ++i) {
	if (h[i] != 0)
	    deltaz[i] = (z[i+1] - z[i]) / h[i];
	else
	    deltaz[i] = 0;
    }
    h[0] = h[numpoints-1];
    deltaz[0] = deltaz[numpoints-1];

    /* step 2 */
    for (i=1; i<numpoints-1; ++i) {
	d[i] = deltaz[i+1] - deltaz[i];
    }
    d[0] = deltaz[1] - deltaz[0];

    /* step 3a */
    a[1] = 2 * (h[0] + h[1]);
    if (a[1] == 0) 
	return(-1);  /* 3 consecutive knots at same point */
    b[1] = d[0];
    c[1] = h[0];

    for (i=2; i<numpoints-1; ++i) {
	ftmp = h[i-1];
	a[i] = ftmp + ftmp + h[i] + h[i] - (ftmp * ftmp)/a[i-1];
	    if (a[i] == 0) 
		return(-1);  /* 3 consec knots at same point */
	b[i] = d[i-1] - ftmp * b[i-1]/a[i-1];
	c[i] = -ftmp * c[i-1]/a[i-1];
    }

    /* step 3b */
    r[numpoints-1] = 1;
    s[numpoints-1] = 0;
    for (i=numpoints-2; i>0; --i) {
	r[i] = -(h[i] * r[i+1] + c[i])/a[i];
	s[i] = (6 * b[i] - h[i] * s[i+1])/a[i];
    }

    /* step 4 */
    d2z[numpoints-1] = (6 * d[numpoints-2] - h[0] * s[1] 
		       - h[numpoints-1] * s[numpoints-2]) 
		     / (h[0] * r[1] + h[numpoints-1] * r[numpoints-2] 
			+ 2 * (h[numpoints-2] + h[0]));
    for (i=1; i<numpoints-1; ++i) {
	d2z[i] = r[i] * d2z[numpoints-1] + s[i];
    }
    d2z[numpoints] = d2z[1];

    /* step 5 */
    for (i=1; i<numpoints; ++i) {
	dz[i] = deltaz[i] - h[i] * (2 * d2z[i] + d2z[i+1])/6;
	if (h[i] != 0)
	    d3z[i] = (d2z[i+1] - d2z[i])/h[i];
	else
	    d3z[i] = 0;
    }

    return(0);
}  /* end IS_PeriodicEnd */


/*
 * This routine solves for the cubic polynomial to fit a spline
 * curve from the points specified by the list of values.  The alogrithms for
 * this curve are from the "Spline Curve Techniques" paper cited below.
 *
 * This is from Gremlin (called NaturalEndSpline in gremlin)
 *
 */
static
IS_NaturalEnd(h, z, dz, d2z, d3z)
double h[MAXPOINTS];		/* parameterization */
double z[MAXPOINTS];		/* point list */
double dz[MAXPOINTS];		/* to return the 1st derivative */
double d2z[MAXPOINTS];		/* 2nd derivative */
double d3z[MAXPOINTS];		/* and 3rd derivative */
{
    double a[MAXPOINTS]; 
    double b[MAXPOINTS]; 
    double d[MAXPOINTS]; 
    double deltaz[MAXPOINTS];
    double ftmp;
    register i;

    /* step 1 */
    for (i=1; i<numpoints; ++i) {
	if (h[i] != 0)
	    deltaz[i] = (z[i+1] - z[i]) / h[i];
	else
	    deltaz[i] = 0;
    }
    deltaz[0] = deltaz[numpoints-1];

    /* step 2 */
    for (i=1; i<numpoints-1; ++i) {
	d[i] = deltaz[i+1] - deltaz[i];
    }
    d[0] = deltaz[1] - deltaz[0];

    /* step 3 */
    a[0] = 2 * (h[2] + h[1]);
    if (a[0] == 0)		/* 3 consec knots at same point */
	return(-1);
    b[0] = d[1];

    for (i=1; i<numpoints-2; ++i) {
	ftmp = h[i+1];
	a[i] = ftmp + ftmp + h[i+2] + h[i+2] - (ftmp * ftmp) / a[i-1];
	if (a[i] == 0)		/* 3 consec knots at same point */
	    return(-1);
	b[i] = d[i+1] - ftmp * b[i-1]/a[i-1];
    }

    /* step 4 */
    d2z[numpoints] = d2z[1] = 0;
    for (i=numpoints-1; i>1; --i) {
	d2z[i] = (6 * b[i-2] - h[i] *d2z[i+1])/a[i-2];
    }

    /* step 5 */
    for (i=1; i<numpoints; ++i) {
	dz[i] = deltaz[i] - h[i] * (2 * d2z[i] + d2z[i+1])/6;
	if (h[i] != 0)
	    d3z[i] = (d2z[i+1] - d2z[i])/h[i];
	else
	    d3z[i] = 0;
    }

    return(0);
}  /* end IS_NaturalEnd */


/* 
 * Use the same algorithm Gremlin uses to interpolate a given
 * set of points, as described in ``Spline Curve Techniques,''
 * by Pattrick Baudelaire, Robert M. Flegal, and Robert F. Sproull,
 * Xerox PARC Tech Report No. 78CSL-059.
 */
static
IS_Initialize(count, list, output)
    struct point	*list;
    FILE		*output;
{
    MakePoints(count, list, output);
    IS_Parameterize();

    /* Solve for derivatives of the curve at each point 
       separately for x and y (parametric). */

    if ((x[1] == x[numpoints]) && (y[1] == y[numpoints])) { /* closed curve */
	IS_PeriodicEnd(h, x, dx, d2x, d3x);
	IS_PeriodicEnd(h, y, dy, d2y, d3y);
    }
    else {
	IS_NaturalEnd(h, x, dx, d2x, d3x);
	IS_NaturalEnd(h, y, dy, d2y, d3y);
    }
}


/* 
 * This routine converts cubic interpolatory spline to Bezier control vertices
 */
static
IS_Convert(output)
FILE	*output;
{
    double t, t2, t3, x2, y2, x3, y3;
    register j, j1;

    for (j = 1; j < numpoints; ++j) {
	t = .25 * h[j];
	t2 = t * t;
	t3 = t2 * t;
	x2 = x[j] + t * dx[j] + t2 * d2x[j]/2.0 + t3 * d3x[j]/6.0;
	y2 = y[j] + t * dy[j] + t2 * d2y[j]/2.0 + t3 * d3y[j]/6.0;
    
	t = 2 * t;
	t2 = t * t;
	t3 = t2 * t;
	x3 = x[j] + t * dx[j] + t2 * d2x[j]/2.0 + t3 * d3x[j]/6.0;
	y3 = y[j] + t * dy[j] + t2 * d2y[j]/2.0 + t3 * d3y[j]/6.0;
    
	j1 = j + 1;
        BezierSegment(output, x[j], y[j], x2, y2, x3, y3, x[j1], y[j1]);
    }
} /* end IS_Convert */


/*
 * This routine converts cubic interpolatory splines to Bezier cubics.
 */
makecurve(count, list, output)
    struct point	*list;
    FILE		*output;
{
    IS_Initialize(count, list, output);

    fprintf(output, "newpath %lg %lg moveto\n", x[1], y[1]);
    IS_Convert(output);
    fputs("stroke\n", output);
    
    return (0);
} /* end makecurve  */



/*
 * This routine computes a point in B-spline segment, given i, and u.
 * Details of this algorithm can be found in the tech. report cited below.
 */
static
BS_ComputePoint(i, u)
int i;
float u;
{
    float u2, u3, b_2, b_1, b0, b1;
    register i1, i_2, i_1;

    i1  = i + 1;
    i_1 = i - 1;
    i_2 = i - 2;

    u2 = u * u;
    u3 = u2 * u;
    b_2 = (1 - 3*u + 3*u2 - u3) / 6.0;
    b_1 = (4 - 6*u2 + 3*u3) / 6.0;
    b0  = (1 + 3*u + 3*u2 - 3*u3) / 6.0;
    b1  = u3 / 6.0;

    Qx = b_2 * x[i_2] + b_1 * x[i_1] + b0 * x[i] + b1 * x[i1];
    Qy = b_2 * y[i_2] + b_1 * y[i_1] + b0 * y[i] + b1 * y[i1];	
} /* end BS_ComputePoint */


/*
 * This routine initializes the array of control vertices
 * We consider two end conditions here:
 *   (1) closed curve -- C2 continuation and end vertex not interpolated, i.e. 
 *		V[0] = V[n-1], and
 *		V[n+1] = V[2].
 *   (2) open curve -- end vertex interpolation, i.e.
 *		V[0] = 2*V[1] - V[2], and
 *		V[n+1] = 2*V[n] - V[n-1].
 * Details of uniform cubic B-splines, including other end conditions
 * and important properties can be found in Chapters 4-5 of
 * Richard H. Bartels and Brian A. Barsky,
 * "An Introduction to the Use of Splines in Computer Graphics",
 * Tech. Report CS-83-136, Computer Science Division,
 * University of California, Berkeley, 1984.
 */
BS_Initialize(count, list, output)
    struct point	*list;
    FILE		*output;
{
    register n_1, n1;

    MakePoints(count, list, output);

    n_1 = numpoints - 1;
    n1  = numpoints + 1;
  
    if ((x[1] == x[numpoints]) && (y[1] == y[numpoints])) { /* closed curve */
	x[0] = x[n_1];				/* V[0] */
	y[0] = y[n_1];
	x[n1] = x[2];				/* V[n+1] */
	y[n1] = y[2];
    }
    else {				/* end vertex interpolation */
	x[0] = 2*x[1] - x[2];			/* V[0] */
	y[0] = 2*y[1] - y[2];
	x[n1] = 2*x[numpoints] - x[n_1];		/* V[n+1] */
	y[n1] = 2*y[numpoints] - y[n_1];
    }
} /* end BS_Initialize */


/* 
 * This routine converts uniform cubic B-spline to Bezier control vertices
 */
static
BS_Convert(output)
    FILE	*output;
{
    double x1, y1, x2, y2, x3, y3;
    register i;

    for (i = 2; i <= numpoints; i++) {
	BS_ComputePoint(i, 0.0);
	x1 = Qx;
	y1 = Qy;
	BS_ComputePoint(i, 0.25);
	x2 = Qx;
	y2 = Qy;
	BS_ComputePoint(i, 0.5);
	x3 = Qx;
	y3 = Qy;
	BS_ComputePoint(i, 1.0);

        BezierSegment(output, x1, y1, x2, y2, x3, y3, Qx, Qy);
    }
} /* end BS_Convert */


/*
 * This routine converts B-spline to Bezier Cubics
 */
makebspline(count, list, output)
    struct point	*list;
    FILE		*output;
{
    BS_Initialize(count, list, output);

    BS_ComputePoint(2, 0.0);
    fprintf(output, "newpath %lg %lg moveto\n", Qx, Qy);
    BS_Convert(output);
    fputs("stroke\n", output);

    return (0);
} /* makebspline */


/* 
 * This routine copies the offset between two consecutive control points
 * into an array.  That is,
 * 	O[i] = (x[i], y[i]) = V[i+1] - V[i],
 * for i=1 to N-1, where N is the number of points given.
 * The starting end point (V[1]) is saved in (Qx, Qy).
 */
static
BZ_Offsets(count, list, output)
    struct point	*list;
    FILE		*output;
{
    register	i;
    register	double	Lx, Ly;

    if (count > MAXPOINTS - 1) {
	error("warning: Too many points given, only first %d used.",
	      MAXPOINTS - 1);
	count = MAXPOINTS - 1;
    }

    /* Assign offsets btwn points to array for convenience of processing */
    Qx = Lx = list[0].p_x;
    Qy = Ly = list[0].p_y;
    for (i = 1; i < count; i++) {
	x[i] = list[i].p_x - Lx;
	y[i] = list[i].p_y - Ly;
	Lx = list[i].p_x;
	Ly = list[i].p_y;
    }
    numpoints = count;
}


/* 
 * This routine contructs paths of piecewise continuous Bezier cubics
 * in PostScript based on the given set of control vertices.
 * Given 2 points, a stringht line is drawn.
 * Given 3 points V[1], V[2], and V[3], a Bezier cubic segment
 * of (V[1], (V[1]+V[2])/2, (V[2]+V[3])/2, V[3]) is drawn.
 * In the case when N (N >= 4) points are given, N-2 Bezier segments will
 * be drawn, each of which (for i=1 to N-2) is translated to PostScript as
 *	Q+O[i]/3  Q+(3*O[i]+O[i+1])/6  K+O[i+1]/2  curveto,
 * where
 *	Q is the current point,
 *	K is the continuation offset = Qinitial + Sigma(1, i)(O[i])
 * Note that when i is 1, the initial point
 *	Q = V[1].
 * and when i is N-2, the terminating point
 *	K+O[i+1]/2 = V[N].
 */
static
BZ_Convert(output)
    FILE	*output;
{
    register	i, i1;
    double	x1, y1, x2, y2, x3, y3, Kx, Ky;
    
    if (numpoints == 2) {
	fprintf(output, "  %lg %lg rlineto\n", x[1], y[1]);
	return;
    }
    if (numpoints == 3) {
	x1 = Qx + x[1];
	y1 = Qy + y[1];
	x2 = x1 + x[2];
	y2 = y1 + y[2];
        fprintf(output,"  %lg %lg %lg %lg %lg %lg curveto\n",
		(Qx+x1)/2.0, (Qy+y1)/2.0, (x1+x2)/2.0, (y1+y2)/2.0, x2, y2);
	return;
    }
  
    /* numpoints >= 4 */
    Kx = Qx + x[1];
    Ky = Qy + y[1];
    x[1] = 2 * x[1];
    y[1] = 2 * y[1];
    i = numpoints - 1;
    x[i] = 2 * x[i];
    y[i] = 2 * y[i];
    i1 = 2;
    for (i = 1; i <= numpoints-2; i++) {
	x1 = Qx + x[i]/3;
	y1 = Qy + y[i]/3;
	x2 = Qx + (3*x[i] + x[i1])/6;
	y2 = Qy + (3*y[i] + y[i1])/6;
	x3 = Kx + x[i1]/2;
	y3 = Ky + y[i1]/2;
        fprintf(output, "  %lg %lg %lg %lg %lg %lg curveto\n",
                x1, y1, x2, y2, x3, y3);
	Qx = x3;
	Qy = y3;
	Kx = Kx + x[i1];
	Ky = Ky + y[i1];
	i1++;
    }
} /* end BZ_Convert */


/*
 * This routine draws piecewise continuous Bezier cubics based on
 * the given list of control vertices.
 */
makebezier(count, list, output)
	struct point	*list;
	FILE		*output;
{
    BZ_Offsets(count, list, output);

    fprintf(output, "newpath %lg %lg moveto\n", Qx, Qy);
    BZ_Convert(output);
    fputs("stroke\n", output);
    
    return (0);
}
