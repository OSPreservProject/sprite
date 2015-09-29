/*
 * main.c --
 *
 *      Main routine for the sprite dump program.
 *
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/dump/RCS/main.c,v 1.12 92/03/28 17:29:31 kupfer Exp $";
#endif

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <option.h>
#include <fs.h>
#include <dev/tape.h>
#include <status.h>
#include <cfuncproto.h>
#include <varargs.h>
#include <assert.h>
#include <dev/exabyte.h>
#include <bstring.h>
#include <sys/errno.h>

#define DEBUG

#ifdef DEBUG
#define debugp(x) ((void) fprintf x, fflush(stderr))
#else
#define debugp(x)
#endif

#define SPRITE_OLD_DUMP_HEADER "SPRITE DUMP TAPE #"
#define SPRITE_DUMP_HEADER 	"SPRITE DUMP TAPE,"
#define SPRITE_DUMP_INFO	" Version %d Level %d Tape %d"
#define SPRITE_DUMP_VERSION	1

#define DUMPDATES              	"/sprite/admin/dump/dumpdates"
#define LOGFILE  	       	"/sprite/admin/dump/dumplog"
#define DRIVELOG		"/sprite/admin/dump/statuslog"
#define TAR		       	"tar.gnu"

#define KBytes(n)	(n * 1024)
#define MBytes(n)	(1024 * KBytes(n))

#define	IOBUF_SIZE	KBytes(62)	/* Exebyte likes 62KB chunks */

#define TOC_SIZE_OLD 	MBytes(10)	/* TOC occupied 10 MB on the tape */
#define	TOC_USEABLE_OLD	MBytes(2)	/* About 2 MB of it can be used */

#define TOC_SIZE 	KBytes(16)	/* No longer needs to be big. */
#define	TOC_USEABLE	TOC_SIZE	/* and we can use it all */

#define TAR_BLOCK_SIZE	KBytes(64)	/* Do the tar in 64K blocks. */

#define LABEL_SIZE_OLD	IOBUF_SIZE	/* Size of useable label. */
#define LABEL_SIZE	TOC_SIZE	/* Size of useable label. */

static char tapeBuffer[IOBUF_SIZE];
static char *directoryToDump;
static tarChild;
static int pipefd[2];
static int archivefd;
static int dumpLevel = 0;
static int tapeNumber;
static int tapeLevel;
static int fileNumber;
static int verbose;
static char *archiveFileName;
static char *mailWho;
static time_t lastTime;
static time_t startTime;
static int fatalErrors;
static int nonFatalErrors;
static int archiveFileIsATapeDrive;
static int printTOC;
static int progArgc;
static char **progArgv;
static int reInitialize = 0;
static int reInitializeSafe = 0;
static long totalBytes;
static int debug;
static int oldFormat;
static int tarBlocks;
static int tapePosition = 0;
static int version = 0;
static int labelSize = 0;
static int lastFile;
static int official = 1;
#ifdef PROG_RESTORE
static int printContents = 0;
#endif

#ifdef PROG_DUMP
static int resetAccessTimes;

static Option OptionArray[] = {
  { OPT_TRUE,   "a", (char *) &resetAccessTimes, "Reset access times"        },
  { OPT_TRUE,   "d", (char *) &debug,            "Debug"                     },
  { OPT_STRING, "f", (char *) &archiveFileName,  "Name of archive file"      },
  { OPT_INT,    "i", (char *) &tapeNumber,       "Initialize the tape"       },
  { OPT_INT,    "l", (char *) &dumpLevel,        "Dump level (0-9)"          },
  { OPT_STRING, "m", (char *) &mailWho,          "Send mail upon completion" },
  { OPT_TRUE,   "r", (char *) &reInitialize,     "Re-initialize the tape"    },
  { OPT_TRUE,   "s", (char *) &reInitializeSafe, "Safely re-initialize tape" },
  { OPT_TRUE,   "t", (char *) &printTOC,         "Print table of contents"   },
  { OPT_TRUE,   "v", (char *) &verbose,          "Verbose"                   },
  { OPT_FALSE,   "u", (char *) &official,      	 "Unofficial dump"           },
};
#endif

#ifdef PROG_RESTORE
static int relativePaths;
static Option OptionArray[] = {
  { OPT_STRING, "f", (char *) &archiveFileName,  "Name of archive"           },
  { OPT_TRUE,   "d", (char *) &debug,            "Debug"                     },
  { OPT_INT,    "n", (char *) &fileNumber,       "File number to use"        },
  { OPT_TRUE,   "r", (char *) &relativePaths,    "Use relative pathnames"    },
  { OPT_TRUE,   "t", (char *) &printTOC,         "Print table of contents"   },
  { OPT_TRUE,   "v", (char *) &verbose,          "Verbose"                   },
  { OPT_TRUE,   "T", (char *) &printContents,    "Print contents of dump file"},
};
#endif

static void cleanup_sighup _ARGS_((int sig));
static void cleanup_sigint _ARGS_((int sig));
static void cleanup_sigquit _ARGS_((int sig));
static void cleanup_sigpipe _ARGS_((int sig));
static void cleanup_sigterm _ARGS_((int sig));

static void openArchiveFile _ARGS_((void));
static void forkOffTar _ARGS_((void));
static void dumpDirectory _ARGS_((char *dir));
static void initializeTape _ARGS_((void));
static void sendMail _ARGS_((const char *msg));
static void rewindTape _ARGS_((void));
static void fatal _ARGS_((char *fmt, ...));
static void warning _ARGS_((char *fmt, ...));
static void readTapeLabel _ARGS_((void));
static void rewindTape _ARGS_((void));
static void openLog _ARGS_((void));
static void skipOverFiles _ARGS_((void));
#ifdef PROG_RESTORE
static void findLastFile _ARGS_((void));
#endif
static void waitForChildToDie _ARGS_((void));
static void setFileNumber _ARGS_((void));
static void quote_string _ARGS_((char *to, const char *from));
static void checkTape _ARGS_((void));
static int parseDumpInfo _ARGS_((char *buf));
static void gotoEOD _ARGS_((void));
static ReturnStatus skipFiles _ARGS_((int num));

#ifdef PROG_DUMP
static int  getNextDumpDateEntry _ARGS_((void));
static void writeTapeLabel _ARGS_((void));
static void flushOutput _ARGS_((void));
static void getDumpDate _ARGS_((void));
static void recordTime _ARGS_((void));
#endif

#ifdef __STDC__
#define VOID void
#endif

