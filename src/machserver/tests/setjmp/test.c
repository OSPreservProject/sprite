/* 
 * Test program to verify that setjmp and longjmp work.
 */

#include <stdio.h>
#include <setjmp.h>

jmp_buf env;

main()
{
    int ch;

    for (;;) {
	if (setjmp(env)) {
	    printf("longjmp.\n");
	}
	printf("? ");
	ch = getone();
	if (ch == EOF) {
	    exit(0);
	} else {
	    printf("%c\n", ch);
	}
    }
}

getone()
{
    return getone_b();
}

getone_b()
{
    int ch;

    ch = getchar();
    if (ch == 'l') {
	longjmp(env, 1);
    } else {
	return ch;
    }
}
