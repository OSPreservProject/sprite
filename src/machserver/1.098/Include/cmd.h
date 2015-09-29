/*
 * cmd.h --
 *
 *	This file declares things that are used to build, use and delete
 *	command bindings tables.
 *
 * Copyright (C) 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/cmd/RCS/cmd.h,v 1.3 89/06/15 21:07:46 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _CMD
#define _CMD

#include <stdio.h>

#ifndef _CLIENTDATA
typedef int * ClientData;
#define _CLIENTDATA
#endif

typedef struct	Table	*Cmd_Table;

/*
 * Return values from Cmd_Dispatch and Cmd_MapKey:
 *
 * CMD_OK -		Everything completed just fine.
 * CMD_AMBIGUOUS -	The name of a command was ambiguous (it was a previx
 *			of more than one command in the table).
 * CMD_NOT_FOUND -	There was no command by that name in the table.
 * CMD_PARTIAL -	In key-mapping, this keystroke is midway through
 *			a sequence:  more keys will have to be typed before
 *			a command mapping can be made.
 * CMD_UNBOUND -	This keystroke sequence ending in this stroke isn't
 *			bound to any command.
 * CMD_BAD_ARGS -	A command had a bad argument (e.g. tried to use
 *			selection but selection doesn't exist).
 * CMD_ERROR -		An error of some sort occurred while executing a
 *			command.
 * CMD_VAR_LOOP -	Variable commands were nested to an outrageous level;
 *			suggests that there's unbounded recursion.
 */

#define CMD_OK			0
#define CMD_AMBIGUOUS		1
#define CMD_NOT_FOUND		2
#define CMD_PARTIAL		3
#define CMD_UNBOUND		4
#define CMD_BAD_ARGS		5
#define CMD_ERROR		6
#define CMD_VAR_LOOP		7

/*
 * Exported procedures.
 */
extern void		Cmd_BindingCreate();
extern char *		Cmd_BindingGet();
extern void		Cmd_BindingDelete();
extern int		Cmd_EnumBindings();
extern int		Cmd_MapKey();
extern Cmd_Table	Cmd_TableCreate();
extern void		Cmd_TableDelete();
#ifdef NOTDEF
/*
 * Routines that were in John's mx.h but no longer seem to exist anywhere.
 */
extern int		Cmd_VarEnum();	/* not here */
extern char *		Cmd_VarGet();	/* not here */
extern void		Cmd_VarSet();	/* not here */
extern void		Cmd_CommandCreate();	/* not here */
extern void		Cmd_CommandDelete();	/* not here */
extern int		Cmd_Dispatch();	/* not here */
extern void		Cmd_Record();	/* not here */
extern void		Cmd_StopRecording();	/* not here */
#endif /* NOTDEF */

#endif _CMD
