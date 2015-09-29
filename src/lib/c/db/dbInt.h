/*
 * dbInt.h --
 *
 *	Declarations of constants and variables shared by the dataBase
 *	routines.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $DBINT: proto.h,v 1.2 88/03/11 08:39:40 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _DBINT
#define _DBINT

#include <syslog.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * All database files are created with the following mode [for now, at
 * least].
 */
#define FILE_MODE 0664

extern int errno;
extern long lseek();
extern char *strerror();

#endif _DBINT
