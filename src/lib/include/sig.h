/*
 * sig.h --
 *
 *     Data structures and used by the signal module for user system
 *     calls.
 *
 * Copyright (C) 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/sig.h,v 1.5 92/04/08 14:42:12 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SIGUSER
#define _SIGUSER

/*
 * The different actions for signals.
 * SIG_IGNORE_ACTION	Ignore the signal.
 * SIG_KILL_ACTION	Kill the process.
 * SIG_DEBUG_ACTION	Enter the debugger for the process.
 * SIG_DEFAULT_ACTION	Take the default action.
 * SIG_HANDLE_ACTION	Call a signal handler for the process.
 * SIG_MIGRATE_ACTION	Migrate the process to another node.  The node
 *		        must have already been set.
 * SIG_SUSPEND_ACTION	Suspend execution of the process.
 */
#define	SIG_IGNORE_ACTION	0
#define	SIG_KILL_ACTION		1
#define	SIG_DEBUG_ACTION	2
#define	SIG_DEFAULT_ACTION	3
#define	SIG_HANDLE_ACTION	4
#define	SIG_MIGRATE_ACTION	5
#define	SIG_SUSPEND_ACTION	6

#define	SIG_NUM_ACTIONS		7

/*
 * The structure to use to specify the action to take for a signal.
 */
typedef	struct {
    int		action;
    int		(*handler)();
    int		sigHoldMask;
} Sig_Action;

/*
 * The different signals.
 *
 * SIG_DEBUG		Enter the debugger.
 * SIG_ARITH_FAULT	Arithmetic instruction fault (e.g., division by zero).
 * SIG_ILL_INST		Illegal instruction.
 * SIG_ADDR_FAULT	Bad operand address.
 * SIG_KILL		Kill the process.
 * SIG_INTERRUPT	Interrupt the process.
 * SIG_BREAKPOINT	A breakpoint trap exception.
 * SIG_TRACE_TRAP	A trace trap exception.
 * SIG_MIGRATE_TRAP	A migration trap exception (privileged).
 * SIG_MIGRATE_HOME	Signal a process to return home after migration.
 * SIG_SUSPEND		Suspend execution.
 * SIG_RESUME		Resume execution after being suspended.
 * SIG_TTY_INPUT	Signal to a background process that tries to read
 *			from the tty.
 * SIG_PIPE		The reader of a pipe has died.
 * SIG_TIMER		An timer set with Proc_SetIntTimer has expired.
 * SIG_URGENT		Urgent condition (i.e., out-of-band data) is present 
 *			on a socket
 * SIG_CHILD		A child's status has changed.
 * SIG_TERM		Software termination.
 * SIG_TTY_SUSPEND	Suspend signal from keyboard.
 * SIG_TTY_OUTPUT	Signal to a background process that tries to write
 *			to the tty when that has been dis-allowed.
 * 
 * The following signals aren't used by Sprite.  They exist for UNIX 
 * compatibility.
 * 
 * SIG_IO_READY		(SIGIO) - Input/output is possible.
 * SIG_WINDOW_CHANGE	(SIGWINCH) - A window has changed size.
 * SIG_IOT		(SIGIOT) - IOT instruction
 * SIG_EMT		(SIGEMT) - EMT instruction
 * SIG_USER1		(SIGUSR1) - user-defined signal #1
 * SIG_USER2		(SIGUSR2) - user-defined signal #2
 */
#define	SIG_DEBUG		1
#define	SIG_ARITH_FAULT		2
#define	SIG_ILL_INST		3
#define	SIG_ADDR_FAULT		4
#define	SIG_KILL		5
#define	SIG_INTERRUPT		6
#define	SIG_BREAKPOINT		7
#define	SIG_TRACE_TRAP		8
#define	SIG_MIGRATE_TRAP 	9
#define	SIG_MIGRATE_HOME 	10
#define	SIG_SUSPEND		11
#define	SIG_RESUME		12
#define	SIG_TTY_INPUT		13
#define SIG_PIPE		14
#define SIG_TIMER		15
#define SIG_URGENT		16
#define SIG_CHILD		17
#define SIG_TERM		18
#define	SIG_TTY_SUSPEND		19
#define SIG_TTY_OUTPUT		20
#define SIG_IO_READY		26
#define SIG_WINDOW_CHANGE	27
#define SIG_IOT			28
#define SIG_EMT			29
#define SIG_USER1		30
#define SIG_USER2		31

/*
 * Define the bounds on signals.
 *
 * SIG_MIN_SIGNAL			The smallest valid signal.
 * SIG_LAST_RESERVED_SIGNAL		All signals after this one can be user
 *					defined.
 * SIG_NUM_SIGNALS			The total number of signals.
 */
#define	SIG_MIN_SIGNAL			1
#define	SIG_LAST_RESERVED_SIGNAL	SIG_TTY_OUTPUT
#define	SIG_NUM_SIGNALS			32

/*
 * A code of zero indicates that there is no code for the signal.
 */
#define	SIG_NO_CODE		-1

/*
 * The different standard codes for an illegal instruction signal
 *
 * SIG_ILL_INST_CODE	This was actually an illegal instruction.
 * SIG_BAD_SYS_CALL	A bad system call number was passed to a system call
 *			trap.
 * SIG_BAD_TRAP		An illegal trap instruction was executed.
 * SIG_PRIV_INST	A privledged instruction was executed.
 */
#define	SIG_ILL_INST_CODE	0
#define	SIG_BAD_SYS_CALL	1
#define	SIG_BAD_TRAP		2
#define	SIG_PRIV_INST		3

/*
 * The different standard codes for an address fault.
 *
 * SIG_ACCESS_VIOL	The address accesses a protected area of memory.
 * SIG_ADDR_ERROR 	The operand address is on an improper boundary.
 */
#define	SIG_ACCESS_VIOL		0
#define	SIG_ADDR_ERROR		1

/*
 * The standard codes for an arithmetic fault.
 *
 * SIG_ZERO_DIV		Division by zero.
 */
#define	SIG_ZERO_DIV		0

#endif /* _SIGUSER */
