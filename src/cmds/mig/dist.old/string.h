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
 * $Log:	string.h,v $
 * Revision 2.3  91/02/05  17:55:57  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:56:03  mrt]
 * 
 * Revision 2.2  90/06/02  15:05:49  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:13:56  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Fixed strNULL to be the null string instead of the null string
 *	pointer.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_STRING_H
#define	_STRING_H

#include <strings.h>

typedef char *string_t;
typedef string_t identifier_t;

#define	strNULL		((string_t) 0)

extern string_t strmake(/* char *string */);
extern string_t strconcat(/* string_t left, right */);
extern void strfree(/* string_t string */);

#define	streql(a, b)	(strcmp((a), (b)) == 0)

extern char *strbool(/* boolean_t bool */);
extern char *strstring(/* string_t string */);

#endif	_STRING_H
