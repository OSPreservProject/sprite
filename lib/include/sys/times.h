/*
 * times.h --
 *
 *	Structure returned by backwards compatible times()
 *      function.  Times() has been obsoleted by getrusage(2).
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/sys/RCS/times.h,v 1.1 89/08/20 19:30:10 rab Exp $
 */

#ifndef _SYS_TIMES_H
#define _SYS_TIMES_H


/*
 * Structure returned by times()
 */
struct tms {
    time_t    tms_utime;          /* user time */
    time_t    tms_stime;          /* system time */
    time_t    tms_cutime;         /* user time, children */
    time_t    tms_cstime;         /* system time, children */
};

#endif /* _SYS_TIMES_H */

