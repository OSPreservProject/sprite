/****************************************************************
 *								*
 *	Definitons used in gremlin files			*
 *								*
 *	Gremlin file format is:					*
 *	"gremlinfile"						*
 *	<orientation> <x> <y>					*
 *	<element>						*
 *	<element>						*
 *	    .							*
 *	    .							*
 *	 -1							*
 *								*
 *	Each element has the following format			*
 *	<type>							*
 *	<points>	i.e. X, Y coords			*
 *	-1.0 -1.0	to mark end of figure			*
 *	<brush> <size>						*
 *	<string length> <string>				*
 *								*
 *	The screen origin is 0, 0 the size is 512, 480		*
 *	116 is edge of menu in vertical mode (menu on left)	*
 *	395 is edge of menu in horizontal mode (menu on top)	*
 *								*
 ****************************************************************/
/* some useful definitions */
#define TRUE	1
#define FALSE	0
#define NULL	0
#define ERROR	-1
#define BLANK	' '
#define COMMENTCHAR ';'
#define TAB '\t'
#define NEWLINE '\n'

#define MAXLINES 400		/* max number of lines to plot */
#define MAXSIZE	5000		/* max number of points to plot */
#define MAXGRAPH 1		/* max number of graphs on a page */

/* device type things */
#define XSCREENMAX 500.0
#define YSCREENMAX 480.0
#define XSCREENMIN 120.0
#define YSCREENMIN 1.0
#define XPLOTMAX 480.0		/* max plot area */
#define YPLOTMAX 450.0
#define XORIGIN 150
#define YORIGIN 50

#define SQRT2 0.707107		/* sqrt(2) / 2.0 */
#define TWOPI 6.283185

#define NFONTS 4
#define NBRUSHES 6
#define NSIZES 4
#define NSTIPPLES 8

#define GR_ERROR -1
#define GR_OK 0

#define SUN_GREMLIN 0
#define AED_GREMLIN 1

/*----Brush definitions type line to draw----*/

#define BRUSH_INVISIBLE	0
#define BRUSH_DOT	1
#define BRUSH_ALT	2
#define BRUSH_THICK	3
#define BRUSH_DASH	4
#define BRUSH_THIN	5
#define BRUSH_NORMAL	6

/*----Font definitions----*/

#define ROMAN		1
#define ITALICS		2
#define BOLD		3
#define SPECIAL		4

/*----Font Size----*/

#define SMALL		1
#define MEDIUM		2
#define LARGE		3
#define EXLARGE		4

/*----Figure types----*/

#define BOTLEFT_TEXT	0
#define BOTRIGHT_TEXT	1
#define TEXT		2
#define LINE		3
#define CIRCLE		4
#define CURVE		5
#define POLYGON		6
#define TOPLEFT_TEXT	10
#define TOPCENTER_TEXT	11
#define TOPRIGHT_TEXT	12
#define CENTERLEFT_TEXT 13
#define CENTERRIGHT_TEXT 14
#define BOTCENTER_TEXT	15
#define CENTERCENTER_TEXT 2
/*----Orientations----*/

#define HORIZONTAL	0
#define VERTICAL	1

/*----Header String----*/

#define FIRSTLINE	"gremlinfile"
#define SFIRSTLINE	"sungremlinfile"

/*----Miscelaneous----*/

#define LASTPOINT	(-1.0)

#define ALLINES MAXLINES+1

/* point symbol types */
#define NOSYMBOL 0
#define BOX 1
#define RING 2
#define CROSS 3
#define STAR 4
#define TRIANGLE 5
#define UTRIANGLE 6
#define CROSSBOX 7
#define STARBOX 8
#define DIAMOND 9
#define CROSSDIAMOND 10

/* Types of graphs */
#define LINEAR 0
#define HIST 1
#define LOG 2
#define PIE 3
#define BAR 4
/* frame for graph */
#define FRAME1X XSCREENMIN
#define FRAME2X XSCREENMIN
#define FRAME3X XSCREENMAX
#define FRAME4X XSCREENMAX
#define FRAME1Y YSCREENMIN
#define FRAME2Y YSCREENMAX
#define FRAME3Y YSCREENMAX
#define FRAME4Y YSCREENMIN

/* type of text */
#define T_TITLE 0
#define T_XLABEL 1
#define T_YLABEL 2
#define T_LEGEND 3

/* types of frames */
#define G_FRAME 0		/* graph frame */
#define L_FRAME 1		/* legend frame */

#define cg graph[curgraph]
#define cl graph[curgraph].lines[curline]

