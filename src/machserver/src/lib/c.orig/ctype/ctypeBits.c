/* 
 * ctypeBits.c --
 *
 *	Contains the array of flags used by the ctype macros and
 *	procedures.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: ctypeBits.c,v 1.1 88/04/27 18:03:22 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "ctype.h"

/* The array below has flags set corresponding to certain types
 * of characters. See the macros defined in ctype.h for details.
 * The first slot in the array corresponds to the character value
 * -1 (EOF).
 */

char _ctype_bits[257] = {
    /*   -1	*/   0,
    /*    0     */   CTYPE_CONTROL,
    /*  0x1     */   CTYPE_CONTROL,
    /*  0x2     */   CTYPE_CONTROL,
    /*  0x3     */   CTYPE_CONTROL,
    /*  0x4     */   CTYPE_CONTROL,
    /*  0x5     */   CTYPE_CONTROL,
    /*  0x6     */   CTYPE_CONTROL,
    /*  0x7     */   CTYPE_CONTROL,
    /*  0x8     */   CTYPE_CONTROL,
    /*  0x9     */   CTYPE_SPACE|CTYPE_CONTROL,
    /*  0xa     */   CTYPE_SPACE|CTYPE_CONTROL,
    /*  0xb     */   CTYPE_SPACE|CTYPE_CONTROL,
    /*  0xc     */   CTYPE_SPACE|CTYPE_CONTROL,
    /*  0xd     */   CTYPE_SPACE|CTYPE_CONTROL,
    /*  0xe     */   CTYPE_CONTROL,
    /*  0xf     */   CTYPE_CONTROL,
    /* 0x10     */   CTYPE_CONTROL,
    /* 0x11     */   CTYPE_CONTROL,
    /* 0x12     */   CTYPE_CONTROL,
    /* 0x13     */   CTYPE_CONTROL,
    /* 0x14     */   CTYPE_CONTROL,
    /* 0x15     */   CTYPE_CONTROL,
    /* 0x16     */   CTYPE_CONTROL,
    /* 0x17     */   CTYPE_CONTROL,
    /* 0x18     */   CTYPE_CONTROL,
    /* 0x19     */   CTYPE_CONTROL,
    /* 0x1a     */   CTYPE_CONTROL,
    /* 0x1b     */   CTYPE_CONTROL,
    /* 0x1c     */   CTYPE_CONTROL,
    /* 0x1d     */   CTYPE_CONTROL,
    /* 0x1e     */   CTYPE_CONTROL,
    /* 0x1f     */   CTYPE_CONTROL,
    /* 0x20 " " */   CTYPE_SPACE|CTYPE_PRINT,
    /* 0x21 "!" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x22 """ */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x23 "#" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x24 "$" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x25 "%" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x26 "&" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x27 "'" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x28 "(" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x29 ")" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x2a "*" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x2b "+" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x2c "," */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x2d "-" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x2e "." */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x2f "/" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x30 "0" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x31 "1" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x32 "2" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x33 "3" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x34 "4" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x35 "5" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x36 "6" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x37 "7" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x38 "8" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x39 "9" */   CTYPE_DIGIT|CTYPE_PRINT,
    /* 0x3a ":" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x3b ";" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x3c "<" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x3d "=" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x3e ">" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x3f "?" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x40 "@" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x41 "A" */   CTYPE_UPPER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x42 "B" */   CTYPE_UPPER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x43 "C" */   CTYPE_UPPER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x44 "D" */   CTYPE_UPPER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x45 "E" */   CTYPE_UPPER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x46 "F" */   CTYPE_UPPER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x47 "G" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x48 "H" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x49 "I" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x4a "J" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x4b "K" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x4c "L" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x4d "M" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x4e "N" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x4f "O" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x50 "P" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x51 "Q" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x52 "R" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x53 "S" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x54 "T" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x55 "U" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x56 "V" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x57 "W" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x58 "X" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x59 "Y" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x5a "Z" */   CTYPE_UPPER|CTYPE_PRINT,
    /* 0x5b "[" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x5c "\" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x5d "]" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x5e "^" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x5f "_" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x60 "`" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x61 "a" */   CTYPE_LOWER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x62 "b" */   CTYPE_LOWER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x63 "c" */   CTYPE_LOWER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x64 "d" */   CTYPE_LOWER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x65 "e" */   CTYPE_LOWER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x66 "f" */   CTYPE_LOWER|CTYPE_PRINT|CTYPE_HEX_DIGIT,
    /* 0x67 "g" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x68 "h" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x69 "i" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x6a "j" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x6b "k" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x6c "l" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x6d "m" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x6e "n" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x6f "o" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x70 "p" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x71 "q" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x72 "r" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x73 "s" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x74 "t" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x75 "u" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x76 "v" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x77 "w" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x78 "x" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x79 "y" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x7a "z" */   CTYPE_LOWER|CTYPE_PRINT,
    /* 0x7b "{" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x7c "|" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x7d "}" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x7e "~" */   CTYPE_PUNCT|CTYPE_PRINT,
    /* 0x7f     */   CTYPE_CONTROL
};
