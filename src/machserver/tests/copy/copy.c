/* Simple file copy program.  */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/tests/copy/RCS/copy.c,v 1.2 91/12/12 22:33:09 kupfer Exp $";
#endif

#include <sprite.h>
#include <mach.h>
#include <mach_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <test.h>
#include <unistd.h>

/* #define USE_MALLOC	1 */

static void CopyFile();

int
main(argc, argv)
    int argc;
    char *argv[];
{
    if (argc != 3) {
	Test_PutMessage("usage: copy from to\n");
	exit(1);
    }
    CopyFile(argv[1], argv[2]);
    
    return 0;
}

static void
CopyFile(fromFileName, toFileName)
    char *fromFileName;
    char *toFileName;
{
    int fromFd;
    int toFd;
    Address buffer;
    int nChars;
#ifndef USE_MALLOC
    kern_return_t kernStatus;
#endif
    int bufferSize;		/* number of bytes in buffer */

    fromFd = open(fromFileName, O_RDONLY);
    if (fromFd < 0) {
	perror(fromFileName);
	exit(1);
    }
    toFd = open(toFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (toFd < 0) {
	perror(toFileName);
	exit(1);
    }
#ifdef USE_MALLOC
    bufferSize = 2 * vm_page_size;
    buffer = malloc(bufferSize);
    if (buffer == NULL) {
	Test_PutMessage("Couldn't malloc buffer\n");
	exit(1);
    } else {
	Test_PutMessage("buffer at ");
	Test_PutHex(buffer);
	Test_PutMessage("\n");
    }
#else
    bufferSize = 2 * vm_page_size;
    buffer = 0;
    kernStatus = vm_allocate(mach_task_self(), (vm_address_t *)&buffer, 
			     bufferSize, TRUE);
    if (kernStatus != KERN_SUCCESS) {
	Test_PutMessage("Couldn't allocate buffer: ");
	Test_PutDecimal(kernStatus);
	Test_PutMessage("\n");
	exit(1);
    }
#endif

    while ((nChars = read(fromFd, buffer, bufferSize)) > 0) {
	Test_PutMessage("Read ");
	Test_PutDecimal(nChars);
	Test_PutMessage("\n");
	if (write(toFd, buffer, nChars) != nChars) {
	    Test_PutMessage("short write.\n");
	    exit(1);
	}
    }
    if (nChars < 0) {
	perror("read");
	exit(1);
    }
}