void
main(argc, argv)
    int argc;
    char **argv;
{
    uid_t	root;
    uid_t	dumper;
    struct passwd	*pwPtr;
    uid_t	euid;
    extern	uid_t	geteuid();
    extern	int	setreuid();
#ifdef PROG_DUMP
    ReturnStatus	status;
#endif
    char 		*date;
    int			i;


    signal(SIGHUP, cleanup_sighup);
    signal(SIGINT, cleanup_sigint);
    signal(SIGQUIT, cleanup_sigquit);
    signal(SIGPIPE, cleanup_sigpipe);
    signal(SIGTERM, cleanup_sigterm);
    startTime = time(0L);
    date = asctime(localtime(&startTime));
    date[24] = '\0';
    debugp((stderr, "\n"));
    for (i = 0; i < argc; i++) {
	debugp((stderr, "%s ", argv[i]));
    }
    debugp((stderr, "\n"));
    debugp((stderr, "%s\n", date));
    argc = Opt_Parse(argc, argv, OptionArray, Opt_Number(OptionArray), 0);
    progArgc = argc;
    progArgv = argv;

    pwPtr = getpwnam("root");
    if (pwPtr != (struct passwd *) 0) {
        root = pwPtr->pw_uid;
	pwPtr = getpwnam("dumper");
	if (pwPtr != (struct passwd *) 0) {
	    dumper = pwPtr->pw_uid;
	    euid = geteuid();
	    if (euid == root && setreuid(dumper, -1) != 0) {
		perror("Couldn't set real userID to dumper");
		exit(1);
	    }
	}
    }
    endpwent();

    if ((TAR_BLOCK_SIZE & (512 - 1)) != 0) {
	fatal("Tar block size must be multiple of 512\n");
    }
    tarBlocks = TAR_BLOCK_SIZE / 512;
    oldFormat = 0;
    openLog();
    if (archiveFileName == NULL) {
	fatal("No archive file specified");
    }
    tapeLevel = dumpLevel;
    if (tapeNumber != 0) {
	openArchiveFile();
	initializeTape();
	close(archivefd);
	if (argc < 2) {
	    exit(EXIT_SUCCESS);
	}
    }
    openArchiveFile();
    if (archiveFileIsATapeDrive) {
	readTapeLabel();
	if (reInitialize || reInitializeSafe) {
	    if (reInitializeSafe) {
		checkTape();
	    }
	    initializeTape();
	    close(archivefd);
	    if (argc < 2) {
		exit(EXIT_SUCCESS);
	    }
	    openArchiveFile();
	}
    }
    if (printTOC) {
	if (!archiveFileIsATapeDrive) {
	    fatal("Cannot print TOC: Archive is not a tapeDrive");
	}
	fprintf(stderr, "%s\n", tapeBuffer);
	if (argc < 2) {
	    exit(EXIT_SUCCESS);
	}
    }
    if (argc < 2) {
	Opt_PrintUsage(argv[0], OptionArray, Opt_Number(OptionArray));
	exit(EXIT_FAILURE);
    }
#ifdef PROG_DUMP
    directoryToDump = argv[1];
    debugp((stderr, "dumping %s\n", directoryToDump));
    if (archiveFileIsATapeDrive) {
	setFileNumber();
	if (oldFormat) {
	    skipOverFiles();
	} else {
	    /* 
	     * We are currently at the end of the label since we just read
	     * it.  The tar file will follow the subsequent file mark.
	     */
	    status = skipFiles(1);
	    if (status != SUCCESS) {
		fatal("Can't skip over last filemark\n");
	    }
	}
    }
    getDumpDate();
    forkOffTar();
    dumpDirectory(directoryToDump);
    flushOutput();
    waitForChildToDie();
    openArchiveFile();
    if (fatalErrors) {
	sendMail("Dump failed with fatal errors");
    } else {
	if (nonFatalErrors) {
	    sendMail("Dump completed with non-fatal errors.");
	} else {
	    sendMail("Dump completed successfully.");
	}
	recordTime();
    }
    debugp((stderr, "finished dumping %s, %.1lf MBytes\n",
	    directoryToDump, (double) totalBytes / (double) MBytes(1)));
#else /* PROG_DUMP */
    assert(progArgc >= 2);
    if (fileNumber == 0) {
	setFileNumber();
    } else {
	findLastFile();
    }
    skipOverFiles();
    forkOffTar();
    waitForChildToDie();
    openArchiveFile();
    rewindTape();
#endif /* PROG_DUMP */
    debugp((stderr,
	"%s exiting, there were %d non-fatal errors, %d hard errors\n",
	argv[0], nonFatalErrors, fatalErrors));
    exit(fatalErrors);
}

/*
 *----------------------------------------------------------------------
 *
 * dumpDirectory --
 *
 *      Procedure to dump a directory tree.  This routine does an
 *      inorder traversal of a directory.  It calls itself recursively
 *      to dump each subdirectory.
 *
 * Results:
 *      None.
 * 
 * Side effects:
 *      Depends on the options, but normally the files in the specified
 *      directory are dumped.
 *  
 *----------------------------------------------------------------------
 */ 

static void
dumpDirectory(dir)
    char *dir;
{
    char pathname[MAXPATHLEN];
    char backslash_pathname[MAXPATHLEN];
    DIR *dirDesc;
    struct direct *d;
    int slash;

    if ((dirDesc = opendir(dir)) == NULL) {
	warning("Cannot open directory `%s'", dir);
	++nonFatalErrors;
	return;
    }
    slash =  (dir[strlen(dir) - 1] == '/');
    while ((d = readdir(dirDesc)) != NULL) {
	struct stat statBuf;

	if (*d->d_name == '.') {
	    if (d->d_name[1] == 0)
		continue;
	    if (d->d_name[1] == '.' && d->d_name[2] == 0)
		continue;
	}
	(void) sprintf(pathname, slash ? "%s%s" : "%s/%s", dir, d->d_name);
	if (lstat(pathname, &statBuf)) {
	    warning("can't lstat %s", pathname);
	    ++nonFatalErrors;
	    continue;
	}
	quote_string(backslash_pathname, pathname);
	if (statBuf.st_mtime >= lastTime || statBuf.st_ctime >= lastTime) {
	    if (puts(backslash_pathname) == EOF) {
		(void) fprintf(stderr, "error in puts\n");
		++fatalErrors;
	    } else if (verbose) {
		fprintf(stderr, "dumping %s\n", backslash_pathname);
	    }
	    totalBytes += statBuf.st_size;
	}
	if (((statBuf.st_mode & S_IFDIR) == S_IFDIR) && 
	    ((statBuf.st_mode & S_IFRLNK) != S_IFRLNK) &&
	    ((statBuf.st_mode & S_IFPDEV) != S_IFPDEV)) {
		dumpDirectory(pathname);
	}
    }
    (void) closedir(dirDesc);
    return;
}

