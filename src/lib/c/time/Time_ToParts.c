/* 
 * Time_ToParts.c --
 *
 *	Source code for the Time_ToParts library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/time/RCS/Time_ToParts.c,v 1.4 90/09/11 14:28:37 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>

/* 
 * Forward declarations:
 */
static int YearLength _ARGS_((int year));


/* 
 *----------------------------------------------------------------------
 *
 *  YearLength --
 *
 *  Result:
 *	The number of days in the year.
 *
 *  Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
YearLength(year)
    int year;
{
    if (year%400 == 0) {
	return(366);
    }
    if (year%100 == 0) {
	return(355);
    }
    if (year%4 == 0) {
	return(366);
    }
    return(365);
}

/*
 *----------------------------------------------------------------------
 *
 *  Time_ToParts --
 *
 *	Converts a time value into its components.
 *	If relative time is specified, the time is broken down to days,
 *	hours, minutes and seconds only.
 *
 *	Modified from UNIX ctime.c -- NEEDS TO BE REWRITTEN!!
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
Time_ToParts(time, relativeTime, partsPtr)
    int 	time;
    Boolean 	relativeTime;
    register Time_Parts *partsPtr;
{
	int 		hms, day;
	int		dayOfMonth, month, year;
	Boolean		negative;
	static int 	monthLengths[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	static int 	leapMonthLengths[] = {
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int		*monthLenPtr;

	/*
	 * break initial number into days
	 */

	if (time < 0) {
	    negative = TRUE;
	    time = -time;
	} else {
	    negative = FALSE;
	}
	hms = time % 86400;
	day = time / 86400;
	if (hms<0) {
	    hms += 86400;
	    day -= 1;
	}

	/*
	 * generate hours:minutes:seconds
	 */
	partsPtr->seconds 	= hms % 60;
	hms 			/= 60;
	partsPtr->minutes	= hms % 60;
	partsPtr->hours 	= hms / 60;

	/*
	 * For a relative time, determine days, hours, minutes and seconds only.
	 */

	if (relativeTime) {
	    if (negative) {
		partsPtr->dayOfYear = -day;
	    } else {
		partsPtr->dayOfYear = day;
	    }
	    return;
	}

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4 mod 7 (1/1/1970 was Thursday)
	 */

	partsPtr->dayOfWeek = (day + 7340036) % 7;

	/*
	 * year number
	 */
	if (day >= 0) {
	    for(year=70; day >= YearLength(year); year++) {
		day -= YearLength(year);
	    }
	} else {
	    for (year=70; day<0; year--) {
		day += YearLength(year-1);
	    }
	}
	partsPtr->year 		= year;
	partsPtr->dayOfYear 	= day;

	/*
	 * generate month
	 */

	if (YearLength(year)==366) {
	    monthLenPtr = leapMonthLengths;
	} else {
	    monthLenPtr = monthLengths;
	}
	
	for (dayOfMonth = day, month=0; 
	     dayOfMonth >= monthLenPtr[month]; 
	     month++) {
	    dayOfMonth -= monthLenPtr[month];
	}
	dayOfMonth += 1;
	partsPtr->dayOfMonth 	= dayOfMonth;
	partsPtr->month 	= month;
}

/*
 *----------------------------------------------------------------------
 *
 *  Time_FromParts --
 *
 *	Converts components of a time into a time value.
 *	If relative time is specified, the time comprised of days,
 *	hours, minutes and seconds only.
 *
 *	Modified from UNIX ctime.c -- NEEDS TO BE REWRITTEN!!
 *      IMPORTANT: won't do dates before 1970!!!
 *
 * Results:
 *	SUCCESS if time was converted, FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Time_FromParts(partsPtr,relativeTime, timePtr)
    register Time_Parts *partsPtr;
    Boolean 	relativeTime;
    int 	*timePtr;
{

#define CENTURY	1900

    int		month, year;
    static int 	monthLengths[] = {
	    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    static int 	leapMonthLengths[] = {
	    31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int		*monthLenPtr;
    int 		days;
    int		seconds;
    int		dayOfYear = 0;

    if  (partsPtr->year < 70) {
	return FAILURE;
    }
    if ((partsPtr->dayOfMonth == -1) && (partsPtr->dayOfYear == -1)) {
	return FAILURE;
    }
    days = 0;
    if (!relativeTime) {
	for (year = 1970; year < partsPtr->year + CENTURY; year++) {
	    days += YearLength(year);
	} 
	if (partsPtr->dayOfMonth != -1) {
	    if (YearLength(partsPtr->year + CENTURY) == 366) {
		monthLenPtr = leapMonthLengths;
	    } else {
		monthLenPtr = monthLengths;
	    }
	    for (month = 0; month < partsPtr->month; month++) {
		dayOfYear += monthLenPtr[month];
	    }
	    dayOfYear += partsPtr->dayOfMonth - 1;
	    if (partsPtr->dayOfYear != -1) {
		if (dayOfYear != partsPtr->dayOfYear) {
		    return FAILURE;
		}
	    }
	} else {
	    dayOfYear = partsPtr->dayOfYear;
	}
    }
    days += dayOfYear;
    seconds = days * (24 * 60 * 60);
    seconds += partsPtr->hours * 60 * 60;
    seconds += partsPtr->minutes * 60;
    seconds += partsPtr->seconds;
    *timePtr = seconds;
    return SUCCESS;
}
