/* 
 * recovinfo.c --
 *
 *	Put together and print out statistics for recovery purposes such
 *	as the number of file handles in the system file handle table, etc.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/recovinfo/RCS/recovinfo.c,v 1.6 90/09/19 09:22:13 mgbaker Exp Locker: mgbaker $ SPRITE (Berkeley)";
#endif /* not lint */

#include	"sprite.h"
#include	"status.h"
#include	"stdio.h"
#include	"list.h"
#include	"hash.h"
#include	"option.h"
#include	"sysStats.h"
#include	"fs.h"
#include	"host.h"

/*
 * The stupid decstation compiler can't handle field names in structures that
 * are the same as macro names.
 */
#ifdef ds3100
#undef major
#undef minor
#endif /* ds3100 */

/*
 * From kernel/fsio.h
 */
#define	FSIO_NUM_STREAM_TYPES	20

/*
 * The format of the information returned from the sys stats system call.
 */
typedef	struct	goo {
    Fs_FileID           fileID;			/* Unique file ID. */
    Boolean		streamHandle;		/* Is this a stream handle? */
    int                 mode;			/* Mode on stream. */
    int                 refCount;		/* Ref count on IO handle. */
    int                 streamRefCount;		/* Ref count on stream. */
    int                 numBlocks;		/* Number of blocks in cache. */
    int                 numDirtyBlocks;		/* Number of diry blocks. */
    char     		name[50];		/* Name of object. */
} Sys_FsRecovNamedStats;

/*
 * The io types from kernel/fsio.h.
 */
#define FSIO_STREAM                     0
#define FSIO_LCL_FILE_STREAM            1
#define FSIO_RMT_FILE_STREAM            2
#define FSIO_LCL_DEVICE_STREAM          3
#define FSIO_RMT_DEVICE_STREAM          4
#define FSIO_LCL_PIPE_STREAM            5
#define FSIO_RMT_PIPE_STREAM            6
#define FSIO_CONTROL_STREAM             7
#define FSIO_SERVER_STREAM              8
#define FSIO_LCL_PSEUDO_STREAM          9
#define FSIO_RMT_PSEUDO_STREAM          10
#define FSIO_PFS_CONTROL_STREAM         11
#define FSIO_PFS_NAMING_STREAM          12
#define FSIO_LCL_PFS_STREAM             13
#define FSIO_RMT_PFS_STREAM             14
#define FSIO_RMT_CONTROL_STREAM         15
#define FSIO_PASSING_STREAM             16
#define FSIO_RAW_IP_STREAM              17
#define FSIO_UDP_STREAM                 18
#define FSIO_TCP_STREAM                 19

/*
 * Hash table of result values, using the unique file ID (Fs_FileID) as
 * a key.  This is okay as long as the ID is a multiple of sizeof (int)!
 */
typedef	struct	oodle	{
    List_Links			streamList;
    Sys_FsRecovNamedStats	*statPtr;
} StatHash;	

/*
 * List of streams per handle.
 */
typedef	struct	doodle	{
    List_Links			streamList;
    Sys_FsRecovNamedStats	*statPtr;
} StatList;

/*
 * Hash table definitions.
 */
Hash_Table	fileHashTableStruct;
Hash_Table	*fileHashTable = &fileHashTableStruct;
#define		FS_HANDLE_TABLE_SIZE_START	600
int		statKeyType[4];

/*
 * The array and number of elements returned from the recov info sys call.
 */
int			numStats = FS_HANDLE_TABLE_SIZE_START;
Sys_FsRecovNamedStats	*stats = (Sys_FsRecovNamedStats *) NULL;

/*
 * Summary statistics.
 */
typedef struct	pfeffernusse {
    int				numFileIOHandles;
    int				numFilesNoStream;
    int				numFilesNoStreamAndNotDirty;
    int				numFilesNoStreamAndNotCached;
    int				numWithDirtyBlocks;
    int				totalNumHandlesRecovered;
} Summary;

/*
 * Declarations for server list of summary information.
 */
typedef	struct	wiggle {
    List_Links		serverList;
    int			id;
    char		name[50];
    Summary		summary;
} ServerList;

