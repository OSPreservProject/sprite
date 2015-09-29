#include "font.h"

struct {
	struct font f;
	short bits[128][16];
} f;

short bits [128][16] = {
#include "gacha12.c"
};

main () {
    register    i,
                j;
    strcpy (f.f.FamilyName, "SAIL");
    f.f.rotation = 0;
    f.f.magic = FONTMAGIC;
    f.f.FaceCode = 0;
    f.f.height = 7;
    f.f.mrows = 16;
    f.f.mcols = 9;
    for (i = 0; i < 128; i++) {
	f.f.chars[i].rows = 16;
	f.f.chars[i].cols = 8;
	f.f.chars[i].orow = 10;
	f.f.chars[i].ocol = 0;
	f.f.chars[i].rdelt = 0;
	f.f.chars[i].cdelt = 9;
	f.f.chars[i].bits = sizeof f.f + 32 * i + ((int) &f) - ((int) &f.f.chars[i]);
    }
    for (i = 0; i < 128; i++)
	for (j = 0; j < 16; j++)
	    f.bits[i][j] = bits[i][j];
    write (creat (fontfname (f.f.FamilyName, f.f.height, f.f.FaceCode, f.f.rotation), 0644), &f, sizeof f);
}
