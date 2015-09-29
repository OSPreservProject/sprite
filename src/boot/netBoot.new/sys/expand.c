
/*
 * @(#)expand.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This routine expands a compressed-format font table into
 * its working (uncompressed) form, for speed.
 *
 * Working font tables contain CHRHEIGHT-1 shorts per character,
 * and 96 characters (starting from 0x20 -- space).  (The top and bottom
 * lines of each character are 0, so we overlap the bottom of char 'x'
 * with the top of char 'x+1' to make them fit.)  Of each short,
 * only the top CHRWIDTH bits are used; the rest of the bits in the
 * short must be zeros for Sun-2 software rasterops.
 *
 * Compressed font tables are in three pieces.
 *
 * The first piece is a bit vector where ones indicate that the current
 * short (line of bits) is different than the previous one.  The vector
 * is scanned from left to right and every time a 1-bit is encountered,
 * the next short is picked up and used.  For each zero bit, the previous
 * short is re-used.  There are  (CHRHEIGHT-2)*96  bits in the vector,
 * because the working font table has size (CHRHEIGHT-1)*96 and we know
 * that the first of those CHRHEIGHT-1 words per character is always zero.
 *
 * The third piece is a list of all of the combinations of bits
 * that occur in the font.  Each one is CHRWIDTH bits wide.  There
 * are no duplicated entries in this piece.
 *
 * The second part is an index which indicates, for each 1-bit in the
 * first piece, which entry from the third piece it represents.
 *
 * A short example:
 *
 * 1st: 1 0 0 0 0 1 0 0 0 1 0 0 1 1 1 0 0 0 0	(nineteen entries*1 bit)
 * 2nd: 0         1       0     2 3 1       	(six entries, 8 bit)
 * 3rd: 0x000  0x0C0  0x487  0x48F 		(four entries, 12 bit)
 *
 * This produces the working table: (19 entries, 16 bit w/4 bits padding)
 *
 *	0x0000	0x0000	0x0000	0x0000	0x0000
 *	0x0C00 	0x0C00	0x0C00	0x0C00
 *	0x0000	0x0000	0x0000
 *	0x4870
 *	0x48F0
 *	0x0C00	0x0C00	0x0C00	0x0C00	0x0C00
 *
 * (which is really a regular matrix of shorts, not a list of variable
 * length vectors as it is depicted here.  The depiction is only to show
 * how it was derived.  NOTE THAT THE ABOVE EXAMPLE DOESN'T DEMONSTRATE
 * THAT EVERY 20 ENTRIES, WE INSERT A ZERO.)
 */

/*
	THIS CODE DEPENDS ON CHRWIDTH = 12.
*/


fexpand(ptable, pbitmap, bitmaplen, pindex, pdata_hi, pdata_lo)
	register unsigned short *ptable;
	register unsigned char *pbitmap;
	register unsigned char *pindex;
	unsigned char *pdata_hi, *pdata_lo;
{
	register short i, j;
	register unsigned short curshort;
	register unsigned char bits;
	register unsigned char index;
	register unsigned char low;
	char twenty = 20;
	
	*ptable++ = 0;
	for (i = bitmaplen-1; i != -1; i--) {
		bits = *pbitmap++;
		for (j = 7; j != -1; j--) {
			if (bits&0x80) {
				/* curshort = data[*pindex++]; */
/* BEGIN SECTION THAT DEPENDS ON CHRWIDTH=12 */
				index = *pindex++;
				curshort = pdata_hi[index]<<8;
				low = pdata_lo[index>>1];
				curshort |= 0x00F0 & ((index&1)? low<<4: low);
				/* Low 4 bits are trash anyway */
/* END SECTION THAT DEPENDS ON CHRWIDTH=12 */
			}
			*ptable++ = curshort;
			if (!--twenty) {
				*ptable++ = 0;
				twenty = 20;
			}
			bits <<= 1;
		}
	}
}
