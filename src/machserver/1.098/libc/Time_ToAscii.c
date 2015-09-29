/* 
 * Time_ToAscii.c --
 *
 *	Source code for the Time_ToAscii library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/time/RCS/Time_ToAscii.c,v 1.4 90/09/11 14:28:34 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>
#include <string.h>

/* 
 * Forward declarations:
 */

static void ConvertToNum _ARGS_((char *cp, int n, Boolean blank));


/* 
 *----------------------------------------------------------------------
 *
 *  ConvertToNum --
 *
 *	Converts a number into ascii string. The blank argument,
 *	if TRUE specifies blank padding, and if FALSE, specifies
 *	zero padding.
 *
 *  Result:
 *	None.
 *
 *  Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ConvertToNum(cp, n, blank)
    register char *cp;
    int n;
    Boolean blank;
{
	if (n>=10) {
	    *cp = (n/10)%10 + '0';
	} else if (blank) {
	    *cp = ' ';
	}
	cp++;
	*cp = n%10 + '0';
}

/*
 *----------------------------------------------------------------------
 *
 *  Time_ToAscii --
 *
 *	Converts a time value into a human-readable string of
 *	the current (absolute) time or relative time. 
 *	The current time has the form:
 *		Wed, 1 Jan 86 10:24:38
 *	If relative time is specified, the time value is interpreted 
 *	as the difference between two absolute time values and has the form:
 *		0 days, 02:45:09
 *
 *	Note: this routine can be simplied once a sprintf routine is available.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Time_ToAscii(time, relativeTime, bufferPtr)
    int		time;
    Boolean	relativeTime;
    char	*bufferPtr;
{
    register char 	*cp, *ncp;
    Time_Parts		parts;

    Time_ToParts(time, relativeTime, &parts);


    if (relativeTime) {
	strcpy(bufferPtr, " 00+00:00:00");
	/*	           012345678901+1 for null byte*/

#define DAY_OFFSET		1
#define HOUR_OFFSET 		4
#define MINUTE_OFFSET 		7
#define SECOND_OFFSET 		10

	cp = &bufferPtr[SECOND_OFFSET];
	ConvertToNum(cp, parts.seconds, FALSE);

	cp = &bufferPtr[MINUTE_OFFSET];
	ConvertToNum(cp, parts.minutes, FALSE);

	cp = &bufferPtr[HOUR_OFFSET];
	ConvertToNum(cp, parts.hours, FALSE);

	cp = &bufferPtr[DAY_OFFSET];
	if (parts.dayOfYear < 0) {
	    bufferPtr[0] = '-';
	    ConvertToNum(cp, -parts.dayOfYear, TRUE);
	} else {
	    ConvertToNum(cp, parts.dayOfYear, TRUE);
	}


    } else {

	/*
	 * Use the Arpanet standard format for date & time.
	 * The timezone is ignored for now (goes at the end of the line).
	 */

	strcpy(bufferPtr, "Day, DD MMM YY 00:00:00");
	/*	           01234567890123456789012 +1 for null byte*/

#define DAY_OF_WEEK 	0
#define DAY_OF_MONTH 	5
#define MONTH 		8
#define YEAR 		12
#define HOUR 		15
#define MINUTE 		18
#define SECOND 		21
#define TIMEZONE 	24

	cp = &bufferPtr[SECOND];
	ConvertToNum(cp, parts.seconds, FALSE);

	cp = &bufferPtr[MINUTE];
	ConvertToNum(cp, parts.minutes, FALSE);

	cp = &bufferPtr[HOUR];
	ConvertToNum(cp, parts.hours, FALSE);

	ncp = &"SunMonTueWedThuFriSat"[3* parts.dayOfWeek ];
	cp = &bufferPtr[DAY_OF_WEEK];
	cp[0] = ncp[0];
	cp[1] = ncp[1];
	cp[2] = ncp[2];
	cp += 3;
	ncp += 3;

	cp = &bufferPtr[YEAR];
	ConvertToNum(cp, parts.year, FALSE);	/* last 2 digits of year */

	cp = &bufferPtr[MONTH];
	ncp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[parts.month*3];
	cp[0] = ncp[0];
	cp[1] = ncp[1];
	cp[2] = ncp[2];
	cp += 3;
	ncp += 3;

	cp = &bufferPtr[DAY_OF_MONTH];
	ConvertToNum(cp, parts.dayOfMonth, TRUE);
    }

}
