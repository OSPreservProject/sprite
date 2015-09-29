/* Convert Alto font files to the ITC format */

#include "ctype.h"
#include "stdio.h"
#include "font.h"
#include "fontmanip.h"
#include "sys/types.h"
#include "sys/stat.h"

struct alfont {
    short height;
    unsigned proportional:1;
    unsigned baseline:7;
    unsigned maxwidth:8;
    short pointers[0377];
};

main (argc, argv)
char  **argv; {
    if (argc <= 1)
	printf ("Usage: cvtalto files...\n");
    while (--argc >= 1)
	convert (*++argv);
}

convert(fn)
char   *fn; {
    struct stat st;
    register struct alfont *p;
    register    i;
    char    family[30];
    int     size = 0,
            facecode = 0;
    int     fd = open (fn, 0);
    if (fd < 0 || fstat (fd, &st) < 0) {
	fprintf (stderr, "Couldn't open %s\n", fn);
	return;
    }
    p = (struct alfont *) malloc (st.st_size);
    if (read (fd, p, st.st_size + 1) != st.st_size) {
	fprintf ("Read error!\n");
	exit (1);
    }
    close (fd);
    {
	register char  *s,
	               *d;
	static struct trans {
	    char   *old,
	           *new;
	}                   trans[] = {
	                        "timesroman", "TimesRoman",
	                        "timesromand", "TimesRoman",
	                        "titanlegal", "TitanLegal",
	                        "smalltalk", "SmallTalk",
	                        "xeroxbook", "XeroxBook",
	                        "cmulogo", "CMUlogo",
	                        "musicfont", "MusicFont",
				"monastary", "Monastery",
				"oldenglish", "OldEnglish",
				"helveticad", "Helvetica",
				"danaten", "DanaTen",
				"danatwelve", "DanaTwelve",
	                        0, 0
	};
	struct trans   *t;
	s = fn;
	for (d = fn; *d;)
	    if (*d++ == '/')
		s = d;
	for (d = family; isalpha (*s);)
	    *d++ = *s++;
	if (!isdigit (*s) || s == fn) {
	    fprintf (stderr, "Badly formed font name: %s\n", fn);
	    return;
	}
	*d++ = 0;
	for (t = trans; t -> old && strcmp (t -> old, family); t++);
	if (t -> old)
	    strcpy (family, t -> new);
	else
	    if (islower (family[0]))
		family[0] = toupper (family[0]);
	while (isdigit (*s))
	    size = size * 10 + *s++ - '0';
	while (*s)
	    switch (*s++) {
		case 'i': 
		    facecode |= ItalicFace;
		    break;
		case 'b': 
		    facecode |= BoldFace;
		    break;
		case 'p': 
		    facecode |= ItalicFace | BoldFace;
		    break;
		case 'e': 
		    break;
	    }
    }
    printf ("%s %d %s%s\n", family, size,
	    facecode & ItalicFace ? "i" : "",
	    facecode & BoldFace ? "b" : "");
    strcpy (fonthead.fn.FamilyName, family);
    fonthead.fn.rotation = 0;
    fonthead.fn.height = size;
    fonthead.fn.FaceCode = facecode;
    for (i = 0; i <= 0177; i++) {
	static struct IconGenericPart   zg;
	static struct BitmapIconSpecificPart    zs;
	unsigned short *c;
	int     xw;
	char    hd,
	        xh;
	int     orow;
	register    r,
	            x;
	c = ((unsigned short *) & p -> pointers[i]) + p -> pointers[i];
	xw = c[0];
	hd = c[1] >> 8;
	xh = c[1] & 0377;
	specific[i] = zs;
	generic[i] = zg;
	bits[i] = 0;
	if (xh == 0)
	    continue;
	if ((xw & 1) == 0) {
	    int     cols,
	            headchop,
	            rows;
	    cols = 1;
	    headchop = hd;
	    rows = hd + xh;
	    while ((xw & 1) == 0) {
		xw >>= 1;
		c = ((unsigned short *) & p -> pointers[xw]) + p -> pointers[xw];
		xw = c[0];
		hd = c[1] >> 8;
		xh = c[1] & 0377;
		if (hd < headchop)
		    headchop = hd;
		if (hd + xh > rows)
		    rows = hd + xh;
		cols++;
	    }
	    {
		register unsigned short *s,
		                       *d;
		register    n;
		bits[i] = d = (unsigned short *) malloc ((n = (rows - headchop) * cols) * 2);
		while (--n >= 0)
		    *d++ = 0;
		specific[i].rows = rows - headchop;
		specific[i].cols = (xw>>1) + (cols-1)*16;
		orow = p -> baseline - headchop - 1;
		xw = i << 1;
		cols = 0;
		while ((xw & 1) == 0) {
		    xw >>= 1;
		    c = ((unsigned short *) & p -> pointers[xw]) + p -> pointers[xw];
		    xw = c[0];
		    hd = c[1] >> 8;
		    xh = c[1] & 0377;
		    s = c - xh;
		    d = bits[i] + (cols * (rows - headchop) + hd - headchop);
		    n = xh;
		    while (--n >= 0)
			*d++ = *s++;
		    cols++;
		}
	    }
	}
	else {
	    bits[i] = c - xh;
	    specific[i].rows = xh;
	    specific[i].cols = xw >> 1;
	    orow = p -> baseline - hd - 1;
	}
	if (orow < 0)
	    orow = 0;
	specific[i].orow = orow;
	generic[i].Spacing.x = specific[i].cols;
    }
    if (generic[' '].Spacing.x == 0)
	generic[' '].Spacing.x = generic['n'].Spacing.x;
    ComputeBoundingBoxes ();
    ImplodeFont ();
    free (p);
}