/*
 * Option declarations.
 */
Boolean		verbose = FALSE;
Boolean		printFileID = FALSE;
Boolean		printStreamInfo = FALSE;
Boolean		printStreamNames = FALSE;
Boolean		printNames = FALSE;
Boolean		filesOnly = FALSE;
Boolean		allTypes = FALSE;
Boolean		debug = FALSE;
char		*server = (char *) NULL;

Option		optionArray[] = {
    {OPT_TRUE, "v", (char *) &verbose, "verbose mode"},
    {OPT_TRUE, "verbose", (char *) &verbose, "verbose mode"},
    {OPT_TRUE, "fileID", (char *) &printFileID, "fileID info"},
    {OPT_TRUE, "fileid", (char *) &printFileID, "fileID info"},
    {OPT_TRUE, "streamInfo", (char *) &printStreamInfo, "stream info"},
    {OPT_TRUE, "streaminfo", (char *) &printStreamInfo, "stream info"},
    {OPT_TRUE, "streamNames", (char *) &printStreamNames, "stream names"},
    {OPT_TRUE, "streamnames", (char *) &printStreamNames, "stream names"},
    {OPT_TRUE, "filesOnly", (char *) &filesOnly, "only local and remote files"},
    {OPT_TRUE, "filesonly", (char *) &filesOnly, "only local and remote les"},
    {OPT_TRUE, "files", (char *) &filesOnly, "only local and remote files"},
    {OPT_TRUE, "all", (char *) &allTypes, "include non-recovered handle types"},
    {OPT_TRUE, "names", (char *) &printNames, "names, cache and IOhandle info"},
    {OPT_TRUE, "debug", (char *) &debug, "Go into the debugger on errors"},
    {OPT_STRING, "server", (char *) &server, "recovery info for objects on this server"}
};
int		numOptions = sizeof (optionArray) / sizeof (Option);

extern	char	*GetModeString();
extern	char	*GetFileTypeString();
extern	void	Init();
extern	void	Install();
extern	void	PrintObject();
extern	void	PrintHeader();
extern	char	*mktemp();


