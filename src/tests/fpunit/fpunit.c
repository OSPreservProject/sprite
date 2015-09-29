/*
 * fpunit.c --
 *
 *	Program for testing floating point instruction problems from user mode.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

int	testValue = 0x33;

/*
 * ----------------------------------------------------------------------
 *
 * main --
 *
 *	Test whatever floating point instructions from user mode.  The
 *	contents of this routine depend upon whatever I'm testing at the
 *	moment.  This is a temporary tester for the sun4 port.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
main()
{
    TestFp();
    printf("Hello there.\n");
    printf("The value is %f\n", 0x33);
    printf("Hello again.\n");
    exit(0);
}
