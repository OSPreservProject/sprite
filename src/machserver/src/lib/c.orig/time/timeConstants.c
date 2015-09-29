/* 
 * timeConstants.c --
 *
 *	Definitions of variables providing frequently-used time values.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: timeConstants.c,v 1.2 88/06/27 17:23:43 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>

Time time_ZeroSeconds		= {        0,      0 };
Time time_OneMicrosecond	= {        0,      1 };
Time time_OneMillisecond	= {        0,   1000 };
Time time_TenMilliseconds	= {        0,  10000 };
Time time_HundredMilliseconds	= {        0, 100000 };
Time time_HalfSecond		= {        0, 500000 };
Time time_OneSecond		= {        1,      0 };
Time time_TwoSeconds		= {        2,      0 };
Time time_TenSeconds		= {       10,      0 };
Time time_OneMinute		= {       60,      0 };
Time time_OneHour		= {     3600,      0 };
Time time_OneDay		= {    86400,      0 };
Time time_OneYear		= { 31536000,      0 };
Time time_OneLeapYear		= { 31622400,      0 };
Time_Parts time_ZeroParts	= {
    0, 		/* year 	*/
    0, 		/* month 	*/
    0, 		/* dayOfYear 	*/
    0, 		/* dayOfMonth 	*/
    0, 		/* dayOfWeek 	*/
    0,		/* hours 	*/
    0,		/* minutes 	*/
    0,		/* seconds 	*/
    0, 		/* localOffset 	*/
    FALSE, 	/* is DST?	*/
};
