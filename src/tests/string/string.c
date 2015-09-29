/* 
 * string.c --
 *
 *	This file contains a program that exercises the string
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
static char rcsid[] = "$Header: string.c,v 1.2 88/07/22 08:59:52 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <string.h>

#define error(string) \
    fprintf(stderr, string); \
    exit(1);

main()
{
    int result = 0;
    char test[100];
    void *x;

    /*
     * strcmp
     */

    if (strcmp("abcdef", "abcdef") != 0) {
	error("strcmp error 1\n");
    }
    if (strcmp("abcd", "abcdef") >= 0) {
	error("strcmp error 2\n");
    }
    if (strcmp("12345", "12354") >= 0) {
	error("strcmp error 3\n");
    }
    if (strcmp("xyz", "abc") <= 0) {
	error("strcmp error 4\n");
    }

    /*
     * strncmp
     */

    if (strncmp("abc\0x", "abc\0y", 6) != 0) {
	error("strncmp error 1\n");
    }
    if (strncmp("abcdefg", "abcxyz", 3) != 0) {
	error("strncmp error 2\n");
    }
    if (strncmp("abcdefg", "abcxyz", 4) >= 0) {
	error("strncmp error 3\n");
    }
    if (strncmp("abcdefg", "abcabc", 4) <= 0) {
	error("strncmp error 4\n");
    }

    /*
     * strcpy
     */

    if (strcpy(test, "") != test) {
	error("strcpy error 1\n");
    }
    if (test[0] != 0) {
	error("strcpy error 2\n");
    }
    strcpy(test, "abcdefg");
    if (strcmp(test, "abcdefg") != 0) {
	error("strcpy error 3\n");
    }

    /*
     * strncpy
     */

    strcpy(test, "abcdefg");
    if (strncpy(test, "xyzqrs", 3) != test) {
	error("strncpy error 1\n");
    }
    if (bcmp(test, "xyzdefg", 8) != 0) {
	error("strncpy error 2\n");
    }
    strncpy(test, "12", 6);
    if (bcmp(test, "12\0\0\0\0g", 8) != 0) {
	error("strncpy error 3\n");
    }

    /*
     * strcat
     */

    strcpy(test, "qrs");
    if (strcat(test, "tuv") != test) {
	error("strcat error 1\n");
    }
    if (strcmp(test, "qrstuv") != 0) {
	error("strcat error 2\n");
    }
    test[0] = 0;
    strcat(test, "12345");
    if (strcmp(test, "12345") != 0) {
	error("strcat error 3\n");
    }
    strcat(test, "");
    if (strcmp(test, "12345") != 0) {
	error("strcat error 4\n");
    }

    /*
     * strncat
     */

    strcpy(test, "Now is the time");
    if (strncat(test, " for all good men", 5) != test) {
	error("strncat error 1\n");
    }
    if (strcmp(test, "Now is the time for ") != 0) {
	error("strncat error 2\n");
    }
    strcpy(test, "abc");
    strncat(test, "xyz", 6);
    if (strcmp(test, "abcxyz") != 0) {
	error("strncat error 3\n");
    }

    /*
     * strchr & index
     */

    strcpy(test, "abcabcabcdef");
    if (strchr(test, 'a') != test) {
	error("strchr error 1");
    }
    if (strchr(test, 'c') != &test[2]) {
	error("strchr error 2");
    }
    if (strchr(test, 'x') != 0) {
	error("strchr error 3");
    }
    if (strchr(test, 0) != &test[12]) {
	error("strchr error 4");
    }
    if (index(test, 'a') != test) {
	error("index error 1");
    }
    if (index(test, 'c') != &test[2]) {
	error("index error 2");
    }
    if (index(test, 'x') != 0) {
	error("index error 3");
    }
    if (index(test, 0) != &test[12]) {
	error("index b error 4");
    }

    /*
     * strrchr & rindex
     */

    if (strrchr(test, 'a') != &test[6]) {
	error("strrchr error 1\n");
    }
    if (strrchr(test, 0) != &test[12]) {
	error("strrchr error 2\n");
    }
    if (strrchr(test, 'x') != 0) {
	error("strrchr error 3\n");
    }
    if (rindex(test, 'a') != &test[6]) {
	error("rindex error 1\n");
    }
    if (rindex(test, 0) != &test[12]) {
	error("rindex error 2\n");
    }
    if (rindex(test, 'x') != 0) {
	error("rindex error 3\n");
    }

    /*
     * strstr
     */

    strcpy(test, "Now is the time for all good men");
    if (strstr(test, "Now") != test) {
	error("strstr error 1\n");
    }
    if (strstr(test, "good men") != &test[24]) {
	error("strstr error 2\n");
    }
    if (strstr(test, "now") != 0) {
	error("strstr error 3\n");
    }
    if (strstr(test, "") != test) {
	error("strstr error 4\n");
    }

    /*
     * strlen
     */

    if (strlen("") != 0) {
	error("strlen error 1\n");
    }
    if (strlen("abc") != 3) {
	error("strlen error 2\n");
    }
    if (strlen("Now is the time") != 15) {
	error("strlen error 3\n");
    }

    /*
     * strpbrk
     */

    strcpy(test, "12341234567");
    if (strpbrk(test, "4567") != &test[3]) {
	error("strpbrk error 1\n");
    }
    if (strpbrk(test, "") != 0) {
	error("strpbrk error 2\n");
    }
    if (strpbrk(test, "abcdefgh") != 0) {
	error("strpbrk error 3\n");
    }

    /*
     * strspn
     */

    strcpy(test, "Now is the time for");
    if (strspn(test, " ") != 0) {
	error("strspn error 1\n");
    }
    if (strspn(test, "wnoNsi ") != 7) {
	error("strspn error 2\n");
    }
    if (strspn(test, "") != 0) {
	error("strspn error 3\n");
    }
    if (strspn(test, "aeioufhmNrstw ") != 19) {
	error("strspn error 4\n");
    }

    /*
     * strcspn
     */

    strcpy(test, "Now is the time for");
    if (strcspn(test, " ") != 3) {
	error("strcspn error 1\n");
    }
    if (strcspn(test, "N") != 0) {
	error("strcspn error 2\n");
    }
    if (strcspn(test, "") != 19) {
	error("strcspn error 3\n");
    }
    if (strcspn(test, "ABC;") != 19) {
	error("strcspn error 4\n");
    }

    return result;
}
