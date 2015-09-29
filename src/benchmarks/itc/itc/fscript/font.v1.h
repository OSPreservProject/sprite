#ifndef FONTMAGIC
/* Font file layout */


/************************************\
* 				     *
* 	James Gosling, 1983	     *
* 	Copyright (c) 1983 IBM	     *
* 	IBM/ITC Internal use only    *
* 				     *
\************************************/

#define v1_CharsPerFont 128
#define v1_FONTMAGIC 0x1fc

/***********************************************\
* 					        *
*	         North			        *
*         |---------WtoE----------->|	        *
*      -  +-------------------------+	        *
*      |  |          /             /|	        *
*      |  |         /             / |	        *
*      |  |        /             /  |	        *
*      |  |       /             /   |	        *
*      N  |      /             /    |	East    *
*      t  |     /             /     |	        *
*      o  |    /             /      |	        *
*      S  |---*-------------/-------|	        *
*      |  |  / Origin      /        |	        *
*      |  | /             /         |	        *
*      V  |/             /          |	        *
*      -  +-------------------------+	        *
*         <---|---Spacing-->|		        *
*           ^Wbase			        *
* 	        South			        *
* 					        *
\***********************************************/

struct v1_SVector {		/* Short Vector */
    short x, y;
};

struct v1_IconGenericPart {	/* information relating to this icon that
				   is of general interest */
    struct v1_SVector  Spacing,	/* The vector which when added to the
				   origin of this character will give the
				   origin of the next character to follow
				   it */
                    NWtoOrigin,	/* Vector from the origin to the North
				   West corner of the characters bounding
				   box */
                    NtoS,	/* North to south vector */
                    WtoE,	/* West to East vector */
                    Wbase;	/* Vector from the origin to the West edge
				   parallel to the baseline */
};

struct v1_BitmapIconSpecificPart {	/* information relating to an icon that is
				   necessary only if you intend to draw it 
				*/
    char    type;		/* The type of representation used for
				   this icon.  (= BitmapIcon) */
    unsigned char   rows,	/* rows and columns in this bitmap */
                    cols;
    char    orow,		/* row and column of the origin */
            ocol;		/* Note that these are signed */
    unsigned short  bits[1];	/* The bitmap associated with this icon */
};

struct v1_icon {			/* An icon is made up of a generic and a
				   specific part.  The address of each
				   is "Offset" bytes from the "icon"
				   structure */
    short   OffsetToGeneric,
            OffsetToSpecific;
};


/* A font name description block.  These are used in font definitions and in
   font directories */
struct v1_FontName {
    char    FamilyName[16];	/* eg. "TimesRoman" */
    short   rotation;		/* The rotation of this font (degrees;
				   +ve=>clockwise) */
    char    height;		/* font height in points */
    char    FaceCode;		/* eg. "Italic" or "Bold" or "Bold Italic"
				   */
};

/* Possible icon types: */
#define v1_AssortedIcon 0		/* Not used in icons, only in fonts: the
				   icons have an assortment of types */
#define v1_BitmapIcon 1		/* The icon is represented by a bitmap */
#define v1_VectorIcon 2		/* The icon is represented as vectors */



struct v1_font {
    short   magic;		/* used to detect invalid font files */
    short   NonSpecificLength;	/* number of bytes in the font and
				   generic parts */
    struct v1_FontName fn;		/* The name of this font */
    struct v1_SVector  NWtoOrigin,	/* These are "maximal" versions of the
				   variables by the same names in each
				   constituent icon */
                    NtoS,	/* North to South */
                    WtoE,	/* West to East */
		    Wbase,	/* From the origin along the baseline to
				   the West edge */
                    newline;	/* The appropriate "newline" vector, its
				   just NtoS with an appropriate fudge
				   factor added */
    char    type;		/* The type of representation used for the
				   icons within this font.  If all icons
				   within the font share the same type,
				   then type is that type, otherwise it is
				   "AssortedIcon" */
    struct v1_icon    chars[v1_CharsPerFont];
 /* at the end of the font structure come the bits for each character */
};
#endif
