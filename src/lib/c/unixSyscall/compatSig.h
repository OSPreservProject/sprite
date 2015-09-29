/*
 * compatSig.h --
 *
 *	Declarations of mapping tables between Sprite and UNIX signals.
 *	This used to be compatSig.c but now it shared between kernel and
 *	user compatibility libraries.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/c/unixSyscall/RCS/compatSig.h,v 1.4 92/04/10 14:46:54 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _COMPATSIG
#define _COMPATSIG

#include <sprite.h>

/*
 * Define the mapping between Unix and Sprite signals. There are two arrays,
 * one to go from Unix to Sprite and one to go from Sprite to Unix.
 *
 * Note that the signals SIGIOT and SIGEMT that people don't usually
 * send from the keyboard and that tend not to be delivered by the
 * kernel but, rather, are used for IPC have been mapped to user-defined
 * signal numbers, rather than a standard Sprite signal. This allows more
 * of a one-to-one mapping.
 */

/*
 * Map Unix signals to Sprite signals.
 */
int compat_UnixSigToSprite[] = {
			NULL,
     /* SIGHUP */	SIG_INTERRUPT,
     /* SIGINT */	SIG_INTERRUPT,
     /* SIGDEBUG */	SIG_DEBUG,	
     /* SIGILL */	SIG_ILL_INST,
     /* SIGTRAP */	SIG_DEBUG,
     /* SIGIOT */	SIG_IOT,
     /* SIGEMT */	SIG_EMT,
     /* SIGFPE */	SIG_ARITH_FAULT,
     /* SIGKILL */	SIG_KILL,
     /* SIGMIG */	SIG_MIGRATE_TRAP,
     /* SIGSEGV */	SIG_ADDR_FAULT,
     /* SIGSYS */	NULL,
     /* SIGPIPE */	SIG_PIPE,
     /* SIGALRM */	SIG_TIMER,
     /* SIGTERM */	SIG_TERM,
     /* SIGURG */	SIG_URGENT,
     /* SIGSTOP */	SIG_SUSPEND,
     /* SIGTSTP */	SIG_TTY_SUSPEND,
     /* SIGCONT */	SIG_RESUME,
     /* SIGCHLD */	SIG_CHILD,
     /* SIGTTIN */	SIG_TTY_INPUT,
     /* SIGTTOU */	SIG_TTY_OUTPUT,
     /* SIGIO */	SIG_IO_READY,
     /* SIGXCPU */	NULL,
     /* SIGXFSZ */	NULL,
     /* SIGVTALRM */	NULL,
     /* SIGPROF */	NULL,
     /* SIGWINCH */	SIG_WINDOW_CHANGE,
     /* SIGMIGHOME */	SIG_MIGRATE_HOME,
     /* SIGUSR1 */	SIG_USER1,	/* user-defined signal 1 */
     /* SIGUSR2 */	SIG_USER2,	/* user-defined signal 1 */
     /* NULL */		32,	/* not a signal, but NSIG is 32 so we need
      				   an entry here. */
};

/*
 * Map Sprite signals to Unix signals.
 */
static int spriteToUnix[] = {
				NULL,
    /* SIG_DEBUG */		SIGDEBUG,
    /* SIG_ARITH_FAULT */	SIGFPE,
    /* SIG_ILL_INST */		SIGILL,
    /* SIG_ADDR_FAULT */	SIGSEGV,
    /* SIG_KILL */		SIGKILL,
    /* SIG_INTERRUPT */		SIGINT,
    /* SIG_BREAKPOINT */	SIGILL,
    /* SIG_TRACE_TRAP */	SIGILL,
    /* SIG_MIGRATE_TRAP */	SIGMIG,
    /* SIG_MIGRATE_HOME */	SIGMIGHOME,
    /* SIG_SUSPEND */		SIGSTOP,
    /* SIG_RESUME */		SIGCONT,
    /* SIG_TTY_INPUT */		SIGTTIN,
    /* SIG_PIPE */		SIGPIPE,
    /* SIG_TIMER */		SIGALRM,
    /* SIG_URGENT */		SIGURG,
    /* SIG_CHILD */		SIGCHLD,
    /* SIG_TERM */		SIGTERM,
    /* SIG_TTY_SUSPEND */	SIGTSTP,
    /* SIG_TTY_OUTPUT */	SIGTTOU,
    /* 21 */			NULL,
    /* 22 */			NULL,
    /* 23 */			NULL,
    /* 24 */			NULL,
    /* 25 */ 			NULL,
    /* SIG_IO_READY */		SIGIO,
    /* SIG_WINDOW_CHANGE */	SIGWINCH,
    /* SIG_IOT */		SIGIOT,
    /* SIG_EMT */		SIGEMT,
    /* SIG_USER1 */		SIGUSR1,
    /* SIG_USER2 */		SIGUSR2,
    /* 32 */			NULL,
};


#endif
