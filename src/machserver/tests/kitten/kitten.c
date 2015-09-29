/* small "cat" program. */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/tests/kitten/RCS/kitten.c,v 1.3 92/03/23 15:10:10 kupfer Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <test.h>
#include <unistd.h>

#define USE_MALLOC	1
#define BUF_SIZE	8192
#define USE_CONSOLE	1	/* 0 to use Test_foo to write each line */
#define DO_SHUTDOWN	0	/* 0 to just exit quietly */

static void CatFile();
static void DoFile();

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int i;

    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    continue;
	}
	DoFile(argv[i]);
    }

#if DO_SHUTDOWN
    (void)Sys_Shutdown(SYS_HALT|SYS_KILL_PROCESSES|SYS_WRITE_BACK, NULL);
#endif
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * DoFile --
 *
 *	Process the named file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If it's a text file, its contents are printed.  For anything else, 
 *	stat information is given for it.
 *
 *----------------------------------------------------------------------
 */

static void
DoFile(fileName)
    char *fileName;
{
    struct stat statBuf;

    if (stat(fileName, &statBuf) < 0) {
	perror(fileName);
	return;
    }

    if ((statBuf.st_mode & S_IFMT) == S_IFREG) {
	CatFile(fileName);
    } else {
	Test_PutMessage("Type: ");
	Test_PutOctal(statBuf.st_mode & S_IFMT);
	Test_PutMessage("\nServer: ");
	Test_PutDecimal(statBuf.st_serverID);
	Test_PutMessage("\nDevice: ");
	Test_PutDecimal(statBuf.st_dev);
	Test_PutMessage("\nI-number: ");
	Test_PutDecimal(statBuf.st_ino);
	Test_PutMessage("\nOwner: ");
	Test_PutDecimal(statBuf.st_uid);
	Test_PutMessage("\n");
    }
}


/*
 *----------------------------------------------------------------------
 *
 * CatFile --
 *
 *	Print the contents of a file.
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
CatFile(fileName)
    char *fileName;
{
#ifdef USE_MALLOC
    char *buf;
#else
    char buf[BUF_SIZE];
#endif
    int file;			/* file descriptor */
    int nChars;			/* number of characters read */
#if USE_CONSOLE
    int cons;			/* console file descriptor */
#endif

#ifdef USE_MALLOC
    buf = malloc(BUF_SIZE);
    if (buf == NULL) {
	Test_PutMessage("malloc failed.\n");
	exit(1);
    }
#endif
    file = open(fileName, O_RDONLY);
    if (file < 0) {
	perror(fileName);
	exit(1);
    }
#if USE_CONSOLE
    cons = open("/dev/console", O_WRONLY, 0666);
    if (cons < 0) {
	perror("can't open console");
	exit(1);
    }
#endif /* USE_CONSOLE */

    while ((nChars = read(file, buf, BUF_SIZE)) > 0) {
#if USE_CONSOLE
	if (write(cons, buf, nChars) < 0) {
	    perror("can't write to console");
	    exit(1);
	}
#else
	Test_PutString(buf, nChars);
#endif
    }
    if (nChars < 0) {
	perror("read");
	exit(1);
    }

    close(file);
#ifdef USE_MALLOC
    free(buf);
#endif
}