main(argc, argv)
    int		argc;
    char	*argv[];
{
    ReturnStatus		status;
    char			*fileType;
    char			*mode;
    int				i;
    int				bufSize;
    Hash_Search			search;
    Hash_Entry			*entryPtr;
    List_Links			*itemPtr;
    StatList			*streamItemPtr;
    StatHash			*valuePtr;
    FILE			*outputFile;
    int				streamRefCount;
    int				modeInt;
    int				length;
    int				maxLength = 0;
    Boolean			printExtraStuff = FALSE;
    int				myID;
    int				serverID;
    Host_Entry			*serverInfoPtr;
    List_Links			serverList;
    ServerList			*serverItemPtr;
    Summary			total;
    char			tmpFile[20];
    char			systemString[30];
#define	IncSummary(fieldName, serverItemPtr)		\
    total.fieldName++;					\
    if (serverItemPtr != (ServerList *) NULL) {		\
	serverItemPtr->summary.fieldName++;		\
    }


    /*
     * Parse arguments.
     */
    Opt_Parse(argc, argv, optionArray, numOptions, 0);

    /*
     * Grab stat info from kernel.
     */
    bufSize = numStats * sizeof (Sys_FsRecovNamedStats);
    stats = (Sys_FsRecovNamedStats *) malloc(bufSize);
    status = Sys_Stats(SYS_FS_RECOV_INFO, &bufSize, stats);

    /*
     * Did we need more space?
     */
    while (status == SUCCESS && (bufSize >
	    numStats * sizeof (Sys_FsRecovNamedStats))) {
	numStats = bufSize / sizeof (Sys_FsRecovNamedStats);

	if (stats != (Sys_FsRecovNamedStats *) NULL) {
	    free(stats);
	}
	stats = (Sys_FsRecovNamedStats	*) malloc(bufSize);
	status = Sys_Stats(SYS_FS_RECOV_INFO, &bufSize, stats);
    }

    if (status != SUCCESS) {
	if (stats != (Sys_FsRecovNamedStats *) NULL) {
	    free(stats);
	}
	printf("Stat call failed, status = 0x%x\n", status);
	exit(1);
    }
	

    /*
     * Clear total summary statistics.
     */
    bzero(&total, sizeof (total));

    /* Init hash table of stats */
    Init();

    /* Install stuff in hash table */
    for (i = 0; i < numStats; i++) {
	Install(&stats[i]);
    }

    /* open temp file */
    strcpy(tmpFile, "/tmp/tempXXXXXX");
    if (mktemp(tmpFile) != tmpFile) {
	panic("Unable to open output file.\n");
    }
    outputFile = fopen(tmpFile, "w+");

    if (outputFile == NULL) {
	panic("Couldn't open output file.\n");
    }

    /*
     * The stream ref count is the ref count on stream header, not io handle.
     * An open gives you a new stream (and hence refcount on the io handle).
     * A fork or dup gives you a new ref count on the stream.
     */

    /*
     * Do we need to print anything other than a summary?
     */
    if (verbose || printFileID || printStreamInfo || printNames) {
	printExtraStuff = TRUE;
    } else {
	printExtraStuff = FALSE;
    }

    status = Proc_GetHostIDs((int *) NULL, &myID);
    if (status != SUCCESS) {
	panic("Couldn't get my host id.\n");
	exit(1);
    }

    if (printExtraStuff) {
	/* calculate longest file name for spacing. */
	for (entryPtr = Hash_EnumFirst(fileHashTable, &search);
		entryPtr != NULL; entryPtr = Hash_EnumNext(&search)) {
	    valuePtr = (StatHash *) Hash_GetValue(entryPtr);
	    if (valuePtr->statPtr == (Sys_FsRecovNamedStats *) NULL) {
		printf("Nil statPtr for stream.\n");
		fflush(stdout);
		continue;
	    }
	    length = strlen(valuePtr->statPtr->name);
	    if (length > maxLength) {
		maxLength = length;
	    }
	}
    }

    /* print header to file */
    if (printExtraStuff) {
	PrintHeader(&maxLength);
    }

    /* Gather info with respect to only one server? */
    if (server != (char *) NULL) {
	serverInfoPtr = Host_ByName(server);
	Host_End();
	serverID = serverInfoPtr->id;
    } else {
	List_Init(&serverList);
    }

    /*
     * Go through hash table of handles, gathering information.
     */
    for (entryPtr = Hash_EnumFirst(fileHashTable, &search);
	    entryPtr != NULL; entryPtr = Hash_EnumNext(&search)) {
	valuePtr = (StatHash *) Hash_GetValue(entryPtr);
	if (valuePtr->statPtr == (Sys_FsRecovNamedStats *) NULL) {
	    printf("Nil statPtr for stream.\n");
	    fflush(stdout);
	    continue;
	}
	if (valuePtr->statPtr->fileID.type == FSIO_STREAM && debug) {
	    panic("Stream type found at first level in table.\n");
	} else if (valuePtr->statPtr->fileID.type == FSIO_STREAM) {
	    printf("Stream type found at first level in table.\n");
	}

	/*
	 * Only remote handles would be recovered.  Is this remote?
	 */
	if (!allTypes && valuePtr->statPtr->fileID.serverID == myID) {
	    continue;
	}

	serverItemPtr = (ServerList *) NULL;
	/*
	 * Are we gathering info for only one server?  Is this the one?
	 */
	if (server != NULL && serverID != valuePtr->statPtr->fileID.serverID) {
	    continue;
	/*
	 * Are we gathering info for all servers?  Then find or add this one
	 * in the list.
	 */
	} else if (server == (char *) NULL) {
	    List_Links	*sPtr;
	    Boolean	foundServer;

	    foundServer = FALSE;
	    LIST_FORALL(&serverList, sPtr) {
		serverItemPtr = (ServerList *) sPtr;
		if (serverItemPtr->id == valuePtr->statPtr->fileID.serverID) {
		    foundServer = TRUE;
		    break;
		}
	    }
	    if (!foundServer) {
		/* Add new server to list. */
		serverItemPtr = (ServerList *) malloc(sizeof (ServerList));
		List_InitElement((List_Links *) serverItemPtr);
		List_Insert((List_Links *) serverItemPtr,
			LIST_ATREAR(&serverList));
		bzero(&(serverItemPtr->summary),
			sizeof (serverItemPtr->summary));
		serverItemPtr->id = valuePtr->statPtr->fileID.serverID;

		serverInfoPtr = Host_ByID(serverItemPtr->id);
		Host_End();
		if (serverInfoPtr == (Host_Entry *) NULL) {
		    panic("Couldn't get info for a server.\n");
		}
		/*
		 * Try to get names without .Berkeley.EDU on the ends.
		 */
		if (serverInfoPtr->aliases != NULL &&
			serverInfoPtr->aliases[0] != NULL) {
		    strncpy(serverItemPtr->name, serverInfoPtr->aliases[0],
			    sizeof (serverItemPtr->name) - 1);
		} else {
		    strncpy(serverItemPtr->name, serverInfoPtr->name,
			    sizeof (serverItemPtr->name) - 1);
		    serverItemPtr->name[sizeof(serverItemPtr->name) - 1] = '\0';
		}
	    }
	}
	IncSummary(totalNumHandlesRecovered, serverItemPtr);
	fileType = GetFileTypeString(valuePtr->statPtr->fileID.type);
	/*
	 * Do statistics specific to files.
	 */
	if (valuePtr->statPtr->fileID.type == FSIO_LCL_FILE_STREAM ||
		valuePtr->statPtr->fileID.type == FSIO_RMT_FILE_STREAM) {
	    IncSummary(numFileIOHandles, serverItemPtr);
	    if (valuePtr->statPtr->refCount == 0) {
		IncSummary(numFilesNoStream, serverItemPtr);
		if (valuePtr->statPtr->numBlocks == 0) {
		    IncSummary(numFilesNoStreamAndNotCached, serverItemPtr);
		}
		if (valuePtr->statPtr->numDirtyBlocks == 0) {
		    IncSummary(numFilesNoStreamAndNotDirty, serverItemPtr);
		}
		LIST_FORALL(&(valuePtr->streamList), itemPtr) {
		    /* I shouldn't find anything here! */
		    if (debug) {
			panic(
			"A stream found for an object with no refCount.\n");
		    } else {
			printf(
			"A stream found for an object with no refCount.\n");
		    }
		}
	    }
	    if (valuePtr->statPtr->numDirtyBlocks > 0) {
		IncSummary(numWithDirtyBlocks, serverItemPtr);
	    }
	}
	/*
	 * Go through loop and acquire top mode value and streamRefCount
	 * (or is streamRefCount interesting to anybody?).
	 */
	modeInt = valuePtr->statPtr->mode;
	streamRefCount = valuePtr->statPtr->streamRefCount;
	LIST_FORALL(&(valuePtr->streamList), itemPtr) {
	    streamItemPtr = (StatList *) itemPtr;
	    modeInt |= streamItemPtr->statPtr->mode++;
	    streamRefCount += streamItemPtr->statPtr->streamRefCount;
	    if (streamItemPtr->statPtr->fileID.serverID != myID) {
		IncSummary(totalNumHandlesRecovered, serverItemPtr);
	    }
	}
	mode = GetModeString(modeInt);

	if (printExtraStuff) {
	    PrintObject(outputFile, valuePtr, maxLength, fileType, mode);
	}
    }
    fclose(outputFile);
    sprintf(systemString, "sort %s\n", tmpFile);
    system(systemString);

    printf("\n");
    fflush(stdout);
    system("echo SUMMARY `hostname` `date`");
    printf("\n");
    fflush(stdout);
    if (server != (char *) NULL) {
	printf("Server:\t\t\t\t%s\n", server);
	printf("File I/O handles:\t\t%d\n",
		total.numFileIOHandles);
	printf("Files with no stream:\t\t%d\n",
		total.numFilesNoStream);
	printf("No stream nor cache blocks:\t%d\n",
		total.numFilesNoStreamAndNotCached);
	printf("No stream nor dirty blocks:\t%d\n",
		total.numFilesNoStreamAndNotDirty);
	printf("Files with dirty blocks:\t%d\n",
		total.numWithDirtyBlocks);
	printf("Total handles to recover:\t%d\n",
		total.totalNumHandlesRecovered);
    } else {
	List_Links	*sPtr;

	printf("Server:\t\t\t\t");
	LIST_FORALL(&serverList, sPtr) {
	    serverItemPtr = (ServerList *) sPtr;
	    printf("%s\t", serverItemPtr->name);
	}
	printf("combined\n");
	printf("File I/O handles:\t\t");
	LIST_FORALL(&serverList, sPtr) {
	    serverItemPtr = (ServerList *) sPtr;
	    printf("%d\t", serverItemPtr->summary.numFileIOHandles);
	    if (strlen(serverItemPtr->name) >= 8) {
		printf("\t");
	    }
	}
	printf("%d\n", total.numFileIOHandles);
	printf("Files with no stream:\t\t");
	LIST_FORALL(&serverList, sPtr) {
	    serverItemPtr = (ServerList *) sPtr;
	    printf("%d\t", serverItemPtr->summary.numFilesNoStream);
	    if (strlen(serverItemPtr->name) >= 8) {
		printf("\t");
	    }
	}
	printf("%d\n", total.numFilesNoStream);
	printf("No stream nor cache blocks:\t");
	LIST_FORALL(&serverList, sPtr) {
	    serverItemPtr = (ServerList *) sPtr;
	    printf("%d\t", serverItemPtr->summary.numFilesNoStreamAndNotCached);
	    if (strlen(serverItemPtr->name) >= 8) {
		printf("\t");
	    }
	}
	printf("%d\n", total.numFilesNoStreamAndNotCached);
	printf("No stream nor dirty blocks:\t");
	LIST_FORALL(&serverList, sPtr) {
	    serverItemPtr = (ServerList *) sPtr;
	    printf("%d\t", serverItemPtr->summary.numFilesNoStreamAndNotDirty);
	    if (strlen(serverItemPtr->name) >= 8) {
		printf("\t");
	    }
	}
	printf("%d\n", total.numFilesNoStreamAndNotDirty);
	printf("Files with dirty blocks:\t");
	LIST_FORALL(&serverList, sPtr) {
	    serverItemPtr = (ServerList *) sPtr;
	    printf("%d\t", serverItemPtr->summary.numWithDirtyBlocks);
	    if (strlen(serverItemPtr->name) >= 8) {
		printf("\t");
	    }
	}
	printf("%d\n", total.numWithDirtyBlocks);
	printf("Total handles to recover:\t");
	LIST_FORALL(&serverList, sPtr) {
	    serverItemPtr = (ServerList *) sPtr;
	    printf("%d\t", serverItemPtr->summary.totalNumHandlesRecovered);
	    if (strlen(serverItemPtr->name) >= 8) {
		printf("\t");
	    }
	}
	printf("%d\n", total.totalNumHandlesRecovered);
    }

    fflush(stdout);

    (void) unlink(tmpFile);
    exit(0);
}



