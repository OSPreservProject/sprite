/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)time.h	1.2 (Berkeley) 3/4/87
 */

#ifndef _TIME
#define _TIME

#include <cfuncproto.h>

#ifndef _TIME_T
#define _TIME_T
typedef	long	time_t;
typedef	long	clock_t;
#endif

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	long	tm_gmtoff;
	char	*tm_zone;
};

_EXTERN clock_t clock _ARGS_((void));
_EXTERN time_t time _ARGS_((time_t *tp));
_EXTERN time_t mktime _ARGS_((struct tm *tp));
_EXTERN double difftime _ARGS_((time_t time2, time_t time1));
_EXTERN struct tm *gmtime _ARGS_((_CONST time_t *tp));
_EXTERN struct tm *localtime _ARGS_((_CONST time_t *tp));
_EXTERN char *asctime _ARGS_((_CONST struct tm *tp));
_EXTERN char *ctime _ARGS_((_CONST time_t *tp));
_EXTERN int strftime _ARGS_ ((char *s, int smax, _CONST char *fmt,
                             _CONST struct tm *tp));

#endif /* _TIME */
