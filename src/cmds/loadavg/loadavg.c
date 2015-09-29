/*
 * loadavg.c --
 *
 *	Provide access to the load average information database.  
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/loadavg/RCS/loadavg.c,v 1.13 90/09/24 14:37:42 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <mig.h>
#include <kernel/net.h>
#include <option.h>
#include <host.h>
#include <syslog.h>
#include <sys/time.h>
#include <sysStats.h>

/*
 * Define constants for defaults values of variables that may be assigned
 * by command-line arguments.
 */

int allHosts = 0;
int getLoad = 0;
int debug = 0;
int zapHostInfo = 0;
int forceEvict = 0;
int stripDomain = 1;

Option optionArray[] = {
	{OPT_TRUE, "l", (char *)&getLoad,
	     "Get the load average of this node (DEFAULT)."},
	{OPT_TRUE, "a", (char *)&allHosts,
	     "Get the load average of all nodes."},
	{OPT_FALSE, "S", (char *)&stripDomain,
	     "Don't strip the domain from host names."},
	{OPT_TRUE, "E", (char *)&forceEvict, "Evict foreign processes immediately."},
	{OPT_TRUE, "D", (char *)&debug, "Enable debugging information."},
	{OPT_TRUE, "Z", (char *)&zapHostInfo,
		 "Zap (remove) the information for hosts specified as remaining arguments."},
};
static int numOptions = sizeof(optionArray) / sizeof(Option);

/*
 * Global variables.
 */
struct timeval currentTime;
int hostID;
#define HOST_NAME_LENGTH 64
char hostname[HOST_NAME_LENGTH];
char *machType;
char *myName = NULL;
int migVersion;
int kernelState;


static void ListHosts();
static void OutputStatus();
static void ZapHostInfo();
extern char *rindex();


/*
 *----------------------------------------------------------------------
 *
 * Main --
 *
 *	The driver for the loadavg program.  Parse options and invoke
 *	subroutines as appropriate.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes global variables.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int status;
    int numScanned;
    int i;
    int host;
    Mig_Info *infoPtr;
    Host_Entry *hostPtr;
    

    argc = Opt_Parse(argc, argv, optionArray, numOptions,
		       OPT_ALLOW_CLUSTERING);

    myName = rindex(argv[0], '/');
    if (myName){
	myName++;
    } else {
	myName = argv[0];
    }

    /*
     * Process interrelated defaults and perform sanity checking on
     * arguments.
     */

    /*
     * Default operation is to get the local load.
     */

    if (! (getLoad || allHosts || zapHostInfo || forceEvict)) {
	if (!strcmp(myName, "rup")) {
	    allHosts = 1;
	} else if (!strcmp(myName, "evict")) {
	    forceEvict = 1;
	} else {
	    getLoad = 1;
	}
    }

    /*
     * Initialize global variables.
     */
    if (gethostname(hostname, sizeof(hostname)) < 0) {
	perror("gethostname");
	exit(1);
    }
    hostPtr = Host_ByName(hostname);
    if (hostPtr == (Host_Entry *) NULL) {
	fprintf(stderr, "error getting host information for '%s'.\n",
	       myName);
	exit(1);
    }
    hostID = hostPtr->id;
    machType = malloc((unsigned) strlen(hostPtr->machType) + 1);
    if (machType == (char *) NULL) {
	panic("Out of memory");
    }
    (void) strcpy(machType, hostPtr->machType);
    
    /*
     * Get our migration version.
     */
    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_VERSION,
		       (Address) &migVersion);
    if (status != SUCCESS) {
	fprintf(stderr, "Error in Sys_Stats getting migration version: %s\n",
		Stat_GetMsg(status));
	exit(Compat_MapCode(status));
    }
    /*
     * Get the kernel's idea of what we're importing and exporting.
     */
    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_STATE,
		       (Address) &kernelState);
    if (status != SUCCESS) {
	(void) fprintf(stderr,
		       "%s: warning: error in Sys_Stats getting migration state.\n",
		       myName);
	kernelState = PROC_MIG_ALLOW_DEFAULT;
    }
    if (debug) {
        (void) fprintf(stderr,
		       "My hostID is %d.  machType is %s.  kernel state is 0x%x.\n",
		       hostID, machType, kernelState);
    }
    status = gettimeofday(&currentTime, (struct timezone *) NULL);
    if (status == -1) {
	perror("Error in gettimeofday");
	exit(1);
    }

    if (zapHostInfo) {
	for (i = 1; i < argc; i++) {
	    ZapHostInfo(argv[i]);
	}
    }
    if (allHosts) {
	ListHosts();
    } else if (getLoad) {
	infoPtr = Mig_GetInfo((int) PROC_MY_HOSTID);
	if (infoPtr == (Mig_Info *) NULL) {
	    perror("Error in Mig_GetInfo");
	    exit(1);
	}
	OutputStatus(infoPtr);
    }

    if (forceEvict) {
	int numEvicted = Mig_Evict();
	if (numEvicted < 0) {
	    perror("Mig_Evict");
	} else {
	    printf("Evicted %d foreign processes.\n", numEvicted);
	}
    }
    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * ListHosts --
 *
 *	Output the load average and status of all nodes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The data are is written to stdout.
 *
 *----------------------------------------------------------------------
 */

