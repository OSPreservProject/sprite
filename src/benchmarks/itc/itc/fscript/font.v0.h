/* Old version of the Font file layout */


/****************************************
 *	James Gosling, 1983		*
 *	Copyright (c) 1983 IBM		*
 ****************************************/

/*******************************************************************\
* 								    *
* An icon is a rectangular patch of bits.  It has a size: (rows,    *
* cols); an origin: (orow, ocol); and a displacement vector	    *
* (rdelt, cdelt).  The patch of bits is pointed to by `bits' and    *
* is an offset; this is done so that files containing icons can be  *
* read in and used directly.  The origin is a distinguished point   *
* within the icon.  It is used to define the tip of a pointer or    *
* the left endpoint of the baseline of a character.  The delta	    *
* defines the spacing from one origin to the next when a row of	    *
* icons is being laid out.					    *
* 								    *
* 	(0,0)							    *
*        *---------------+					    *
*        |      A        |					    *
*        |     A A       |					    *
*        |    A   A      |					    *
*        |   A     A     |					    *
*        |  A       A    |					    *
*        | AAAAAAAAAAA   |					    *
*        |A           A  |					    *
*        |A           A  |					    *
*        |*--------------|*(orow+rdelt, ocol+cdelt)		    *
*        | (orow,ocol)   |					    *
*        |               |					    *
*        |               |					    *
*        +---------------*(rows-1,cols-1)			    *
* 								    *
\*******************************************************************/

struct v0_font {
    short   magic;		/* used to detect invalid font files */
    char    FamilyName[10];	/* eg. "TimesRoman" */
    short   rotation;		/* The rotation of this font (degrees;
				   +ve=>clockwise) */
    char    height;		/* font height in points */
    char    FaceCode;		/* eg. "Italic" or "Bold" or "Bold Italic"
				   */
    char    mrows;		/* maximum number of rows in any character 
				*/
    char    mcols;		/* 	"	     columns		" 
				*/
    struct v0_icon {
	char    rdelt;		/* row delta from the origin of this
				   character to the origin of the next */
	char    cdelt;		/* column delta ....   This is commonly
				   what one might think of as the 'width'
				   of the character.  rdelt will be 0 for
				   a horizontal font */
	char    rows;		/* rows in this bitmap */
	char    cols;		/* columns in this bitmap */
	char    orow;		/* row number of the origin of this
				   character -- the baseline (counting
				   from the top) */
	char    ocol;		/* column number ... */
	short   bits;		/* offset (relative to the beginning of
				   the icon block) of the bits for this
				   character */
    }           chars[128];
 /* at the end of the font structure come the bits for each character */
};

/* FaceCode flags */
