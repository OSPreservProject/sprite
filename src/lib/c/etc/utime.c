/* 
 * utime.c --
 *
 *	Compatiblity routine for obsolete function.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/utime.c,v 1.1 90/01/23 14:49:44 douglis Exp $";
#endif /* not lint */

#include <sys/time.h>

/*
 *----------------------------------------------------------------------
 *
 * utime --
 *
 *     utime() sets the access and modification times of  the  file
 *     named by file.
 *
 *     If the timep argument is NULL, the access  and  modification
 *     times  are  set  to the current time.  A process must be the
 *     owner of the file or have write permission for the  file  to
 *     use utime() in this manner.
 *
 *     If the timep argument is not NULL, it is assumed to point to
 *     an  array  of  two time_t values.  The access time is set to
 *     the value of the first member, and the modification time  is
 *     set  to the value of the second member.  The times contained
 *     in that array are measured in seconds since 00:00:00 GMT Jan
 *     1,  1970.   Only the owner of the file or the super-user may
 *     use utime() in this manner.
 *
 *     In either case, the ``inode-changed'' time of  the  file  is
 *     set to the current time.
 *
 * Results:
 *
 *      Upon successful completion, a value of 0 is returned.  Otherwise,
 *      a value of -1 is returned and errno is set to indicate the error.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */
int
utime(file, timep)
        char *file;
        int *timep;
{
        struct timeval tv[2];

        tv[0].tv_sec = timep[0];
	tv[0].tv_usec = 0;
        tv[1].tv_sec = timep[1];
	tv[1].tv_usec = 0;
        return utimes(file, tv);
}
