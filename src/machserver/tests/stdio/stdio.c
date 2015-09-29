/* 
 * stdio.c --
 *
 *	This file contains a program that exercises the stdio
 *	library facilities.  Invoke it with no parameters;  it
 *	will print messages on stderr for any problems it detects
 *	with the stdio procedures.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/tests/stdio/RCS/stdio.c,v 1.3 92/03/23 15:12:06 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <signal.h>
#include <varargs.h>

#ifdef sun3
#define BIGENDIAN
#endif
#ifdef sun4
#define BIGENDIAN
#endif
#ifdef ds3100
#define LITTLEENDIAN
#endif

#if 1
#define error(string) \
    fprintf(stderr, string); \
    exit(1);
#else
#define error(string) \
    fprintf(stderr, string);
#endif

/*
 *----------------------------------------------------------------------
 *
 * CheckFile --
 *
 *	Utility procedure to make sure that a given file contains a
 *	given value.  Aborts with error if it doesn't.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
CheckFile(name, value, errMsg)
    char *name;			/* Name of file to read. */
    char *value;		/* Should match contents of file. */
    char *errMsg;		/* Error message if there's a mismatch. */
{
    char buf[BUFSIZ];
    int count, id;

    id = open(name, O_RDONLY, 0);
    if (id < 0) {
	error(errMsg);
    }
    count = read(id, buf, 1000);
    if ((count == -1) || (count == 1000)) {
	error(errMsg);
    }
    buf[count] = 0;
    if (strcmp(buf, value) != 0) {
	error(errMsg);
    }
    close(id);
}

/*
 *----------------------------------------------------------------------
 *
 * TsCheck --
 *
 *	Procedure for comparing two special-purpose structures, for
 *	testing fread and fwrite.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Aborts if ts1 and ts2 aren't equal.
 *
 *----------------------------------------------------------------------
 */

typedef struct {
    int x;
    char c[6];
    short y;
} testStruct;

