/* Pack and unpack font structures */

#include "stdio.h"
#include "font.h"
#include "font.v0.h"
#include "fontmanip.h"

ExplodeFont (f)
register struct font   *f; {
    register    i;
    fonthead = *f;
    for (i = 0; i < 0177; i++) {
	register struct icon   *p = &f -> chars[i];
	if (p -> OffsetToSpecific && p -> OffsetToGeneric) {
	    generic[i] = *(struct IconGenericPart *) (((int) p) + p -> OffsetToGeneric);
	    specific[i] = *(struct BitmapIconSpecificPart *) (((int) p) + p -> OffsetToSpecific);
	    bits[i] = (unsigned short *) ((struct BitmapIconSpecificPart   *) (((int) p) + p -> OffsetToSpecific)) -> bits;
	}
	else {
	    static struct IconGenericPart   gz;
	    static struct BitmapIconSpecificPart    sz;
	    generic[i] = gz;
	    specific[i] = sz;
	    bits[i] = 0;
	}
    }
}

ExplodeV0Font (v0f)
register struct v0_font *v0f; {
    register c;
    if (v0f -> magic != FONTMAGIC) {
	printf ("  Bad magic number\n");
	return;
    }
    fonthead.magic = FONTMAGIC;
    strncpy (fonthead.fn.FamilyName, v0f -> FamilyName, sizeof fonthead.fn.FamilyName);
    fonthead.fn.rotation = v0f -> rotation;
    fonthead.fn.height = v0f -> height;
    fonthead.fn.FaceCode = v0f -> FaceCode;
    fonthead.NWtoOrigin.x = 0;
    fonthead.NWtoOrigin.y = 0;
    fonthead.NtoS.x = 0;
    fonthead.NtoS.y = v0f -> mrows;
    fonthead.WtoE.x = v0f -> mcols;
    fonthead.WtoE.y = 0;
    fonthead.newline.x = 0;
    fonthead.newline.y = v0f -> mrows;
    fonthead.type = BitmapIcon;
    for (c = 0; c <= 0177; c++) {
	generic[c].Spacing.x = v0f -> chars[c].cdelt;
	generic[c].Spacing.y = v0f -> chars[c].rdelt;
	generic[c].NWtoOrigin.x = v0f -> chars[c].ocol;
	generic[c].NWtoOrigin.y = v0f -> chars[c].orow;
	generic[c].NtoS.x = 0;
	generic[c].NtoS.y = v0f -> chars[c].rows;
	generic[c].WtoE.x = v0f -> chars[c].cols;
	generic[c].WtoE.y = 0;
	generic[c].Wbase.x = -v0f -> chars[c].ocol;
	generic[c].Wbase.y = 0;
	specific[c].type = BitmapIcon;
	specific[c].rows = v0f -> chars[c].rows;
	specific[c].cols = v0f -> chars[c].cols;
	specific[c].orow = v0f -> chars[c].orow;
	specific[c].ocol = v0f -> chars[c].ocol;
	bits[c] = (unsigned short *) (((int) & v0f -> chars[c]) + v0f -> chars[c].bits);
    }
}

ComputeBoundingBoxes () {
    if (fonthead.fn.rotation == 0) {
	register struct IconGenericPart *p;
	register    c;
	int     above = 0,
	        below = 0,
	        left = 0,
	        right = 0;
	for (c = 0; c <= 0177; c++) {
	    register    t;
	    p = &generic[c];
	    if (specific[c].rows==0 || specific[c].cols==0) {
		specific[c].rows=0;
		specific[c].cols=0;
		specific[c].orow=0;
		specific[c].ocol=0;
	    }
	    p -> NtoS.y = specific[c].rows;
	    p -> NtoS.x = 0;
	    p -> WtoE.y = 0;
	    p -> WtoE.x = specific[c].cols;
	    p -> NWtoOrigin.y = specific[c].orow;
	    p -> NWtoOrigin.x = specific[c].ocol;
	    if (p -> NWtoOrigin.x > left)
		left = p -> NWtoOrigin.x;
	    if (p -> NWtoOrigin.y > above)
		above = p -> NWtoOrigin.y;
	    if ((t = p -> NtoS.y - p -> NWtoOrigin.y) > below)
		below = t;
	    if ((t = p -> WtoE.x - p -> NWtoOrigin.x) > right)
		right = t;
	}
	fonthead.NWtoOrigin.x = left;
	fonthead.NWtoOrigin.y = above;
	fonthead.Wbase.y = 0;
	fonthead.Wbase.x = -left;
	fonthead.NtoS.x = 0;
	fonthead.NtoS.y = above + below;
	fonthead.WtoE.x = left + right;
	fonthead.WtoE.y = 0;
	fonthead.newline.x = 0;
	fonthead.newline.y = above + below;
    }
}

