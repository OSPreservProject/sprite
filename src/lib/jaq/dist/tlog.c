/* 
 * tlog.c--
 *
 *	Simple logging functions for Jaquith archive package.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 * 
 * Quote:
 *      The mass of men lead lives of quiet desperation.
 *      -- Thoreau
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/taputils.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

/* file globals */
static char printBuf[T_MAXSTRINGLEN];
static FILE *activeLog = NULL;
static int   activeLogDetail = LOG_FAIL;


/*
 *----------------------------------------------------------------------
 *
 * Log_SetLevel
 *
 *	Specify which events will appear in logfile.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Sets global variable activeLogDetail.
 *
 * Presently, the logging stuff is set up as levels of 
 * increased detail, but some day bits may divide the logging
 * into types, controlled by a bit vector. 
 *
 *----------------------------------------------------------------------
 */

void
Log_SetDetail(detail)
    int detail;               /* level of logging detail */
{

    sprintf(printBuf, "Setting detail level to %d\n", detail);
    Log_Event("Log_SetDetail", printBuf, LOG_MAJOR);

    activeLogDetail = detail;

}



/*
 *----------------------------------------------------------------------
 *
 * Log_Open
 *
 *	Open a logging file.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Consumes a file descriptor.
 *
 *----------------------------------------------------------------------
 */

int
Log_Open(logName)
    char *logName;            /* name of logging file */
{
    char *pathName;

    if ((logName == NULL) || (*logName == '\0')) {
	Utils_Bailout("LogOpen: got null ptr\n", BAIL_PRINT);
    }
    
    if (activeLog != NULL) {
	Log_Event("Log_Open", "Closing logfile\n", LOG_MAJOR);
	Log_Close();
    }
	
    pathName = Utils_MakeFullPath(logName);
    activeLog = fopen(pathName, "a");
    if (activeLog != NULL) {
	sprintf(printBuf, "Opening log file %s\n", pathName);
	Log_Event("LogOpen", printBuf, LOG_MAJOR);
    }
    MEM_FREE("Log_Open", pathName);

    return(errno);
    
}


/*
 *----------------------------------------------------------------------
 *
 * Log_Close
 *
 *	Closes the active logging file
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Frees a file descriptor.
 *
 *----------------------------------------------------------------------
 */

int
Log_Close()
{

    if (activeLog != NULL) {
	Log_Event("LogClose", "Closing log", LOG_MAJOR);
	fclose(activeLog);
	activeLog = NULL;
    }

    return(errno);

}


/*
 *----------------------------------------------------------------------
 *
 * Log_Event
 *
 *	Record interesting events in the log file
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Puts output into logfile
 *
 *----------------------------------------------------------------------
 */

int
Log_Event(src, msg, detail)
    char *src;                /* name of caller */
    char *msg;                /* log message */
    int detail;               /* importance of message */
{
    time_t currentTime;

    if ((activeLog == NULL) || (detail > activeLogDetail)) {
	return(T_FAILURE);
    }

    currentTime = Time_GetCurDate();

    fprintf(activeLog, "%s %s: %s", 
	    Time_CvtToString(&currentTime), src, msg);
    if (msg[strlen(msg)-1] != '\n') {
	fputc('\n', activeLog);
    }
    fflush(activeLog);
    return(0);

}

/*
 *----------------------------------------------------------------------
 *
 * Log_AtomicEvent
 *
 *	Record interesting events in a shared log file
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Puts output into logfile
 *
 *----------------------------------------------------------------------
 */

int
Log_AtomicEvent(src, msg, logName)
    char *src;                /* name of caller */
    char *msg;                /* log message */
    char *logName;            /* name of shared log file */
{
    time_t currentTime;
    FILE *logStream;
    int pid = getpid();
    Lock_Handle lockHandle;

    currentTime = Time_GetCurDate();

    if (Lock_Acquire(logName, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    }

    if ((logStream=fopen(logName, "a")) != (FILE *)NULL) {
	fprintf(logStream, "%s %s (0x%x): %s",
		Time_CvtToString(&currentTime), src, pid, msg);
	if (msg[strlen(msg)-1] != '\n') {
	    fputc('\n', logStream);
	}
	fclose(logStream);
    }

    Lock_Release(&lockHandle);

    return T_SUCCESS;

}
