/* Benchmark to test copyin/copyout performance. */
/* $Header: /user5/kupfer/spriteserver/src/benchmarks/copyin/RCS/copyin.c,v 1.4 92/07/16 18:07:59 kupfer Exp $ */

#include <sprite.h>
#include <option.h>
#include <stdio.h>
#include <spriteTime.h>
#include <status.h>
#include <sys/time.h>
#include <unistd.h>
#include <vmTypes.h>

/* #define DEBUG */

char *buf;			/* buffer to transfer with Sprite */

int copySize[] = {0, 10, 100, 1024, 4096, 8192}; /* various amounts to copy */
int numSizes = sizeof(copySize) / sizeof(int);

int iterations = 10000;		/* number of iterations for each copy size */

int spoilCOW = 0;		/* write to the buffer to prevent COW from 
				 * giving optimistic results */
int useMakeAccessible = 0;	/* use Vm_MakeAccessible & bcopy instead of 
				 * copyin/out */ 
int copyout = 0;		/* copy kernel->user instead of vice versa */

int offsetBuf = 0;		/* make sure our copy of the buffer isn't 
				 * page-aligned */ 
int inband = 0;			/* do the copies using the "inband" 
				 * interface */ 

Option optionArray[] = {
    {OPT_DOC, NULL, NULL,
	 "Exercise Sprite's copyin routine by passing various amounts\n\
from a dummy buffer to the kernel and reporting the elapsed time."},
    {OPT_TRUE, "cow", (Address)&spoilCOW,
	 "Prevent copy-on-write from improving the numbers"},
    {OPT_INT, "iter", (Address)&iterations,
	 "Number of times to copy the buffer for each copy amount"},
    {OPT_TRUE, "ma", (Address)&useMakeAccessible,
	 "Use Vm_MakeAccessible instead of Vm_CopyIn"},
    {OPT_TRUE, "out", (Address)&copyout,
	 "Do copyout instead of copyin"},
    {OPT_TRUE, "offset", (Address)&offsetBuf,
	 "Prevent copy-on-write by making the buffer non-page-aligned."},
    {OPT_TRUE, "inband", (Address)&inband,
	 "Use MIG to transfer the buffer instead of copyin/copyout."},
};
int	numOptions = Opt_Number(optionArray);

void WriteToBuffer();
#ifdef DEBUG
void InitBuf(), CheckBuf();
#endif

main(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    Time startTime, endTime, totalTime;
    int sizeIndex;
    ReturnStatus status;
    int command;		/* "copyin" versus "make accessible" */
    char *operation;		/* printable name of operation */

    (void)Opt_Parse(argc, argv, optionArray, numOptions, 0);
    buf = valloc(2 * VM_DO_COPY_MAX_SIZE);
    if (buf == NULL) {
	fprintf(stderr, "copyin: no memory.\n");
	exit(1);
    }
    if (offsetBuf) {
	buf += 10;
    }
#ifdef DEBUG
    InitBuf();
#endif
    if (useMakeAccessible) {
	if (inband) {
	    fprintf(stderr, "No inband version of Vm_MakeAccessible.\n");
	    exit(1);
	}
	if (copyout) {
	    operation = "MakeAccessible (out)";
	    command = VM_DO_MAKE_ACCESS_OUT;
	} else {
	    operation = "MakeAccessible (in)";
	    command = VM_DO_MAKE_ACCESS_IN;
	}
    } else {
	if (copyout) {
	    if (inband) {
		operation = "Vm_CopyOutInband";
		command = VM_DO_COPY_OUT_INBAND;
	    } else {
		operation = "Vm_CopyOut";
		command = VM_DO_COPY_OUT;
	    }
	} else {
	    if (inband) {
		operation = "Vm_CopyInInband";
		command = VM_DO_COPY_IN_INBAND;
	    } else {
		operation = "Vm_CopyIn";
		command = VM_DO_COPY_IN;
	    }
	}
    }

    for (sizeIndex = 0; sizeIndex < numSizes; ++sizeIndex) {
	totalTime = time_ZeroSeconds;
	gettimeofday((struct timeval *)&startTime,0);	
	for (i = 0; i < iterations; i++) {
#ifdef DEBUG
	    if (command == VM_DO_COPY_OUT
		|| command == VM_DO_COPY_OUT_INBAND) {
		bzero(buf, VM_DO_COPY_MAX_SIZE);
	    }
#endif
	    status = Vm_Cmd(command, copySize[sizeIndex], buf);
	    if (status != SUCCESS) {
		fprintf(stderr, "Can't copy %d bytes: %s\n",
			copySize[sizeIndex], Stat_GetMsg(status));
		exit(1);
	    }
#ifdef DEBUG
	    if (command == VM_DO_COPY_OUT
		|| command == VM_DO_COPY_OUT_INBAND) {
		CheckBuf(copySize[sizeIndex]);
	    }
#endif
	    if (spoilCOW) {
		WriteToBuffer(VM_DO_COPY_MAX_SIZE, buf);
	    }
	}
	gettimeofday((struct timeval *)&endTime,0);
	Time_Subtract(endTime, startTime, &totalTime);
	printf("Using %s: copy %4d bytes %d times: %2d.%03d sec\n",
	       operation, copySize[sizeIndex], iterations,
	       totalTime.seconds, totalTime.microseconds/1000);
    }
}

void
WriteToBuffer(length, buffer)
    int length;			/* bytes in buffer */
    char *buffer;		/* buffer to write to */
{
    char *cPtr;

    for (cPtr = buffer; cPtr < buffer + length; cPtr += 1024) {
	*cPtr = 'a';
    }
}

#ifdef DEBUG
/* 
 * Fill the buffer with a character.
 */
#define TEST_CHAR	'y'
void
InitBuf()
{
    char *bufPtr;

    for (bufPtr = buf; bufPtr < buf + VM_DO_COPY_MAX_SIZE; ++bufPtr) {
	*bufPtr = TEST_CHAR;
    }
}

/* 
 * Verify that the buffer contains the character from InitBuf.
 */
void
CheckBuf(numToCheck)
    int numToCheck;		/* number of characters to check */
{
    char *bufPtr;

    for (bufPtr = buf; bufPtr < buf + numToCheck; ++bufPtr) {
	if (*bufPtr != TEST_CHAR) {
	    fprintf(stderr, "CheckBuf: found \\%o at offset %d\n",
		    *bufPtr, bufPtr - buf);
	    exit(1);
	}
    }
}
#endif /* DEBUG */
