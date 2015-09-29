/* 
 * common.c --
 *
 *	Collect infomation on a new account request.
 *      Make sure the information is valid.  Then mail it
 *      to the staff.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/adduser/RCS/common.c,v 1.5 91/06/10 12:08:33 kupfer Exp $";
#endif

#include "common.h"
#include <sprite.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Forward references: */
#if 0
static void mail _ARGS_((CONST char *whom, CONST char *msg));
#endif


/*
 *----------------------------------------------------------------------
 *
 * raw_getchar --
 *
 *	Get a character in cbreak mode, without waiting for a carriage
 *      return.
 *
 * Results:
 *	Returns the character read, or EOF if no more input is available.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
raw_getchar()
{
    struct sgttyb sgtty_buf;
    int c;

    ioctl(fileno(stdin), TIOCGETP, &sgtty_buf);
    sgtty_buf.sg_flags |= CBREAK;
    ioctl(fileno(stdin), TIOCSETP, &sgtty_buf);
    c = getchar();
    sgtty_buf.sg_flags &= ~CBREAK;
    ioctl(fileno(stdin), TIOCSETP, &sgtty_buf);
    return c;
}


/*
 *----------------------------------------------------------------------
 *
 * yes --
 *
 *	Get a yes/no response from the user.
 *
 * Results:
 *	1 if the user said yes, 0 if no.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
yes(prompt)
    char *prompt;
{
    int x;

    for (;;) {
	printf("\n%s  (y or n) ", prompt);
	x = raw_getchar();
	printf("\n");
	switch (x) {

	case 'y':
	case 'Y':
	    x = 1;
	    break;

	case 'n':
	case 'N':
	    x = 0;
	    break;

	default:
	    continue;
	}
	break;
    }
    return x;
}

void
getString(forbid, prompt, string)
    CONST char *forbid;
    CONST char *prompt;
    char *string;
{
    char buffer[BUFFER_SIZE];
    CONST char *f;

    for (;;) {
	printf(string[0] ? "%s(%s): " : "%s: ", prompt, string);
	fgets(buffer, BUFFER_SIZE - 1, stdin);
	if (buffer[strlen(buffer) - 1] == '\n') {
	    buffer[strlen(buffer) - 1] = '\0';
	}
	if (buffer[0] != '\0') {
	    strcpy(string, buffer);
	}
	for (f = forbid;; ++f) {
	    if (*f == '\0') {
		return;
	    }
	    if (strchr(buffer, *f)) {
		fprintf(stderr, "This entry cannot contain a `%c'!\n", *f);
		break;
	    }
	}
    }
}

void
getPasswd(p)
    char *p;
{
    char passwd1[BUFFER_SIZE];
    char passwd2[BUFFER_SIZE];
    static char salt_chars[] = 
      "./abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static char salt[3];
    int c;
    struct sgttyb sgtty_buf;

    printf("Press 1 to enter an encrypted passwd\n");
    printf("Press 2 to enter a plaintext passwd\n");
    printf("Press 3 to leave this field blank\n");
    for (;;) {
	printf("Please enter 1, 2 or 3: ");
	c = raw_getchar();
	printf("\n");
	switch (c) {

	case '1':
	    getString("", "Enter encrypted passwd", p);
	    if ((strlen(p) != 13) || strspn(p, salt_chars) != 13) {
		printf("%s is not a valid encrypted password!\n", passwd1);
		printf("Please try again.\n");
		continue;
	    }
	    return;

	case '2':
	    passwd1[0] = '\0';
	    passwd2[0] = '\0';
	    ioctl(0, TIOCGETP, &sgtty_buf);
	    sgtty_buf.sg_flags &= ~ECHO;
	    ioctl(0, TIOCSETP, &sgtty_buf);
	    getString("", "Password", passwd1);
	    printf("\n");
	    getString("", "Retype passwd", passwd2);
	    printf("\n");
	    if (*passwd1 == '\0' || strcmp(passwd1, passwd2) != 0) {
		printf("Sorry, try again\n");
		continue;
	    }
	    sgtty_buf.sg_flags |= ECHO;
	    ioctl(0, TIOCSETP, &sgtty_buf);
	    srandom(time(0));
	    salt[0] = salt_chars[random() % sizeof(salt_chars)];
	    salt[1] = salt_chars[random() % sizeof(salt_chars)];
	    strcpy(p, crypt(passwd1, salt));
	    return;

	case '3':
	    strcpy(p, "*");
	    return;

	default:
	    continue;
	}
    }
}

char *
getShell()
{
    int c;

    printf("Please select a shell\n");
    printf("\t1  csh (default)\n");
    printf("\t2  tcsh\n");
    printf("\t3  sh\n");
    for (;;) {
	printf("Please enter 1, 2 or 3: ");
	c = raw_getchar();
	printf("\n");
	switch (c) {

	case '1':
	case '\n':
	    return "/sprite/cmds/csh";

	case '2':
	    return "/sprite/cmds/tcsh";

	case '3':
	    return "/sprite/cmds/sh";

	default:
	    continue;
	}
    }
}

#if 0
static void
mail(whom, msg)
    CONST char *whom;
    CONST char *msg;
{
    int pipeFd[2];
    int childPid;
    int w;

    pipe(pipeFd);
    switch (childPid = fork()) {

    case -1:
	fprintf(stderr, "Cannot fork: %s\n", strerror(errno));
	exit(1);

    case 0: /* child */
	close(pipeFd[1]);
	dup2(pipeFd[0], 0);
	execlp("mail", "mail", whom, NULL);
	fprintf(stderr, "Can't exec mail: %s\n", strerror(errno));
	exit(1);

    default: /* parent */
        close(pipeFd[0]);
	if (write(pipeFd[1], msg, strlen(msg)) != strlen(msg)) {
	    fprintf(stderr, "Error writing to pipe: %s\n", strerror(errno));
	    exit(1);
	}
	close(pipeFd[1]);
	while ((w = wait(0)) > 0 && w != childPid) {
	    continue;
	}
	printf("done sending mail\n");
	break;
    }
    return;
}
#endif /* 0 */