static void
ListHosts()
{
    int numRecs;
    register Mig_Info *infoPtr;
    Mig_Info *infoArray;
    int i;
    int maxSize;
    int hostID;
    int status;

    if (debug) {
	(void) fprintf(stderr, "ListHosts called.\n");
 	(void) fflush(stderr);
    }

    i = gettimeofday(&currentTime, (struct timezone *) NULL);
    if (i == -1) {
	perror("Error in gettimeofday");
	exit(1);
    }

    maxSize = NET_NUM_SPRITE_HOSTS * sizeof(Mig_Info);
    infoArray = (Mig_Info *) malloc((unsigned) maxSize);
    numRecs = Mig_GetAllInfo(infoArray, NET_NUM_SPRITE_HOSTS);
    if (numRecs < 0) {
	perror("Error in Mig_GetAllInfo");
	exit(1);
    }
    if (debug && numRecs == 0) {
	(void) fprintf(stderr, "No host records found.\n");
 	(void) fflush(stderr);
	return;
    }
	
    printf("            HOST     TYPE      STATUS       MIG     LOAD AVERAGE   IDLE TIME\n");

    for (i = 0; i < numRecs; i++) {
	OutputStatus(&infoArray[i]);
    }
    free((char *) infoArray);
}


/*
 *----------------------------------------------------------------------
 *
 * OutputStatus --
 *
 *	Determine the status of the given host and write a "ruptime"-like
 *	record to stdout.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*
 * Size of the buffer for the current host's name.
 */
#define HOST_NAME_SIZE 64

/*
 * Size of the buffer for reporting up- or down-time.
 */
#define TIME_STR_LEN 13

/*
 * For Time_ToAscii, if relative time is specified, the time value is
 * interpreted as the difference between two absolute time values and
 * has the form:
 *		0 days, 02:45:09
 *			     ^^^
 * SECOND_OFFSET is used to hide the seconds field by overwriting the colon
 * with a null character.
 */
#define SECOND_OFFSET 9

