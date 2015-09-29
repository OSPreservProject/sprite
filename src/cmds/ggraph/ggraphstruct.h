#include "ggraph.h"

/****************************************************************
 *	All the structures used globaly by ggraph		*
 *								*
 ****************************************************************/

/* The generic text structure used for all text structures on the graph */
struct gtext{			/* text for the graph */
    int t_type;			/* type of thing this is */
    int t_font;			/* font to use */
    int t_size;			/* size to use */
    int t_just;			/* justification to use */
    float t_xpos;		/* X connection point in user coord */
    float t_ypos;		/* Y connection point in user coord */
    float t_vertsz;		/* vertical size of text in dev coord */
    float t_hortsz;		/* horizontal size of text in dev coord */
    int t_text[80];		/* text string */
};

/* The frame structure used for all frames  */
struct frame{			/* a frame for something */
    int	frame_type;		/* what is it for */
    int     fsize;		/* thickness of frame */
    float   frame1x, frame1y;	/* one corner of the frame */
    float   frame2x, frame2y;	/* one corner of the frame */
    float   frame3x, frame3y;	/* one corner of the frame */
    float   frame4x, frame4y;	/* one corner of the frame */
};

/* A line on the graph and its properties */
struct aline {
	struct aline *forw_line;/* pointer to next line */
	struct aline *back_line;/* pointer to prev line */
	int     lonoff;		/* is this line visible */
	int     llabsw;		/* line label on/off */
	int     ctype;		/* curve type to use */
	int     ltype;		/* brush type to use */
	int     lthick;		/* brush thickness to use */
	int     lsize;		/* line size to use */
	int     mtype;		/* type of mark for points */
	int     maxpoint;	/* max number of points this line */
	float   maxx, maxy;	/* maximum coordinate values for line */
	float   minx, miny;	/* minimum coordinate values for line */
	float   xpoints[MAXSIZE]; /* X coordinates in user coords */
	float   ypoints[MAXSIZE]; /* Y coordinates in user coords */
	char    lname[20];	/* user name of line */
	struct gtext llabel;	/* line label */
	struct gtext llelabel;	/* line legend label */
    };

/* A graph and its properties */
struct agraph{
    struct aline *lines[MAXLINES];
    int     maxlines;		/* number of datasets in plot */
    int     axline_type;	/* type of line for axis */
    int     xgrid_type;		/* type of line for X grid */
    int     ygrid_type;		/* type of line for Y grid */
    int	    yvert;		/* should we do vertical chars */
    int	    xpreci1, xpreci2;	/* precision for X labels */
    int	    ypreci1, ypreci2;	/* precision for Y labels */
    int     maxtickx, maxticky;	/* maximum value of X and Y ticks on axis */
    int     mintickx, minticky;	/* minimum value of X and Y ticks on axis */
    int     numtickx, numticky;	/* count of X and Y ticks on axis */
    float   xplotmax, yplotmax;	/* plot area */
    float   logxtick, logytick;	/* tick factor for log graphs */
    float   xoffset, yoffset;   /* offsets for coordinates */
    float   dtickx, dticky;	/* delta for ticks */
    float   stickx, sticky;	/* starting value for ticks */
    float   scalex, scaley;	/* scaling factor for graph */
    float   xorigin, yorigin;	/* the origin of the graph */
    float   gmaxx, gmaxy;	/* maximum coordinate values for line */
    float   gminx, gminy;	/* minimum coordinate values for line */
    float   xcenter, ycenter;	/* center of each axis */
    float   symbolsz;		/* pixel size for symbols */
    int     gtype;		/* type of graph */
    int     logxsw, logysw;	/* switches for log axis */
    int     firstxsw, firstysw;	/* switches for log axis */
    struct frame gframe;	/* graph frame */
    struct frame lframe;	/* legend frame */
    struct gtext gtitle;	/* title for graph */
    struct gtext xlabel;	/* label for X axi */
    struct gtext ylabel;	/* label for Y axis */
    struct gtext xtick;		/* tick for X axis */
    struct gtext ytick;		/* tick for Y axis */
    struct gtext legend;	/* lengend title */
    int graph_units;	        /* type of units graph is specifed in */
    int xaxis_length;		/* length of x axis in inches */
    int yaxis_length;		/* length of y axis in inches */
};

