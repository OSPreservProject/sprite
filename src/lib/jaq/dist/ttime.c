/* 
 * ttime.c --
 *
 *	time conversion utilities
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Quote:
 *      "A moment's thought would have shown him,
 *       but a moment is a long time and thought 
 *       is a painful process."
 *       -- A. E. Housman
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/ttime.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

#define T_DATELEN 21
#define DATESEPARATORS "/-"
#define TIMESEPARATORS ".:"
#define SECONDSPERDAY 60*60*24

static int ValidateDateParts _ARGS_ ((char *dateStr, struct tm *partsPtr));
static int ValidateTimeParts _ARGS_ ((char *timeStr, struct tm *partsPtr));

static int daysPerMonth[] = 
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static char *months[] = {
    "january",   "february", "march",    "april",
    "may",       "june",     "july",     "august",
    "september", "october",  "november", "december" };




/*
 *----------------------------------------------------------------------
 *
 * Time_Stamp --
 *
 *	Return time-of-day stamp in milliseconds.
 *
 * Results:
 *	time stamp
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

long
Time_Stamp()
{
    struct timeval curTime;

    gettimeofday(&curTime, NULL);
    curTime.tv_sec % (LONG_MAX/2);
    return (curTime.tv_sec*1000) + (curTime.tv_usec/1000);
}


/*
 *----------------------------------------------------------------------
 *
 * Time_CvtToString --
 *
 *	Convert internal format to printable form
 *
 * Results:
 *	static char string. If year is current then include
 *      time in string, else include year instead (a la 'ls' cmd)..
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

char *
Time_CvtToString(timePtr)
    time_t *timePtr;          /* time object to be made printable */
{
    struct UnixTimeStr {
	char dayOfWeek[4];
	char month[4];
	char day[3];
	char time[9];
	char year[4];
	char cr;
	char null;
    } *timeStr;
    char curYear[5];
    time_t curTime;

    curTime = Time_GetCurDate();
    timeStr = (struct UnixTimeStr *) ctime(&curTime);
    strcpy(curYear, timeStr->year);

    timeStr = (struct UnixTimeStr *) ctime(timePtr);
    timeStr->cr = '\0';

    if (strncmp(curYear, timeStr->year, 4) != 0) {
	strcpy(timeStr->time, timeStr->year);
	strcpy(timeStr->time+4, "    ");
    } else {
	timeStr->time[8] = '\0';
    }

    return timeStr->month;

}


/*
 *----------------------------------------------------------------------
 *
 * Time_Compare --
 *
 *	Compare 2 dates for equality.
 *
 * Results:
 *	<1, 0, >0
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
Time_Compare(time1, time2, dateOnly)
    time_t time1;             /* time object1 */
    time_t time2;             /* time object2 */
    int dateOnly;             /* 1 == round off the time to day units */
{
    if (dateOnly) {
	time1 -= (time1 % SECONDSPERDAY);
	time2 -= (time2 % SECONDSPERDAY);
    }

    return ((unsigned)time1-(unsigned)time2);
}



/*
 *----------------------------------------------------------------------
 *
 * Time_GetCurDate --
 *
 *	Calculate current date/time including zone offset.
 *
 * Results:
 *	One long word giving # secs since 1 Jan 1970 GMT.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

time_t
Time_GetCurDate()
{
    struct timeval  curTime;
    struct timezone curZone;
    long time;

    gettimeofday(&curTime, &curZone);
    time = curTime.tv_sec;

/*    fprintf(stderr,"GetCurDate: %s\n", ctime(&time));*/
    return (time);
}


/*
 *----------------------------------------------------------------------
 *
 * getindate --
 *
 *	Convert a date-time string into Unix internal format.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 * Note: This whole thing is a hack. Daylight savings time is not
 *      accounted for.  The attempt is to save the number of seconds
 *      since 1 Jan 1970 0:0:0 GMT.
 *
 *      Collyer and Moraes (U. of Toronto) have written a much more
 *      thorough package but I can't find it and don't know if I could
 *      distribute it even if I had it. I've adhered to their interface
 *      (including not capitalizing the procedure names) so that 
 *      you can plug their stuff in and get better parsing.
 *
 *      Too bad not all systems have SYSV's getdate() routine.
 *
 *----------------------------------------------------------------------
 */

int
getindate(dateStr, timebPtr)
    char *dateStr;            /* incoming date in string form */
    struct timeb *timebPtr;   /* result structure */
{
    char *timeStr;
    int retCode = T_SUCCESS;
    struct timeval curTime;
    struct tm *curTimePtr;
    struct tm curTimeParts;
    time_t *timePtr = &timebPtr->time;

    gettimeofday(&curTime, (struct timezone *)NULL);
    curTimePtr = localtime((time_t *)&curTime.tv_sec);
    curTimePtr->tm_isdst = -1;
    curTimeParts = *curTimePtr;

    if (dateStr == (char *)NULL) {
	return T_FAILURE;
    }

    dateStr = (char *)strtok(dateStr, TIMESEPARATORS);

    if (dateStr == (char *)NULL) {
	return T_FAILURE;
    }

    timeStr = (char *)strtok((char *)NULL, ".");

    retCode = ValidateDateParts(dateStr, &curTimeParts);
    if (retCode == T_SUCCESS) {
	retCode = ValidateTimeParts(timeStr, &curTimeParts);
    }