static void
OutputStatus(infoPtr)
    register Mig_Info *infoPtr;
{
    int state;
    char idleTime[TIME_STR_LEN];
    char *idlePtr;
    char timeStr[TIME_STR_LEN];
    char upStr[5];
    char stateStr[8];
    char migOK;
    char hostName[HOST_NAME_SIZE];
    register char *hp;
    register char *sp;
    register int i;
    int diff;
    Host_Entry *hostPtr;

    hostPtr = Host_ByID(infoPtr->hostID);
    if (hostPtr == (Host_Entry *) NULL) {
	syslog(LOG_WARNING, 
	       "error in Host_ByID(%d).\n", infoPtr->hostID);
	return;
    }
    /*
     * Copy the name of the host, up to HOST_NAME_SIZE or the first
     * '.' so we don't output the domain.
     */
    for (sp = hostName, hp = hostPtr->name, i = 0;
	 i < HOST_NAME_SIZE && *hp != '\0' && (!stripDomain || *hp != '.');
	 sp++, hp++, i++) {
	*sp = *hp;
    }
    *sp = '\0';
    

    /*
     * See if the entry is out of date.  (Assume that anything updated
     * after the current time is fairly recent and is caused by clock
     * skew, so negative diffs are okay.)
     */
    state = infoPtr->state;
    if (state == MIG_HOST_DOWN) {
	diff = currentTime.tv_sec - infoPtr->loadVec.timestamp;
    } else {
	/*
	 * We want to know how long the machine has been up.
	 */
	diff = infoPtr->loadVec.timestamp - infoPtr->bootTime;
	if (diff < 0) {
	    (void) fprintf(stderr,
			   "Change was made before boot time??  modTime %d, bootTime %d, host %s.\n",
			   infoPtr->loadVec.timestamp, infoPtr->bootTime,
			   hostPtr->name);
	    diff = 0;
	}
    }
	    

    Time_ToAscii(diff, 1, timeStr);
    timeStr[SECOND_OFFSET] = '\0';

    /*
     * We allow migration if it's the right version,
     * the file says to allow it, the entry in the
     * file is current, we're exporting, and it is the same machine type.
     * We're only concerned with normal priority processes, so we check
     * against the max count if the host is partially in use.
     */
    migOK = ' ';
    if (state != MIG_HOST_DOWN) {
	if (infoPtr->loadVec.allowMigration) {
	    if ((state == MIG_HOST_PART_USED) || (state == MIG_HOST_FULL)) {
		strcpy(stateStr, "hasmig");
	    } else {
		strcpy(stateStr, "avail");
	    }
	    if (!strcmp(hostPtr->machType, machType)) {
		if (infoPtr->migVersion != migVersion) {
		    strcpy(stateStr, "refuses");
		} else if (hostPtr->id != hostID &&
			   state != MIG_HOST_FULL &&
			   infoPtr->foreign[MIG_NORMAL_PRIORITY] <
			   infoPtr->maxProcs) {
		    migOK = '*';
		}
	    }
	} else if (infoPtr->state == MIG_HOST_REFUSES) {
	    strcpy(stateStr, "refuses");
	} else {
	    strcpy(stateStr, "inuse");
	}
	strcpy(upStr, "up");
    } else {
	strcpy(upStr, "down");
	stateStr[0] = '\0';
    }
    (void) fprintf(stdout,
		   "%16s%c  %6s %4s %s %7s",
		   hostName, migOK,
		   hostPtr->machType,
		   upStr,
		   timeStr,
		   stateStr);
    if (state != MIG_HOST_DOWN) {
	(void) fprintf(stdout,
		       " %5.2lf %5.2lf %5.2lf",
		       infoPtr->loadVec.lengths[0], infoPtr->loadVec.lengths[1],
		       infoPtr->loadVec.lengths[2]);
	if ((state != MIG_HOST_ACTIVE) || (infoPtr->loadVec.noInput > 60)) {
	    Time_ToAscii(infoPtr->loadVec.noInput, 1, idleTime);
	    idleTime[SECOND_OFFSET] = '\0';
	    for (idlePtr = idleTime; *idlePtr == ' '; idlePtr++) {
	    }
	    fprintf(stdout, " (%s)", idlePtr);
	}
    }
    (void) fprintf(stdout, "\n");
}



/*
 *----------------------------------------------------------------------
 *
 * ZapHostInfo --
 *
 *	Zero out the information for a host.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The global database is modified.
 *
 *----------------------------------------------------------------------
 */

static void
ZapHostInfo(hostname)
    char *hostname;
{
    ReturnStatus status;
    Mig_Info info;
    Host_Entry *hostPtr;
    

    hostPtr = Host_ByName(hostname);
    if (hostPtr == (Host_Entry *) NULL) {
	(void) fprintf(stderr, "ZapHostInfo: couldn't get info for %s.\n",
		       hostname);
	return;
    }
    status = Mig_DeleteHost(hostPtr->id);
    if (status != 0) {
	perror("Error in Mig_DeleteHost");
    }
}