void
getNumber(prompt, buf)
    CONST char *prompt;
    char *buf;
{

    for (;;) {
	getString("", prompt, buf);
	if (checkNumber(buf)) {
	    return;
	}
	fprintf(stderr, "Only digits [0-9] should be used!\n");
	*buf = '\0';
    }
}

int
checkNumber(buf)
    char *buf;
{
    int i;

    for (i = strlen(buf); --i >= 0;) {
	if (!isascii(buf[i]) || !isdigit(buf[i])) {
	    return 0;
	}
    }
    return 1;
}


/*
 *----------------------------------------------------------------------
 *
 * makedb --
 *
 *	Regenerate the hashed password file.
 *
 * Results:
 *	0 for success, an exit status otherwise.
 *
 * Side effects:
 *	Creates the .dir and .pag files to match the plain-text 
 *	password file.
 *
 *----------------------------------------------------------------------
 */

int
makedb(file)
    char *file;
{
    int status, pid, w;
    
    if (!(pid = vfork())) {
	execl(_PATH_MKPASSWD, "mkpasswd", "-p", file, NULL);
	(void) fprintf(stderr, "Can't exec %s: %s\n",
		       _PATH_MKPASSWD, strerror(errno));
	_exit(127);
    }
    while ((w = wait(&status)) != pid && w != -1) {
	continue;
    }
    return(w == -1 || status);
}


/*
 *----------------------------------------------------------------------
 *
 * rcsCheckOut --
 *
 *	Check out a file.
 *
 * Results:
 *	Returns the exit status of the "co" invocation, or -1 if there 
 *	was a system error (and co couldn't be run).
 *
 * Side effects:
 *	Checks out the file.
 *
 *----------------------------------------------------------------------
 */
int
rcsCheckOut(file)
    char *file;
{
    int child;
    union wait ws;
    int w;

    switch (child = fork()) {

    case -1:
	fprintf(stderr, "Fork failed: %s\n", strerror(errno));
	return -1;
    case 0:
	execlp("co", "co", "-l", file, NULL);
	fprintf(stderr, "Cannot exec co: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    default:
	while ((w = wait(&ws)) > 0 && w != child) {
	    continue;
	}
	return ws.w_retcode;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * rcsCheckIn --
 *
 *	Check in a file.
 *
 * Results:
 *	Returns the exit code of the "ci" invocation, or -1 if there 
 *	was a system error (and ci couldn't be run).
 *
 * Side effects:
 *	Checks in the file, whether or not it had actually changed.
 *
 *----------------------------------------------------------------------
 */
int
rcsCheckIn(file, logMsg)
    char *file;			/* file to check in */
    char *logMsg;		/* message for the RCS log, with 
				 * leading -m */
{
    int child;
    union wait ws;
    int w;

    switch (child = fork()) {
    case -1:
	fprintf(stderr, "Fork failed: %s\n", strerror(errno));
	return -1;
    case 0:
	dup2(open("/dev/null", O_RDONLY), 0);
	execlp("ci", "ci", logMsg, "-f", "-u", file, NULL);
	fprintf(stderr, "Cannot exec ci: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    default:
	while ((w = wait(&ws)) > 0 && w != child) {
	    continue;
	}
	return ws.w_retcode;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * SecurityCheck --
 *
 *	Verify that we're running as root.  Make the real ID and 
 *	effective ID both be root.
 *	
 *	The real ID is made root to avoid problems with subprocesses 
 *	that exec random programs and act differently if the effective 
 *	ID is different from the real ID.  This is not a security 
 *	problem, because the program is installed so that only root or 
 *	wheels can run it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Exits if any of the checks fail.
 *
 *----------------------------------------------------------------------
 */

void
SecurityCheck()
{
    if (setreuid(0, 0) < 0) {
	perror("Can't setuid to root");
	fprintf(stderr,
		"Check whether the program was correctly installed.\n");
	exit(EXIT_FAILURE);
    }
}