/*
 *----------------------------------------------------------------------
 *
 * GetFileTypeString --
 *
 *	Get a string representing file type.
 *
 * Results:
 *	The string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
char *
GetFileTypeString(type)
    int	type;
{
    char	*fileType;

    switch (type) {
    case FSIO_STREAM:
	if (debug) {
	    panic("Stream type found at first level in table.\n");
	} else {
	    printf("Stream type found at first level in table.\n");
	}
    case FSIO_LCL_FILE_STREAM:
	fileType = "File";
	break;
    case FSIO_RMT_FILE_STREAM:
	fileType = "RmtFile";
	break;
    case FSIO_LCL_DEVICE_STREAM:
	fileType = "Device";
	break;
    case FSIO_RMT_DEVICE_STREAM:
	fileType = "RmtDevice";
	break;
    case FSIO_LCL_PIPE_STREAM:
	fileType = "Pipe";
	break;
    case FSIO_RMT_PIPE_STREAM:
	fileType = "RmtPipe";
	break;
#ifdef notdef
    case FS_LCL_NAMED_PIPE_STREAM:
	fileType = "NamedPipe";
	break;
    case FS_RMT_NAMED_PIPE_STREAM:
	fileType = "RmtNamedPipe";
	break;
#endif
    case FSIO_CONTROL_STREAM:
	fileType = "PdevControlStream";
	break;
    case FSIO_SERVER_STREAM:
	fileType = "SrvStream";
	break;
    case FSIO_LCL_PSEUDO_STREAM:
	fileType = "LclPdev";
	break;
    case FSIO_RMT_PSEUDO_STREAM:
	fileType = "RmtPdev";
	break;
    case FSIO_PFS_CONTROL_STREAM:
	fileType = "PfsControlStream";
	break;
    case FSIO_PFS_NAMING_STREAM:
	fileType = "PfsNamingStream";
	break;
    case FSIO_LCL_PFS_STREAM:
	fileType = "LclPfs";
	break;
    case FSIO_RMT_PFS_STREAM:
	fileType = "RmtPfs";
	break;
#ifdef INET
    case FSIO_RAW_IP_STREAM:
	fileType = "RawIp Socket";
	break;
    case FSIO_UDP_STREAM:
	fileType = "UDP Socket";
	break;
    case FSIO_TCP_STREAM:
	fileType = "TCP Socket";
	break;
#endif
#ifdef notdef
    case FS_RMT_UNIX_STREAM:
	fileType = "UnixFile";
	break;
    case FS_RMT_NFS_STREAM:
	fileType = "NFSFile";
	break;
#endif
    default:
	/*
	 * Brent puts <>'s around this in all his error printing.  But without
	 * them, it's the same length as the longest file type, which is
	 * convenient for printing.  And we should never see this anyway.
	 */
	fileType = "unknown file type";
	break;
    }

    return fileType;
}

