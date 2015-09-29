/* 
 * syscall.c --
 *
 *	Program for doing Sprite "system" call benchmarks.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/benchmarks/syscall/RCS/syscall.c,v 1.1 92/07/17 16:27:55 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <option.h>
#include <stdio.h>
#include <stdlib.h>
#include <test.h>
#include <sys/time.h>

Boolean doTest1 = FALSE;	/* invoke "return1" test */
Boolean doTest2 = FALSE;	/* invoke "return2" test */

int iterations = 10000;		/* number of iterations */

Option optionArray[] = {
    {OPT_DOC, NULL, NULL,
      "This routine times the invocation of one or more \"system\" calls.\n"},
    {OPT_TRUE, "1", (Address)&doTest1,
	 "Invoke the \"return1\" call."},
    {OPT_TRUE, "2", (Address)&doTest2,
	 "Invoke the \"return2\" call."},
    {OPT_INT, "iter", (Address)&iterations,
	 "Number of times to invoke a call."},
};
int numOptions = Opt_Number(optionArray);

static void DoTest();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Get arguments; invoke a particular routine N times.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;
    char *argv[];
{
    (void)Opt_Parse(argc, argv, optionArray, numOptions, 0);

    if (doTest1) {
	printf("Calling Test_Return1: ");
	fflush(stdout);
	DoTest(iterations, Test_Return1);
    }
    if (doTest2) {
	printf("Calling Test_Return2: ");
	fflush(stdout);
	DoTest(iterations, Test_Return2);
    }

    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * DoTest --
 *
 *	Invoke a call a bunch of times and time them.  We assume that the 
 *	calls are short, so that the total elapsed time can be conveniently 
 *	measured in microseconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
DoTest(numCalls, callProc)
    int numCalls;		/* number of times to call callProc */
    void (*callProc)();		/* the function to invoke */
{
    struct timeval startTime;	/* when we started the calls */
    struct timeval endTime;	/* when we finished */
    int i;
    long micros;		/* elapsed time */

    gettimeofday(&startTime, (struct timezone *) NULL);
    for (i = 0; i < numCalls; ++i) {
	callProc();
    }
    gettimeofday(&endTime, (struct timezone *) NULL);
    micros = 1000000*(endTime.tv_sec - startTime.tv_sec)
	+ endTime.tv_usec - startTime.tv_usec;

    printf("time per call: %.2f microseconds\n",
	   (float)micros / numCalls);
}
