/* 
 * Check what happens if the handler for SIGSEGV doesn't try to correct for 
 * whatever caused the signal.
 *
 * $Header: /user5/kupfer/spriteserver/tests/repeatfault/RCS/segv.c,v 1.1 92/03/12 20:51:45 kupfer Exp $
 */

#include <signal.h>

int ntimes = 0;

handler()
{
    Test_PutMessage("ding\n");
    ++ntimes;
    if (ntimes > 5) {
	exit(1);
    }
}

main()
{
    int *bogusPtr = (int *)0x98765433;
    int i;

    signal(SIGSEGV, handler);
    i = *bogusPtr;
    printf("i = 0x%x\n", i);
    exit(0);
}