int
MaxFileTypeNameLength()
{
    return strlen("PdevControlStream");
}



/*
 *----------------------------------------------------------------------
 *
 * GetModeString --
 *
 *	Get a string representing mode.
 *
 * Results:
 *	The string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
char *
GetModeString(modeInt)
    int	modeInt;
{
    char	*mode;

    if ((modeInt & FS_READ) && (modeInt & FS_WRITE) && (modeInt & FS_EXECUTE) &&
	    (modeInt & FS_APPEND)) {
	mode = "RWXA";
    } else if ((modeInt & FS_READ) && (modeInt & FS_WRITE) &&
	    (modeInt & FS_EXECUTE)) {
	mode = "RWX";
    } else if ((modeInt & FS_READ) && (modeInt & FS_WRITE) &&
	    (modeInt & FS_APPEND)) {
	mode = "RWA";
    } else if ((modeInt & FS_READ) && (modeInt & FS_WRITE)) {
	mode = "RW";
    } else if ((modeInt & FS_READ) && (modeInt & FS_EXECUTE)) {
	mode = "RX";
    } else if ((modeInt & FS_READ) && (modeInt & FS_APPEND)) {
	mode = "RA";
    } else if (modeInt & FS_READ) {
	mode = "R";
    } else if ((modeInt & FS_WRITE) && (modeInt & FS_EXECUTE) &&
	    (modeInt & FS_APPEND)) {
	mode = "WXA";
    } else if ((modeInt & FS_WRITE) && (modeInt & FS_EXECUTE)) {
	mode = "WX";
    } else if ((modeInt & FS_WRITE) && (modeInt & FS_APPEND)) {
	mode = "WA";
    } else if (modeInt & FS_WRITE) {
	mode = "W";
    } else if ((modeInt & FS_EXECUTE) && (modeInt & FS_APPEND)) {
	mode = "XA";
    } else if (modeInt & FS_EXECUTE) {
	mode = "X";
    } else if (modeInt & FS_APPEND) {
	mode = "A";
    } else {
	mode = "none";
    }

    return mode;
}


/*
 *----------------------------------------------------------------------
 *
 * Init --
 *
 *	Initialize the hash table of statistics.   We hash on the fileID of
 *	of the fileHandle info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hash table created.
 *
 *----------------------------------------------------------------------
 */
