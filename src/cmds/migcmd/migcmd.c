/*
 * migCmd.c --
 *
 *	Program to manipulate process migration characteristics and
 *	perform other tasks related to migration.
 *
 * Copyright 1988, 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/migcmd/RCS/migcmd.c,v 1.7 90/09/24 14:39:06 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <sysStats.h>
#include <stdio.h>
#include <stdlib.h>
#include <option.h>
#include <proc.h>
#include <spriteTime.h>
#include <kernel/sysSysCall.h>
#include <kernel/trace.h>
#include <kernel/procMigrate.h>
#include <host.h>

#include "syscalls.h"

/*
 * Variables for options.
 */

int allowMigration = 0;
int refuseMigration = 0;
int getStatus = 0;
int getMigStats = 0;
int resetStats = 0;
int input = -1;
int load = -1;
char *importStr = NULL;
char *exportStr = NULL;
int dumpTrace = 0;
int startTrace = 0;
int stopTrace = 0;
int debugLevel = -1;
int newVersion = -1;
int numRecords = 200;

Option optionArray[] = {
    {OPT_TRUE, "s", (Address)&getStatus,
     "Print migration version, and conditions under which process migration is allowed."},
    {OPT_TRUE, "S", (Address)&getMigStats,
     "Print migration statistics."},
    {OPT_TRUE, "Z", (Address)&resetStats,
     "Reset migration statistics."},
    {OPT_TRUE, "a", (Address)&allowMigration,
     "Allow all future migrations to this machine (must be root)."},
    {OPT_TRUE, "r", (Address)&refuseMigration,
     "Disallow all future migrations to this machine (must be root)."},
    {OPT_INT, "V", (Address)&newVersion,
     "New migration version to set kernel version to."},
    {OPT_INT, "i", (Address)&input,
     "Whether to ignore idle time for allowing migrations to this machine."},
    {OPT_INT, "l", (Address)&load,
     "Whether to ignore load average for allowing migrations to this machine."},
    {OPT_STRING, "I", (Address)&importStr,
     "what users to allow to import (none, all, root)."},
    {OPT_STRING, "E", (Address)&exportStr,
     "what users to allow to export (none, all, root)."},
    {OPT_TRUE, "p", (Address)&dumpTrace,
     "Print process migration trace records."},
    {OPT_INT, "P", (Address)&numRecords,
     "Number of process migration records to print."},
    {OPT_TRUE, "t", (Address)&startTrace,
     "Enable tracing of process migration."},
    {OPT_TRUE, "T", (Address)&stopTrace,
     "Disable tracing of process migration."},
    {OPT_INT, "d", (Address)&debugLevel,
     "Level to set proc_MigDebugLevel."},
};
/*
 * Constants used by tracing routines:
 * 	PROC_NUM_EVENTS - the number of valid trace events for proc.
 */

#define PROC_NUM_EVENTS 5

/*
 * For imports and exports, we either allow no one to migrate, just root,
 * or everyone.
 */
#define ALLOW_NONE 0
#define ALLOW_ROOT 1
#define ALLOW_ALL 2

#define CheckBool(num) if((num) != 0 && (num) != 1) { \
			        Opt_PrintUsage(argv[0], optionArray, \
					       Opt_Number(optionArray)); \
			       }

char *myName;
void PrintState();

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Driver.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */


