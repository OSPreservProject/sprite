/* 
 * stdlib.c --
 *
 *	This file contains a program that exercises the stdlib
 *	library facilities.  Invoke it with no parameters;  it
 *	will print messages on stderr for any problems it detects
 *	with the string procedures.
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
static char rcsid[] = "$Header: stdlib.c,v 1.2 88/06/08 13:28:53 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define error(string) \
    fprintf(stderr, string); \
    exit(1);

int
sortFunc(a, b)
    char **a, **b;
{
    return strcmp(*a, *b);
}

int
checkDifferent(ptr, count)
    int *ptr;			/* Pointer to array of integers to check
				 * to see if they're all different. */
    int count;			/* How many integers in array. */
{
    int i, j;

    for (i = 0; i < count-1; i++) {
	for (j = i+1; j < count; j++) {
	    if (ptr[i] == ptr[j]) {
		return 1;
	    }
	}
    }
    return 0;
}

int exitValue = 22;

void
exitFunc1()
{
    _exit(exitValue);
}

void
exitFunc2()
{
    exitValue = 14;
}

main()
{
    int result = 0, i;
    long int l, m, n;
    char *term, *arg, *p;
    double d;
    div_t dt;
    ldiv_t ldt;
    int nums[20], nums2[20];
    static char *strings[] = {
	"beta",
	"alpha",
	"gamma",
	"delta",
	"charlie",
	"smoke",
	"aleph"
    };
    union wait status;
    int pid, savedStdout, pipes[2];
    char line[100];
    static struct sigvec vec;

    /*
     * atof
     */

    if (atof("1a") != 1.0) {
	error("atof error 1\n");
    }
    if (atof(".123,") != .123) {
	error("atof error 2\n");
    }
    if (atof("-47.e2") != -4700.0) {
	error("atof error 3\n");
    }
    if (atof("+0.2E-2") != .002) {
	error("atof error 4\n");
    }
    if (atof("-123456789.01234") != -123456789.01234) {
	error("atof error 5\n");
    }
    if (atof("a") != 0.0) {
	error("atof error 6\n");
    }
    d = atof("e");
    if (d != 0.0) {
	error("atof error 6\n");
    }
    if (atof("-e5") != 0.0) {
	error("atof error 7\n");
    }

    /*
     * atoi
     */

    if (atoi("") != 0) {
	error("atoi error 1\n");
    }
    if (atoi("47") != 47) {
	error("atoi error 2\n");
    }
    if (atoi("-42a") != -42) {
	error("atoi error 3\n");
    }
    if (atoi("a") != 0) {
	error("atoi error 4\n");
    }
    if (atoi("09/6") != 9) {
	error("atoi error 5\n");
    }
    if (atoi("55:") != 55) {
	error("atoi error 6\n");
    }
    if (atoi("+189223") != 189223) {
	error("atoi error 7\n");
    }

    /*
     * atol
     */

    if (atol("") != 0) {
	error("atol error 1\n");
    }
    if (atol("47") != 47) {
	error("atol error 2\n");
    }
    if (atol("-42a") != -42) {
	error("atol error 3\n");
    }
    if (atol("a") != 0) {
	error("atol error 4\n");
    }
    if (atol("09/6") != 9) {
	error("atol error 5\n");
    }
    if (atol("55:") != 55) {
	error("atol error 6\n");
    }
    if (atol("+189223") != 189223) {
	error("atol error 7\n");
    }

    /*
     * strtod
     */

    arg = "1a";
    if (strtod(arg, &term) != 1.0) {
	error("strtod error 1\n");
    }
    if (term != (arg+1)) {
	error("strtod error 2\n");
    }
    arg = ".123,";
    if (strtod(arg, &term) != .123) {
	error("strtod error 3\n");
    }
    if (term != (arg+4)) {
	error("strtod error 4\n");
    }
    arg = "-47.e2";
    if (strtod(arg, &term) != -4700.0) {
	error("strtod error 5\n");
    }
    if (term != (arg+6)) {
	error("strtod error 6\n");
    }
    if (strtod("+0.2E-2", (char *) NULL) != .002) {
	error("strtod error 7\n");
    }
    if (strtod("-123456789.01234", (char *) NULL) != -123456789.01234) {
	error("strtod error 8\n");
    }
    arg = "a";
    if (strtod(arg, &term) != 0.0) {
	error("strtod error 9\n");
    }
    if (term != arg) {
	error("strtod error 10\n");
    }
    if (strtod("e", (char *) NULL) != 0.0) {
	error("strtod error 11\n");
    }
    arg = "-e5";
    if (strtod(arg, &term) != 0.0) {
	error("strtod error 12\n");
    }
    if (term != arg) {
	error("strtod error 13\n");
    }

    /*
     * strtol
     */

    arg = "442x";
    if (strtol(arg, &term, 10) != 442) {
	error("strtol error 1\n");
    }
    if (term != (arg+3)) {
	error("strtol error 2\n");
    }
    arg = "+01009";
    if (strtol(arg, &term, 0) != 0100) {
	error("strtol error 3\n");
    }
    if (term != (arg+5)) {
	error("strtol error 4\n");
    }
    arg = "0x1abcDefgh";
    if (strtol(arg, &term, 0) != 0x1abcdef) {
	error("strtol error 5\n");
    }
    if (term != (arg+9)) {
	error("strtol error 6\n");
    }
    arg = "18";
    if (strtol(arg, &term, 0) != 18) {
	error("strtol error 7\n");
    }
    if (term != (arg+2)) {
	error("strtol error 8\n");
    }
    if (strtol("-0x10", (char *) NULL, 16) != -16) {
	error("strtol error 9\n");
    }
    if (strtol("j0k", (char *) NULL, 20) != 380) {
	error("strtol error 10\n");
    }
    arg = "0xq";
    if (strtol(arg, &term, 0) != 0) {
	error("strtol error 11\n");
    }
    if (term != arg) {
	error("strtol error 12\n");
    }

    /*
     * strtoul
     */

    arg = "12345";
    if (strtoul(arg, &term, 0) != 12345) {
	error("strtoul error 1\n");
    }
    if (term != &arg[5]) {
	error("strtoul error 2\n");
    }
    arg = "-22";
    if (strtoul(arg, &term, 0) != 0) {
	error("strtoul error 3\n");
    }
    if (term != arg) {
	error("strtoul error 4\n");
    }
    arg = "+14";
    if (strtoul(arg, &term, 0) != 0) {
	error("strtoul error 5\n");
    }
    if (term != arg) {
	error("strtoul error 6\n");
    }

    /*
     * abs
     */

    if (abs(-1) != 1) {
	error("abs error 1\n");
    }
    if (abs(22) != 22) {
	error("abs error 2\n");
    }
    if (abs(0) != 0) {
	error ("abs error 3\n");
    }

    /*
     * labs
     */

    l = labs((long int) -1);
    if (l != 1) {
	error("labs error 1\n");
    }
    l = labs((long int) 22);
    if (l != 22) {
	error("labs error 2\n");
    }
    l = labs((long int) 0);
    if (l != 0) {
	error ("labs error 3\n");
    }

    /*
     * div
     */

    dt = div(22, 3);
    if ((dt.quot != 7) || (dt.rem != 1)) {
	error("div error 1\n");
    }
    dt = div(-22, 3);
    if ((dt.quot != -7) || (dt.rem != -1)) {
	error("div error 2\n");
    }
    dt = div(22, -3);
    if ((dt.quot != -7) || (dt.rem != 1)) {
	error("div error 3\n");
    }
    dt = div(-22, -3);
    if ((dt.quot != 7) || (dt.rem != -1)) {
	error("div error 4\n");
    }

    /*
     * ldiv
     */

    ldt = ldiv((long int) 22, (long int) 3);
    if ((ldt.quot != (long int) 7) || (ldt.rem != (long int) 1)) {
	error("ldiv error 1\n");
    }
    ldt = ldiv((long int) -22, (long int) 3);
    if ((ldt.quot != (long int) -7) || (ldt.rem != (long int) -1)) {
	error("ldiv error 2\n");
    }
    ldt = ldiv((long int) 22, (long int) -3);
    if ((ldt.quot != (long int) -7) || (ldt.rem != (long int) 1)) {
	error("ldiv error 3\n");
    }
    ldt = ldiv((long int) -22, (long int) -3);
    if ((ldt.quot != (long int) 7) || (ldt.rem != (long int) -1)) {
	error("ldiv error 4\n");
    }

    /*
     * qsort
     */

    qsort((char *) strings, 7, sizeof(char *), sortFunc);
    for (i = 0; i < 6; i++) {
	if (strcmp(strings[i+1], strings[i]) < 0) {
	    error("qsort error 1\n");
	}
    }

    /*
     * rand
     */

    for (i = 0;  i < 20; i++) {
	nums[i] = rand();
    }
    if (checkDifferent(nums, 20)) {
	error("rand error 1\n");
    }
    srand(1);
    for (i = 0; i < 20; i++) {
	if (rand() != nums[i]) {
	    error("rand error 2\n");
	}
    }
    srand(12345);
    for (i = 0; i < 20; i++) {
	nums2[i] = rand();
    }
    if (checkDifferent(nums2, 20)) {
	error("rand error 3\n");
    }
    for (i = 0; i < 20; i++) {
	if (nums2[i] == nums[i]) {
	    error("rand error 4\n");
	}
    }

    /*
     * atexit
     */

    if (fork() == 0) {
	atexit(exitFunc1);
	atexit(exitFunc2);
	exit();
    }
    wait(&status);
    if (status.w_T.w_Retcode != 14) {
	error("atexit error 1\n");
    }	

    /*
     * setenv/getenv
     */

    setenv("TEST1", "foo");
    setenv("TEST2", "bar");
    setenv("TEST1", "foo2");
    setenv("TEST3", "helpMe");
    p = getenv("TEST1");
    if ((p == 0) || (strcmp(p, "foo2") != 0)) {
	error("getenv error 1\n");
    }
    p = getenv("TEST2");
    if ((p == 0) || (strcmp(p, "bar") != 0)) {
	error("getenv error 2\n");
    }
    p = getenv("TEST3");
    if ((p == 0) || (strcmp(p, "helpMe") != 0)) {
	error("getenv error 3\n");
    }
    if (getenv("IdontEXIST") != 0) {
	error("getenv error 4\n");
    }

    /*
     * abort
     */

    pid = fork();
    if (pid == 0) {
	abort();
    }
    while (1) {
	int pid2;
	pid2 = wait3(&status, WUNTRACED, 0);
	if (pid2 == -1) {
	    error("abort error 1\n");
	}
	if (status.w_status == 0) {
	    continue;
	}
	if (pid2 == pid) {
	    break;
	}
    }
    kill(pid, SIGKILL);
    if ((status.w_S.w_Stopval != WSTOPPED)
	    || (status.w_S.w_Stopsig != SIGQUIT)) {
	error("abort error 2\n");
    }

    /*
     * system
     */

    pipe(pipes);
    savedStdout = dup(1);
    dup2(pipes[1], 1);
    system("echo This is just a test");
    dup2(savedStdout, 1);
    i = read(pipes[0], line, 99);
    if (i < 0) {
	error("system error 1\n");
    }
    line[i] = 0;
    if (strcmp(line, "This is just a test\n") != 0) {
	error("system error 2\n");
    }
    close(pipes[0]);
    close(pipes[1]);

    return result;
}
