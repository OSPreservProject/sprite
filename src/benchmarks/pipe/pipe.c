#include "sprite.h"
#include "option.h"
#include "fs.h"
#include "time.h"
#include "sysStats.h"
#include "kernel/sched.h"
#include "io.h"

#ifdef UNIX_SYTLE
#include <sys/file.h>
extern	int errno;
#endif

int	numIterations = 1000;
int	size = 1;
Boolean devNull = FALSE;
Boolean copyTest = FALSE;
Boolean getTime = FALSE;
Boolean getPid = FALSE;
Boolean nonBlockingTest = FALSE;

Option optionArray[] = {
    {OPT_INT, 'n', (Address)&numIterations, 
     "Number of iterations (Default 1000)."},
    {OPT_INT, 'd', (Address)&size,
     "Size of each read/write."},
    {OPT_TRUE, 'Z', (Address)&devNull,
     "Use /dev/null instead of pipes"},
    {OPT_TRUE, 'C', (Address)&copyTest,
     "Measure cost of Byte_Copy"},
    {OPT_TRUE, 'T', (Address)&getTime,
     "Measure cost of Sys_GetTimeOfDay"},
    {OPT_TRUE, 'P', (Address)&getPid,
     "Measure cost of Proc_GetIDs"},
    {OPT_TRUE, 'B', (Address)&nonBlockingTest,
     "Test non-blocking writes"},
};
int	numOptions = Opt_Number(optionArray);
int	i = 0;
Sched_Instrument startSchedStats, endSchedStats;
char buffer[4096];

main(argc, argv)
    int	 argc;
    char *argv[];
{
    register	int	numTimes;
    Time		before, after;
    int			parIn, parOut, childIn, childOut;
    int			count;
    int			pid;

    (void)Opt_Parse(&argc, argv, numOptions, optionArray);
    numTimes = numIterations;
    if (devNull) {
	/* 
	 * Measure the cost of N read & write calls on "/dev/null"
	 */
	Fs_Open("/dev/null", FS_READ, 0, &childOut);
	Fs_Open("/dev/null", FS_WRITE, 0, &childIn);
	Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
	Sys_GetTimeOfDay(&before, NULL, NULL);
	while (numTimes > 0) {
	    Fs_Read(childIn, size, buffer, &count);
	    Fs_Write(childOut, size, buffer, &count);
	    numTimes--;
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
	Time_Subtract(after, before, &after);
	printf("%d read-writes of /dev/null at %dus each\n", numIterations,
	    (after.seconds * 1000000 + after.microseconds) / numIterations);
	PrintIdleTime(io_StdErr, &startSchedStats, &endSchedStats, &after);
    } else if (copyTest) {
	/* 
	 * Measure the cost of N read & write calls on "/dev/null"
	 */
	char dest[5000];
	Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
	Sys_GetTimeOfDay(&before, NULL, NULL);
	while (numTimes > 0) {
	    Byte_Copy(size, buffer, dest);
	    numTimes--;
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
	Time_Subtract(after, before, &after);
	printf("%d Byte_Copy of %d bytes at %dus each\n", numIterations, size,
	    (after.seconds * 1000000 + after.microseconds) / numIterations);
	PrintIdleTime(io_StdErr, &startSchedStats, &endSchedStats, &after);
    } else if (getTime) {
	/*
	 * Measure the cost of N calls to Sys_GetTimeOfDay
	 */
	Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
	Sys_GetTimeOfDay(&before, NULL, NULL);
	while (numTimes > 0) {
	    Sys_GetTimeOfDay(&after, NULL, NULL);
	    numTimes--;
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
	Time_Subtract(after, before, &after);
	printf("%d Sys_GetTimeOfDay at %dus each\n", numIterations,
	    (after.seconds * 1000000 + after.microseconds) / numIterations);
	PrintIdleTime(io_StdErr, &startSchedStats, &endSchedStats, &after);
    } else if (getPid) {
	/*
	 * Measure the cost of N calls to Proc_GetIDs
	 */
	int pid;

	Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
	Sys_GetTimeOfDay(&before, NULL, NULL);
	while (numTimes > 0) {
	    Proc_GetIDs(&pid, NULL, NULL, NULL);
	    numTimes--;
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
	Time_Subtract(after, before, &after);
	printf("%d Proc_GetIDs at %dus each\n", numIterations,
	    (after.seconds * 1000000 + after.microseconds) / numIterations);
	PrintIdleTime(io_StdErr, &startSchedStats, &endSchedStats, &after);
    } else if (nonBlockingTest) {
	/*
	 * Verify the behavior of non-blocking writes.
	 */
	char buffer[5000];
#ifdef UNIX_STYLE 
	int id[2];
	int count;

	pipe(id);
	fcntl(id[1], F_SETFL, FNDELAY);
	count = write(id[1], buffer, 5000);
	if (count < 0) {
	    printf("Error %d\n", errno);
	} else {
	    printf("Transferred %d bytes\n", count);
	}
#else
	int readID, writeID, count;
	ReturnStatus status;

	Fs_CreatePipe(&readID, &writeID);
	Ioc_SetBits(writeID, IOC_NON_BLOCKING);
	status = Fs_Write(writeID, sizeof(buffer), buffer, &count);
	Io_Print("Transferred %d bytes\n", count);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Fs_Write: ");
	}
#endif
    } else {
	/*
	 * Measure the cost of N exchanges of D bytes via pipes.
	 */
	Fs_CreatePipe(&parIn, &childOut);
	Fs_CreatePipe(&childIn, &parOut);
	if (Proc_Fork(TRUE, &pid) == PROC_CHILD_PROC) {
	    while (numTimes > 0) {
		Fs_Read(childIn, size, buffer, &count);
		Fs_Write(childOut, size, buffer, &count);
		numTimes--;
	    }
	} else {
	    Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
	    Sys_GetTimeOfDay(&before, NULL, NULL);
	    while (numTimes > 0) {
		Fs_Write(parOut, size, buffer, &count);
		Fs_Read(parIn, size, buffer, &count);
		numTimes--;
	    }
	    Sys_GetTimeOfDay(&after, NULL, NULL);
	    Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
	    Time_Subtract(after, before, &after);
	    printf("%d read-writes at %dus each\n", numIterations,
		(after.seconds * 1000000 + after.microseconds) / numIterations);
	    PrintIdleTime(io_StdErr, &startSchedStats, &endSchedStats, &after);
	}
    }
}
