/*
 * loadAvg.h --
 *
 *	Declarations internal to the loadavg program.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /a/newcmds/loadavg/RCS/loadavg.h,v 1.7 89/07/31 17:51:57 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _LOADAVG
#define _LOADAVG

#include <time.h>
#include <db.h>
#include <mig.h>
#include <stdio.h>
#include <stdlib.h>

#include <proc.h>
#include <kernel/net.h>

/*
 * Subscripts into the queueThreshold array.
 */
#define MIN_THRESHOLD 0
#define MAX_THRESHOLD 1

/*
 * Arbitrary value larger than the load average on any node (we hope!)
 */
#define MAX_LOAD 1000.0

/*
 * Buffer lengths.
 */
#define HOST_NAME_SIZE 64
#define MAX_PATH_NAME_LENGTH 1024


/*
 * For each machine, keep track of the timestamp for its information and the
 * different load averages reported.   Also, the architecture type
 * (e.g., sun2/sun3/spur) is stored to make sure we can migrate to a machine
 * of the same type.
 */

/*
 * If we are not allowing foreign processes, but our time since last input
 * is greater than noInput and our average queue lengths are ALL less than
 * the corresponding values in min, start accepting foreign processes.
 *
 * If we are allowing foreign processes and either the idle time drops
 * or ANY of the average queue lengths exceeds its corresponding value in max,
 * stop accepting them.
 */

typedef struct {
    int		noInput;
    double	min[MIG_NUM_LOAD_VALUES];
    double	max[MIG_NUM_LOAD_VALUES];
} Thresholds;

typedef enum {
    EVICT,
    CHECK_COUNT
} FindForeignParam;

/*
 * Global variables.  (Options, plus other global vars initialized at startup.)
 */

extern int debug;
extern int verbose;
extern int loadInterval;
extern int writeInterval;
extern int timeOut;
extern int neverAccept;
extern int alwaysAccept;
extern int migVersion;
extern int numLowPris;
extern int useKernelIdleTime;
extern char *dataFile;
extern char *weightString;
extern double weights[];
extern Thresholds thresholds;

extern int errno;

extern int hostID;
extern char *myName;
extern char hostname[];
extern char *machType;
extern int kernelState;
#ifdef INTERIM
extern int oldKernel;
#endif /* INTERIM */

/*
 * Procedures.
 */

extern void RunDaemon();
extern int FindForeign();

extern char *malloc();
extern char *strcpy();
extern char *strerror();
#endif _LOADAVG

