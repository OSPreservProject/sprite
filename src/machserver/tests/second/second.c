/* Quick test program to get exec'd. */

#include <sprite.h>
#include <cfuncproto.h>
#include <errno.h>
#include <mach.h>
#include <status.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <test.h>
#include <vm.h>

static void CatFile();
static int GetLength _ARGS_((char *fileName));
static void MapFile _ARGS_((char *fileName, boolean_t readOnly,
			    int length, Address *startAddrPtr));

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int i;

    /* print the arguments */
    Test_PutDecimal(argc);
    Test_PutMessage(" arguments: \n");
    for (i = 0; i < argc; i++) {
	Test_PutMessage(argv[i]);
	Test_PutMessage("\n");
    }

    /* if there was a file named, cat it */
    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    continue;
	}
	CatFile(argv[i]);
    }

    return 0;
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
    int fileLength;
    char *buffer;

    Test_PutMessage("----- ");
    Test_PutMessage(fileName);
    Test_PutMessage(" -----\n");

    if (strcmp(fileName, "killMe") == 0) {
	*(char *)0xffffffff = fileName[0];
    }
    if (strcmp(fileName, "nullArg") == 0) {
	fileName = (char *)0;
    }
    fileLength = GetLength(fileName);
    if (fileLength > 0) {
	MapFile(fileName, TRUE, fileLength, &buffer);
	if (buffer != 0) {
	    Test_PutString(buffer, fileLength);
	    vm_deallocate(mach_task_self(), (vm_address_t)buffer, fileLength);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * GetLength --
 *
 *	Get the length of a file.
 *
 * Results:
 *	Returns the length of the file, in bytes.  Returns -1 if there 
 *	was an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetLength(fileName)
    char *fileName;
{
    struct stat statBuf;

    if (stat(fileName, &statBuf) < 0) {
	Test_PutMessage("Couldn't get length of `");
	Test_PutMessage(fileName);
	Test_PutMessage("': ");
	Test_PutMessage(strerror(errno));
	Test_PutMessage("\n");
	return -1;
    }

    return statBuf.st_size;
}


/*
 *----------------------------------------------------------------------
 *
 * MapFile --
 *
 *	Map the named file into our address space.
 *
 * Results:
 *	Fills in the starting location, which is set to 0 
 *	if there was a problem.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
MapFile(fileName, readOnly, length, startAddrPtr)
    char *fileName;		/* name of file to map */
    Boolean readOnly;		/* map read-only or read-write? */
    int length;			/* number of bytes to map */
    Address *startAddrPtr;	/* OUT: where the file was mapped to */
{
    ReturnStatus status;

    status = Vm_MapFile(fileName, readOnly, 0, length, startAddrPtr);
    if (status != SUCCESS) {
	Test_PutMessage("Couldn't map `");
	Test_PutMessage(fileName);
	Test_PutMessage("': ");
	Test_PutMessage(Stat_GetMsg(status));
	Test_PutMessage("\n");
	*startAddrPtr = 0;
    }
}

