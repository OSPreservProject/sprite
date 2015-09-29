/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	string.c,v $
 * Revision 2.3  91/02/05  17:55:52  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:55:57  mrt]
 * 
 * Revision 2.2  90/06/02  15:05:46  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:13:46  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Declare and initialize charNULL here for strNull def in string.h
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#define	EXPORT_BOOLEAN
#include <mach/boolean.h>
#include <sys/types.h>
#include "error.h"
#include "alloc.h"
#include "string.h"

string_t
strmake(string)
    char *string;
{
    register string_t saved;

    saved = malloc((u_int) (strlen(string) + 1));
    if (saved == strNULL)
	fatal("strmake('%s'): %s", string, unix_error_string(errno));
    return strcpy(saved, string);
}

string_t
strconcat(left, right)
    string_t left, right;
{
    register string_t saved;

    saved = malloc((u_int) (strlen(left) + strlen(right) + 1));
    if (saved == strNULL)
	fatal("strconcat('%s', '%s'): %s",
	      left, right, unix_error_string(errno));
    return strcat(strcpy(saved, left), right);
}

void
strfree(string)
    string_t string;
{
    free(string);
}

char *
strbool(bool)
    boolean_t bool;
{
    if (bool)
	return "TRUE";
    else
	return "FALSE";
}

char *
strstring(string)
    string_t string;
{
    if (string == strNULL)
	return "NULL";
    else
	return string;
}
