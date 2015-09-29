/*
 * NAME
 *      mtest - test memory allocation.
 *
 * COMPILATION
 *	cc -o mtest mtest.c
 *
 * SYNOPSIS
 *	mtest
 *
 * DESCRIPTION
 *	mtest demonstrates that malloc is broken.  Specifically, that
 *	malloc is not smart enough to use a piece of memory of exactly
 *	the right size when it is available (unless it is at the end
 *	of memory).
 */
#include <stdio.h>
#include <sys/errno.h>
#include <sys/types.h>

#define	BIGBUF		1000000	/* 1 MBytes */
#define SMALLBUF	10240	/* 10 KBytes */

char	*malloc();
caddr_t sbrk();
extern int errno;

main()
{
	char	*buf1, *buf2;	/* buffer pointers */
	caddr_t	topmem;		/* upper limit of memory */

	printf("beginning memory allocation test\n");
	printf("All values are reported in decimal.\n");
	topmem = sbrk(0);
	printf("The end of memory is initially at %d\n\n",(int) topmem);

	/*
	 * Demonstrate that you can (sometimes) re-use memory which has
	 * been malloc'ed and subsequently free'd.
	 */
	buf1 = malloc(BIGBUF);
	if (buf1 == 0) {
		perror("test");
		errno = 0;
		exit(1);
        }
	else {
		printf("requested %d bytes of memory\n", BIGBUF);
		topmem = sbrk(0);
		printf("The end of memory is now at %d\n", (int) topmem);
	}

	if (free(buf1) == 0) {
		perror("test");
		errno = 0;
		exit(2);
	}
	else {
		printf("%d bytes of memory freed\n", BIGBUF);
		printf("There is now a %d byte piece of memory available.\n", BIGBUF);
		topmem = sbrk(0);
		printf("The end of memory is still at %d\n", (int) topmem);
	}

	buf1 = malloc(BIGBUF);
	if (buf1 == 0) {
        	perror("test");
		errno = 0;
		exit(1);
	}
	else {
		printf("requested another %d bytes of memory -- ", BIGBUF);
		printf("Should use available space.\n");
		topmem = sbrk(0);
		printf("The end of memory is still at %d\n\n", (int) topmem);
	}

	/*
	 * Now demonstrate that if a block of free memory is surrounded
	 * by other chunks of memory, then a memory request of that size
	 * will NOT use the available piece.
	 */
	buf2 = malloc(SMALLBUF);
	if (buf2 == 0) {
		perror("test");
		errno = 0;
		exit(1);
	}
        else {
		printf("Requested %d bytes of memory.\n", SMALLBUF);
		topmem = sbrk(0);
		printf("The end of memory is now at %d\n", (int) topmem);
	}

	if (free(buf1) == 0) {
		perror("test");
		errno = 0;
		exit(2);
	}
	else {
		printf("Freed %d bytes of memory.\n", BIGBUF);
		printf("There is now a %d byte piece of memory available.\n", BIGBUF);
		topmem = sbrk(0);
		printf("The end of memory is still at %d\n", (int) topmem);
	}

	buf1 = malloc(BIGBUF);
	if (buf1 == 0) {
		perror("test");
        	errno = 0;
		exit(1);
	}
	else {
		printf("Requested another %d bytes of memory -- ", BIGBUF);
		printf("Should use available space.\n");
		topmem = sbrk(0);
		printf("WHOA!  The end of memory is now at %d\n",(int) topmem);
	}

	exit(0);
}
