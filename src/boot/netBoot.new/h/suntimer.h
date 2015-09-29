
/*	@(#)suntimer.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * suntimer.h
 *
 * Sun-2 timer definitions
 */

/* timer channel assignments */

#define	TIMER_NMI	1	/* Non-maskable, level 7, no gate */
#define	TIMER_MISC	2	/* Misc timer, level 5, no gate */
#define	TIMER_STEAL	3	/* Cycle steals,level 5,gate="CPU Not Master" */
#define	TIMER_USER	4	/* User State timer, level 5, gate=User state */
#define	TIMER_SUPER	5	/* Supervisor State timer, level 5, gate=Sup */
