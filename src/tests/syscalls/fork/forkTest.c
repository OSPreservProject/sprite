/* 
 * forkTest.c --
 *
 *	Test the fork system call.
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
static char rcsid[] = "$Header$";
#endif /* not lint */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/wait.h>

extern int errno;
static int errors;

static int staticValue;
static int parentPID;

#ifdef __STDC__
static void testProcessLimits(void);
static void testFileOperations(void);
#else
static void testProcessLimits();
static void testFileOperations();
#endif

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	
 *      
 *
 * Results:
 *	Exits with a zero exit status if everything works properly,
 *      non-zero otherwise.
 *
 * Side effects:
 *	
 *      
 *
 *----------------------------------------------------------------------
 */

void
main()
{


    parentPID = getpid();
    if (parentPID == 0 || parentPID < 0) {
	(void) fprintf(stderr, "Bad process id: %d\n", parentPID);
	exit(EXIT_FAILURE);
    }

    testFileOperations();


    testProcessLimits();


}




/*
 *----------------------------------------------------------------------
 *
 * testProcessLimit --
 *
 *
 * Results:
 *	none.
 *
 * Side effects:
 *      Prints a message to stderr nad increments `errors' if there is
 *      an error.
 *
 *----------------------------------------------------------------------
 */

static void
testProcessLimits()
{
    int localValue;
    int *heapValuePtr;
    int w;
    union wait ws;

    while (staticValue < 10) {
	++staticValue;
	localValue = staticValue;
	if ((heapValuePtr = (int *) malloc(sizeof(int))) == NULL) {
	    (void) fprintf(stderr, "Malloc failed: %s\n", strerror(errno));
	    ++errors;
	    if (getpid() != parentPID) {
		exit(EXIT_FAILURE);
	    }
	    return;
	}
	*heapValuePtr = staticValue;
	switch (fork()) {

	case -1:
	    switch (errno) {

	    case EAGAIN:
	    case ENOMEM:
		break;

	    default:
		(void) fprintf(stderr, "Bad errno (%d): %s\n",
		    errno, strerror(errno));
		++errors;
		break;
	    }
	    break;

	case 0:     /* child */
	    if (localValue != staticValue) {
		(void) fprintf(stderr, "Child's local value is wrong.\n");
		++errors;
		break;
	    }
	    if (*heapValuePtr != staticValue) {
		(void) fprintf(stderr, "Child's heap value is wrong.\n");
		++errors;
		break;
	    }
	    continue;

	default:    /* parent */
	    if (localValue != staticValue) {
		(void) fprintf(stderr, "Parent's local value is wrong.\n");
		++errors;
		break;
	    }
	    if (*heapValuePtr != staticValue) {
		(void) fprintf(stderr, "Parent's heap value is wrong.\n");
		++errors;
		break;
	    }
	    continue;
	}
	break;
    }
    while ((w = wait(&ws)) > 0) {
	if (ws.w_retcode != EXIT_SUCCESS) {
	    ++errors;
	}
    }
    if (getpid() != parentPID) {
	exit((errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
    }
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * testProcessLimit --
 *
 *      Make sure that the child inherits file descriptors properly.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *      Prints a message to stderr nad increments `errors' if there is
 *      an error.
 *
 *----------------------------------------------------------------------
 */

static void
testFileOperations()
{
    int fd;
    static char tempfile[MAXPATHLEN];
    char buffer[100];
    int w;
    union wait ws;

    if (tmpnam(tempfile) != tempfile) {
	(void) fprintf(stderr, "tmpnam() returned incorrect value.\n");
	++errors;
	return;
    }
    if ((fd = open(tempfile, O_RDWR|O_CREAT, 0666)) < 0) {
	(void) fprintf(stderr, "Cannot create %s: %s\n",
	    tempfile, strerror(errno));
	++errors;
	return;
    }
    if (write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
	(void) fprintf(stderr, "error writing to %s: %s\n",
	    tempfile, strerror(errno));
	++errors;
	(void) unlink(tempfile);
	return;
    }
    if (lseek(fd, 0L, L_INCR) != sizeof(buffer)) {
	(void) fprintf(stderr, "lseek error: %s\n", strerror(errno));
	++errors;
	(void) unlink(tempfile);
	return;
    }
    switch (fork()) {

    case -1:
	(void) fprintf(stderr, "fork failed: %s\n", strerror(errno));
	++errors;
	(void) unlink(tempfile);
	return;

    case 0:
	if (lseek(fd, 0L, L_INCR) != sizeof(buffer)) {
	    (void) fprintf(stderr, "Child did not inherit file offset\n");
	    ++errors;
	}
	if (write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
	    (void) fprintf(stderr, "error writing to %s: %s\n",
		tempfile, strerror(errno));
	    ++errors;
	}
	exit(errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);

    default:
	while ((w = wait(&ws)) > 0) {
	    if (ws.w_retcode != EXIT_SUCCESS) {
		(void) fprintf(stderr, "Child exited with non-zero status\n");
		++errors;
		return;
	    }
	}
	break;
    }
    if (lseek(fd, 0L, L_INCR) != 2 * sizeof(buffer)) {
	(void) fprintf(stderr, "Parents file offset is wrong\n");
	++errors;
	(void) unlink(tempfile);
	return;
    }
    switch (fork()) {

    case -1:
	(void) fprintf(stderr, "fork failed: %s\n", strerror(errno));
	++errors;
	(void) unlink(tempfile);
	return;

    case 0:
	if (close(fd)) {
	    (void) fprintf(stderr, "child close failed: %s\n", strerror(errno));
	    ++errors;
	}
	exit(errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);

    default:
	while ((w = wait(&ws)) > 0) {
	    if (ws.w_retcode != EXIT_SUCCESS) {
		(void) fprintf(stderr, "Child exited with non-zero status\n");
		++errors;
		return;
	    }
	}
	break;
    }
    if (close(fd)) {
	(void) fprintf(stderr, "parent close failed: %s\n", strerror(errno));
	++errors;
    }
    (void) unlink(tempfile);
    return;
}

