/*
 * ulogInt.h --
 *
 *	Declarations of constants and variables shared by the ulog
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
 * $ulogInt: proto.h,v 1.2 88/03/11 08:39:40 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _ULOGINT
#define _ULOGINT

#include <stdio.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <string.h>
#include <sys/time.h>
#include <db.h>
#include <host.h>


/*
 * DEBUG_ULOG controls printing error messages when returning failure
 * conditions.
 */

#define DEBUG_ULOG
    
/*
 * Define the length of a record in the database.  Records are fixed-length
 * strings, padded by null characters if necessary.  Each integer in the
 * string is of length MAX_INT_STR_LEN or less (10 digits, plus a sign,
 * plus a  byte for the intervening space).
 */

#define MAX_INT_STR_LEN 12
#define ULOG_RECORD_LENGTH ((4 * MAX_INT_STR_LEN) + ULOG_LOC_LENGTH)
#define ULOG_FORMAT_STRING "%d %d %d %d %s"
#define ULOG_ITEM_COUNT 5

extern int Ulog_WriteLogEntry();

extern int errno;

#endif _ULOGINT
