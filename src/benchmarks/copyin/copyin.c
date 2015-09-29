/* Benchmark to test copyin/copyout performance. */
/* $Header: /sprite/src/benchmarks/copyin/RCS/copyin.c,v 1.3 92/06/07 18:10:48 kupfer Exp $ */

#include <sprite.h>
#include <option.h>
#include <stdio.h>
#include <spriteTime.h>
#include <status.h>
#include <sys/time.h>
#include <unistd.h>
#include <vmTypes.h>

#define BUFFER_SIZE	8192	/* see Vm_Cmd; XXX should go into VM header 
				 * file */ 

char *buf;			/* Buffer to copy in from. */

int copySize[] = {0, 10, 100, 1024, 4096, 8192}; /* various amounts to copy */
int numSizes = sizeof(copySize) / sizeof(int);

int iterations = 10000;		/* number of iterations for each copy size */

int spoilCOW = 0;		/* write to the buffer to prevent COW from 
				 * giving optimistic results */
int useMakeAccessible = 0;	/* use Vm_MakeAccessible & bcopy instead of 
				 * copyin/out */ 
int copyout = 0;		/* copy kernel->user instead of vice versa */

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
};
int	numOptions = Opt_Number(optionArray);

void WriteToBuffer();

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
    buf = valloc(BUFFER_SIZE);
    if (buf == NULL) {
	fprintf(stderr, "copyin: no memory.\n");
	exit(1);
    }
    if (useMakeAccessible) {
	if (copyout) {
	    operation = "MakeAccessible (out)";
	    command = VM_DO_MAKE_ACCESS_OUT;
	} else {
	    operation = "MakeAccessible (in)";
	    command = VM_DO_MAKE_ACCESS_IN;
	}
    } else {
	if (copyout) {
	    operation = "Vm_CopyOut";
	    command = VM_DO_COPY_OUT;
	} else {
	    operation = "Vm_CopyIn";
	    command = VM_DO_COPY_IN;
	}
    }

    for (sizeIndex = 0; sizeIndex < numSizes; ++sizeIndex) {
	status = Vm_Cmd(VM_SET_COPY_SIZE, copySize[sizeIndex]);
	if (status != SUCCESS) {
	    fprintf(stderr, "Can't set copy size to %d: %s\n",
		    copySize[sizeIndex], Stat_GetMsg(status));
	    exit(1);
	}
	totalTime = time_ZeroSeconds;
	gettimeofday((struct timeval *)&startTime,0);	
	for (i = 0; i < iterations; i++) {
	    status = Vm_Cmd(command, buf);
	    if (status != SUCCESS) {
		fprintf(stderr, "Can't copy %d bytes: %s\n",
			copySize[sizeIndex], Stat_GetMsg(status));
		exit(1);
	    }
	    if (spoilCOW) {
		WriteToBuffer(BUFFER_SIZE, buf);
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