main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status = SUCCESS;
    char    	  version[128];
    int Status;
    int changingState = 0;
    int import = -1;
    int export = -1;
    int curVersion;

    (void) Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray),
		     OPT_ALLOW_CLUSTERING);

    myName = argv[0];
    
    if (importStr != NULL) {
	import = CheckString(importStr);
	changingState = 1;
    }
    if (exportStr != NULL) {
	export = CheckString(exportStr);
	changingState = 1;
    }
    if (input != -1) {
	CheckBool(input);
	changingState = 1;
    }
    if (load != -1) {
	CheckBool(load);
	changingState = 1;
    }
			 
    if (allowMigration || refuseMigration) {
	changingState = 1;
    }
	

    if (debugLevel != -1) {
	status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_SET_DEBUG,
			   (Address) &debugLevel);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Sys_Stats (set debug level)");
	    exit(status);
	}
    } 

    if (changingState || (newVersion != -1) || getStatus) {
	status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_STATE,
			   (Address) &Status);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Sys_Stats (getting migration state)");
	    exit(status);
	}
	status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_VERSION,
			   (Address) &curVersion);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Sys_Stats (getting migration version)");
	    exit(status);
	}
	printf("\t\tImport\tExport\tVersion\tIgnore\n");
	PrintState(Status, curVersion, "Current:");

	if (changingState) {
	    if (allowMigration) {
		Status |= PROC_MIG_IMPORT_ALL;
	    } else if (refuseMigration) {
		Status &= ~PROC_MIG_IMPORT_ALL;
	    } else if (import != -1) {
		if (import == ALLOW_ALL) {
		    Status |= PROC_MIG_IMPORT_ALL;
		} else if (import == ALLOW_NONE) {
		    Status &= ~PROC_MIG_IMPORT_ALL;
		} else if (import == ALLOW_ROOT) {
		    Status &= ~PROC_MIG_IMPORT_ALL;
		    Status |= PROC_MIG_IMPORT_ROOT;
		}
	    }
	    if (export != -1) {
		if (export == ALLOW_ALL) {
		    Status |= PROC_MIG_EXPORT_ALL;
		} else if (export == ALLOW_NONE) {
		    Status &= ~PROC_MIG_EXPORT_ALL;
		} else if (export == ALLOW_ROOT) {
		    Status &= ~PROC_MIG_EXPORT_ALL;
		    Status |= PROC_MIG_EXPORT_ROOT;
		}
	    }
	    if (input == 0) {
		Status &= ~PROC_MIG_IMPORT_ANYINPUT;
	    } else if (input == 1) {
		Status |= PROC_MIG_IMPORT_ANYINPUT;
	    }
	    if (load == 0) {
		Status &= ~PROC_MIG_IMPORT_ANYLOAD;
	    } else if (load == 1) {
		Status |= PROC_MIG_IMPORT_ANYLOAD;
	    }
		
	    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_SET_STATE,
			       (Address) &Status);
	    if (status != SUCCESS) {
		Stat_PrintMsg(status, "Sys_Stats (setting migration state)");
		exit(status);
	    }
	}
	if (newVersion != -1) {
	    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_SET_VERSION,
			       (Address) &newVersion);
	    if (status != SUCCESS) {
		Stat_PrintMsg(status, "Sys_Stats (setting migration version)");
		exit(status);
	    }
	}
	if (changingState || (newVersion != -1)) {
	    PrintState(Status, (newVersion != -1) ? newVersion : curVersion,
		       "New:\t");
	}
    }
    if (startTrace) {
	status = Sys_Stats(SYS_PROC_TRACE_STATS, SYS_PROC_TRACING_ON,
			(Address) NULL);
	if (status != SUCCESS) {
	    (void) fprintf(stderr, "Error %x returned from Test_Stats.\n",
			   status);
	    Stat_PrintMsg(status, "");
	    exit(status);
	}
    }

    if (stopTrace) {
	status = Sys_Stats(SYS_PROC_TRACE_STATS, SYS_PROC_TRACING_OFF,
			(Address) NULL);
	if (status != SUCCESS) {
	    (void) fprintf(stderr, "Error %x returned from Test_Stats.\n",
			   status);
	    Stat_PrintMsg(status, "");
	    exit(status);
	}
    }

    if (dumpTrace) {
	status = PrintMigration();
    }

    if (resetStats) {
	status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_RESET_STATS,
			(Address) NULL);
	if (status != SUCCESS) {
	    (void) fprintf(stderr,
			   "Error %x returned from Sys_Stats resetting statistics.\n",
			   status);
	    Stat_PrintMsg(status, "");
	    exit(status);
	}
    }

    if (getMigStats) {
	status = PrintStats();
    }


    exit(status);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintMigration --
 *
 *	Print the most recent process migration trace records.
 *
 * Results:
 *	The return status from Sys_Stats is returned.
 *
 * Side effects:
 *	Trace records are written to stdout.
 *
 *----------------------------------------------------------------------
 */


