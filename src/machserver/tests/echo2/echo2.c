/* Test program to echo lines typed at the keyboard. */

#include <stdio.h>
#include <string.h>
#include <sys.h>

#define BUF_SIZE	1024
#define USE_CONSOLE	1	/* 0 to use the inherited stdin and stdout */
#define DO_SHUTDOWN	1	/* 0 to just exit */

main()
{
    char buf[BUF_SIZE];
    FILE *inFile;
    FILE *outFile;
    int status = 0;

#if !USE_CONSOLE
    inFile = stdin;
    outFile = stdout;
#else
    inFile = fopen("/dev/console", "r+");
    outFile = inFile;
    if (inFile == NULL) {
	printf("couldn't open console\n");
	status = 1;
	goto done;
    }
#endif
    for (;;) {
	printf("Type something: ");
	fflush(stdout);
	if (fgets(buf, BUF_SIZE, inFile) == NULL) {
	    goto done;
	}
	if (strcmp(buf, ".\n") == 0) {
	    goto done;
	}
	fputs(buf, outFile);
    }

 done:
#if DO_SHUTDOWN
    (void)Sys_Shutdown(SYS_HALT|SYS_KILL_PROCESSES|SYS_WRITE_BACK, NULL);
#endif
    exit(status);
}
