/*
 * stdtypes.h --
 *
 *	Typedefs for ANSI C and POSIX.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/sys/RCS/stdtypes.h,v 1.2 92/09/24 15:54:06 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _SYS_STDTYPES_H
#define _SYS_STDTYPES_H

typedef int             sigset_t;
typedef unsigned int    speed_t;
typedef unsigned long   tcflag_t;
typedef unsigned char   cc_t;
#ifndef _PID_T
#define _PID_T
typedef int             pid_t;
#endif
#ifndef _MODE_T
#define _MODE_T
typedef	unsigned short  mode_t;
#endif
typedef short           nlink_t;
#ifndef _CLOCK_T
#define _CLOCK_T
typedef long            clock_t;
#endif
#ifndef _TIME_T
#define _TIME_T
typedef long            time_t;
#endif
#ifndef _SIZE_T
#define _SIZE_T
typedef int             size_t;
#endif
typedef int             ptrdiff_t;
typedef unsigned short  wchar_t;

#endif /* _SYS_STDTYPES_H */

