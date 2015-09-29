/*
 * fsattach.h --
 *
 *	Declarations for fsattach.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/admin/fsattach/RCS/fsattach.h,v 1.7 91/01/12 16:48:54 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _FSATTACH
#define _FSATTACH

#include <assert.h>
#include <errno.h>
#include <fs.h>
#include <fsCmd.h>
#include <host.h>
#include <list.h>
#include <option.h>
#include <stdio.h>
#include <string.h>
#include <status.h>
#include <stdlib.h>
#include <sysStats.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <disk.h>

/* constants */

/*
 * Maximum limits on things.
 */
#define MAX_FIELD_LENGTH	256
#define MAX_LINE_LENGTH		1024
#define MAX_EXEC_ARGS		20
#define MAX_PASS 		10

/*
 * Return codes from fscheck.
 */
#define FSCHECK_OK		(char) 0
#define FSCHECK_SOFT_ERROR	(char) 1
#define FSCHECK_OUT_OF_MEMORY	(char) 2
#define FSCHECK_NOREBOOT	(char) 3
#define FSCHECK_REBOOT		(char) 4

#define FSCHECK_HARD_ERROR	(char) -1
#define FSCHECK_READ_FAILURE	(char) -2
#define FSCHECK_WRITE_FAILURE	(char) -3
#define FSCHECK_BAD_ARG		(char) -4
#define FSCHECK_MORE_MEMORY	(char) -5
#define FSCHECK_DISK_FULL	(char) -6

/*
 * Return code from child process if the exec fails.
 */
#define EXEC_FAILED		(char) 32

/* 
 * Exit codes.
 */
#define OK 		0
#define REBOOT		1
#define HARDERROR 	2
#define SOFTERROR	3
#define NOREBOOT	4

/* 
 * Status of entry in mount table. Starts off as CHILD_OK, changes to 
 * CHILD_RUNNING while fscheck is running. If fscheck completes ok, 
 * then status changes back to CHILD_OK to indicate that the prefix should 
 * be attached. Otherwise the status changes to CHILD_FAILURE.
 */
#define CHILD_OK	0
#define CHILD_RUNNING	1
#define CHILD_FAILURE	2


/* data structures */


/*
 * Information stored about active child processes.
 */
typedef struct {
    int		pid;		/* process id of child */
    int		mountIndex;	/* index in mount table that child corresponds
				 * to */
} ChildInfo;

/*
 * Used to build a linked list of arguments.
 */
typedef struct {
    List_Links	links;		/* Used to make a list */
    char	*arg;		/* Ptr to argument string */
} ArgHeader;


/*
 * Information about arguments to fscheck. We build up a table of these
 * for devices we haven't seen yet in the mount table. This allows "Arg"
 * commands for a device to precede the "Attach" or "Export" command.
 * Also arguments for "all" devices are kept in the table until the
 * entire mount file is parsed.
 */
typedef struct {
    char	source[MAX_FIELD_LENGTH]; 	/* name of device arguments
						 * are for */
    int		line;				/* line number of "args" cmd */
    ArgHeader	argList;			/* list of arguments */
} ArgInfo;

/* 
 * Entry in the mount table.
 */
typedef struct {
    char	source[MAX_FIELD_LENGTH];	/* name of source device or
						 * prefix */
    char 	dest[MAX_FIELD_LENGTH];		/* name of destination prefix */
    int		group;				/* check group of device */
    Boolean	device;				/* TRUE => source is a device */
    Boolean	export;				/* TRUE => export prefix */
    Boolean	readonly;			/* TRUE => prefix should be
						 * mounted as read only */
    Boolean	doCheck;			/* device should be checked */
    int		status;				/* status of this entry */
    ArgInfo	argInfo;			/* arguments to fscheck */
    Boolean	checked;			/* TRUE => fscheck was run */
} MountInfo;

/*
 * Used to store information about groups (disks that cannot be checked
 * at the same time.
 */

typedef struct {
    char	name[MAX_FIELD_LENGTH];		/* name of group */
    Boolean	running;			/* is there an fscheck
						 * running for a device in
						 * in this group? */
} GroupInfo;

/*
 * Used to allocate memory.
 */
#define Alloc(ptr,type, number, msg) { \
	(ptr) = (type *) malloc((unsigned) (sizeof(type) * (number))); \
	if ((ptr) == NULL) { \
	    (void) fprintf(stderr, "Out of memory: %s.\n",msg); \
	    (void) exit(HARDERROR); \
	} \
    }

/* 
 * Handy comparison macros.
 */
#define min(a,b) (((a) < (b)) ? (a) : (b) )
#define max(a,b) (((a) > (b)) ? (a) : (b) )

/*
 * Global variables.
 */
extern char		*progName;
extern int		mountTableSize;
extern int		mountTableSizeIncrement;
extern char		*tempOutputFile;
extern char		*heapLimitString;
extern int		tempOutputFileSize;
extern char		*outputDir;
extern Boolean		verbose;
extern MountInfo	*mountTable;
extern Boolean		printOnly;
extern char		*fscheckPath;
extern char		*bootPrefix;
extern int		numGroups;
extern GroupInfo	*groupInfo;
extern int		groupInfoSize;
extern int		groupInfoSizeIncrement;
extern int		debug;

/*
 * Global functions.
 */
extern	void		GetRecoveryInfo();
extern	ReturnStatus	ParseMount();
extern	ReturnStatus	CheckDisks();
extern	void		CacheWriteBack();
extern	ReturnStatus	RunChild();
extern	void		StartExec();
extern	void		AddExecArgs();
extern	int		DoExec();
extern	void		MoveRootOutput();
extern	ReturnStatus	ParseConfig();
extern  void		Prefix();
extern  void		PrintFscheckError();
extern	void		AddList();
extern	void		MergeList();
extern	void		DeleteList();
extern  void		PreloadPrefixTable();
extern	char		*GetAttachName();
extern void		bzero();
extern int		exit();

#endif /* _FSATTACH */