void
Init()
{
    Hash_InitTable(fileHashTable, numStats,
	    sizeof (statKeyType) / sizeof (int)); 
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * Install --
 *
 *	Install and entry into the hash table of statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New entry installed if it wasn't previously there.
 *
 *----------------------------------------------------------------------
 */
void
Install(statPtr)
    Sys_FsRecovNamedStats	*statPtr;
{
    Hash_Entry		*hashEntryPtr;
    Boolean		newP;
    StatHash		*valuePtr;
    StatList		*itemPtr;

    /*
     * For a key, use everything except the type.
     */
    if (statPtr->name[0] == '\0' && statPtr->fileID.serverID == 0 &&
	    statPtr->fileID.major == 0 && statPtr->fileID.minor == 0) {
	return;
    }
    statKeyType[0] = statPtr->fileID.serverID;
    statKeyType[1] = statPtr->fileID.major;
    statKeyType[2] = statPtr->fileID.minor;
    statKeyType[3] = statPtr->fileID.type;

    hashEntryPtr = Hash_CreateEntry(fileHashTable, (Address) statKeyType,
	    &newP);
    if (newP) {
	valuePtr = (StatHash *) malloc(sizeof (StatHash));
	Hash_SetValue(hashEntryPtr, (Address) valuePtr);
	valuePtr->statPtr = (Sys_FsRecovNamedStats *) NULL;
	List_Init(&(valuePtr->streamList));
    } else {
	valuePtr = (StatHash *) Hash_GetValue(hashEntryPtr);
    }

    if (! statPtr->streamHandle) {
	/* Make into base object */
	if (valuePtr->statPtr == (Sys_FsRecovNamedStats *) NULL) {
	    valuePtr->statPtr = statPtr;
	} else if (debug) {
	    panic("More than one IO handle object for this ID!\n");
	} else {
	    printf("More than one IO handle object for this ID!\n");
	}
    } else {
	/* Add stream to stream list */
	itemPtr = (StatList *) malloc(sizeof (StatList));
	itemPtr->statPtr = statPtr;
	List_InitElement((List_Links *) itemPtr);
	List_Insert(itemPtr, LIST_ATREAR(&(valuePtr->streamList)));
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * PrintHeader --
 *
 *	Print out a header for object information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The length of maxLength may be increased.
 *
 *----------------------------------------------------------------------
 */
void
PrintHeader(maxLengthPtr)
    int		*maxLengthPtr;
{
    int		i;

    if (*maxLengthPtr < strlen("Name")) {
	*maxLengthPtr = strlen("Name");
    }
    printf("Name");
    for (i = 0; i < *maxLengthPtr - strlen("Name") + 2; i++) {
	printf(" ");
    }
    printf("Type");
    for (i = 0; i < MaxFileTypeNameLength() - strlen("Type") + 2; i++) {
	printf(" ");
    }
    if (verbose || printFileID) {
	printf("FileID");
	for (i = 0; i < strlen("(xx,xxxxxx,xxxxxxxx)") - strlen("FileID") + 2;
		i++) {
	    printf(" ");
	}
    }
    if (verbose || printStreamInfo) {
	printf("Mode  Dup/ForkCount  ");
    }
    printf("RefCount  NumBlocks  NumDirtyBlocks\n");
    fflush(stdout);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * PrintObject --
 *
 *	Print information for one file system object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
PrintObject(outputFile, valuePtr, maxLength, fileType, mode)
    FILE	*outputFile;
    StatHash	*valuePtr;
    int		maxLength;
    char	*fileType;
    char	*mode;

{
    int		i;
    List_Links	*itemPtr;
    StatList	*streamItemPtr;

    if (filesOnly && (valuePtr->statPtr->fileID.type != FSIO_LCL_FILE_STREAM &&
	    valuePtr->statPtr->fileID.type != FSIO_RMT_FILE_STREAM)) {
	return;
    }
    fprintf(outputFile, valuePtr->statPtr->name);
    for (i = 0; i < maxLength - strlen(valuePtr->statPtr->name) + 2; i++) {
	fprintf(outputFile, " ");
    }
    fprintf(outputFile, fileType);
    for (i = 0; i < MaxFileTypeNameLength() - strlen(fileType) + 2; i++) {
	fprintf(outputFile, " ");
    }
    if (verbose || printFileID) {
	fprintf(outputFile, "(%-2d,%6x,%8x)",
		valuePtr->statPtr->fileID.serverID,
		valuePtr->statPtr->fileID.major,
		valuePtr->statPtr->fileID.minor);
	fprintf(outputFile, "  ");
    }
    if (verbose || printStreamInfo) {
	fprintf(outputFile, mode);
	for (i = 0; i < strlen("mode") - strlen(mode) + 2; i++) {
	    fprintf(outputFile, " ");
	}
	fprintf(outputFile, "%d", valuePtr->statPtr->streamRefCount);
	if (valuePtr->statPtr->streamRefCount < 10) {
	    for (i = 0; i < strlen("Dup/ForkCount") + 2 - 1; i++) {
		fprintf(outputFile, " ");
	    }
	} else if (valuePtr->statPtr->streamRefCount < 100) {
	    for (i = 0; i < strlen("Dup/ForkCount") + 2 - 2; i++) {
		fprintf(outputFile, " ");
	    }
	} else if (valuePtr->statPtr->streamRefCount < 1000) {
	    for (i = 0; i < strlen("Dup/ForkCount") + 2 - 3; i++) {
		fprintf(outputFile, " ");
	    }
	} else if (debug) {
	    panic("The stream ref count is too long for my printer routine.\nFix the silly printer routine.\n");
	} else {
	    printf("The stream ref count is too long for my printer routine.\nFix the silly printer routine.\n");
	}
    }

    fprintf(outputFile, "%d", valuePtr->statPtr->refCount);
    if (valuePtr->statPtr->refCount < 10) {
	for (i = 0; i < strlen("RefCount") + 2 - 1; i++) {
	    fprintf(outputFile, " ");
	}
    } else if (valuePtr->statPtr->refCount < 100) {
	for (i = 0; i < strlen("RefCount") + 2 - 2; i++) {
	    fprintf(outputFile, " ");
	}
    } else if (valuePtr->statPtr->refCount < 1000) {
	for (i = 0; i < strlen("RefCount") + 2 - 3; i++) {
	    fprintf(outputFile, " ");
	}
    } else if (debug) {
	panic("The ref count is too long for my printer routine.\nFix the silly printer routine.\n");
    } else {
	printf("The ref count is too long for my printer routine.\nFix the silly printer routine.\n");
    }
    fprintf(outputFile, "%d", valuePtr->statPtr->numBlocks);
    if (valuePtr->statPtr->numBlocks < 10) {
	for (i = 0; i < strlen("NumBlocks") + 2 - 1; i++) {
	    fprintf(outputFile, " ");
	}
    } else if (valuePtr->statPtr->numBlocks < 100) {
	for (i = 0; i < strlen("NumBlocks") + 2 - 2; i++) {
	    fprintf(outputFile, " ");
	}
    } else if (valuePtr->statPtr->numBlocks < 1000) {
	for (i = 0; i < strlen("NumBlocks") + 2 - 3; i++) {
	    fprintf(outputFile, " ");
	}
    } else if (debug) {
	panic("The count of blocks is too long for my printer routine.\nFix the silly printer routine.\n");
    } else {
	printf("The count of blocks is too long for my printer routine.\nFix the silly printer routine.\n");
    }
    fprintf(outputFile, "%d", valuePtr->statPtr->numDirtyBlocks);

    fprintf(outputFile, "\n");

    if (printStreamNames) {
	/* Handle list of streams now, take this out after testing. */
	LIST_FORALL(&(valuePtr->streamList), itemPtr) {
	    streamItemPtr = (StatList *) itemPtr;
	    fprintf(outputFile, "%s", streamItemPtr->statPtr->name);
	    for (i = 0; i < maxLength - strlen(valuePtr->statPtr->name) + 2;
		    i++) {
		fprintf(outputFile, " ");
	    }
	    fprintf(outputFile, "Stream: %s\n",
		    GetFileTypeString(streamItemPtr->statPtr->fileID.type));
	}
    }
    fflush(outputFile);

    return;
}