int
PrintMigration()
{
    int index;		/* index of current table entry */
    Time baseTime, deltaTime, startTime;	/* Times for print out */
    Address buffer;	/* Buffer for trace records */
    int numRecs;		/* number of records actually copied */
    Trace_Record *traceArray;
    Proc_TraceRecord *procTraceArray;
    register Trace_Record *tracePtr;
    register Proc_TraceRecord *procTracePtr;
    int status;
    static char *flagsArray[] = {"RE", "RS", "HE", "HS", "  "};
    static char *eventArray[] = {"start", "end", "xfer", "call", "migtrap"};
    static char *commandArray[] = {"proc", "vm", "files", "stream", "user",
				 "resume"};
    /*
     * Get a copy of the trace table.
     */

    buffer = malloc((unsigned) (sizeof(int) + numRecords *
				(sizeof(Trace_Record) +
				 sizeof(Proc_TraceRecord))));
    status = Sys_Stats(SYS_PROC_TRACE_STATS, numRecords, buffer);
    if (status != SUCCESS) {
	(void) fprintf(stderr, "Error from Sys_Stats.\n");
	Stat_PrintMsg(status, "");
	return(status);
    }

    numRecs = * ((int *) buffer);
    buffer += sizeof(int);
    (void) fprintf(stderr, "Number of records is %d.\n", numRecs);
    (void) fflush(stderr);
    if (numRecs == 0) {
	return(0);
    }

    traceArray = (Trace_Record *) buffer;
    procTraceArray = (Proc_TraceRecord *) (buffer + numRecs *
					 sizeof(Trace_Record));


    (void) printf("\n");
#define PRINT_MIGHEADER() \
    (void) printf("%10s %10s %10s %2s %10s %24s %7s\n", \
	"Time", "Delta", "ProcessID", "HR", "Event", "Call", "Sta")
    PRINT_MIGHEADER();

    baseTime = traceArray[0].time;
    startTime = traceArray[0].time;

    for (index = 0; index < numRecs; index++) {
	tracePtr = &traceArray[index];
	procTracePtr = &procTraceArray[index];

	Time_Subtract(tracePtr->time, startTime, &deltaTime);
	(void) printf(" %3d.%06d",
			   deltaTime.seconds,
			   deltaTime.microseconds);
	Time_Subtract(tracePtr->time, baseTime, &deltaTime);
	(void) printf(" %3d.%06d",
			   deltaTime.seconds,
			   deltaTime.microseconds);
	baseTime = tracePtr->time;

	if (tracePtr->flags & TRACE_DATA_INVALID) {
	    procTracePtr->flags = 4;
	    procTracePtr->processID = (Proc_PID) NULL;
	}
	if (((unsigned) procTracePtr->flags) > 3 ||
	    ((unsigned) tracePtr->event)  >= PROC_NUM_EVENTS) {
		(void) fprintf(stderr,
			       "Entry %d: invalid flags (%d) or event (%d).\n",
			       index, procTracePtr->flags, tracePtr->event);
		return(-1);
	    }

	(void) printf("%10x %3s %10s", procTracePtr->processID,
		      flagsArray[procTracePtr->flags],
		      eventArray[tracePtr->event]);

	if (tracePtr->event == PROC_MIGTRACE_COMMAND) {
	    (void) printf(" %-10s",
		     commandArray[(int) procTracePtr->info.command.type]);
	    if (procTracePtr->info.command.data != (ClientData) NIL) {
		(void) printf(" %20d",
			       procTracePtr->info.command.data);
	    }
	} else if (tracePtr->event == PROC_MIGTRACE_CALL) {
	    (void) printf(" %24s",
		     sysCallArray[(int) procTracePtr->info.call.callNumber].name);
	    if (!(procTracePtr->flags & PROC_MIGTRACE_START)) {
		(void) printf(" %10x",
			       procTracePtr->info.call.status);
	    }
	}

	(void) printf("\n");
    }
    PRINT_MIGHEADER();
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintState --
 *
 *	Print the process migration state defined by the status word in
 *	the kernel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */

void
PrintState(state, version, prelude)
    int state;
    int version;
    char *prelude;
{
    int tmp;
    char *import;
    char *export;
    int extra = 0;

    tmp = state & PROC_MIG_IMPORT_ALL;
    if (tmp == 0) {
	import = "none";
    } else if (tmp == PROC_MIG_IMPORT_ROOT) {
	import = "root";
    } else{
	import = "all";
    }
    tmp = state & PROC_MIG_EXPORT_ALL;
    if (tmp == 0) {
	export = "none";
    } else if (tmp == PROC_MIG_EXPORT_ROOT) {
	export = "root";
    } else{
	export = "all";
    }
	
    printf("%s\t%4s\t%4s\t%4d\t", prelude, import, export, version);
    if (state & PROC_MIG_IMPORT_ANYLOAD) {
	printf("load");
	extra = 1;
    }
    if (state & PROC_MIG_IMPORT_ANYINPUT) {
	if (extra) {
	    printf(", ");
	}
	printf("input");
    }
    printf("\n");
}




/*
 *----------------------------------------------------------------------
 *
 * CheckString --
 *
 *	Verify that a string conforms to one of the allowable options
 *	and return that option.
 *
 * Results:
 *	The constant corresponding to the string is returned.
 *
 * Side effects:
 *	If the string is not allowable, an error message is printed and
 *	the process exits.
 *
 *----------------------------------------------------------------------
 */

int
CheckString(string)
    char *string;
{
    if(!strcmp(string, "all")) {
	return(ALLOW_ALL);
    }
    if(!strcmp(string, "root")) {
	return(ALLOW_ROOT);
    }
    if(!strcmp(string, "none")) {
	return(ALLOW_NONE);
    }
    Opt_PrintUsage(myName, optionArray, Opt_Number(optionArray));
    exit(1);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintStats --
 *
 *	Print migration statistics.
 *
 * Results:
 *	The return status from Sys_Stats is returned.
 *
 * Side effects:
 *	Statistics are written to stdout.
 *
 *----------------------------------------------------------------------
 */


int
PrintStats()
{
    Proc_MigStats stats;		/* statistics */
    int status;
    Time avgTime;
    double time;
    double time2;
    int i;
    Host_Entry *hostPtr;

    /*
     * Get a copy of the statistics record.  Make sure it's zeroed in case the
     * kernel provides us with a shorter (older) structure.
     */

    bzero((Address) &stats, sizeof(stats));
    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_STATS,
		       (Address) &stats);
    if (status != SUCCESS) {
	(void) fprintf(stderr, "Error from Sys_Stats.\n");
	Stat_PrintMsg(status, "");
	return(status);
    }

    if (stats.statsVersion != PROC_MIG_STATS_VERSION) {
	(void) fprintf(stderr, "Different versions of statistics buffer.\n");
	return(FAILURE);
    }

#define PRINT(string, val) printf("%40s\t%d\n", string, stats.val)

    PRINT("Number of foreign processes", foreign);
    PRINT("Number of remote processes", remote);
    PRINT("Number of exports", exports);
    PRINT("Number of execs", execs);
    PRINT("Number of imports", imports);
    PRINT("Number of errors", errors);
    PRINT("Number of eviction requests", evictCalls);
    PRINT("Number resulting in evictions", evictsNeeded);
    PRINT("Number of evicted processes", varStats.evictions);
    PRINT("Number voluntary migrations home", migrationsHome);
    PRINT("Number of returns to us", returns);
    PRINT("Number of evictions back to us", evictionsToUs);
    if (stats.evictionsToUs > 0) {
	time = stats.varStats.evictionCPUTime / stats.evictionsToUs;
    } else {
	time = 0;
    }
    printf("%40s\t%7.3f\n", "Average time used after eviction", time);

    PRINT("Number of pages written", varStats.pagesWritten);
    if (stats.exports - stats.execs > 0) {
	time = stats.varStats.timeToMigrate /
	    (stats.exports + stats.varStats.evictions - stats.execs);
    } else {
	time = 0;
    }
    printf("%40s\t%7.3f\n", "Average time to export", time);
    if (stats.execs > 0) {
	time = stats.varStats.timeToExec / stats.execs;
    } else {
	time = 0;
    }
    printf("%40s\t%7.3f\n", "Average time to exec", time);

    if (stats.varStats.evictions > 0) {
	time = stats.varStats.timeToEvict / stats.varStats.evictions;
    } else {
	time = 0;
    }
    printf("%40s\t%7.3f\n", "Average time to evict 1 process", time);

    if (stats.evictsNeeded > 0) {
	time = stats.varStats.totalEvictTime / stats.evictsNeeded;
    } else {
	time = 0;
    }
    printf("%40s\t%7.3f\n", "Average time to complete eviction", time);


    printf("%40s\t%d\n", "Average Kbytes/migration",
	   ((stats.exports + stats.varStats.evictions) > 0) ? 
	   stats.varStats.rpcKbytes / (stats.exports + stats.varStats.evictions) :
	   0);
    printf("%40s\t%d\n", "Average Kbytes/migration",
	   ((stats.exports + stats.varStats.evictions) > 0) ? 
	   stats.varStats.rpcKbytes / (stats.exports + stats.varStats.evictions) :
	   0);
    printf("%40s\t%d/%d (%5.2f%%)\n", "Remote/Total CPU seconds",
	   stats.varStats.remoteCPUTime / 10, stats.varStats.totalCPUTime / 10,
	   ((double) stats.varStats.remoteCPUTime)/
	   stats.varStats.totalCPUTime * 100.0);
    printf("\n%40s\t%s\n", "Host", "count");
    Host_Start();
    while (1) {
	hostPtr = Host_Next();
	if (hostPtr == (Host_Entry *) NULL) {
	    break;
	}
	if (stats.hostCounts[hostPtr->id]) {
	    printf("%40s\t%d\n", hostPtr->name, stats.hostCounts[hostPtr->id]);
	}
    }
    Host_End();
    return(SUCCESS);
}
