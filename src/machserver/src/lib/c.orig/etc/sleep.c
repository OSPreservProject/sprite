/* 
 * sleep.c --
 *
 *	Source for "sleep" library procedure.
 *
 * Copyright 1986, 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/sleep.c,v 1.4 90/09/05 18:56:41 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sys/time.h>
#include <signal.h>

#define setvec(vec, a) \
        vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0

static int ringring;
static void sleepx();


/*
 *----------------------------------------------------------------------
 *
 * sleep --
 *
 *	Delay process for a given number of seconds.
 *
 * Results:
 *	Always returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
sleep(n)
        unsigned n;
{
        long omask;
        struct itimerval itv, oitv;
        register struct itimerval *itp = &itv;
        struct sigvec vec, ovec;

        if (n == 0)
                return;
        timerclear(&itp->it_interval);
        timerclear(&itp->it_value);
        if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
                return;
        itp->it_value.tv_sec = n;
        if (timerisset(&oitv.it_value)) {
                if (timercmp(&oitv.it_value, &itp->it_value, >))
                        oitv.it_value.tv_sec -= itp->it_value.tv_sec;
                else {
                        itp->it_value = oitv.it_value;
                        /*
                         * This is a hack, but we must have time to
                         * return from the setitimer after the alarm
                         * or else it'll be restarted.  And, anyway,
                         * sleep never did anything more than this before.
                         */
                        oitv.it_value.tv_sec = 1;
                        oitv.it_value.tv_usec = 0;
                }
        }
        setvec(vec, sleepx);
        (void) sigvec(SIGALRM, &vec, &ovec);
        omask = sigblock(sigmask(SIGALRM));
        ringring = 0;
        (void) setitimer(ITIMER_REAL, itp, (struct itimerval *)0);
        while (!ringring)
                sigpause(omask &~ sigmask(SIGALRM));
        (void) sigvec(SIGALRM, &ovec, (struct sigvec *)0);
        (void) sigsetmask(omask);
        (void) setitimer(ITIMER_REAL, &oitv, (struct itimerval *)0);
}

static void
sleepx()
{

        ringring = 1;
}

#ifdef WRONG
int
sleep(seconds)
    int seconds;
{
    struct timeval tv;

    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    (void) select(0, (int *) 0, (int *) 0, (int *) 0, &tv);
    return 0;
}
#endif

