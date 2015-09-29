#include "font.h"
#include "fontmanip.h"

unsigned short fbits [128][16] = {
#include "sail7.c"
};

main () {
    register    i,
                j;
    register unsigned short *p;
    strcpy (fonthead.fn.FamilyName, "SAIL");
    fonthead.fn.rotation = 0;
    fonthead.fn.FaceCode = 0;
    fonthead.fn.height = 7;
    for (i = 0; i < 128; i++) {
	specific[i].rows = 9;
	specific[i].cols = 6;
	specific[i].orow = 5;
	specific[i].ocol = 0;
	generic[i].Spacing.x = 6;
	generic[i].Spacing.y = 0;
	bits[i] = &fbits[i][6];
    }
    for (p = &fbits[0][0], i = sizeof fbits/sizeof(short); --i>=0; ) {
	*p = *p<<1;
	p++;
    }
    ImplodeFont ();
}