static struct {
    int     tape;
    int     file;
    int     level;
    double  mbytes;
    double  remaining;
    double  errorRate;
    time_t  time;
    char    dirname[MAXPATHLEN];
} dumpDateEntry;

#ifdef PROG_DUMP
/*
 *----------------------------------------------------------------------
 *
 * recordTime --
 *
 *	Append the start time of the current dump to the dump database.
 *
 * Results:
 *      None.	
 * 
 * Side effects:
 *      The start time of the current dump is appended to the end
 *      of the dump database.  A fatal error occurs if the database
 *      doesn't exist or is unwritable.
 *      
 *----------------------------------------------------------------------
 */ 

static void
recordTime()
{
    FILE 		*fp = NULL;
    char 		*date;
    Dev_TapeStatus	tapeStatus;
    int			errors;
    double		mbytes;
    double		errorRate;
    ReturnStatus	status = FAILURE;
    double		remaining;
    double		mbytesPerBlock;
    char		*name = "UNKNOWN"; 
    char		*serial = "UNKNOWN";
    int			blocks;
    char		shortDate[16];
    char 		buf[128];

    if (official) {
	if ((fp = fopen(DUMPDATES, "a")) == NULL) {
	    fatal("Can't open %s", DUMPDATES);
	}
    }
    date = asctime(localtime(&startTime));
    date[24] = '\0';
    mbytes = ((double) totalBytes) / ((double) MBytes(1));
    debugp((stderr, "mbytes = %lf\n", mbytes));
    errorRate = -1;
    remaining = -1;
    if (!oldFormat) {
	bzero((char *) &tapeStatus, sizeof(tapeStatus));
	debugp((stderr, "Getting tape status\n"));
	status = Fs_IOControl(archivefd, IOC_TAPE_STATUS, 0, NULL, 
	    sizeof(tapeStatus), &tapeStatus);
	if (status == SUCCESS) {
	    mbytesPerBlock = 
		(double) tapeStatus.blockSize / (double) MBytes(1);
	    debugp((stderr, "position = %d, remaining = %d\n",
			tapeStatus.position, tapeStatus.remaining));
	    blocks = tapeStatus.position - tapePosition;
	    mbytes = blocks * mbytesPerBlock;
	    errors = tapeStatus.readWriteRetry + tapeStatus.dataError;
	    errorRate = ((double) errors / (double) blocks) * 100.0;
	    remaining = tapeStatus.remaining * mbytesPerBlock;
	    debugp((stderr, 
		"mbytes = %lf,  errorRate = %lf, remaining = %lf\n",
		mbytes, errorRate, remaining));
	}
    }
    (void) sprintf(buf, "%03d %03d %d %6.1lf %6.1lf %s  %s\n",
	       tapeNumber, fileNumber, dumpLevel, mbytes, remaining,
	       date, directoryToDump);
    if (archiveFileIsATapeDrive) {
	strcat(tapeBuffer, buf);
	writeTapeLabel();
	if (close(archivefd) != 0) {
	    fatal("Close of archive failed");
	}
    }
    if (official) {
	fprintf(fp, "%s", buf);
	(void) fclose(fp);
    }

    if ((fp = fopen(DRIVELOG, "a")) == NULL) {
	fatal("Can't open %s", DRIVELOG);
    }
    if (status == SUCCESS) {
	switch(tapeStatus.type) {
	    case DEV_TAPE_EXB8200:
		name = "EXB-8200";
		break;
	    case DEV_TAPE_EXB8500:
		name = "EXB-8500";
		break;
	    case DEV_TAPE_TLZ04:
		name = "DEC-TLZ04";
		break;
	    default: {
		if (tapeStatus.type & DEV_TAPE_8MM) {
		    name = "UNKNOWN-8MM";
		} else if (tapeStatus.type & DEV_TAPE_4MM) {
		    name = "UNKNOWN-4MM";
		}
	    }
	}
	if (tapeStatus.serial[0] != '\0') {
	    serial = tapeStatus.serial;
	}
    }
    bcopy(&date[4], shortDate, 7);
    bcopy(&date[20], &shortDate[7], 4);
    shortDate[11] = '\0';
    fprintf(fp, "%-10s %-10s %3d %7.1lf %5.1lf%% %s %s\n", name, serial,
	tapeNumber, mbytes, errorRate, shortDate, archiveFileName);
    (void) fclose(fp);

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * getNextDumpDateEntry --
 *
 *	Reads the next entry from the dump database file,
 *	and puts the information into a structure.
 *
 * Results:
 *	Returns non-zero if the next entry is read successfully.
 * 
 * Side effects:
 *      A fatal error occurs if the database doesn't exist, is unreadable
 *      or if it is improperly formated.
 *      
 *----------------------------------------------------------------------
 */ 
static int
getNextDumpDateEntry()
{
    static FILE *fp;
    static int lineCnt;
    char buf[0x1000];
    char *s;

    if (fp == NULL) {
	if ((fp = fopen(DUMPDATES, "r")) == NULL) {
	    fatal("Can't open %s", DUMPDATES);
	}
    }

    /*
     * read in the file line by line until we get a valid entry.
     */
    while (fgets(buf, sizeof(buf), fp)) {
	++lineCnt;
	/*
	 * If the line is empty, only white space, or is a
	 * comment, then throw it away and continue.
	 */
	s = buf;
	while (*s == ' ' || *s == '\t') {
	    ++s;
	}
	if (*s == '#' || *s == '\n' || *s == '\0') {
	    continue;
	}
	if (parseDumpInfo(s) == 0) {
	    fatal("format error in %s: line %d |%s|",
		DUMPDATES, lineCnt, buf);
	}
	return 1;
    }
    (void) fclose(fp);
    return 0;
}
#endif

static int
parseDumpInfo(buf)
    char *buf;
{
    extern time_t mktime();
    static char months[][4] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    struct tm tm;
    char *s;
    int i;
    int n;
    int	token = 1;

    bzero((char *) &dumpDateEntry, sizeof(dumpDateEntry));
    /*
     * The first token is the tape number
     */
    if ((s = strtok(buf, " \t\n")) == NULL || !isdigit(*s)) {
	goto mismatch;
    }
    dumpDateEntry.tape = atoi(s);
    token++;
    /*
     * The next token is the file number
     */
    if ((s = strtok(NULL, " \t\n")) == NULL || !isdigit(*s)) {
	goto mismatch;
    }
    dumpDateEntry.file = atoi(s);
    token++;

    /* 
     * The next token should be a single digit in the range 0-9 that
     * indicates the level of the dump.
     */
    s = strtok((char *) NULL, " \t\n");
    if (s == NULL || !isdigit(*s) || strlen(s) != 1) {
	goto mismatch;
    }
    dumpDateEntry.level = *s - '0';
    token++;

    /* the next token should be the number of bytes */
    s = strtok((char *) NULL, " \t\n");
    if (strchr(s, (int) '.')) {
	/* Number of MBytes. */
	n = sscanf(s, "%lf", &dumpDateEntry.mbytes);
	if (n != 1) {
	    goto mismatch;
	}
	token++;
	/* Number of remaining MBytes on tape. */
	s = strtok((char *) NULL, " \t\n");
	n = sscanf(s, "%lf", &dumpDateEntry.remaining);
	if (n != 1) {
	    goto mismatch;
	}
	token++;
    } else {
	n = sscanf(s, "%d", &i);
	if (n != 1) {
	    goto mismatch;
	}
    }
    /*
     * The time should be formated just like the return string
     * from asctime(3).  So, parse the string and make sure the
     * format is correct.
     */

    if ((s = strtok((char *) NULL, " \t")) == NULL) {
	goto mismatch;
    }
    token++;
    if ((s  = strtok((char *) NULL, " ")) == NULL) {
	goto mismatch;
    }
    token++;
    for (i = 13; --i >= 0;) {
	if (strcmp(months[i], s) == 0) {
	    break;
	}
    }
    if ((tm.tm_mon = i) < 0) {
	goto mismatch;
    }
    if ((s = strtok((char *) NULL, " ")) == NULL) {
	goto mismatch;
    }
    token++;
    tm.tm_mday = atoi(s);
    if ((s = strtok((char *) NULL, ":")) == NULL) {
	goto mismatch;
    }
    token++;
    tm.tm_hour = atoi(s);
    if ((s = strtok((char *) NULL, ":")) == NULL) {
	goto mismatch;
    }
    token++;
    tm.tm_min = atoi(s);
    if ((s = strtok((char *) NULL, " ")) == NULL) {
	goto mismatch;
    }
    token++;
    tm.tm_sec = atoi(s);
    if ((s = strtok((char *) NULL, " \t\n")) == NULL) {
	goto mismatch;
    }
    tm.tm_year = atoi(s) - 1900;
    /* Convert to old-fashioned calander time */
    if((dumpDateEntry.time = mktime(&tm)) == -1) {
	debugp((stderr, "parseDumpInfo:  mktime failed\n"));
	return 0;
    }

    token++;
    if ((s = strtok((char *) NULL, " \t\n")) == NULL) {
	goto mismatch;
    }

    /*
     * The remainder of the line is the directory name.
     */
    strcpy(dumpDateEntry.dirname, s);
    return 1;
mismatch:
    debugp((stderr, "parseDumpInfo:  parse error on token %d \"%s\"\n", 
	    token, s));
    return 0;

}


#ifdef PROG_DUMP
/*
 *----------------------------------------------------------------------
 *
 * getDumpDate --
 *
 *	Reads the dump database file, and determines the last time
 *      that a lower level dump of the directory was done.
 *
 * Results:
 *	Returns the time of the last lower level dump.
 * 
 * Side effects:
 *      A fatal error occurs if the database doesn't exist, is unreadable
 *      or if it is improperly formated.
 *      
 *----------------------------------------------------------------------
 */ 

static void
getDumpDate()
{
    time_t t = 0;

    while (getNextDumpDateEntry()) {
	if (strcmp(dumpDateEntry.dirname, directoryToDump) != 0) {
	    continue;
	}
	if (dumpDateEntry.level < dumpLevel) {
	    t = dumpDateEntry.time;
	}
    }
    lastTime = t;
    return;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * openArchiveFile --
 *
 *	Opens the archive file and checks to see if it is a tapedrive.
 *
 * Results:
 *	None
 * 
 * Side effects:
 *      Opens the archive file.
 *      
 *----------------------------------------------------------------------
 */ 

static void
openArchiveFile()
{
    struct stat statBuf;
    extern int Fs_GetAttributes();
    Fs_Attributes attrs;
    Dev_TapeStatus	tapeStatus;
    ReturnStatus	status;

    debugp((stderr, "opening %s as archive file\n", archiveFileName));
#ifdef PROG_DUMP
    if (strcmp(archiveFileName, "-") == 0) {
	archivefd = 1;
    } else if ((archivefd = open(archiveFileName,O_RDWR)) < 0) {
	if ((archivefd = open(archiveFileName,
			      O_RDWR|O_CREAT, 0664)) < 0) {
	    fatal("Can't open `%s'", archiveFileName);
	}
    }
#else    
    if (strcmp(archiveFileName, "-") == 0) {
	archivefd = 0;
    } else if ((archivefd = open(archiveFileName,O_RDONLY)) < 0) {
	fatal("Can't open `%s'", archiveFileName);
    }
#endif
    if (fstat(archivefd, &statBuf)) {
	fatal("Cannot stat %s", archiveFileName);
    }
    switch (statBuf.st_mode & S_IFMT) {

    case S_IFREG:		/* regular file */
        break;

    case S_IFCHR:		/* character dev. -- probably a tape drive */
	if (Fs_GetAttributes(archiveFileName, 0, &attrs) != SUCCESS) {
	    fatal("Cannot get attributes of %s", archiveFileName);
	}
	if (attrs.devType == 5) {
	    archiveFileIsATapeDrive = TRUE;
	    bzero((char *) &tapeStatus, sizeof(tapeStatus));
	    status = Fs_IOControl(archivefd, IOC_TAPE_STATUS, 0, NULL, 
		sizeof(tapeStatus), &tapeStatus);
	    debugp((stderr, "IOC_TAPE_STATUS returned 0x%x\n", status));
	    if (status != SUCCESS) {
		oldFormat = TRUE;
		debugp((stderr, "Using old format.\n", status));
	    } else {
		debugp((stderr, "tapeStatus.type is 0x%x\n", tapeStatus.type));
		if (tapeStatus.type == DEV_TAPE_EXB8200) {
		    oldFormat = TRUE;
		    debugp((stderr, "Using old format.\n", status));
		}

#ifndef PROG_DUMP
		if ((tapeStatus.type == DEV_TAPE_EXB8500) &&
		   (tapeStatus.density == DEV_EXB8500_8200_MODE)) {
		    oldFormat = TRUE;
		    debugp((stderr, "Using old format.\n", status));
		}
#endif
	    }
	    break;
	}
	/* FALLTHROUGH */
    default:
	warning("%s: not a normal file or tape drive", archiveFileName);
	break;
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * forkOffTar --
 *
 *	Forks off tar and pipes stdout into it.
 *
 * Results:
 *	None
 * 
 * Side effects:
 *      Creates a child process.
 *      
 *----------------------------------------------------------------------
 */ 
static void
forkOffTar()
{
    char 		tarargs[20];
    char 		blocks[20];
    Dev_TapeStatus	tapeStatus;
    ReturnStatus	status;
#ifdef PROG_RESTORE
    char **arg;
    int i, j;
#endif

    if (!oldFormat) {
	bzero((char *) &tapeStatus, sizeof(tapeStatus));
	status = Fs_IOControl(archivefd, IOC_TAPE_STATUS, 0, NULL, 
	    sizeof(tapeStatus), &tapeStatus);
	if (status != SUCCESS) {
	    debugp((stderr, "IOC_TAPE_STATUS returned 0x%x\n", status));
	} else {
	    tapePosition = tapeStatus.position;
	    debugp((stderr, "position = %d\n", tapePosition));
	}
    }
#ifdef PROG_DUMP    

    if (pipe(pipefd) < 0) {
	fatal("Cannot open pipe");
    }
    switch (tarChild = fork()) {

    case -1:
	fatal("Can't fork");

    case 0:
	(void) close(pipefd[1]);
	if (dup2(archivefd, 1) < 0) {
	    fatal("dup2");
	}
	if (dup2(pipefd[0], 0) < 0) {
	    fatal("dup2");
	}
	*tarargs = '\0';
	if (resetAccessTimes) {
	    strcat(tarargs, "a");
	}
	if (verbose) {
	    strcat(tarargs, "v");
	}
	if (debug) {
	    strcat(tarargs, "e");
	}
	if (oldFormat) {
	    strcat(tarargs, "ncfTPL");
	    debugp((stderr, "execing tar %s - -\n", tarargs));
	    execlp(TAR, TAR, tarargs, "-", "-", NULL);
	} else {
	    strcat(tarargs, "ncbfTPL");
	    sprintf(blocks, "%d", tarBlocks);
	    debugp((stderr, "execing tar %s %s - -\n", tarargs, blocks));
	    execlp(TAR, TAR, tarargs, blocks, "-", "-", NULL);
	}
	fatal("exec failed");

    default:
	(void) close(pipefd[0]);
	if (dup2(pipefd[1], 1)) {
	    fatal("dup2");
	}
    }
#else

    sprintf(blocks, "%d", tarBlocks);
    close(archivefd);
    switch (tarChild = fork()) {

    case -1:
	fatal("Can't fork");

    case 0: {
	char **ptr;
	arg = (char **) malloc(sizeof(char **) * (progArgc + 5));
	ptr = arg;
	*ptr++ = TAR;
	if (printContents) {
	    strcpy(tarargs, "tv");
	} else {
	    strcpy(tarargs, "x");
	}
	if (oldFormat) {
	    strcat(tarargs, "pf");
	} else {
	    strcat(tarargs, "pfb");
	}
	if (verbose) {
	    strcat(tarargs, "v");
	}
	if (!relativePaths) {
	    strcat(tarargs, "P");
	}
	if (debug) {
	    strcat(tarargs, "e");
	}
	*ptr++ = tarargs;
	*ptr++ = archiveFileName;
	if (!oldFormat) {
	    *ptr++ = blocks;
	}
	for (i = 1; (*ptr++ = progArgv[i]) != NULL; ++i) {
	    continue;
	}
	debugp((stderr, "execing tar"));
	for (j = 0; j < i + 3; j++) {
	    debugp((stderr, " %s", arg[j]));
	}
	debugp((stderr, "\n"));
	execvp(TAR, arg);
	fatal("exec failed");
    }
    default:
	break;
    }
#endif    
    debugp((stderr, "successfully forked tar\n"));
    return;
}

#ifdef PROG_DUMP
/*
 *----------------------------------------------------------------------
 *
 * flushOutput --
 *
 *	Wait for tar to finish reading all the data in the pipe.
 *	Check the error return code.
 *
 * Results:
 *	None
 * 
 * Side effects:
 *      Closes files, increments a global variable.
 *      
 *----------------------------------------------------------------------
 */ 
static void
flushOutput()
{

    debugp((stderr, "flushing output\n"));
    (void) fflush(stdout);
    (void) close(1);
    (void) close(pipefd[1]);
    return;
}
#endif

static void
waitForChildToDie()
{
    int w;
    union wait ws;

    while ((w = wait(&ws)) > 0 && w != tarChild) {
	continue;
    }
    switch (ws.w_retcode) {

    case 0:     /* normal exit */
	break;

    case 1:
        fatal("tar invoked with invalid arguments");
	break;

    case 2:
	warning("tar encountered at least one invalid filename");
	++nonFatalErrors;
	break;

    case 3:
	fatal("bad archive");
	break;

    case 4:
	fatal("system error in tar");
	break;

    default:
	fatal("tar exited with nozero status: %d", ws.w_retcode);
	break;
    }
    (void) close(archivefd);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * readTapeLabel --
 *
 *	Read the label on a tape, and put the label into the tapeBuffer.
 *
 * Results:
 *	None
 * 
 * Side effects:
 *      The tape is rewound.
 *      
 *----------------------------------------------------------------------
 */

static void
readTapeLabel()
{
    ReturnStatus	status = SUCCESS;
    int			n;
    int			bytesRead;
    Dev_TapeStatus	tapeStatus;

    debugp((stderr, "reading tape label\n"));
    labelSize = LABEL_SIZE_OLD;
    bzero((char *) &tapeStatus, sizeof(tapeStatus));
    status = Fs_IOControl(archivefd, IOC_TAPE_STATUS, 0, NULL, 
	sizeof(tapeStatus), &tapeStatus);
    if ((status != SUCCESS) || (tapeStatus.type == DEV_TAPE_EXB8200) ||
	(reInitialize) || (reInitializeSafe)) {
	/*
	 * The tape may be in the old format if the tape drive is an
	 * exb8200, or if we are reinitializing an old format tape.
	 * Try reading the label at the beginning of the tape.
	 */
	rewindTape();
    
	bytesRead = read(archivefd, tapeBuffer, labelSize);
	debugp((stderr, "attempt to read old label read %d bytes\n", 
		    bytesRead));
	if (bytesRead == labelSize) { 
	    if (!strncmp(tapeBuffer, SPRITE_OLD_DUMP_HEADER, 
		    strlen(SPRITE_OLD_DUMP_HEADER)) != 0) {
		oldFormat = TRUE;
		debugp((stderr, "Using old format\n"));
		n = sscanf(tapeBuffer + strlen(SPRITE_OLD_DUMP_HEADER), 
			"%d", &tapeNumber);
		if (n != 1) {
		    warning("Couldn't read tape number from label\n");
		}
	    } else if (oldFormat) {
		fatal("The tape does not have a correct label");
	    }
	}
    }
    if (!oldFormat) {
	Boolean	beginning = FALSE;

	labelSize = LABEL_SIZE;
	gotoEOD();
	/*
	 * Keep backing up one file at a time until we find a label, or
	 * the beginning of the tape. In order to back up one file at
	 * a time we must back up over both the current file and the 
	 * previous file, hence the skip backwards over two files.
	 * 
	 */
	status = SUCCESS;
	while(status == SUCCESS) {
	    status = skipFiles(-2);
	    if (status == DEV_END_OF_TAPE) {
		/*
		 * We must have hit the beginning of the tape.
		 * Keep track so we only do this once.
		 */
		debugp((stderr, "hit beginning of tape\n"));
		if (beginning == TRUE) {
		    fatal("The tape does not have a correct label");
		}
		beginning = TRUE;
		status = SUCCESS;
	    } else if (status == SUCCESS) {
		/*
		 * We are now on the BOT side of a file mark. Skip to the
		 * EOT side.
		 */
		debugp((stderr, "skipping over file mark\n"));
		status = skipFiles(1);
		if (status != SUCCESS) {
		    fatal("Error skipping over file mark");
		}
	    }
	    /*
	     * Try reading the label. 
	     */
	    debugp((stderr, "trying to read label (%d bytes)\n",
		labelSize));
	    bytesRead = read(archivefd, tapeBuffer, labelSize);
	    if (bytesRead == labelSize) { 
		debugp((stderr, "read succeeded\n"));
		if (!strncmp(tapeBuffer, SPRITE_DUMP_HEADER, 
			strlen(SPRITE_DUMP_HEADER))){
		    /* 
		     * We found a label.
		     */
		    debugp((stderr, "found a label\n"));
		    break;
		} else {
		    debugp((stderr, "found \"%s\"\n", tapeBuffer));
		}
	    } else {
		debugp((stderr, "only read %d bytes\n", bytesRead));
	    }
	}
	if (status != SUCCESS) {
	    fatal("The tape does not have a correct label");
	}
	n = sscanf(tapeBuffer + strlen(SPRITE_DUMP_HEADER), 
		"%*s %d %*s %d %*s %d\n", &version, &tapeLevel, 
		    &tapeNumber);
	if (n != 3) {
	    fatal("Couldn't read information from tape label\n");
	}
	if (version != SPRITE_DUMP_VERSION) { 
	    fatal("Invalid dump tape version \"%d\"\n", version);
	}
    }
    debugp((stderr, "Tape: %d\n", tapeNumber));
    debugp((stderr, "Version: %d\n", version));
    debugp((stderr, "Level: %d\n", tapeLevel));
    debugp((stderr, "TapeLabel=|%s|\n", tapeBuffer));
    return;
}

#ifdef PROG_DUMP
/*
 *----------------------------------------------------------------------
 *
 * writeTapeLabel --
 *
 *	Copy the tapeBuffer onto the tape.
 *
 * Results:
 *	None
 * 
 * Side effects:
 *      The tape is rewound.
 *      
 *----------------------------------------------------------------------
 */

static void
writeTapeLabel()
{

    debugp((stderr, "writing tape label\n"));
    if (oldFormat) {
	rewindTape();
    } else {
	gotoEOD();
    }
    if (write(archivefd, tapeBuffer, labelSize) != labelSize) {
	fatal("Error writing tape label");
    }
    /*
     * Rewinding the tape prevents a file mark from being written, which
     * is what we want on the old format tapes.
     */
    if (oldFormat) {
	rewindTape();
    }
    return;
}
#endif

static void
initializeTape()
{   
    int nbufs;
    FILE *fp;
    Dev_TapeStatus	tapeStatus;
    int			tocSize;
    static char		tempBuffer[IOBUF_SIZE];
    ReturnStatus	status;

    debugp((stderr, "initializing tape #%d\n", tapeNumber));
    if (archiveFileIsATapeDrive == 0) {
	fatal("Cannot initialize:  Archive file is not a tape drive");
    }
    rewindTape();
    bzero((char *) &tapeStatus, sizeof(tapeStatus));
    status = Fs_IOControl(archivefd, IOC_TAPE_STATUS, 0, NULL, 
	sizeof(tapeStatus), &tapeStatus);
    if ((status != SUCCESS) || (tapeStatus.type == DEV_TAPE_EXB8200)) {
	oldFormat = TRUE;
	labelSize = LABEL_SIZE_OLD;
    } else {
	oldFormat = FALSE;
	labelSize = LABEL_SIZE;
    }
    memset(tapeBuffer, '\0', labelSize);
    if (oldFormat) {
	sprintf(tapeBuffer, "%s%d\n", SPRITE_OLD_DUMP_HEADER, tapeNumber);
    } else {
	char	format[80];
	strcpy(format, SPRITE_DUMP_HEADER);
	strcat(format, SPRITE_DUMP_INFO);
	sprintf(tapeBuffer, format, SPRITE_DUMP_VERSION, tapeLevel,tapeNumber);
	strcat(tapeBuffer, "\n");
    }

    debugp((stderr, "writing label\n"));
    if (write(archivefd, tapeBuffer, labelSize) != labelSize) {
	fatal("Write error while initializing tape label");
    }
    debugp((stderr, "done writing label\n"));
    if (oldFormat) {
	tocSize = TOC_SIZE_OLD;
    } else {
	tocSize = TOC_SIZE;
    }
    debugp((stderr, "writing padding\n"));
    memset(tempBuffer, '\0', sizeof(tempBuffer));
    for (nbufs = tocSize / IOBUF_SIZE; --nbufs > 0;) {
	if (write(archivefd, tempBuffer,
		  sizeof(tempBuffer)) != sizeof(tempBuffer)) {
	    fatal("Write error while initializing tape label");
	}
    }
    debugp((stderr, "done writing padding\n"));
    if (!oldFormat) {
	Dev_TapeCommand args;
	args.command = IOC_TAPE_WEOF;
	args.count = 1;
	status = Fs_IOControl(archivefd, IOC_TAPE_COMMAND, sizeof(args),
		    (Address)&args, 0, (Address) 0);
	if (status != SUCCESS) {
	    fatal("Can't write file mark(s), status = 0x%x", status);
	    exit(status);
	}
    }
    rewindTape();
    if (official) {
	if ((fp = fopen(DUMPDATES, "a")) == NULL) {
	    fatal("Can't open %s", DUMPDATES);
	}
	/*
	 *   Put a comment in the dump data file.
	 */
	fprintf(fp, "# Initializing tape number %03d\n", tapeNumber);
	(void) fclose(fp);
    }
    return;
}

static void
rewindTape()
{
    int oldOffset;
    int status;

    debugp((stderr, "rewinding tape ...\n"));
    status = Ioc_Reposition(archivefd, IOC_BASE_ZERO, 0, &oldOffset);
    if (status != SUCCESS) {
	fatal("Can't rewind tape drive, status = 0x%08x", status);
    }
    debugp((stderr, "done rewinding tape.\n"));
    return;
}

static void
setFileNumber()
{
    char *s;
#ifdef PROG_DUMP
    fileNumber = 1;
    for (s = tapeBuffer; *s != '\0'; ++s) {
	if (s[0] == '\n' && s[1] > ' ') {
	    ++fileNumber;
	}
    }
#else
    char *end;
    int len = 0;
    int n = 1;

    /*
     * Check each file on the dumptape, and pick the one that is the
     * longest prefix of the file to be restored.
     */

    if ((end = strchr(tapeBuffer, '\n')) == NULL) {
	fatal("Bad tape label\n");
    }
    for (s = end; *s != '\0'; ++n) {
	s = end + 1;
	if ((end = strchr(s, '\n')) == NULL) {
	    break;
	}
	*end = '\0';
	if (parseDumpInfo(s) == 0) {
	    fatal("format error in tapelabel, line %d\n", n);
	}
	assert(dumpDateEntry.file == n);
	lastFile = n;
	if (strlen(dumpDateEntry.dirname) >= len) {
	    if (strncmp(dumpDateEntry.dirname, progArgv[1],
		        strlen(dumpDateEntry.dirname)) == 0) {
                len = strlen(dumpDateEntry.dirname);
		fileNumber = n;
            }
#if 0	    
	    else if (dumpDateEntry.dirname[0] == '/') {
		if (strncmp(dumpDateEntry.dirname + 1, progArgv[1],
		            strlen(dumpDateEntry.dirname) - 1) == 0) {
		    len = strlen(dumpDateEntry.dirname) - 1;
		    fileNumber = n;
                }
	    }
#endif	    
	}
    }
    (void) fprintf(stderr, "Using file #%d\n", fileNumber);
    if (fileNumber == 0) {
	fatal("None of the files on this tape contain %s.\n", progArgv[1]);
    }
#endif    
    return;
}

#ifdef PROG_RESTORE
static void
findLastFile()
{
    char *s;
    int n = 1;
    char *end;

    if ((end = strchr(tapeBuffer, '\n')) == NULL) {
	fatal("Bad tape label\n");
    }
    for (s = end; *s != '\0'; ++n) {
	s = end + 1;
	if ((end = strchr(s, '\n')) == NULL) {
	    break;
	}
	*end = '\0';
	lastFile = n;
    }
}
#endif /* PROG_RESTORE */

static void
skipOverFiles()
{
    int 		status;
    int			 num;
#ifdef PROG_DUMP
    Dev_TapeCommand 	args;
#endif

    assert(fileNumber);
    if (oldFormat) {
	rewindTape();
	num = fileNumber;
    } else {
	/*
	 * In the new format we don't rewind the tape.  Instead we back
	 * up from where we currently are to the file we want.
	 */
	num = - (lastFile - fileNumber + 1) * 2;
    }
    status = skipFiles(num);
    if (status != SUCCESS) {
	fatal("Can't skip files, status = 0x%08x", status);
    }
#ifdef PROG_DUMP
    if (oldFormat) {
	debugp((stderr, "Backing up over filemark\n"));
	status = skipFiles(-1);
	if (status != SUCCESS) {
	    fatal("Can't skip files, status = 0x%08x", status);
	}
    } 
    debugp((stderr, "rewriting file mark\n"));
    args.command = IOC_TAPE_WEOF;
    args.count = 1;
    status = Fs_IOControl(archivefd, IOC_TAPE_COMMAND, sizeof(args),
			  (char *) &args, 0, (char *) 0);
#else
    if (!oldFormat) {
	/*
	 * Since we backed up we are on the BOT side of a file mark.
	 * Skip over it.
	 */
	debugp((stderr, "Skipping over filemark\n"));
	status = skipFiles(1);
	if (status != SUCCESS) {
	    fatal("Can't skip files, status = 0x%08x", status);
	}
    }
#endif
    debugp((stderr, "successfully skipped %d files\n", fileNumber));
    return;
}

/*ARGSUSED*/
static void
cleanup_sighup(sig)
    int sig;
{

    fatal("Received SIGHUP signal, terminating abnormally");
    return;
}

/*ARGSUSED*/
static void
cleanup_sigint(sig)
    int sig;
{

    fatal("Received SIGINT signal, terminating abnormally");
    return;
}

/*ARGSUSED*/
static void
cleanup_sigquit(sig)
    int sig;
{

    fatal("Received SIGQUIT signal, terminating abnormally");
    return;
}

/*ARGSUSED*/
static void
cleanup_sigpipe(sig)
    int sig;
{
#if 1
{
    int w;
    union wait ws;

    debugp((stderr, "Received SIGPIPE signal, terminating abnormally\n"));
    while ((w = wait(&ws)) > 0 && w != tarChild) {
	continue;
    }
    debugp((stderr, "SIGPIPE: tar exited with code = 0x%x\n", ws.w_retcode));

    if (ws.w_retcode) {
	warning("tar exited with nozero status: %d", ws.w_retcode);
	++fatalErrors;
    }
    (void) close(archivefd);
}
#endif

    fatal("Received SIGPIPE signal, terminating abnormally");
    return;
}

/*ARGSUSED*/
static void
cleanup_sigterm(sig)
    int sig;
{

    fatal("Received SIGTERM signal, terminating abnormally");
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * sendMail --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sends mail to the person specified by the -m option.
 *
 *----------------------------------------------------------------------
 */

static void
sendMail(msg)
    const char *msg;
{
    int p[2];
    int childPid;
    int w;
    FILE *fp;

    if (mailWho == NULL) {
	return;
    }
    pipe(p);

    fprintf(stderr, "sending mail to `%s'\n", mailWho);

    switch (childPid = fork()) {

    case -1:
	fatal("Fork failed");

    case 0: /* child */
	close(pipefd[1]);
	dup2(pipefd[0], 0);
	execlp("mail", "mail", mailWho, NULL);
	fatal("Can't exec `mail'");
	/* NOTREACHED */

    default: /* parent */
        close(pipefd[0]);
	fp = fdopen(pipefd[1], "w");
	fprintf(fp, "%s.\n\n", msg);
	fprintf(fp, "Level %d dump of %s on %s",
		dumpLevel, directoryToDump,
		asctime(localtime(&startTime)));
	(void) fclose(fp);
	while ((w = wait(0)) > 0 && w != childPid)
	  ;
	fprintf(stderr, "done sending mail\n");
	break;
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * fatal --
 *
 *	Print an error message and terminate the program.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      A formated error message is printed to stderr, and
 *      the program is terminated.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
static void
fatal(va_alist)
    va_dcl
{
    char *errmsg;
    va_list args;
    char mailBuf[0x1000];
    char	*fmt;

    errmsg = strerror(errno);
    va_start(args);
    fmt = va_arg(args, char *);
    (void) fprintf(stderr, "Dump: ");
    (void) vfprintf(stderr, fmt, args);
    (void) fprintf(stderr, ": %s\n", errmsg);
    (void) fflush(stderr);

    (void) vsprintf(mailBuf, fmt, args);
    (void) strcat(mailBuf, ": ");
    (void) strcat(mailBuf, errmsg);
    sendMail(mailBuf);

    va_end(args);
    exit(1);
}
#else
/*VARARGS1*/
/*ARGSUSED*/
void
fatal(fmt)
    char *fmt;
{
    return;
}
#endif  /* !lint */


/*
 *----------------------------------------------------------------------
 *
 * warning --
 *
 *	Print a warning message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      A formated error message is printed to stderr.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
static void
warning(va_alist)
    va_dcl
{
    const char *errmsg;
    va_list args;
    char	*fmt;

    errmsg = strerror(errno);
    va_start(args);
    fmt = va_arg(args, char *);
    (void) fprintf(stderr, "Dump: ");
    (void) vfprintf(stderr, fmt, args);
    (void) fprintf(stderr, ": %s\n", errmsg);
    (void) fflush(stderr);
    va_end(args);
    return;
}
#else
/*VARARGS1*/
/*ARGSUSED*/
void
warning(fmt)
    char *fmt;
{
    return;
}
#endif  /* !lint */


/*
 *----------------------------------------------------------------------
 *
 * openLog --
 *
 *	Open the dump log file and tee stderr so that all
 *      error messages are appended to the log.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      The log file is opened, and stderr is redirected to it.
 *
 *----------------------------------------------------------------------
 */

static void
openLog()
{
    int pfd[2];
    char buf[BUFSIZ];
    int fd;

    pipe(pfd);
    switch (fork()) {

    case -1:
	fatal("fork failed");
	/* NOTREACHED */

    case 0:     /* child */
        close(pfd[1]);
	if ((fd = open(LOGFILE, O_APPEND | O_WRONLY | O_CREAT, 0666)) < 0) {
	    warning("Can't open log file %s", LOGFILE);
	    return;
	}
	for (;;) {
	    int r;

	    switch (r = read(pfd[0], buf, sizeof(buf))) {

	    case -1:
		fatal("Error reading from pipe");
		/* NOTREACHED */

	    case 0:
		close(fd);
		exit(0);

	    default:
		write(2, buf, r);
		if (write(fd, buf, r) != r) {
		    warning("Error writing to %s", LOGFILE);
		}
		continue;
	    }
	}
	/* NOTREACHED */

    default:    /* parent */
        close(pfd[0]);
	dup2(pfd[1], 2);        /* write all errors to log */
	break;
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * quote_string --
 *
 *	Expand all control characters in a string to the equivalent
 *      backslashed excape sequence.  For instance, a tab is converted
 *      to the two characters '\' and 't'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modified the destination string.
 *
 *----------------------------------------------------------------------
 */

static void
quote_string(to, from)
    char *to;
    const char *from;
{
    int c;

    for (;;) {
	switch (c = *from++) {

	case '\0':
	    *to = '\0';
	    return;

	case '\\':
	    *to++ = '\\';
	    *to++ = '\\';
	    break;

	case '\n':
	    *to++ = '\\';
	    *to++ = 'n';
	    break;

	case '\t':
	    *to++= '\\';
	    *to++= 't';
	    break;

	case '\f':
	    *to++= '\\';
	    *to++= 'f';
	    break;

	case '\b':
	    *to++= '\\';
	    *to++= 'b';
	    break;

	default:
	    if (c < 0x20) {
		*to++ = '\\';
		*to++ = '0' + ((c >> 3) & 7);
		*to++ = '0' + (c & 7);
		break;
	    } else {
		*to++ = c;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * checkTape --
 *
 *	Check the dumpdates file to make sure it is safe to initialize
 *	the tape.  We do this by grepping through the last 100 lines
 *	of the dumpdates file to see if the tape has been used.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will exit with failure if it is not safe.
 *
 *----------------------------------------------------------------------
 */

static void
checkTape()
{
    char buf[1000];
    int status;

    sprintf(buf,"tail -100 %s | grep \"^%3.3d\" > /dev/null", DUMPDATES,
	    tapeNumber);
    status = system(buf);
    if (status==0) {
	fprintf(stderr,"dump: tape %d recently used; init failed\n",
		tapeNumber);
    } else if (status==256 || status==1) {
	return;
    } else {
	fprintf(stderr,"dump: unable to do dumpdates safety check\n");
    }
    exit(123);
}

/*
 *----------------------------------------------------------------------
 *
 * gotoEOD --
 *
 *	Move tape to end-of-data.	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The tape is moved.
 *
 *----------------------------------------------------------------------
 */

static void
gotoEOD()
{
    Dev_TapeCommand 	args;
    ReturnStatus	status;
    int			files;


    debugp((stderr, "skipping to end of data\n"));
    args.command = IOC_TAPE_SKIP_EOD;
    args.count = 0;
    status = Fs_IOControl(archivefd, IOC_TAPE_COMMAND, sizeof(args),
	(char *) &args, 0, NULL);
    if (status != SUCCESS) {
	warning("Skip to end of data returned 0x%x", status);
	/*
	 * If we are doing a restore then we want to do our best to
	 * find the end of usable data. Otherwise we return an error.
	 */
	fprintf(stderr, "Trying to find last file on tape\n");
	files = 0;
	rewindTape();
	/*
	 * Skip over initial label.
	 */
	status = skipFiles(1);
	if (status == SUCCESS) {
	    files++;
	    /*
	     * Now skip over pairs of dump files and labels. 
	     */
	    while(1) {
		status = skipFiles(2);
		if (status != SUCCESS) {
		    break;
		}
		files += 2;
	    }
	}
	fprintf(stderr, "Tape ends with file %d\n", files);
	rewindTape();
	status = skipFiles(files);
	if (status != SUCCESS) {
	    fatal("Can't find end of tape");
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * skipFiles --
 *
 *	Skip the given number of files.
 *
 * Results:
 *	Return status from the ioctl.
 *
 * Side effects:
 *	The tape is moved around.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
skipFiles(num)
    int		num;	/* Number of files to skip. */
{
    ReturnStatus	status;
    Dev_TapeCommand 	args;

    debugp((stderr, "skipping %d files\n", num));
    args.command = IOC_TAPE_SKIP_FILES;
    args.count = num;
    status = Fs_IOControl(archivefd, IOC_TAPE_COMMAND, sizeof(args),
			  (char *) &args, 0, (char *) 0);
    return status;
}