    if (retCode == T_SUCCESS) {
	*timePtr = mktime(&curTimeParts);
    }

    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * getindatepair --
 *
 *	Convert a pair of date-time strings into Unix internal format.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
getindatepair(dateStr, timebPtr1, timebPtr2)
    char *dateStr;            /* incoming date in string form */
    struct timeb *timebPtr1;  /* result structures */
    struct timeb *timebPtr2;
{
    char *ptr1, *ptr2;

    if (dateStr == (char *)NULL) {
	return T_FAILURE;
    }

    ptr1 = (char *)strtok(dateStr, ",");
    if (ptr1 == (char *)NULL) {
	return T_FAILURE;
    }

    ptr2 = (char *)strtok((char *)NULL, ",");
    if (ptr2 == (char *)NULL) {
	return T_FAILURE;
    }

    if (getindate(ptr1, timebPtr1) == T_SUCCESS) {
	return getindate(ptr2, timebPtr2);
    }
    
    return T_FAILURE;
}


/*
 *----------------------------------------------------------------------
 *
 * ValidateDateParts --
 *
 *      Check and convert date pieces
 *
 * Results:
 *	Return Code.
 *
 * Side effects:
 *	none.
 *
 * Note: To simplify parsing, we only accept dates of the forms:
 *                  dd-month (or) dd-month-yy
 *       where dd, yy are integers and month is a char string.
 *
 *----------------------------------------------------------------------
 */

static int
ValidateDateParts(dateStr, partsPtr)
    char *dateStr;            /* date with time stripped off */
    struct tm *partsPtr;      /* structure to receive parts */
{
    int i;
    int length;
    int count = 0;
    char *dateParts[3];
    int leapYear = 0;
    int day;
    int month;
    int year;

    if ((dateParts[0]=(char *)strtok(dateStr, DATESEPARATORS)) !=
	(char *)NULL) {
	count++;
	if ((dateParts[1]=(char *)strtok(NULL, DATESEPARATORS)) !=
	    (char *)NULL) {
	    count++;
	    if ((dateParts[2]=(char *)strtok(NULL, DATESEPARATORS)) !=
		(char *)NULL) {
		count++;
	    }
	}
    }

    if ((count < 2) || (count > 3) || (strlen(dateParts[1]) < 3)) {
	return T_FAILURE;
    }
    
    /* verify year */
    /* Note: 68 years = MAXDECINT seconds / (60 * 60 * 24 * 7 * 52) */
    if (count > 2) {
	if (Utils_CvtInteger(dateParts[2], 1970, 1970+68, &year) != T_SUCCESS) {
	    return T_FAILURE;
	}
    } else {
	year = partsPtr->tm_year + 1900;
    }
    if ((year % 4 == 0) &&
	((year % 100 != 0) || (year % 400 == 0))) {
	leapYear = 1;
    }

    /* verify month */
    length = strlen(dateParts[1]);
    partsPtr->tm_yday = -1;
    for (i=0; i<12; i++) {
	if ((length <= strlen(months[i])) &&
	    (strncasecmp(dateParts[1], months[i], length) == 0)) {
	    month = i;
	    break;
	}
	partsPtr->tm_yday += daysPerMonth[i];
    }
    if (i >= 12) {
	return T_FAILURE;
    }

    /* verify day */
    if (Utils_CvtInteger(dateParts[0], 0, daysPerMonth[month], &day) !=
	T_SUCCESS) {
	return T_FAILURE;
    }
    if ((day == 29) && (month == 1) && (!leapYear)) {
	return T_FAILURE;
    }
    partsPtr->tm_mday = day;
    partsPtr->tm_yday += day;
    if ((month > 1) && (!leapYear)) {
	partsPtr->tm_yday -= 1;
    }
    partsPtr->tm_year = year - 1900;
    partsPtr->tm_mon = month;
    partsPtr->tm_wday = (partsPtr->tm_yday + partsPtr->tm_year - 1) % 7;

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * ValidateTimeParts --
 *
 *      Check and convert time pieces
 *
 * Results:
 *	Return Code.
 *
 * Side effects:
 *	none.
 *
 * Note: To simplify parsing, we only accept times of the forms:
 *       hh[-mm[-ss]]
 *
 *----------------------------------------------------------------------
 */

static int
ValidateTimeParts(timeStr, partsPtr)
    char *timeStr;            /* time with date stripped off */
    struct tm *partsPtr;      /* structure to receive parts */
{
    char *timeParts[3];
    int count = 0;
    
    partsPtr->tm_hour = 0;
    partsPtr->tm_min = 0;
    partsPtr->tm_sec = 0;

    if (timeStr == (char *)NULL) {
	return T_SUCCESS;
    }

    if ((timeParts[0]=(char *)strtok(timeStr, TIMESEPARATORS)) !=
	(char *)NULL) {
	count++;
	if ((timeParts[1]=(char *)strtok(NULL, TIMESEPARATORS)) !=
	    (char *)NULL) {
	    count++;
	    if ((timeParts[2]=(char *)strtok(NULL, TIMESEPARATORS)) !=
		(char *)NULL) {
		count++;
	    }
	}
    }

    if ((count < 1) || (count > 3)) {
	return T_FAILURE;
    }

    if (count > 2) {
	if (Utils_CvtInteger(timeParts[2], 0, 59, &partsPtr->tm_sec) != T_SUCCESS) {
	    return T_FAILURE;
	}
    }

    if (count > 1) {
	if (Utils_CvtInteger(timeParts[1], 0, 59, &partsPtr->tm_min) != T_SUCCESS) {
	    return T_FAILURE;
	}
    }

    return (Utils_CvtInteger(timeParts[0], 0, 23, &partsPtr->tm_hour));

}


