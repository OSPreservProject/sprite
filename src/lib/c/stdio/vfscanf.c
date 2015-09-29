/* 
 * vfscanf.c --
 *
 *	Source code for the "vfscanf" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdio/RCS/vfscanf.c,v 1.6 92/01/21 16:27:07 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <varargs.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*
 * Maximum number of bytes in allowable ASCII representation of a
 * floating-point number:
 */

#define MAX_FLOAT_SIZE 350
/*
 *----------------------------------------------------------------------
 *
 * vfscanf --
 *
 *	This utility routine does all of the real work of scanning
 *	fields under control of a format string.  It is called by
 *	scanf, fscanf, and sscanf.
 *
 * Results:
 *	Values addressed by elements of args are modified to hold
 *	values scanned from stream.  The return value is a count of
 *	the number of fields successfully scanned from stream, or
 *	EOF if no characters could be input from stream.
 *
 * Side effects:
 *	Information is input from stream.
 *
 *----------------------------------------------------------------------
 */

int
vfscanf(stream, format, args)
    register FILE *stream;	/* Where to read characters for parsing. */
    register char *format;	/* Contains literal text and format control
				 * sequences indicating how args are to be
				 * scanned.  See the man page for details. */
    va_list args;		/* Addresses of a variable number of arguments
				 * to be modified. */
{
    int suppress;		/* TRUE means scan value but don't actual
				 * modify an element of args. */
    int storeShort;		/* TRUE means store a short value. */
    int storeLong;		/* TRUE means store a long value. */
    int width;			/* Field width. */
    register char formatChar; 	/* Current character from format string.
				 * Eventually it ends up holding the format
				 * type (e.g. 'd' for decimal). */
    register int streamChar;	/* Next character from stream. */
    int assignedFields;		/* Counts number of successfully-assigned
				 * fields. */
    int base;			/* Gives base for numbers:  0 means float,
				 * -1 means not a number.
				 */
    int sign;			/* TRUE means negative sign. */
    char buf[MAX_FLOAT_SIZE+1];
				/* Place to accumulate floating-point
				 * number for processing. */
    register char *ptr = (char *) NIL;
    char *savedPtr, *end, *firstPtr;

    assignedFields = 0;
    streamChar = getc(stream);
    if (streamChar == EOF) {
	return(EOF);
    }

    /*
     * The main loop is to scan through the characters in format.
     * Anything but a '%' must match the next character from stream.
     * A '%' signals the start of a format field;  the formatting
     * information is parsed, the next value is scanned from the stream
     * and placed in memory, and the loop goes on.
     */

    for (formatChar = *format; (formatChar != 0) && (streamChar != EOF);
	    format++, formatChar = *format) {
	
	/*
	 * A white-space format character matches any number of
	 * white-space characters from the stream.
	 */

	if (isspace(formatChar)) {
	    while (isspace(streamChar)) {
		streamChar = getc(stream);
	    }
	    continue;
	}

	/*
	 * Any character but % must be matched exactly by the stream.
	 */

	if (formatChar != '%') {
	    if (streamChar != formatChar) {
		break;
	    }
	    streamChar = getc(stream);
	    continue;
	}

	/*
	 * Parse off the format control fields.
	 */

	suppress = FALSE;
	storeLong = FALSE;
	storeShort = FALSE;
	width = -1;
	format++;  
	formatChar = *format;
	if (formatChar == '*') {
	    suppress = TRUE;
	    format++; 
	    formatChar = *format;
	}
	if (isdigit(formatChar)) {
	    width = strtoul(format, &end, 10);
	    format = end;
	    formatChar = *format;
	}
	if (formatChar == 'l') {
	    storeLong = TRUE;
	    format++; 
	    formatChar = *format;
	}
	if (formatChar == 'h') {
	    storeShort = TRUE;
	    format++; 
	    formatChar = *format;
	}

	/*
	 * Skip any leading blanks in the input (except for 'c' format).
	 * Also, default the width to infinity, except for 'c' format.
	 */
	
	if ((formatChar != 'c') && (formatChar != '[')) {
	    while (isspace(streamChar)) {
		streamChar = getc(stream);
	    }
	}
	if ((width <= 0) && (formatChar != 'c')) {
	    width = 1000000;
	}

	/*
	 * Check for EOF again after parsing away the white space.
	 */
	if (streamChar == EOF) {
	    break;
	}
	
	/*
	 * Process the conversion character.  For numbers, this just means
	 * turning it into a "base" number that indicates how to read in
	 * a number.
	 */
	
	base = -1;
	switch (formatChar) {

	    case '%':
		if (streamChar != '%') {
		    goto done;
		}
		streamChar = getc(stream);
		break;
	    
	    case 'D':
		storeShort = FALSE;
	    case 'd':
		base = 10;
		break;
	    
	    case 'O':
		storeShort = FALSE;
	    case 'o':
		base = 8;
		break;
	    
	    case 'X':
		storeShort = FALSE;
	    case 'x':
		base = 16;
		break;
	    
	    case 'E':
	    case 'F':
		storeLong = TRUE;
	    case 'e':
	    case 'f':
		base = 0;
		break;

	    /*
	     * Characters and strings are handled in exactly the same way,
	     * except that for characters the default width is 1 and spaces
	     * are not considered terminators.
	     */

	    case 'c':
		if (width <= 0) {
		    width = 1;
		}
	    case 's':
		if (suppress) {
		    while ((width > 0) && (streamChar != EOF)) {
			if (isspace(streamChar) && (formatChar == 's')) {
			    break;
			}
			streamChar = getc(stream);
			width--;
		    }
		} else {
		    ptr = va_arg(args, char *);
		    while ((width > 0) && (streamChar != EOF)) {
			if (isspace(streamChar) && (formatChar == 's')) {
			    break;
			}
			*ptr = streamChar;
			ptr++;
			streamChar = getc(stream);
			width--;
		    }
		    if (formatChar == 's') {
			*ptr = 0;
		    }
		    assignedFields++;
		}
		break;
	    
	    case '[':
		format++; formatChar = *format;
		if (formatChar == '^') {
		    format++;
		}
		if (!suppress) {
		    firstPtr = ptr = va_arg(args, char *);
		}
		savedPtr = format;
		while ((width > 0) && (streamChar != EOF)) {
		    format = savedPtr;
		    while (TRUE) {
			if (*format == streamChar) {
			    if (formatChar == '^') {
				goto stringEnd;
			    } else {
				break;
			    }
			}
			if ((*format == ']') || (*format == 0)) {
			    if (formatChar == '^') {
				break;
			    } else {
				goto stringEnd;
			    }
			}
			format++;
		    }
		    if (!suppress) {
			*ptr = streamChar;
			ptr++;
		    }
		    streamChar = getc(stream);
		    width--;
		}
		stringEnd:
		if (ptr == firstPtr) {
		    goto done;
		}
		while ((*format != ']') && (*format != 0)) {
		    format++;
		}
		formatChar = *format;
		if (!suppress) {
		    *ptr = 0;
		    assignedFields++;
		}
		break;

	    /*
	     * Don't ask why, but for compatibility with UNIX, a null
	     * conversion character must always return EOF, and any
	     * other conversion character must be treated as decimal.
	     */
	    
	    case 0:
		ungetc(streamChar, stream);
		return(EOF);
		
	    default:
		base = 10;
		break;
	}

	/*
	 * If the field wasn't a number, then everything was handled
	 * in the switch statement above.  Otherwise, we still have
	 * to read in a number.  This gets handled differently for
	 * integers and floating-point numbers.
	 */
	
	if (base < 0) {
	    continue;
	}

	if (streamChar == '-') {
	    sign = TRUE;
	    width -= 1;
	    streamChar = getc(stream);
	} else {
	    sign = FALSE;
	    if (streamChar == '+') {
		width -= 1;
		streamChar = getc(stream);
	    }
	}

	/*
	 * If we're supposed to be parsing a floating-point number, read
	 * the digits into a temporary buffer and use the conversion library
	 * routine to convert them.
	 */
	
#define COPYCHAR \
    *ptr = streamChar; ptr++; width--; streamChar = getc(stream);

	if (base == 0) {
	    if (width > MAX_FLOAT_SIZE) {
		width = MAX_FLOAT_SIZE;
	    }
	    ptr = buf;
	    while ((width > 0) && isdigit(streamChar)) {
		COPYCHAR;
	    }
	    if ((width > 0) && (streamChar == '.')) {
		COPYCHAR;
	    }
	    while ((width > 0) && isdigit(streamChar)) {
		COPYCHAR;
	    }
	    if ((width > 0) && ((streamChar == 'e') || (streamChar == 'E'))) {
		COPYCHAR;
		if ((width > 0) &&
			((streamChar == '+') || (streamChar == '-'))) {
		    COPYCHAR;
		}
		while ((width > 0) && isdigit(streamChar)) {
		    COPYCHAR;
		}
	    }
	    *ptr = 0;

	    if (ptr == buf) {		/* Not a valid number. */
		goto done;
	    }

	    if (!suppress) {
		double d;
		d = atof(buf);
		if (sign) {
		    d = -d;
		}
		if (storeLong) {
		    *(va_arg(args, double *)) = d;
		} else {
		    *(va_arg(args, float *)) = d;
		}
		assignedFields++;
	    }
	} else {
	    /*
	     * This is an integer.  Use special-purpose code for the
	     * three supported bases in order to make it run fast.
	     */

	    int i;
	    int anyDigits;

	    i = 0;
	    anyDigits = FALSE;
	    if (base == 10) {
		while ((width > 0) && isdigit(streamChar)) {
		    i = (i * 10) + (streamChar - '0');
		    streamChar = getc(stream);
		    anyDigits = TRUE;
		    width -= 1;
		}
	    } else if (base == 8) {
		while ((width > 0) && (streamChar >= '0')
			&& (streamChar <= '7')) {
		    i = (i << 3) + (streamChar - '0');
		    streamChar = getc(stream);
		    anyDigits = TRUE;
		    width -= 1;
		}
	    } else {
		while (width > 0) {
		    if (isdigit(streamChar)) {
			i = (i << 4) + (streamChar - '0');
		    } else if ((streamChar >= 'a') && (streamChar <= 'f')) {
			i = (i << 4) + (streamChar + 10 - 'a');
		    } else if ((streamChar >= 'A') && (streamChar <= 'F')) {
			i = (i << 4) + (streamChar + 10 - 'A');
		    } else {
			break;
		    }
		    streamChar = getc(stream);
		    anyDigits = TRUE;
		    width--;
		}
	    }
	    if (!anyDigits) {
		goto done;
	    }
	    if (sign) {
		i = -i;
	    }
	    if (!suppress) {
		if (storeShort) {
		    *(va_arg(args, short *)) = i;
		} else {
		    *(va_arg(args, int *)) = i;
		}
		assignedFields++;
	    }
	}
    }

    done:
    ungetc(streamChar, stream);
    if ((streamChar == EOF) && (assignedFields == 0)) {
	return(EOF);
    }
    return(assignedFields);
}
