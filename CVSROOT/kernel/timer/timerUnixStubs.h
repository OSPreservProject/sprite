/*
 * timerUnixStubs.h --
 *
 *	These are the Unix compatibility stubs for the timer module.
 *
 * Copyright (C) 1991 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _TIMER_UNIX_STUBS
#define _TIMER_UNIX_STUBS

#include <sprite.h>
#include <user/sys/time.h>

extern int Timer_GettimeofdayStub _ARGS_((struct timeval *tp,
	struct timezone *tzpPtr));
extern int Timer_SettimeofdayStub _ARGS_((struct timeval *tp,
	struct timezone *tzpPtr));
extern int Timer_AdjtimeStub _ARGS_((struct timeval *delta,
	struct timeval *olddelta));
#endif /* _TIMER_UNIX_STUBS */
