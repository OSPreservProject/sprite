/* Font reading and writing and string drawing */

/*********************************\
* 				  *
* 	James Gosling, 1983	  *
* 	Copyright (c) 1983 IBM	  *
* 				  *
\*********************************/


#include "font.h"
#include "window.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "ctype.h"
#include "sys/dir.h"


char   *FormatFontname ();

BuildFontDirectory () {
    char    fontpath[120];
    char   *p,
           *st;
    p = getprofile ("fontpath");
    if (p == 0)
	p = "/usr/local/fonts";
    strncpy (fontpath, p, sizeof fontpath - 1);
    p = fontpath;
    fonts = (struct FontSet *) malloc ((FontASize = 10) * sizeof *fonts);
    nFonts = 1;
    while (1) {
	int     size;
	char   *dirname;
	DIR *dir;
	struct direct  *ent;
	st = p;
	while (*p && *p != ':')
	    p++;
	size = p - st;
	dirname = (char *) malloc (size + 1);
	strncpy (dirname, st, size);
	dirname[size] = 0;
	if (dir = opendir (dirname)) {
	    while (ent = readdir (dir)) {
		if (nFonts >= FontASize) {
		    fonts = (struct FontSet *) realloc (fonts, sizeof *fonts
			*                       (FontASize = FontASize * 3 / 2));
		}
		if ((ent -> d_name[ent->d_namlen-4] != '.' ||
			ent -> d_name[ent->d_namlen-3] != 'B' ||
			ent -> d_name[ent->d_namlen-2] != 'A' ||
			ent -> d_name[ent->d_namlen-1] != 'K') &&
			!parsefname (ent -> d_name, &fonts[nFonts].fn)) {
		    fonts[nFonts].directory = dirname;
		    fonts[nFonts].this = 0;
		    nFonts++;
		}
	    }
	    closedir (dir);
	} else debug(("	Couldn't open %s\n", dirname));
	if (*p)
	    p++;
	else
	    break;
    }
}

struct font *
GetFontFromFile (dir, n)
char *dir;
struct FontName *n; {
    char   *basename = FormatFontname (n);
    int     fd;
    if (*dir == 0)
	fd = open (basename, 0);
    else {
	char    fnbuf[200];
	sprintf (fnbuf, "%s/%s", dir, basename);
	fd = open (fnbuf, 0);
    }
    if (fd > 0) {
	struct stat st;
	register struct font   *f;
	if (fstat (fd, &st) >= 0) {
	    int     n;
	    f = (struct font   *) malloc (st.st_size);
	    n = read (fd, f, st.st_size);
	    close (fd);
	    return f;
	}
	close (fd);
    }
    return 0;
}

int FontIndex;

struct font *
getfont (n)
register struct FontName   *n; {
    register struct FontSet *p,
                           *BestF = 0;
    int     BestCost;
    int     nf;
    for (nf = nFonts, p = fonts; --nf>=0; p++) {
	register    cost;
	cost = n -> height - p -> fn.height;
	if (cost < 0)
	    cost = -cost;
	if (p -> fn.rotation != n -> rotation)
	    cost |= 01000;
	{
	    register    i = 1;
	    do {
		if ((n -> FamilyName[i] ^ p -> fn.FamilyName[i]) & ~040) {
		    cost |= 0400;
		    break;
		}
	    } while (n -> FamilyName[i++]);
	}
	if (p -> fn.FaceCode != n -> FaceCode)
	    cost += 2;
	if (BestF == 0 || cost < BestCost)
	    BestCost = cost, BestF = p;
	if (cost == 0)
	    break;
    }
    if (BestF == 0) {
	FontIndex = 0;
	return 0;
    }
    if (BestF -> this == 0)
	BestF -> this = GetFontFromFile (BestF -> directory, &BestF -> fn);
    FontIndex = BestF - fonts;
    return BestF -> this;
}

struct icon *geticon (f, c)
register struct font *f; {
    return f ? &f->chars[c] : 0;
}

struct font *
getpfont (name)
char   *name; {
    struct FontName fn;
    if (parsefname (name, &fn))
	return 0;
    return getfont (&fn);
}

StringWidth (font, s)
register struct font   *font;
register char  *s; {
    register    x = 0,
                y = 0;
    while (*s) {
	register struct icon   *c = &font -> chars[*s++];
	if (c -> OffsetToGeneric) {
	    register struct IconGenericPart *g =
		(struct IconGenericPart *) (((int) c) + c -> OffsetToGeneric);
	    x += g -> Spacing.x;
	    y += g -> Spacing.y;
	}
    }
    LastX = x;
    LastY = y;
    return x;
}
