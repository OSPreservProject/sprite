/* 
 * test.c --
 *
 *	Some tests for malloc().
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/tests/malloc/RCS/test.c,v 1.1 91/12/12 22:39:07 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <bstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <vm.h>

#define PARENT_DOES_MALLOC	1

/* Forward references: */

void CheckBuffer(), FillBuffer();
void DoChild(), DoParent();

main()
{
    char *buf;
    int pageSize;
    int fds[2];			/* descriptors for pipe */
    int childPid;

#if PARENT_DOES_MALLOC
    /* Try allocating something smaller than a page. */
    buf = malloc(100);
    strcpy(buf, "small buffer");
    bzero(buf, 100);
    free(buf);

    /* Try allocating an entire page. */
    Vm_PageSize(&pageSize);
    buf = malloc(pageSize);
    bzero(buf, pageSize);

    /* 
     * Write into the one-page buffer.  Fork.  Do some tests.  
     */

    FillBuffer(buf, pageSize, 'a');
#endif /* PARENT_DOES_MALLOC */
    pipe(fds);

    childPid = fork();
    switch (childPid) {
    case -1:
	perror("fork");
	exit(1);
	break;
    case 0:
	DoChild(buf, pageSize, fds[0]);
	break;
    default:
	DoParent(buf, pageSize, fds[1]);
	break;
    }

    exit(0);
}

/* 
 * Verify that the child inherited the buffer correctly and can continue to
 * do its own mallocs.  Verify that the parent can overwrite the buffer w/o
 * the child seeing it.
 */
void
DoChild(buf, bufSize, fd)
    char *buf;
    int bufSize;
    int fd;			/* descriptor for pipe to parent */
{
    char *dummy;
    char waitChar[10];

#if PARENT_DOES_MALLOC
    CheckBuffer(buf, bufSize, 'a', "child");
#endif

#if 0
    Test_PutMessage("child: CR to continue\n");
    Test_GetString(waitChar, 10);
#endif
    dummy = malloc(bufSize / 2);
    if (dummy == NULL) {
	fprintf(stderr, "child: first malloc failed.\n");
	exit(1);
    }
#if PARENT_DOES_MALLOC
    bcopy(buf, dummy, bufSize/2);
#endif
    free(dummy);
    dummy = malloc(bufSize);
    if (dummy == NULL) {
	fprintf(stderr, "child: second malloc failed.\n");
	exit(1);
    }
#if PARENT_DOES_MALLOC
    bcopy(buf, dummy, bufSize);
#endif
    free(dummy);

    if (read(fd, dummy, 1) < 0) {
	perror("child: read");
	exit(1);
    }
#if PARENT_DOES_MALLOC
    CheckBuffer(buf, bufSize, 'a', "child");
#endif
    free(dummy);
}

void
DoParent(buf, bufSize, fd)
    char *buf;
    int bufSize;
    int fd;			/* descriptor for pipe to child */
{
    char dummy;
    union wait status;

#if PARENT_DOES_MALLOC
    CheckBuffer(buf, bufSize, 'a', "parent");
    FillBuffer(buf, bufSize, 'b');
#endif
    if (write(fd, &dummy, 1) < 0) {
	perror("parent: write");
	exit(1);
    }
#if PARENT_DOES_MALLOC
    CheckBuffer(buf, bufSize, 'b', "parent");
#endif

    /* Wait for the child to exit. */
    wait(&status);
}

/* Fill the buffer with the given character. */
void
FillBuffer(buf, bufSize, ch)
    char *buf;
    int bufSize;
    char ch;
{
    for (; bufSize > 0; buf++, bufSize--) {
	*buf = ch;
    }
}

/* Verify that the buffer is filled with the given character. */
void
CheckBuffer(buf, bufSize, ch, who)
    char *buf;
    int bufSize;
    char ch;
    char *who;			/* "parent" or "child" */
{
    char *bufPtr;

    for (bufPtr = buf; bufPtr < buf + bufSize; bufPtr++) {
	if (*bufPtr != ch) {
	    fprintf(stderr, "%s: wanted 0x%x, found 0x%x at buf[%d]\n",
		    who, ch, *bufPtr, bufPtr - buf);
	    exit(1);
	}
    }
}
