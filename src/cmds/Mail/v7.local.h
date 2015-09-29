/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)v7.local.h	5.6 (Berkeley) 6/29/88
 */

/*
 * Declarations and constants specific to an installation.
 *
 * Vax/Unix version 7.
 */

#define SENDMAIL	"/sprite/cmds/sendmail"
					/* Name of classy mail deliverer */
#define	EDITOR		"/sprite/cmds/ex"	/* Name of text editor */
#define	VISUAL		"/sprite/cmds/vi"	/* Name of display editor */
#define	SHELL		"/sprite/cmds"	/* Standard shell */
#define	MORE		"/sprite/cmds/more"	/* Standard output pager */
#define	HELPFILE	"/sprite/lib/Mail/Mail.help"
					/* Name of casual help file */
#define	THELPFILE	"/sprite/lib/Mail/Mail.tildehelp"
					/* Name of casual tilde help */
#define	POSTAGE		"/tmp/maillog"
					/* Where to audit mail sending */
#define	MASTER		"/sprite/lib/Mail/Mail.rc"
#define	APPEND				/* New mail goes to end of mailbox */
