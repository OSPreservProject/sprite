/* 
 * bstring.c --
 *
 *	This file contains a program that exercises the bstring
 *	library procedures.  Invoke it with no parameters;  it
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
static char rcsid[] = "$Header: /sprite/src/tests/bstring/RCS/bstring.c,v 1.2 88/12/15 09:07:52 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <bstring.h>

#define error(string) \
    fprintf(stderr, string); \
    exit(1);

int
check(srcPtr, numBytes, start)
    unsigned char *srcPtr;	/* First byte to check. */
    int numBytes;		/* How many bytes to check. */
    int start;			/* Bytes should have ascending values starting
				   at start. */
{
    int i;
    for (i = 0; i < numBytes; i++) {
	if (srcPtr[i] != ((start+i)&0377)) {
	    return 0;
	}
    }
    return 1;
}

int
checkZero(srcPtr, numBytes)
    unsigned char *srcPtr;	/* First byte to check. */
    int numBytes;		/* How many bytes to check. */
{
    int i;
    for (i = 0; i < numBytes; i++) {
	if (srcPtr[i] != 0) {
	    return 0;
	}
    }
    if (srcPtr[numBytes] == 0) {
	return 0;
    }
    return 1;
}

main()
{
    int result = 0;
    static unsigned char test1[400], test2[400], test3[400];
    int i;

    for (i = 0; i < 400; i++) {
	test3[i] = i;
    }

    /*
     * bcopy
     */

    bcopy(test3, test1, 400);
    if (!check(test1, 400, 0)) {
	error("bcopy error 1\n");
    }
    bcopy(test1+16, test1+32, 127);
    if (!check(test1+32, 127, 16)) {
	error("bcopy error 2\n");
    }
    if ((test1[160] != 160) || (test1[31] != 31)) {
	error("bcopy error 3\n");
    }
    bcopy(test3, test1, 400);
    bcopy(test1+64, test1+16, 127);
    if (!check(test1+16, 127, 64)) {
	error("bcopy error 4\n");
    }
    if ((test1[144] != 144) || (test1[15] != 15)) {
	error("bcopy error 5\n");
    }
    bcopy(test3, test1, 100);
    bcopy(test1+1, test1+16, 32);
    if (!check(test1+16, 32, 1)) {
	error("bcopy error 6\n");
    }
    if ((test1[15] != 15) || (test1[48] != 48)) {
	error("bcopy error 7\n");
    }
    bcopy(test3, test1, 100);
    bcopy(test1+17, test1+8, 32);
    if (!check(test1+8, 32, 17)) {
	error("bcopy error 8\n");
    }
    if ((test1[7] != 7) || (test1[40] != 40)) {
	error("bcopy error 9\n");
    }
    bcopy(test3, test1, 100);
    bcopy(test1+16, test1+3, 32);
    if (!check(test1+3, 32, 16)) {
	error("bcopy error 10\n");
    }
    if ((test1[2] != 2) || (test1[35] != 35)) {
	error("bcopy error 11\n");
    }
    bcopy(test3, test1, 100);
    bcopy(test1+16, test1+33, 32);
    if (!check(test1+33, 32, 16)) {
	error("bcopy error 12\n");
    }
    if ((test1[32] != 32) || (test1[65] != 65)) {
	error("bcopy error 13\n");
    }
    bcopy(test3, test1, 100);
    bcopy(test1+2, test1+6, 64);
    if (!check(test1+6, 64, 2)) {
	error("bcopy error 14\n");
    }
    bcopy(test3, test1, 100);
    bcopy(test1+6, test1+2, 64);
    if (!check(test1+2, 64, 6)) {
	error("bcopy error 15\n");
    }

    /*
     * bcmp
     */

    bcopy(test3+1, test1, 200);
    bcopy(test3+5, test2, 200);
    test1[135] = 66;
    if (bcmp(test1+8, test2+4, 127) != 0) {
	error("bcmp error 1\n");
    }
    test1[134] = 20;
    if (bcmp(test1+8, test2+4, 127) == 0) {
	error("bcmp error 2\n");
    }
    test1[133] = 19;
    if (bcmp(test1+6, test2+2, 127) != 0) {
	error("bcmp error 3\n");
    }    test1[132] = 18;
    if (bcmp(test1+6, test2+2, 127) == 0) {
	error("bcmp error 4\n");
    }
    if (bcmp(test1+5, test2+1, 127) != 0) {
	error("bcmp error 5\n");
    }
    test1[131] = 17;
    if (bcmp(test1+5, test2+1, 127) == 0) {
	error("bcmp error 6\n");
    }
    test1[72] = 16;
    if (bcmp(test1+8, test2+4, 64) != 0) {
	error("bcmp error 7\n");
    }
    test1[71] = 15;
    if (bcmp(test1+8, test2+4, 64) ==  0) {
	error("bcmp error 8\n");
    }
    bcopy(test3+4, test1, 200);
    if (bcmp(test1+4, test2+3, 64) != 0) {
	error("bcmp error 9\n");
    }
    if (bcmp(test1+5, test2+4, 64) != 0) {
	error("bcmp error 10\n");
    }

    /*
     * bzero
     */

    bcopy(test3, test1, 200);
    bzero(test1, 1);
    if (!checkZero(test1, 1)) {
	error("bzero error 1\n");
    }
    bcopy(test3, test1, 200);
    bzero(test1, 8);
    if (!checkZero(test1, 8)) {
	error("bzero error 2\n");
    }
    bcopy(test3, test1, 200);
    bzero(test1, 127);
    if (!checkZero(test1, 127)) {
	error("bzero error 3\n");
    }
    bcopy(test3, test1, 200);
    bzero(test1+2, 127);
    if (!checkZero(test1+2, 127)) {
	error("bzero error 4\n");
    }
    bcopy(test3, test1, 200);
    bzero(test1+3, 127);
    if (!checkZero(test1+3, 127)) {
	error("bzero error 5\n");
    }

    /*
     * ffs
     */

    if (ffs(0) != -1) {
	error("ffs error 1\n");
    }
    if (ffs(1) != 1) {
	error ("ffs error 2\n");
    }
    if (ffs(1<<31) != 32) {
	error ("ffs error 3\n");
    }

    return result;
}
