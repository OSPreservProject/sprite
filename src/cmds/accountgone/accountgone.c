/*
 * accountgone.c --
 *
 *      Inform a user that their account has been deactivated.
 *      This program can be listed as their shell in /etc/passwd.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void
main(argc, argv)
    int argc;
    char **argv;
{

    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGSTOP, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    puts("Sorry, but your account has been deactivated.");
    puts("If you need to login, please send mail to");
    puts("root@sprite.Berkeley.EDU, or call 642-8282.");
    exit(EXIT_SUCCESS);
}

