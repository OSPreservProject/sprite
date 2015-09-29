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
 * $Log:	statement.c,v $
 * Revision 2.3  91/02/05  17:55:44  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:55:45  mrt]
 * 
 * Revision 2.2  90/06/02  15:05:37  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:13:24  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include "error.h"
#include "alloc.h"
#include "statement.h"

statement_t *stats = stNULL;
static statement_t **last = &stats;

statement_t *
stAlloc()
{
    register statement_t *new;

    new = (statement_t *) malloc(sizeof *new);
    if (new == stNULL)
	fatal("stAlloc(): %s", unix_error_string(errno));
    *last = new;
    last = &new->stNext;
    new->stNext = stNULL;
    return new;
}
