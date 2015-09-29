
/*
 * @(#)ktab.s2.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	keytab.s2.c
 *
 *	This module contains the translation tables for the up-down encoded
 *	Sun-2 keyboard.
 *
 *	A fuller explanation of the entries is in keyboard.h.
 *
 *	Note that the "break" key is defined as a meta key; it turns on the
 *	0x80 bit of the returned character.  The "upper left" key turns on the 
 *	0x100 bit, which is ignored by most callers since they expect only a
 *	byte.  NOTE that UpperLeft-A, pressed in that order with nothing else
 *	in between, aborts whatever program is running, and never passes on
 *	the A to the user program.
 */

#include "../h/keyboard.h"

/* handy way to define control characters in the tables */
#define	c(char)	(char&0x1F)

/* Unshifted keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_lc[1] = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	OOPS,
/*  8 */	OOPS, 	OOPS, 	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 16 */	OOPS, 	OOPS, 	OOPS,	ALT,	HOLE,	OOPS,	OOPS,	OOPS,
/* 24 */	HOLE, 	OOPS, 	OOPS,	OOPS,	HOLE,	c('['),	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	OOPS,	OOPS,	OOPS,
/* 48 */	HOLE,	OOPS,	OOPS,	OOPS,	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'[',	']',	0x7F,	HOLE,	OOPS,	STRING+UPARROW,
								OOPS,	HOLE,
/* 72 */	OOPS,	OOPS,	OOPS,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						OOPS,	STRING+RIGHTARROW,
								HOLE,	OOPS,
/* 96 */	OOPS,	OOPS,	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'z',	'x',	'c',	'v',
/*104 */	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	OOPS,	STRING+DOWNARROW,
				OOPS,	HOLE,	HOLE,	HOLE,	HOLE, CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Shifted keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_uc[1] = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				OOPS,	OOPS,	HOLE,	OOPS,	OOPS,	OOPS,
/*  8 */	OOPS, 	OOPS, 	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 16 */	OOPS, 	OOPS, 	OOPS,	ALT,	HOLE,	OOPS,	OOPS,	OOPS,
/* 24 */	HOLE, 	OOPS, 	OOPS,	OOPS,	HOLE,	c('['),	'!',	'@',
/* 32 */	'#',	'$',	'%',	'^',	'&',	'*',	'(',	')',
/* 40 */	'_',	'+',	'~',	'\b',	HOLE,	OOPS,	OOPS,	OOPS,
/* 48 */	HOLE,	OOPS,	OOPS,	OOPS,	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	0x7F,	HOLE,	OOPS,	STRING+UPARROW,
								OOPS,	HOLE,
/* 72 */	OOPS,	OOPS,	OOPS,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	':',	'"',
/* 88 */	'|',	'\r',	HOLE,	STRING+LEFTARROW,
						OOPS,	STRING+RIGHTARROW,
								HOLE,	OOPS,
/* 96 */	OOPS,	OOPS,	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	OOPS,	STRING+DOWNARROW,
				OOPS,	HOLE,	HOLE,	HOLE,	HOLE, CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Index to keymaps for Sun-2 keyboard */
static struct keyboard keyindex_s2 [1] = {
	keytab_s2_lc, keytab_s2_uc, 0, 0, 0,
	0x0000,		/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
};

/***************************************************************************/
/*   Index table for the whole shebang					   */
/***************************************************************************/
struct keyboard *keytables[] = { keyindex_s2 };