ImplodeFont () {
    char    matchg[128];
    char    matchs[128];
    int     UniqueGenerics = 0;
    int     UniqueSpecifics = 0;
    register    FILE * outf;
    register    c,
                d;
    {
	register char  *p = fonthead.fn.FamilyName;
	int     n = sizeof fonthead.fn.FamilyName;
	while (*p && --n > 0)
	    p++;
	while (--n >= 0)
	    *p++ = 0;
    }
    {
	char   *out = (char *) FormatFontname (&fonthead.fn);
	char    fnb[200];
	char    backup[200];
	static char fontdir[120];
	static  fontdirInitialized = 0;
	static  BackedUp;
	register char  *p,
	               *d;
	if (!fontdirInitialized) {
	    fontdirInitialized++;
	    p = (char *) getprofile ("fontpath");
	    if (p == 0)
		p = "/usr/local/fonts";
	    strncpy (fontdir, p, sizeof fontdir - 1);
	}
	p = fontdir;
	d = fnb;
	while (*p && *p != ':')
	    *d++ = *p++;
	if (d != fnb)
	    *d++ = '/';
	while (*d++ = *out++);
	if (!BackedUp) {
	    BackedUp++;
	    sprintf (backup, "%s.BAK", fnb);
	    rename (fnb, backup);
	}
	outf = fopen (fnb, "w");
    }
    if (outf == 0)
	return 1;
    fonthead.magic = FONTMAGIC;
    fonthead.type = BitmapIcon;
    for (c = 0; c <= 0177; c++) {
	specific[c].type = BitmapIcon;
	matchg[c] = -1;
	UniqueGenerics++;
	for (d = 0; d < c; d++)
	    if (generic[c].Spacing.x == generic[d].Spacing.x &&
		    generic[c].Spacing.y == generic[d].Spacing.y &&
		    generic[c].NWtoOrigin.x == generic[d].NWtoOrigin.x &&
		    generic[c].NWtoOrigin.y == generic[d].NWtoOrigin.y &&
		    generic[c].NtoS.x == generic[d].NtoS.x &&
		    generic[c].NtoS.y == generic[d].NtoS.y &&
		    generic[c].WtoE.x == generic[d].WtoE.x &&
		    generic[c].WtoE.y == generic[d].WtoE.y &&
		    generic[c].Wbase.x == generic[d].Wbase.x &&
		    generic[c].Wbase.y == generic[d].Wbase.y) {
		matchg[c] = d;
		UniqueGenerics--;
		break;
	    }
	matchs[c] = -1;
	UniqueSpecifics++;
	for (d = 0; d < c; d++) {
	    register unsigned short *p1,
	                           *p2;
	    register    n;
	    p1 = bits[c];
	    p2 = bits[d];
	    n = (specific[c].cols + 15) / 16 * specific[c].rows;
	    if (p1 && p2)
		while (n > 0 && *p1++ == *p2++)
		    n--;
	    if (n == 0 &&
		    specific[c].type == specific[d].type &&
		    specific[c].rows == specific[d].rows &&
		    specific[c].cols == specific[d].cols &&
		    specific[c].orow == specific[d].orow &&
		    specific[c].ocol == specific[d].ocol) {
		matchs[c] = d;
		UniqueSpecifics--;
		break;
	    }
	}
    }
    {
	int     CurGOffset = sizeof (struct font);
	int     CurSOffset = sizeof (struct font) + UniqueGenerics * sizeof (struct IconGenericPart);

#define nbsz (((int)specific[0].bits)-((int)&specific[0]))
	for (c = 0; c <= 0177; c++) {
	    if (matchs[c] >= 0)
		fonthead.chars[c].OffsetToSpecific = fonthead.chars[matchs[c]].OffsetToSpecific;
	    else {
		fonthead.chars[c].OffsetToSpecific = CurSOffset;
		CurSOffset += nbsz
		    + ((specific[c].cols + 15) / 16 * specific[c].rows) * 2;
	    }
	    if (matchg[c] >= 0)
		fonthead.chars[c].OffsetToGeneric = fonthead.chars[matchg[c]].OffsetToGeneric;
	    else {
		fonthead.chars[c].OffsetToGeneric = CurGOffset;
		CurGOffset += sizeof (struct IconGenericPart);
	    }
	}
	for (c = 0; c < 0177; c++) {
	    register struct icon   *p = &fonthead.chars[c];
	    int     offset = ((int) p) - ((int) & fonthead);
	    p -> OffsetToSpecific -= offset;
	    p -> OffsetToGeneric -= offset;
	}
	fonthead.magic = FONTMAGIC;
	fonthead.NonSpecificLength = CurGOffset;
	fwrite (&fonthead, sizeof fonthead, 1, outf);
	for (c = 0; c <= 0177; c++)
	    if (matchg[c] < 0)
		fwrite (&generic[c], sizeof generic[c], 1, outf);
	for (c = 0; c < 0177; c++)
	    if (matchs[c] < 0) {
		fwrite (&specific[c], nbsz, 1, outf);
		fwrite (bits[c], (specific[c].cols + 15) / 16 * specific[c].rows * 2,
			1, outf);
	    }
	fclose (outf);
    }
    return 0;
}
