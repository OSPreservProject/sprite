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
 * $Log:	error.c,v $
 * Revision 2.3  91/02/05  17:54:11  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:53:53  mrt]
 * 
 * Revision 2.2  90/06/02  15:04:25  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:10:17  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include <stdio.h>
#include <varargs.h>
#include "global.h"
#include "error.h"

extern int yylineno;
extern char *yyinname;

static char *program;
int errors = 0;

/*ARGSUSED*/
/*VARARGS1*/
void
fatal(format, va_alist)
    char *format;
    va_dcl
{
    va_list pvar;
    va_start(pvar);
    fprintf(stderr, "%s: fatal: ", program);
    (void) vfprintf(stderr, format, pvar);
    fprintf(stderr, "\n");
    va_end(pvar);
    exit(1);
}

/*ARGSUSED*/
/*VARARGS1*/
void
warn(format, va_alist)
    char *format;
    va_dcl
{
    va_list pvar;
    va_start(pvar);
    if (!BeQuiet && (errors == 0))
    {
	fprintf(stderr, "\"%s\", line %d: warning: ", yyinname, yylineno-1);
	(void) vfprintf(stderr, format, pvar);
	fprintf(stderr, "\n");
    }
    va_end(pvar);
}

/*ARGSUSED*/
/*VARARGS1*/
void
error(format, va_alist)
    char *format;
    va_dcl
{
    va_list pvar;
    va_start(pvar);
    fprintf(stderr, "\"%s\", line %d: ", yyinname, yylineno-1);
    (void) vfprintf(stderr, format, pvar);
    fprintf(stderr, "\n");
    va_end(pvar);
    errors++;
}

char *
unix_error_string(errno)
    int errno;
{
    extern int sys_nerr;
    extern char *sys_errlist[];
    static char buffer[256];
    char *error_mess;

    if ((0 <= errno) && (errno < sys_nerr))
	error_mess = sys_errlist[errno];
    else
	error_mess = "strange errno";

    sprintf(buffer, "%s (%d)", error_mess, errno);
    return buffer;
}

void
set_program_name(name)
    char *name;
{
    program = name;
}