void
TsCheck(ts1, ts2, msg)
    testStruct *ts1, *ts2;		/* Two structures to compare for
					 * equality. */
    char *msg;				/* Message to print if mismatch. */
{
    int i;
    if ((ts1->x != ts2->x) || (ts1->y != ts2->y)) {
	error(msg);
    }
    for (i = 0; i < 6; i++) {
	if (ts1->c[i] != ts2->c[i]) {
	    error(msg);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckNext --
 *
 *	Procedure for testing fprintf:  reads back from a file and
 *	makes sure what was printed had the right length and content..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Aborts if the contents of the file's next line don't match string
 *	and length.
 *
 *----------------------------------------------------------------------
 */

void
CheckNext(f, string, length, msg)
    FILE *f;			/* File from which to read a line. */
    char *string;		/* This should match exactly the line. */
    int length;			/* The line should be this long. */
    char *msg;			/* Error message to print if there's
				 * a problem. */
{
    char buffer[500];
    if (fgets(buffer, 500, f) == NULL) {
	error(msg);
    }
    if (strlen(buffer) != length) {
	error(msg);
    }
    if (strcmp(buffer, string) != 0) {
	error(msg);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckFp --
 *
 *	This procedure is used to check floating-point scanning.  It
 *	prints the numbers, so as to do an approximate check, rather than
 *	an exact one (fp conversion isn't exact, after all)..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Aborts if the printed version of f1-f4 don't match string.
 *
 *----------------------------------------------------------------------
 */

void
CheckFp(count1, count2, f1, f2, f3, f4, string, msg)
    int count1, count2;		/* These two counts better match. */
    double f1, f2, f3, f4;	/* Four numbers; when printed in %e format,
				 * they better match string. */
    char *string;		/* Desired values of numbers. */
    char *msg;			/* Error message. */
{
    char printed[200];

    if (count1 != count2) {
	error(msg);
    }
    sprintf(printed, "%e %e %e %e", f1, f2, f3, f4);
    if (strcmp(printed, string) != 0) {
	printf("CheckFP: %s vs. %s\n", printed, string);
	error(msg);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckString --
 *
 *	Utility procedure for checking sscanf on strings.  Scans input
 *	according to format, into four result strings, and compares them
 *	against s1-s4.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Aborts if there's a mismatch.
 *
 *----------------------------------------------------------------------
 */


void
CheckString(input, format, count, s1, s2, s3, s4, msg)
    char *format;		/* Format to use for scanning. */
    char *input;		/* Input string to scan. */
    int count;			/* Expected return value from sscanf. */
    char *s1, *s2, *s3, *s4;	/* Scanned strings should match this. */
    char *msg;			/* Message to print on error. */
{
    char c1[100], c2[100], c3[100], c4[100];
    int result, i;

    for (i = 0; i < 100; i++) {
	char c;
	if (i < 5) {
	    c = 'X';
	} else {
	    c = 0;
	}
	c1[i] = c2[i] = c3[i] = c4[i] = c;
    }
    result = sscanf(input, format, c1, c2, c3, c4);
    if ((result != count) || (strcmp(c1, s1) != 0) || (strcmp(c2, s2) != 0)
	    || (strcmp(c3, s3) != 0) || (strcmp(c4, s4) != 0)) {
	error(msg);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckVprintf --
 *
 *	This procedure is used to check vprintf:  it just packages its
 *	arguments and calls vprintf.
 *
 * Results:
 *	Whatever vprintf returns.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
CheckVprintf(va_alist)
    va_dcl		/* Format string followed by one or more arguments
			 * to printf. */
{
    char *format;
    va_list args;

    va_start(args);
    format = va_arg(args, char *);
    return vprintf(format, args);
}

/*
 *----------------------------------------------------------------------
 *
 * CheckVsprintf --
 *
 *	This procedure is used to check vsprintf:  it just packages its
 *	arguments and calls vsprintf.
 *
 * Results:
 *	Whatever vsprintf returns.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
CheckVsprintf(va_alist)
    va_dcl		/* Format string followed by one or more arguments
			 * to printf. */
{
    char *string;
    char *format;
    va_list args;

    va_start(args);
    string = va_arg(args, char *);
    format = va_arg(args, char *);
    return vsprintf(string, format, args);
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for stdio testing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Runs a whole bunch of little tests on the stdio procedures,
 *	and aborts with an error message if any unexpected behavior
 *	is observed.  Exits with 0 status and no output if all goes
 *	well.
 *
 *----------------------------------------------------------------------
 */

main()
{
    FILE *f1, *f2, *f3;
    char buf[BUFSIZ], c;
    int i, fd, fd2, pid, length, count;
    testStruct tsArray1[20], tsArray2[20]; 
    union wait status;
    int d1, d2, d3, d4;
    float fp1, fp2, fp3, fp4;
    double dbl1, dbl2, dbl3, dbl4;
    short s1, s2, s3, sa1[4], sa2[4];

    /*
     * putc, fputc, fputs
     */

    f1 = fopen("test", "w");
    if (f1 == NULL) {
	error("putc error 1\n");
    }
    if (putc('a', f1) == EOF) {
	error("putc error 2\n");
    }
    if (putc('b', f1) == EOF) {
	error("putc error 3\n");
    }
    if (fputc('c', f1) != 'c') {
	error("putc error 4\n");
    }
    if (fputc('\n', f1) != '\n') {
	error("putc error 5\n");
    }
    if (fputs("This is the second line.\n", f1) == EOF) {
	error("putc error 6\n");
    }
    i = fclose(f1);
    if (i != 0) {
	error("putc error 7\n");
    }
    CheckFile("test", "abc\nThis is the second line.\n", "putc error 8\n");
    f1 = fopen("test2", "w");
    if (fputc(0377, f1) != 0377) {
	error("putc error 9\n");
    }
    if (putc(0377, f1) != 0377) {
	error("putc error 10\n");
    }
    if (fputc(-1, f1) != 0377) {
	error("putc error 11\n");
    }
    if (putc(-1, f1) != 0377) {
	error("putc error 12\n");
    }
    fclose(f1);
    f1 = fopen("test", "r");
    if (putc('a', f1) != EOF) {
	error("putc error 13\n");
    }
    if (fputc('a', f1) != EOF) {
	error("putc error 14\n");
    }
    if (fputs("abcde", f1) != EOF) {
	error("putc error 15\n");
    }

    /*
     * getc, fgetc, fgets
     */

    c = getc(f1);
    if (c != 'a') {
	error("getc error 1\n");
    }
    buf[4] = 'Z';
    if (fgets(buf, 100, f1) == NULL) {
	error("getc error 2\n");
    }
    if (strcmp(buf, "bc\n") != 0) {
	error("getc error 3\n");
    }
    if (buf[4] != 'Z') {
	error("getc error 4\n");
    }
    c = fgetc(f1);
    if (c != 'T') {
	error("getc error 5\n");
    }
    buf[10] = 'X';
    if (fgets(buf, 10, f1) == NULL) {
	error("getc error 6\n");
    }
    if (strcmp(buf, "his is th") != 0) {
	error("getc error 7\n");
    }
    if (buf[10] != 'X') {
	error("getc error 8\n");
    }
    buf[16] = 0377;
    if (fgets(buf, 100, f1) == NULL) {
	error("getc error 9\n");
    }
    if (strcmp(buf, "e second line.\n") != 0) {
	error("getc error 8\n");
    }
    if (buf[16] != -1) {
	error("getc error 9\n");
    }
    if (getc(f1) != EOF) {
	error("getc error 10\n");
    }
    if (fgetc(f1) != EOF) {
	error("getc error 11\n");
    }
    if (fgets(buf, 1000, f1) != NULL) {
	error("getc error 12\n");
    }
    i = fclose(f1);
    if (i != 0) {
	error("getc error 13\n");
    }
    f1 = fopen("test", "w");
    if (f1 == NULL) {
	error("getc error 14\n");
    }
    if (getc(f1) != EOF) {
	error("getc error 15\n");
    }
    fputs("Line with no return", f1);
    fclose(f1);
    f1 = fopen("test", "r");
    if (f1 == NULL) {
	error("getc error 16\n");
    }
    if (fgets(buf, 1000, f1) != NULL) {
	error("getc error 17\n");
    }
    fclose(f1);

    /*
     * putw, getw
     */

    f1 = fopen("test", "w");
    if (f1 == NULL) {
	error("putw error 1\n");
    }
    putc('a', f1);
    if (putw(0x010203ff, f1) != 0) {
	error("putw error 2\n");
    }
    if (putw(0x04050607, f1) != 0) {
	error("putw error 3\n");
    }
    if (getw(f1) != EOF) {
	error("putw error 4\n");
    }
    fclose(f1);
#ifndef LITTLEENDIAN
    CheckFile("test", "a\1\2\3\377\4\5\6\7", "putw error 5(B)\n");
#endif
#ifndef BIGENDIAN
    CheckFile("test", "a\377\3\2\1\7\6\5\4", "putw error 5(L)\n");
#endif
    f1 = fopen("test", "r");
    if (f1 == NULL) {
	error("putw error 6\n");
    }
    (void) getc(f1);
    d1 = getw(f1);
    if (feof(f1)) {
	error("putw error 7\n");
    }
    d2 = getw(f1);
    if (feof(f1)) {
	error("putw error 8\n");
    }
    d3 = getw(f1);
    if (!feof(f1)) {
	error("putw error 9\n");
    }
    if ((d1 != 0x010203ff) || (d2 != 0x04050607) || (d3 != EOF)) {
	error("putw error 10\n");
    }
    if (putw(1, f1) != EOF) {
	error("putw error 11\n");
    }
    fclose(f1);

    /*
     * fileno
     */

    if (fileno(stdin) != 0) {
	error("fileno error 1\n");
    }
    if (fileno(stdout) != 1) {
	error("fileno error 2\n");
    }
    if (fileno(stderr) != 2) {
	error("fileno error 3\n");
    }
    fd = dup(1);
    fclose(stdout);
    f1 = fopen("test", "w");
    if (fileno(f1) != 1) {
	error("fileno error 4\n");
    }

    /*
     * putchar, puts
     */

    if (putchar('x') == EOF) {
	error("putchar error 1\n");
    }
    if (puts(" more on this line.") == EOF) {
	error("putchar error 2\n");
    }
    if (puts("A second line.") == EOF) {
	error("putchar error 3\n");
    }
    fclose(f1);
    if (putchar('x') != EOF) {
	error("putchar error 4\n");
    }
    if (puts("More junk.") != EOF) {
	error("putchar error 5\n");
    }
    dup2(fd, 1);
    (void) fdopen(1, "w");
    CheckFile("test", "x more on this line.\nA second line.\n",
	    "putchar error 6\n");
    close(fd);

    /*
     * getchar, gets
     */

    fd = dup(0);
    fclose(stdin);
    f1 = fopen("test", "r");
    if (fileno(f1) != 0) {
	error("getchar error 1\n");
    }
    if (getchar() != 'x') {
	error("getchar error 2\n");
    }
    if (getchar() != ' ') {
	error("getchar error 3\n");
    }
    if (gets(buf) != buf) {
	error("gets error 1\n");
    }
    if (strcmp(buf, "more on this line.") != 0) {
	error("gets error 2\n");
    }
    if (gets(buf) != buf) {
	error("gets error 3\n");
    }
    if (strcmp(buf, "A second line.") != 0) {
	error("gets error 4\n");
    }
    if (gets(buf) != NULL) {
	error("gets error 5\n");
    }
    if (getchar() != EOF) {
	error("getchar error 4\n");
    }
    dup2(fd, 0);
    (void) fdopen(0, "r");
    close(fd);

    /*
     * fwrite, fread
     */

    for (i = 0; i < 10; i++) {
	int j;

	tsArray1[i].x = 0x52ff6984 + i;
	tsArray1[i].y = 47+i;
	for (j = 0; j < 6; j++) {
	    tsArray1[i].c[j] = 'A' + 2*i + j;
	}
    }
    f1 = fopen("test", "w");
    f2 = fopen("test", "r");
    if (f1 == NULL) {
	error("fwrite error 1\n");
    }
    if (f2 == NULL) {
	error("fread error 1\n");
    }
    if (fwrite(tsArray1, sizeof(testStruct), 10, f1) != 10) {
	error("fwrite error 2\n");
    }
    if (fwrite(&tsArray1[3], sizeof(testStruct), 5, f1) != 5) {
	error("fwrite error 3\n");
    }
    if (fwrite(tsArray1, sizeof(testStruct), 1, f1) != 1) {
	error("fwrite error 4\n");
    }
    if (fwrite(tsArray1, sizeof(testStruct), 1, f2) != 0) {
	error("fwrite error 5\n");
    }
    fclose(f1);
    if (fread(tsArray2, sizeof(testStruct), 2, f2) != 2) {
	error("fread error 2\n");
    }
    TsCheck(&tsArray1[0], &tsArray2[0], "fread error 3\n");
    TsCheck(&tsArray1[1], &tsArray2[1], "fread error 4\n");
    if (fread(tsArray2, sizeof(testStruct), 10, f2) != 10) {
	error("fread error 5\n");
    }
    for (i = 0; i < 10; i++) {
	if (i >= 8) {
	    TsCheck(&tsArray1[i-5], &tsArray2[i], "fread error 6\n");
	} else {
	    TsCheck(&tsArray1[i+2], &tsArray2[i], "fread error 7\n");
	}
    }
    if (fread(tsArray2, sizeof(testStruct), 1, f2) != 1) {
	error("fread error 8\n");
    }
    TsCheck(&tsArray1[5], &tsArray2[0], "fread error 9\n");
    if (fread(tsArray2, sizeof(testStruct), 5, f2) != 3) {
	error("fread error 10\n");
    }
    TsCheck(&tsArray1[6], &tsArray2[0], "fread error 11\n");
    TsCheck(&tsArray1[7], &tsArray2[1], "fread error 12\n");
    TsCheck(&tsArray1[0], &tsArray2[2], "fread error 13\n");
    if (fread(tsArray2, sizeof(testStruct), 1, f2) != 0) {
	error("fread error 14\n");
    }
    fclose(f2);

    /*
     * fseek, rewind, and ftell
     */

    f1 = fopen("test", "w+");
    if (f1 == NULL) {
	error("fseek error 1\n");
    }
    fputs("abcdefghijklmnopqrstuvwxyz", f1);
    if (ftell(f1) != 26) {
	error("fseek error 2\n");
    }
    fseek(f1, 2, SEEK_SET);
    putc('1', f1);
    if (ftell(f1) != 3) {
	error("fseek error 3\n");
    }
    fseek(f1, 4, SEEK_CUR);
    putc('2', f1);
    if (ftell(f1) != 8) {
	error("fseek error 4\n");
    }
    fseek(f1, -1, SEEK_END);
    fputs("345", f1);
    if (ftell(f1) != 28) {
	error("fseek error 5\n");
    }
    fflush(f1);
    CheckFile("test", "ab1defg2ijklmnopqrstuvwxy345", "fseek error 6\n");
    fseek(f1, 1, SEEK_SET);
    if (getc(f1) != 'b') {
	error("fseek error 7\n");
    }
    if (ftell(f1) != 2) {
	error("fseek error 8\n");
    }
    fseek(f1, -2, SEEK_END);
    if (ftell(f1) != 26) {
	error("fseek error 9\n");
    }
    if (getc(f1) != '4') {
	error("fseek error 10\n");
    }
    if (getc(f1) != '5') {
	error("fseek error 11\n");
    }
    if (getc(f1) != EOF) {
	error("fseek error 12\n");
    }
    if (fseek(f1, -1, SEEK_CUR) != 0) {
	error("fseek error 13\n");
    }
    if (getc(f1) != '5') {
	error("fseek error 14\n");
    }
    rewind(f1);
    if (getc(f1) != 'a') {
	error("fseek error 15\n");
    }
    fclose(f1);
    /*
     * Check fseeks across buffer boundaries and with both read-only
     * streams and read-write streams.
     */
    f1 = fopen("test", "w");
    if (f1 == NULL) {
	error("fseek error 16\n");
    }
    for (i = 0; i <= 3 * BUFSIZ - 1; i++) {
	putc(i&0xff, f1);
    }
    fclose(f1);
    f1 = fopen("test", "r");
    if (f1 == NULL) {
	error("fseek error 16a\n");
    }
    if (getc(f1) != 0) {
	error("fseek error 17\n");
    }
    fseek(f1, 0, SEEK_SET);
    if (getc(f1) != 0) {
	error("fseek error 17a\n");
    }
    fseek(f1, BUFSIZ-1, SEEK_SET);
    if (getc(f1) != ((BUFSIZ-1) & 0xff)) {
	error("fseek error 18\n");
    }
    fseek(f1, BUFSIZ+1, SEEK_SET);
    if (getc(f1) != ((BUFSIZ+1) & 0xff)) {
	error("fseek error 19\n");
    }
    if (getc(f1) != ((BUFSIZ+2) & 0xff)) {
	error("fseek error 20\n");
    }
    fseek(f1, 3 * BUFSIZ - 2, SEEK_SET);
    if (getc(f1) != ((3 * BUFSIZ - 2) & 0xff)) {
	error("fseek error 21\n");
    }
    if (getc(f1) != ((3 * BUFSIZ - 1) & 0xff)) {
	error("fseek error 22\n");
    }
    if (getc(f1) != EOF) {
	error("fseek error 23\n");
    }
    fclose(f1);
    f1 = fopen("test", "r+");
    if (f1 == NULL) {
	error("fseek error 24\n");
    }
    for (i = 0; i < 3 * BUFSIZ - 1; i++) {
	putc(i&0xff, f1);
    }
    fseek(f1, 0, SEEK_SET);
    if (getc(f1) != 0) {
	error("fseek error 25\n");
    }
    fseek(f1, BUFSIZ-1, SEEK_SET);
    if (getc(f1) != ((BUFSIZ-1) & 0xff)) {
	error("fseek error 26\n");
    }
    fseek(f1, BUFSIZ+1, SEEK_SET);
    if (getc(f1) != ((BUFSIZ+1) & 0xff)) {
	error("fseek error 27\n");
    }
    if (getc(f1) != ((BUFSIZ+2) & 0xff)) {
	error("fseek error 28\n");
    }
    fseek(f1, 3 * BUFSIZ - 2, SEEK_SET);
    if (getc(f1) != ((3 * BUFSIZ - 2) & 0xff)) {
	error("fseek error 29\n");
    }
    if (getc(f1) != ((3 * BUFSIZ - 1) & 0xff)) {
	error("fseek error 30\n");
    }
    if (getc(f1) != EOF) {
	error("fseek error 31\n");
    }
    fclose(f1);
	
    

    /*
     * Buffering: setbuf, setbuffer, setlinebuf, setvbuf, fflush
     */

    f1 = fopen("test", "w");
    if (f1 == NULL) {
	error("setvbuf error 1\n");
    }
    if (setvbuf(f1, buf, _IOFBF, 10) != 0) {
	error("setvbuf error 2\n");
    }
    fputs("abcd\nefgh", f1);
    if (strncmp(buf, "abcd\nefgh", 9) != 0) {
	error("setvbuf error 3\n");
    }
    CheckFile("test", "", "setvbuf error 4\n");
    putc('i', f1);
    CheckFile("test", "abcd\nefghi", "setvbuf error 5\n");
    fseek(f1, 0, SEEK_SET);
    if (setvbuf(f1, 0, _IOLBF, 100) != 0) {
	error("setvbuf error 6\n");
    }
    buf[0] = buf[1] = 0;
    fputs("123456\n7890", f1);
    if (buf[0] != 0) {
	error("setvbuf error 7\n");
    }
    CheckFile("test", "123456\nghi", "setvbuf error 8\n");
    if (fflush(f1) != 0) {
	error("setvbuf error 9\n");
    }
    CheckFile("test", "123456\n7890", "setvbuf error 10\n");
    putc('X', f1);
    CheckFile("test", "123456\n7890", "setvbuf error 11\n");
    if (setvbuf(f1, 0, _IONBF, BUFSIZ) != 0) {
	error("setvbuf error 11\n");
    }
    CheckFile("test", "123456\n7890X", "setvbuf error 12\n");
    putc('Y', f1);
    CheckFile("test", "123456\n7890XY", "setvbuf error 13\n");
    if (setvbuf(f1, 0, _IOFBF, 1) != 0) {
	error("setvbuf error 14\n");
    }
    putc('Z', f1);
    CheckFile("test", "123456\n7890XYZ", "setvbuf error 15\n");
    setbuf(f1, buf);
    for (i = 0; i < BUFSIZ-1; i++) {
	putc(i&0xff, f1);
    }
    if ((buf[0] != 0) || ((buf[BUFSIZ-2] & 0xff) != ((BUFSIZ-2)&0xff))) {
	error("setbuf error 1\n");
    }
    CheckFile("test", "123456\n7890XYZ", "setbuf error 2\n");
    f2 = fopen("test", "r");
    fseek(f2, 14, SEEK_SET);
    if (getc(f2) != EOF) {
	error("setbuf error 2\n");
    }
    putc('a', f1);
    clearerr(f2);
    c = getc(f2);
    if (c != 0) {
	error("setbuf error 3\n");
    }
    fseek(f2, BUFSIZ-3, SEEK_CUR);
    c = getc(f2);
    if ((c&0xff) != ((BUFSIZ-2)&0xff)) {
	error("setbuf error 4\n");
    }
    if (getc(f2) != 'a') {
	error("setbuf error 5\n");
    }
    if (getc(f2) != EOF) {
	error("setbuf error 6\n");
    }
    fseek(f1, 0, SEEK_SET);
    clearerr(f2);
    fseek(f2, 0, SEEK_SET);
    setlinebuf(f1);
    putc('a', f1);
    c = getc(f2);
    if (c != '1') {
	error("setlinebuf error 1\n");
    }
    putc('\n', f1);
    f2 = freopen("test", "r", f2);
    if (getc(f2) != 'a') {
	error("setlinebuf error 2\n");
    }
    if (getc(f2) != '\n') {
	error("setlinebuf error 3\n");
    }
    fclose(f2);
    fclose(f1);
    f1 = fopen("test", "w");
    if (f1 == NULL) {
	error("setbuffer error 1\n");
    }
    setbuffer(f1, buf, 5);
    fputs("ABCD", f1);
    if (strncmp(buf, "ABCD", 4) != 0) {
	error("setbuffer error 2\n");
    }
    CheckFile("test", "", "setbuffer error 3\n");
    putc('E', f1);
    CheckFile("test", "ABCDE", "setbuffer error 4\n");
    setbuffer(f1, 0, 1000);
    putc('F', f1);
    CheckFile("test", "ABCDEF", "setbuffer error 5\n");
    fclose(f1);
    fclose(f2);

    /*
     * ungetc
     */

    f1 = fopen("test", "r");
    (void) getc(f1);
    (void) getc(f1);
    if (ungetc('x', f1) != 'x') {
	error("ungetc error 1\n");
    }
    if (ungetc('y', f1) != 'y') {
	error("ungetc error 2\n");
    }
    if (ungetc('a', f1) != EOF) {
	error("ungetc error 3\n");
    }
    fgets(buf, 100, f1);
    if (getc(f1) != EOF) {
	error("ungetc error 4\n");
    }
    if (ungetc('#', f1) != '#') {
	error("ungetc error 5\n");
    }
    if (ungetc(EOF, f1) != EOF) {
	error("ungetc error 6\n");
    }
    if (getc(f1) != '#') {
	error("ungetc error 7\n");
    }
    if (getc(f1) != EOF) {
	error("ungetc error 8\n");
    }

    /*
     * fopen
     */

    f1 = fopen("test", "w");
    if (f1 == NULL) {
	error("fopen error 1\n");
    }
    CheckFile("test", "", "fopen error 2\n");
    fputs("Test output", f1);
    fseek(f1, 0, SEEK_SET);
    if (getc(f1) != EOF) {
	error("fopen error 3\n");
    }
    fclose(f1);
    f1 = fopen("test", "r");
    if (f1 == NULL) {
	error("fopen error 4\n");
    }
    if (getc(f1) != 'T') {
	error("fopen error 5\n");
    }
    if (putc('x', f1) != EOF) {
	error("fopen error 6\n");
    }
    fclose(f1);
    f1 = fopen("test", "a");
    if (f1 == NULL) {
	error("fopen error 7\n");
    }
    fputs(" continued", f1);
    fseek(f1, 0, SEEK_SET);
    fputs(" again", f1);
    fclose(f1);
    CheckFile("test", " againutput continued", "fopen error 8\n");
    f1= fopen("test", "w");
    fputs("Test output continued again", f1);
    fclose(f1);
    f1= fopen("test", "rb+");
    if (f1 == NULL) {
	error("fopen error 9\n");
    }
    if (getc(f1) != 'T') {
	error("fopen error 10\n");
    }
    fseek(f1, 0, SEEK_CUR);
    if (fputs("urd", f1) == EOF) {
	error("fopen error 11\n");
    }
    fclose(f1);
    CheckFile("test", "Turd output continued again", "fopen error 12\n");
    f1 = fopen("test", "w+");
    if (f1 == NULL) {
	error("fopen error 13\n");
    }
    CheckFile("test", "", "fopen error 14\n");
    if (fputs("Again", f1) == EOF) {
	error("fopen error 15\n");
    }
    fclose(f1);
    f1 = fopen("test", "a+b");
    if (f1 == NULL) {
	error("fopen error 16\n");
    }
    fputs(" written", f1);
    fflush(f1);
    CheckFile("test", "Again written", "fopen error 17\n");
    fseek(f1, 0, SEEK_SET);
    if (getc(f1) != 'A') {
	error("fopen error 18\n");
    }
    fseek(f1, 0, SEEK_CUR);
    fputs(" to\n", f1);
    fclose(f1);
    CheckFile("test", "A to\n written", "fopen error 19\n");
    f1 = fopen("test", "w");
    fputs("Again written to\n", f1);
    fclose(f1);
    if (fopen("test", "q") != NULL) {
	error("fopen error 20\n");
    }
    if (fopen("test", "ba") != NULL) {
	error("fopen error 21\n");
    }

    /*
     * fdopen, freopen
     */

    fd = open("test", O_RDONLY, 0666);
    if (fd == -1) {
	error("fdopen error 1\n");
    }
    f1 = fdopen(fd, "r");
    if (fgets(buf, 100, f1) == NULL) {
	error("fdopen error 2\n");
    }
    if (strcmp(buf, "Again written to\n") != 0) {
	error("fdopen error 3\n");
    }
    /*
     * Tricky stuff:  it must be possible to have two FILE's open
     * simultaneously on the same stream id.  Sounds crazy, but some
     * UNIX programs depend on it.
     */
    f2 = fopen("test2", "w");
    f3 = fdopen(fileno(f2), "w");
    if (fputs("Test1", f2) == NULL) {
	error("fdopen error 4\n");
    }
    if (fputs("Test2", f3) == NULL) {
	error("fdopen error 5\n");
    }
    if (fputs(" and test3", f2) == NULL) {
	error("fdopen error 6\n");
    }
    if (fputs(" and test4", f3) == NULL) {
	error("fdopen error 7\n");
    }
    fflush(f2);
    fflush(f3);
    CheckFile("test2", "Test1 and test3Test2 and test4", "fdopen error 8\n");
    fclose(f2);
    fclose(f3);
    /*
     * More tricky stuff:  fdopen must seek to the end of the file in
     * "a" mode:  some UNIX programs depend on it.
     */
    fd = open("test2", O_WRONLY, 0666);
    if (fd == -1) {
	error("fdopen error 9\n");
    }
    f2 = fdopen(fd, "a");
    fputs(" and more", f2);
    fclose(f2);
    CheckFile("test2", "Test1 and test3Test2 and test4 and more",
	    "fdopen error 10\n");

    f1 = freopen("test2", "w", f1);
    if (f1 == NULL) {
	error("freopen error 1\n");
    }
    if (fputs("Output data\n", f1) == EOF) {
	error("freopen error 2\n");
    }
    f1 = freopen("test2", "r", f1);
    if (f1 == NULL) {
	error("freopen error 3\n");
    }
    if (fgets(buf, 100, f1) == NULL) {
	error("freopen error 4\n");
    }
    if (strcmp(buf, "Output data\n") != 0) {
	error("freopen error 5\n");
    }
    CheckFile("test2", "Output data\n", "freopen error 6\n");
    fclose(f1);

    /*
     * feof, ferror, clearerr
     */

    f1 = fopen("test", "r");
    f2 = fopen("test", "w");
    if ((f1 == NULL) || (f2 == NULL)) {
	error("ferror error 1\n");
    }
    if (getc(f1) != EOF) {
	error("ferror error 2\n");
    }
    if (!feof(f1)) {
	error("ferror error 3\n");
    }
    putc('a', f2);
    fclose(f2);
    if (getc(f1) != EOF) {
	error("ferror error 4\n");
    }
    if (!feof(f1)) {
	error("ferror error 5\n");
    }
    clearerr(f1);
    if (feof(f1)) {
	error("ferror error 6\n");
    }
    if (getc(f1) != 'a') {
	error("ferror error 7\n");
    }
    if (feof(f1)) {
	error("ferror error 8\n");
    }
    if (getc(f1) != EOF) {
	error("ferror error 9\n");
    }
    if (!feof(f1)) {
	error("ferror error 10\n");
    }
    fclose(f1);
    fd = open("test", O_RDONLY, 0666);
    if (fd == -1) {
	error("ferror error 11\n");
    }
    f1 = fdopen(fd, "r+");
    setvbuf(f1, 0, _IONBF, 1);
    if (putc('x', f1) != EOF) {
	error("ferror error 12\n");
    }
    if (ferror(f1) == 0) {
	error("ferror error 13\n");
    }
    fseek(f1, 0, 0);
    if (getc(f1) != EOF) {
	error("ferror error 14\n");
    }
    if (ferror(f1) == 0) {
	error("ferror error 15\n");
    }
    clearerr(f1);
    if (ferror(f1) != 0) {
	error("ferror error 16\n");
    }
    if (getc(f1) != 'a') {
	error("ferror error 17\n");
    }
    fclose(f1);

    /*
     * _cleanup
     */

    if (fork() == 0) {
	f1 = fopen("test", "w");
	if (f1 == NULL) {
	    error("_cleanup error 1\n");
	}
	fputs("Test string", f1);
	exit(0);
    }
    wait(&status);
    if (status.w_T.w_Retcode == 0) {
	CheckFile("test", "Test string", "_cleanup error 2\n");
    }

    /*
     * fprintf
     */

    f1 = fopen("test", "w");
    f2 = fopen("test", "r");
    if ((f1 == NULL) || (f2 == NULL)) {
	error("sprintf error 1\n");
    }
    setvbuf(f1, 0, _IOLBF, BUFSIZ);
    length = fprintf(f1, "%*d %d %d %d\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "    34 16923 -12 -1\n", length, "fprintf error 2\n");
    length = fprintf(f1, "%4d %4d %4d %4d %d %#x %#X\n", 6, 34,
	    16923, -12, -1, 0, 0);

    CheckNext(f2, "   6   34 16923  -12 -1 0 0\n", length,
	    "fprintf error 3\n");

    length = fprintf(f1, "%4u %4u %4u %4u %d %#o\n", 6, 34, 16923, -12, -1, 0);
    CheckNext(f2, "   6   34 16923 4294967284 -1 0\n", length,
	    "fprintf error 4\n");

    length = fprintf(f1, "%-4d %-4d %-4d %-4ld\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "6    34   16923 -12 \n", length,
	    "fprintf error 5\n");

    length = fprintf(f1, "%04d %04d %04d %04d\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "0006 0034 16923 -012\n", length,
	    "fprintf error 6\n");

    length = fprintf(f1, "%00*d\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "000034\n", length, "fprintf error 7\n");

    length = fprintf(f1, "%4x %4x %4x %4x\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "   6   22 421b fffffff4\n", length,
	    "fprintf error 8\n");

    length = fprintf(f1, "%#x %#X %#X %#x\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "0x6 0X22 0X421B 0xfffffff4\n", length,
	    "fprintf error 9\n");

    length = fprintf(f1, "%#20x %#20x %#20x %#20x\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "                 0x6                 0x22               0x421b           0xfffffff4\n", length, "fprintf error 10\n");

    length = fprintf(f1, "%-#20x %-#20x %-#20x %-#20x\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "0x6                  0x22                 0x421b               0xfffffff4          \n", length, "fprintf error 11\n");

    length = fprintf(f1, "%-#20o %#-20o %#-20o %#-20o\n", 6, 34, 16923, -12, -1);
    CheckNext(f2, "06                   042                  041033               037777777764        \n", length, "fprintf error 12\n");

    length = fprintf(f1, "%s %s %c %s\n", "abcd",
	    "This is a very long test string.", 'x', "x");
    CheckNext(f2, "abcd This is a very long test string. x x\n", length,
	    "fprintf error 13\n");

    length = fprintf(f1, "%20s %20s %20c %20s\n", "abcd",
	    "This is a very long test string.", 'x', "x");
    CheckNext(f2, "                abcd This is a very long test string.                    x                    x\n", length, "fprintf error 14\n");

    length = fprintf(f1, "%.10s %.10s %c %.10s\n", "abcd",
	    "This is a very long test string.", 'x', "x");
    CheckNext(f2, "abcd This is a  x x\n", length, "fprintf error 15\n");

    length = fprintf(f1, "%s %s %1 %% %c %s\n", "abcd",
	    "This is a very long test string.", 'x', "x");
    CheckNext(f2, "abcd This is a very long test string.  % x x\n", length,
	    "fprintf error 16\n");

#ifdef SPRITED_FLOAT
    length = fprintf(f1, "%e %e %e %e\n", 34.2e102, 16923.0/247,
	    -.125, -16000., .000053);
    CheckNext(f2, "3.420000e+103 6.851417e+01 -1.250000e-01 -1.600000e+04\n", length, "fprintf error 17\n");

    length = fprintf(f1, "%20e %20e %20e %20e\n", 34.2e102, 16923.0/247,
	    -.125, -16000., .000053);
    CheckNext(f2, "       3.420000e+103         6.851417e+01        -1.250000e-01        -1.600000e+04\n", length, "fprintf error 18\n");

    length = fprintf(f1, "%.1e %.1e %.1e %.1e\n", 34.2e102, 16923.0/247,
	    -.125, -16000., .000053);
    CheckNext(f2, "3.4e+103 6.9e+01 -1.3e-01 -1.6e+04\n", length,
	    "fprintf error 19\n");

    length = fprintf(f1, "%020e %020e %020e %020e\n", 34.2e102, 16923.0/247,
	    -.125, -16000., .000053);
    CheckNext(f2, "00000003.420000e+103 000000006.851417e+01 -00000001.250000e-01 -00000001.600000e+04\n", length, "fprintf error 20\n");

    length = fprintf(f1, "%7.1e %7.1e %7.1e %7.1e\n", 34.2e102, 16923.0/247,
	    -.125, -16000., .000053);
    CheckNext(f2, "3.4e+103 6.9e+01 -1.3e-01 -1.6e+04\n", length,
	    "fprintf error 21\n");

    length = fprintf(f1, "%f %f %f %f\n", 34.2e102, 16923.0/247,
	    -.125, -16000., .000053);
    CheckNext(f2, "34200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.000000 68.514170 -0.125000 -16000.000000\n", length, "fprintf error 22\n");

    length = fprintf(f1, "%.4f %.4f %.4f %.4f %.4f\n", 34.2e102, 16923.0/247,
	    -.125, -16000., .000053);
    CheckNext(f2, "34200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.0000 68.5142 -0.1250 -16000.0000 0.0001\n", length, "fprintf error 23\n");
#endif /* SPRITED_FLOAT */

    length = fprintf(f1, "%.4e %.5e %.6e\n", -9.99995, -9.99995, 9.99995);
    CheckNext(f2, "-1.0000e+01 -9.99995e+00 9.999950e+00\n", length,
	    "fprintf error 24\n");

    length = fprintf(f1, "%.4f %.5f %.6f\n", -9.99995, -9.99995, 9.99995);
    CheckNext(f2, "-10.0000 -9.99995 9.999950\n", length,
	    "fprintf error 25\n");

    length = fprintf(f1, "%20f %-20f %020f\n", -9.99995, -9.99995, 9.99995);
    CheckNext(f2, "           -9.999950 -9.999950            0000000000009.999950\n", length, "fprintf error 26\n");

    length = fprintf(f1, "%-020f %020f\n", -9.99995, -9.99995, 9.99995);
    CheckNext(f2, "-9.999950            -000000000009.999950\n", length,
	    "fprintf error 27\n");

    length = fprintf(f1, "%.0e %#.0e\n", -9.99995, -9.99995, 9.99995);
    CheckNext(f2, "-1e+01 -1.e+01\n", length, "fprintf error 28\n");

    length = fprintf(f1, "%.0f %#.0f\n", -9.99995, -9.99995, 9.99995);
    CheckNext(f2, "-10 -10.\n", length, "fprintf error 29\n");

    length = fprintf(f1, "%.4f %.5f %.6f\n", -9.99995, -9.99995, 9.99995);
    CheckNext(f2, "-10.0000 -9.99995 9.999950\n", length,
	    "fprintf error 30\n");

    length = fprintf(f1, "%.3g\n", 12341.0);
    CheckNext(f2, "1.23e+04\n", length, "fprintf error 31\n");

    length = fprintf(f1, "%.3G\n", 1234.12345);
    CheckNext(f2, "1.23E+03\n", length, "fprintf error 32\n");

    length = fprintf(f1, "%.3g\n", 123.412345);
    CheckNext(f2, "123\n", length, "fprintf error 33\n");

    length = fprintf(f1, "%.3g\n", 12.3412345);
    CheckNext(f2, "12.3\n", length, "fprintf error 34\n");

    length = fprintf(f1, "%.3g\n", 1.23412345);
    CheckNext(f2, "1.23\n", length, "fprintf error 35\n");

    length = fprintf(f1, "%.3g\n", .123412345);
    CheckNext(f2, "0.123\n", length, "fprintf error 36\n");

    length = fprintf(f1, "%.3g\n", .012341);
    CheckNext(f2, "0.0123\n", length, "fprintf error 37\n");

    length = fprintf(f1, "%.3g\n", .0012341);
    CheckNext(f2, "0.00123\n", length, "fprintf error 38\n");

    length = fprintf(f1, "%.3g\n", .00012341);
    CheckNext(f2, "0.000123\n", length, "fprintf error 39\n");

    length = fprintf(f1, "%.3g\n", .00001234);
    CheckNext(f2, "1.23e-05\n", length, "fprintf error 40\n");

    length = fprintf(f1, "%.4g\n", 9999.5);
    CheckNext(f2, "1e+04\n", length, "fprintf error 41\n");

    length = fprintf(f1, "%.4g\n", 999.95);
    CheckNext(f2, "1000\n", length, "fprintf error 42\n");

    length = fprintf(f1, "%.3g\n", 1.0);
    CheckNext(f2, "1\n", length, "fprintf error 43\n");

    length = fprintf(f1, "%.\n", 1.0);
    CheckNext(f2, "\n", length, "fprintf error 44\n");

    length = fprintf(f1, "%.3g\n", .1);
    CheckNext(f2, "0.1\n", length, "fprintf error 45\n");

    length = fprintf(f1, "%.3g\n", .01);
    CheckNext(f2, "0.01\n", length, "fprintf error 46\n");

    length = fprintf(f1, "%.3g\n", .001);
    CheckNext(f2, "0.001\n", length, "fprintf error 47\n");

    length = fprintf(f1, "%.3g\n", .0001);
    CheckNext(f2, "1e-04\n", length, "fprintf error 48\n");

    length = fprintf(f1, "%.3g\n", .00001);
    CheckNext(f2, "1e-05\n", length, "fprintf error 49\n");

    length = fprintf(f1, "%#.3g\n", 1234.0);
    CheckNext(f2, "1.23e+03\n", length, "fprintf error 50\n");

    length = fprintf(f1, "%#.3G\n", 9999.5);
    CheckNext(f2, "1.00E+04\n", length, "fprintf error 51\n");

    length = fprintf(f1, "%e %f %g\n", 0.0, 0.0, 0.0, 0.0);
    CheckNext(f2, "0.000000e+00 0.000000 0\n", length, "fprintf error 52\n");

    length = fprintf(f1, "%.4e %.4f %.4g\n", 0.0, 0.0, 0.0, 0.0);
    CheckNext(f2, "0.0000e+00 0.0000 0\n", length, "fprintf error 53\n");

    length = fprintf(f1, "%#.4e %#.4f %#.4g\n", 0.0, 0.0, 0.0, 0.0);
    CheckNext(f2, "0.0000e+00 0.0000 0.000\n", length, "fprintf error 54\n");

    length = fprintf(f1, "%.0e %.0f %.0g\n", 0.0, 0.0, 0.0, 0.0);
    CheckNext(f2, "0e+00 0 0\n", length, "fprintf error 55\n");

    length = fprintf(f1, "%#.0e %#.0f %#.0g\n", 0.0, 0.0, 0.0, 0.0);
    CheckNext(f2, "0.e+00 0. 0.\n", length, "fprintf error 56\n");

    length = fprintf(f1, "%3.0f %3.0f %3.0f %3.0f\n", 0.0, 0.1, 0.01, 0.001);
    CheckNext(f2, "  0   0   0   0\n", length, "fprintf error 57\n");

    length = fprintf(f1, "%3.0f %3.0f %3.0f %3.0f\n", 1.0, 1.1, 1.01, 1.001);
    CheckNext(f2, "  1   1   1   1\n", length, "fprintf error 58\n");

    length = fprintf(f1, "%3.1f %3.1f %3.1f %3.1f\n", 0.0, 0.1, 0.01, 0.001);
    CheckNext(f2, "0.0 0.1 0.0 0.0\n", length, "fprintf error 59\n");

    fclose(f1);
    fclose(f2);

    /*
     * sscanf
     */

    count = sscanf("-20 1476 \n33 0", "%d %d %d %d", &d1, &d2, &d3, &d4);
    if ((count != 4) || (d1 != -20) || (d2 != 1476) || (d3 != 33)
	    || (d4 != 0)) {
	error("sscanf error 1\n");
    }

    d4 = -1;
    count = sscanf("-45 16 7890 +10", "%2d %*d %10d %d", &d1, &d2, &d3, &d4);
    if ((count != 3) || (d1 != -4) || (d2 != 16) || (d3 != 7890)
	    || (d4 != -1)) {
	error("sscanf error 2\n");
    }

    count = sscanf("-45 16 +10 987", "%D %  %hD %d", &d1, &d2, &d3, &d4);
    if ((count != 4) || (d1 != -45) || (d2 != 16) || (d3 != 10)
	    || (d4 != 987)) {
	error("sscanf error 3\n");
    }

    sa1[0] = sa1[1] = sa1[2] = sa2[0] = sa2[1] = sa2[2] = -1;
    count = sscanf("14 1ab 62 10", "%hd %x %O %hx", &sa1[1], &d2, &d3,
	    &sa2[1]);
    if ((count != 4) || (sa1[0] != -1) || (sa1[1] != 14) || (sa1[2] != -1)
	    || (d2 != 427) || (d3 != 50) || (sa2[0] != -1) || (sa2[1] != 0x10)
	    || (sa2[2] != -1)) {
	error("sscanf error 4\n");
    }

    count = sscanf("12345670 1234567890ab cdefg", "%o	 %o %x %X",
	    &d1, &d2, &d3, &d4);
    if ((count != 4) || (d1 != 2739128) || (d2 != 342391) || (d3 != 561323)
	    || (d4 != 52719)) {
	error("sscanf error 5\n");
    }

    count = sscanf("ab123-24642", "%2x %3hx %3o %2ho", &d1, &s1, &d3, &s2);
    if ((count != 4) || (d1 != 171) || (s1 != 0x123) || (d3 != -20)
	    || (s2 != 0x34)) {
	error("sscanf error 6\n");
    }

    d2 = d3 = d4 = -1;
    count = sscanf("42 43", "%d %", &d1, &d2, &d3, &d4);
    if ((count != -1) || (d1 != 42) || (d2 != -1) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 7\n");
    }

    d2 = d3 = d4 = -1;
    count = sscanf("42", "%d", &d1, &d2, &d3, &d4);
    if ((count != 1) || (d1 != 42) || (d2 != -1) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 8\n");
    }

    d2 = d3 = d4 = -1;
    count = sscanf("1 2 3 4 5", "%q %r", &d1, &d2, &d3, &d4);
    if ((count != 2) || (d1 != 1) || (d2 != 2) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 9\n");
    }

    d3 = d4 = -1;
    count = sscanf("1234567 234 567  ", "%*3x %x %*o %4o", &d1, &d2, &d3, &d4);
    if ((count != 2) || (d1 != 17767) || (d2 != 375) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 10\n");
    }

    d3 = d4 = -1;
    count = sscanf("1234567 234 567  ", "%*3x %x %*o %4o", &d1, &d2, &d3, &d4);
    if ((count != 2) || (d1 != 17767) || (d2 != 375) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 11\n");
    }

    d1 = d2 = d3 = d4 = -1;
    count = sscanf("a	1234", "%d %d", &d1, &d2, &d3, &d4);
    if ((count != 0) || (d1 != -1) || (d2 != -1) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 11\n");
    }

    count = sscanf("12345678", "%2d %2d %2d %2d", &d1, &d2, &d3, &d4);
    if ((count != 4) || (d1 != 12) || (d2 != 34) || (d3 != 56)
	    || (d4 != 78)) {
	error("sscanf error 12\n");
    }

    d3 = d4 = -1;
    count = sscanf("1 2 ", "%d %d %d %d", &d1, &d2, &d3, &d4);
    if ((count != 2) || (d1 != 1) || (d2 != 2) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 13\n");
    }

    d1 = d2 = d3 = d4 = -1;
    count = sscanf("  a", " a%d %d %d %d", &d1, &d2, &d3, &d4);
    if ((count != -1) || (d1 != -1) || (d2 != -1) || (d3 != -1)
	    || (d4 != -1)) {
	error("sscanf error 14\n");
    }

    fp4 = -1.0;
    count = sscanf("2.1 -3.0e8 .99962 a", "%f%f%f%f", &fp1, &fp2, &fp3, &fp4);
    CheckFp(count, 3, fp1, fp2, fp3, fp4,
	    "2.100000e+00 -3.000000e+08 9.996200e-01 -1.000000e+00",
	    "sscanf error 15\n");

    count = sscanf("-1.2345 +8.2 9", "%3e %3f %f %f", &fp1, &fp2, &fp3, &fp4);
    CheckFp(count, 4, fp1, fp2, fp3, fp4,
	    "-1.000000e+00 2.340000e+02 5.000000e+00 8.200000e+00",
	    "sscanf error 16\n");

    fp4 = -1.0;
    count = sscanf("1e00004 332E-4 3e+4", "%f %*2e %f %f", &fp1,
	    &fp2, &fp3, &fp4);
    CheckFp(count, 3, fp1, fp2, fp3, fp4,
	    "1.000000e+04 2.000000e-04 3.000000e+04 -1.000000e+00",
	    "sscanf error 17\n");

    fp4 = -1.0;
    count = sscanf("1. 47.6 2.e2 3.e-", "%f %*f %f %f", &fp1, &fp2,
	    &fp3, &fp4);
    CheckFp(count, 3, fp1, fp2, fp3, fp4,
	    "1.000000e+00 2.000000e+02 3.000000e+00 -1.000000e+00",
	    "sscanf error 18\n");

#ifdef SPRITED_FLOAT
    /* 
     * I don't know if the mismatch between the format string and the 
     * number of arguments would cause a problem.  Even if you drop fp3 and 
     * fp4 from the sscanf, this causes a crash with the "mipsfree" MK63.
     */
    fp3 = fp4 = -1.0;
    count = sscanf("1.eabc", "%f %x", &fp1, &fp2, &fp3, &fp4);
    CheckFp(count, 2, fp1, fp2, fp3, fp4,
	    "1.000000e+00 3.850768e-42 -1.000000e+00 -1.000000e+00",
	    "sscanf error 19\n");
#endif

    count = sscanf("4.6 99999.7 876.43e-1 118", "%f %f %f %e",
	    &fp1, &fp2, &fp3, &fp4);
    CheckFp(count, 4, fp1, fp2, fp3, fp4,
	    "4.600000e+00 9.999970e+04 8.764300e+01 1.180000e+02",
	    "sscanf error 20\n");

    count = sscanf("1.2345 697.0e-3 124 .00005", "%F %E %lf %le",
	    &dbl1, &dbl2, &dbl3, &dbl4);
    CheckFp(count, 4, dbl1, dbl2, dbl3, dbl4,
	    "1.234500e+00 6.970000e-01 1.240000e+02 5.000000e-05",
	    "sscanf error 21\n");

    count = sscanf("4.6 99999.7 876.43e-1 118", "%F %F %F %F",
	    &dbl1, &dbl2, &dbl3, &dbl4);
    CheckFp(count, 4, dbl1, dbl2, dbl3, dbl4,
	    "4.600000e+00 9.999970e+04 8.764300e+01 1.180000e+02",
	    "sscanf error 22\n");

    dbl2 = dbl3 = dbl4 = -1.0;
    count = sscanf("4.6abc", "%F %F %f %f",
	    &dbl1, &dbl2, &dbl3, &dbl4);
    CheckFp(count, 1, dbl1, dbl2, dbl3, dbl4,
	    "4.600000e+00 -1.000000e+00 -1.000000e+00 -1.000000e+00",
	    "sscanf error 23\n");

    dbl3 = dbl4 = -1.0;
    count = sscanf("4.6 5.2", "%F %F %F %F",
	    &dbl1, &dbl2, &dbl3, &dbl4);
    CheckFp(count, 2, dbl1, dbl2, dbl3, dbl4,
	    "4.600000e+00 5.200000e+00 -1.000000e+00 -1.000000e+00",
	    "sscanf error 24\n");

    CheckString("abc defghijk dum ", "%s %3s %20s %s", 4,
	    "abc", "def", "ghijk", "dum",
	    "sscanf error 25\n");

    CheckString("abc       bcdef", "%5c%c%1s %s", 4,
	    "abc  ", " XXXX", "b", "cdef",
	    "sscanf error 26\n");

    CheckString("123456 test ", "%*c%*s %s %s %s", 1,
	    "test", "XXXXX", "XXXXX", "XXXXX",
	    "sscanf error 27\n");

    CheckString("ababcd01234  f 123450", "%4[abcd] %4[abcd] %[^abcdef] %[^0]",
	    4, "abab", "cd", "01234  ", "f 12345",
	    "sscanf error 28\n");

    CheckString("aaaaaabc aaabcdefg  + +  XYZQR",
	    "%*4[a] %s %*4[a]%s%*4[ +]%10c", 3,
	    "aabc", "bcdefg", "+  XYZQR", "XXXXX",
	    "sscanf error 29\n");

    /*
     * printf, scanf, fscanf, sprintf, vprintf, vsprintf
     */

    fd = dup(1);
    fclose(stdout);
    f1 = fopen("test", "w");
    count = printf("%d and %d", 47, 102);
    if (count != 10) {
	error("printf error 1\n");
    }
    count = CheckVprintf("  %x then %x", 15, 65);
    if (count != 11) {
	error("printf error 2\n");
    }
    fclose(stdout);
    CheckFile("test", "47 and 102  f then 41", "printf error 3\n");
    dup2(fd, 1);
    (void) fdopen(1, "w");
    close(fd);

    fd = dup(0);
    fclose(stdin);
    f1 = fopen("test", "r");
    count = scanf("%d and %d %d", &d1, &d2, &d3);
    if ((count != 2) || (d1 != 47) || (d2 != 102)) {
	error("scanf error 1\n");
    }
    fclose(stdin);
    dup2(fd, 0);
    (void) fdopen(0, "r");
    close(fd);

    f1 = fopen("test", "r");
    count = fscanf(f1, "%d and %d %d", &d1, &d2, &d3);
    if ((count != 2) || (d1 != 47) || (d2 != 102)) {
	error("fscanf error 1\n");
    }
    fclose(f1);

    if (sprintf(buf, "%s %d", "The value is", 19) != buf) {
	error("sprintf error 1\n");
    }
    if (strcmp(buf, "The value is 19") != 0) {
	error("sprintf error 2\n");
    }

    if (CheckVsprintf(buf, "%s %d", "The value is", 19) != buf) {
	error("vsprintf error 1\n");
    }
    if (strcmp(buf, "The value is 19") != 0) {
	error("vsprintf error 2\n");
    }
}
