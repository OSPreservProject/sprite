/* 
 * fsflatCmd.c --
 *
 *	Commands for fsflat.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: fsflatCmd.c,v 1.1 88/10/03 12:45:42 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sys/types.h>
#include <sys/stat.h>
#include "sx.h"
#include "util.h"
typedef	int	Boolean;
#define	FALSE	0
#define	TRUE	1
#include "monitorClient.h"
#include "fsflatInt.h"

/*
 * Range of chars for which the standard insert binding is to be used,
 * unless overridden by something else.
 */
static	char	insertFirst = 040;
static	char	insertLast = 0176;



/*
 *----------------------------------------------------------------------
 *
 * FsflatBindCmd --
 *
 *	Create (or replace or delete) a keystroke binding.  I should change
 *	this to match mx so that a missing command argument means to return
 *	the enumeration of the bindings with the prefix, rather than to
 *	delete the binding.
 *
 * Syntax:
 *	bind sequence [command]
 *
 * Results:
 *	Returns TCL results.
 *
 * Side effects:
 *	A new keystroke binding gets added to or deleted from the
 *	window's command table.
 *
 *----------------------------------------------------------------------
 */
int
FsflatBindCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    if ((argc != 3) && (argc != 2)) {
	sprintf(interp->result, "%s should be \"%.50s [sequence [command]]\"",
		"wrong number of args:", argv[0]);
	return TCL_ERROR;
    }
    if (argc == 3) {
	Cmd_BindingCreate(aWindow->cmdTable, argv[1], argv[2]);
    } else {
	Cmd_BindingDelete(aWindow->cmdTable, argv[1]);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatChangeDirCmd --
 *
 *	Change the directory of the display.  Force associated shell
 *	window(s) to update current directory also.
 *
 * Syntax:
 *	changeDirectory dirName
 *
 * Results:
 *	Returns TCL_OK if all went well, or various TCL errors if not.
 *
 * Side effects:
 *	Current directory changes.
 *
 *----------------------------------------------------------------------
 */
int
FsflatChangeDirCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    struct	stat	dirAtts;
    char		*newDir;

    if (argc != 2) {
	sprintf(interp->result, "%s \"%.50s dirName\"",
		"wrong # args: should be", argv[0]);
	return TCL_ERROR;
    }
    if ((newDir = Util_CanonicalDir(argv[1], aWindow->dir, (char *) NULL))
	    == NULL) {
	sprintf(interp->result,
		"Couldn't figure out directory name %s.", argv[1]);
	return TCL_ERROR;
    }
    /* Check the validity of the new dir. */
    if (lstat(newDir, &dirAtts) != 0) {
	sprintf(interp->result,
		"Cannot switch to dir %s.  Maybe it doesn't exist?", newDir);
	free(newDir);
	return TCL_ERROR;	/* should above message be passed to routine? */
    }
    if ((dirAtts.st_mode & S_IFMT) != S_IFDIR) {	/* not a directory */
	sprintf(interp->result, "%s is not a directory.", newDir);
	free(newDir);
	return TCL_ERROR;	/* should above message be passed to routine? */
    }
    FsflatChangeDir(aWindow, newDir);	/* does everything */
    free(newDir);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatChangeFieldsCmd --
 *
 *	Change the method of sorting directory entries.
 *
 * Syntax:
 *	changeFields [fields]
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL errors if not.
 *
 * Side effects:
 *	The fields displayed for each file will change.  The user will be
 *	prompted to see if he wishes to make the new sorting method the
 *	default method for the directory.
 *
 *----------------------------------------------------------------------
 */
int
FsflatChangeFieldsCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    int	which;
    int	length;
    int	arg;

    /*
     * any number of args may be okay here, depending on how many fields they
     * wish to display.
     */
    aWindow->displayInstructions = 0;
    if (argc > 1) {
	for (arg = 1; arg < argc; arg++) {
	    length = strlen(argv[arg]);
	    if (strncmp(argv[arg], "name", length) == 0 ||
		    strncmp(argv[arg], "Name", length) == 0) { /* default */
		continue;
	    }
	    /*
	     * atime = access time = time file data last read or modified.
	     * ctime = desc modify time = time file status last changed, by
	     *		writing or inode changes.
	     * mtime = data modify time = time data last modified.
	     */

	    if (strncmp(argv[arg], "size", length) == 0 ||
		    strncmp(argv[arg], "Size", length) == 0) {
		aWindow->displayInstructions |= FSFLAT_SIZE_FIELD;
		continue;
	    }
	    if (strncmp(argv[arg], "atime", length) == 0 ||
		    strncmp(argv[arg], "Atime", length) == 0 ||
		    strncmp(argv[arg], "accesstime", length) == 0 ||
		    strncmp(argv[arg], "accessTime", length) == 0 ||
		    strncmp(argv[arg], "AccessTime", length) == 0) {
		aWindow->displayInstructions |= FSFLAT_ATIME_FIELD;
		continue;
	    }
	    if (strncmp(argv[arg], "ctime", length) == 0 ||
		    strncmp(argv[arg], "Ctime", length) == 0 ||
		    strncmp(argv[arg], "descmodtime", length) == 0 ||
		    strncmp(argv[arg], "descModTime", length) == 0 ||
		    strncmp(argv[arg], "DescModTime", length) == 0 ||
		    strncmp(argv[arg], "descmodifytime", length) == 0 ||
		    strncmp(argv[arg], "descModifyTime", length) == 0 ||
		    strncmp(argv[arg], "DescModifyTime", length) == 0 ||
		    strncmp(argv[arg], "descriptormodifytime", length) == 0 ||
		    strncmp(argv[arg], "descriptorModifyTime", length) == 0 ||
		    strncmp(argv[arg], "DescriptorModifyTime", length) == 0) {
		aWindow->displayInstructions |= FSFLAT_DTIME_FIELD;
		continue;
	    }
	    if (strncmp(argv[arg], "mtime", length) == 0 ||
		    strncmp(argv[arg], "Mtime", length) == 0 ||
		    strncmp(argv[arg], "datamodtime", length) == 0 ||
		    strncmp(argv[arg], "dataModTime", length) == 0 ||
		    strncmp(argv[arg], "DataModTime", length) == 0 ||
		    strncmp(argv[arg], "datamodifytime", length) == 0 ||
		    strncmp(argv[arg], "dataModifyTime", length) == 0 ||
		    strncmp(argv[arg], "DataModifyTime", length) == 0) {
		aWindow->displayInstructions |= FSFLAT_MTIME_FIELD;
		continue;
	    }
	    sprintf(interp->result, "%s %s %s", "bad argument, the possible",
		    "arguments are size, atime (AccessTime),",
		    "mtime (DataModifyTime), and dtime (DescriptorModifyTime)");
	    return TCL_ERROR;
	}
	goto finishedFields;
    }
    which = Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
	    "Display Size?", NULL, TRUE, "Yes", "No", "Stop",
	    (char *) NULL);
    if (which == 0) {
	aWindow->displayInstructions |= FSFLAT_SIZE_FIELD;
    } else if (which == 2) {
	goto finishedFields;
    }
    which = Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
	    "Display AccessTime?", NULL, TRUE, "Yes", "No", "Stop",
	    (char *) NULL);
    if (which == 0) {
	aWindow->displayInstructions |= FSFLAT_ATIME_FIELD;
    } else if (which == 2) {
	goto finishedFields;
    }
    which = Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
	    "Display DataModifyTime?", NULL, TRUE, "Yes", "No", "Stop",
	    (char *) NULL);
    if (which == 0) {
	aWindow->displayInstructions |= FSFLAT_MTIME_FIELD;
    } else if (which == 2) {
	goto finishedFields;
    }
    which = Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
	    "Display DescriptorModifyTime?", NULL, TRUE, "Yes", "No",
	    "Stop", (char *) NULL);
    if (which == 0) {
	aWindow->displayInstructions |= FSFLAT_DTIME_FIELD;
    } else if (which == 2) {
	goto finishedFields;
    }
finishedFields:
    ;	/* nothing */

    if (aWindow->dontDisplayChangesP == FALSE) {
	/* update display */
	if (aWindow->firstElement == UNINITIALIZED) {
	    aWindow->firstElement = 1;
	}
	FsflatSetPositions(aWindow);
    }

    return TCL_OK;
}

#ifdef NOTDEF

/*
 *----------------------------------------------------------------------
 *
 * FsflatChangeGroupsCmd --
 *
 *	Change the definition of a group.
 *
 * Syntax:
 *	changeGroups
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL errors if not.
 *	error occurred.
 *
 * Side effects:
 *	The pattern matching rules or associated command bindings for the
 *	group may change.
 *
 *----------------------------------------------------------------------
 */
int
FsflatChangeGroupsCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    static	int	count = 0;
    FILE	*stream;
    int	pid;
    char	buffer[MAXPATHLEN];
    char	**args;
    int	pidArray[1];
    FsflatGroup	*grpPtr;
    int	childPid;

    if (strcmp(argv[0], "deleteGroup") == 0 ||
	    strcmp(argv[0], "deletegroup") == 0) {
	/* delete the group and clean up */
    }
    if (
    /*
     * open a file
     */
    Proc_GetIDs(&pid, NULL, NULL, NULL, NULL);
    sprintf(buffer, "%s%d.%d", "/tmp/tmpFsflat", pid, count);
    count++;
    if ((stream = fopen(buffer, "w")) == NULL) {
	sprintf(interp->result, "%s %s %s",
		"Couldn't open file", buffer,
		"in which to let you edit the selection rules.");
	return TCL_ERROR;
    }
    for (grpPtr = aWindow->groupList; grpPtr != NULL;
	    grpPtr = grpPtr->nextPtr) {
	fputs("Select:\n", stream);
	fputs(grpPtr->rule, stream);
	fputs("\n", stream);
	if (grpPtr->name != NULL) {
	    fputs("GroupName:\n", stream);
	    fputs(grpPtr->name, stream);
	    fputs("\n\n", stream);
	}
    }
    fclose(stream);
    /* I should be checking ferror for above PutStrings and Close return. */
    /* close file and mx it. */
    /* parse file when mx returns. */
    if (Proc_Fork(TRUE, &childPid) == PROC_CHILD_PROC) {
	/*child*/
	/* what to do about this pathname? */
	args = (char **) malloc(4 * sizeof (char *));
	args[0] = Util_Strcpy(NULL, "mx");
	args[1] = Util_Strcpy(NULL, "-D");
	args[2] = Util_Strcpy(NULL, buffer);	/* CORE LEAK */
	args[3] = NULL;
	Proc_Exec("/sprite2/users/ouster/mx10/mx", args, FALSE);
	/* error if returned. */
	sprintf(fsflatErrorMsg, "Exec of %s returned.",
		"/sprite2/users/oustser/mx10/mx");
	Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
		fsflatErrorMsg, NULL, TRUE, "Skip command", (char *) NULL);
	exit(-1);
    } else {
	/*parent waits */
	pidArray[0] = childPid;
	Proc_Wait(1, pidArray, PROC_WAIT_BLOCK, NULL, NULL, NULL, NULL, NULL);
    }
    if ((stream = fopen(buffer, "a")) == NULL) {
	sprintf(fsflatErrorMsg, "%s %s %s",
		"Couldn't open file", buffer,
		"in which you edited the selection rules.");
	Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
		fsflatErrorMsg, NULL, TRUE, "Skip command", (char *) NULL);
	return TCL_ERROR;
    }
    if (aWindow->sortingInstructions & FSFLAT_ALPHA_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    fputs("Sort:\nAlphaReverse\n\n", stream);
	} else {
	    fputs("Sort:\nAlpha\n\n", stream);
	}
    }
    if (aWindow->sortingInstructions & FSFLAT_ATIME_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    fputs("Sort:\nAccessTimeReverse\n\n", stream);
	} else {
	    fputs("Sort:\nAccessTime\n\n", stream);
	}
    }
    if (aWindow->sortingInstructions & FSFLAT_MTIME_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    fputs("Sort:\nDataModifyTimeReverse\n\n", stream);
	} else {
	    fputs("Sort:\nDataModifyTime\n\n", stream);
	}
    }
    if (aWindow->sortingInstructions & FSFLAT_DTIME_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    fputs("Sort:\nDescriptorModifyTimeReverse\n\n", stream);
	} else {
	    fputs("Sort:\nDescriptorModifyTime\n\n", stream);
	}
    }
    if (aWindow->sortingInstructions & FSFLAT_SIZE_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    fputs("Sort:\nSizeReverse\n\n", stream);
	} else {
	    fputs("Sort:\nSize\n\n", stream);
	}
    }
    if (aWindow->displayInstructions & FSFLAT_NAME_FIELD) {
	fputs("Display:\nName\n\n", stream);
    }
    if (aWindow->displayInstructions & FSFLAT_ATIME_FIELD) {
	fputs("Display:\nAccessTime\n\n", stream);
    }
    if (aWindow->displayInstructions & FSFLAT_MTIME_FIELD) {
	fputs("Display:\nDataModifyTime\n\n", stream);
    }
    if (aWindow->displayInstructions & FSFLAT_DTIME_FIELD) {
	fputs("Display:\nDescriptorModifyTime\n\n", stream);
    }
    if (aWindow->displayInstructions & FSFLAT_SIZE_FIELD) {
	fputs("Display:\nSize\n\n", stream);
    }
    fclose(stream);

    FsflatGarbageCollect(aWindow);
#ifdef FUTURE
    /*
     * This routine was changed not to take second arg!
     */
    if (FsflatGatherNames(aWindow, buffer) != TCL_OK) {
	return bad value and switch directories?
	error string will be displayed in top-level display thingy?
    }
#endif FUTURE
	
    /* should it repick the size here if aWindow->pickSizeP is true? */
    aWindow->firstElement = 1;
    FsflatSetPositions(aWindow);
    /* FsflatRedraw will be called from event caused in FsflatSetPositions() */

    /* ask if they want to make it permanent. */

    return TCL_OK;
}
#endif NOTDEF



/*
 *----------------------------------------------------------------------
 *
 * FsflatChangeGroupCmd --
 *
 *	Change the definition of a group.
 *
 * Syntax:
 *	changeGroup name newDefType newDef
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL errors if not.
 *
 * Side effects:
 *	The specified group will be changed.
 *
 *----------------------------------------------------------------------
 */
int
FsflatChangeGroupCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    FsflatGroup	*grpPtr;
    FsflatGroup	*newGrpPtr;
    FsflatGroup	*bPtr;

    if (argc != 4) {
	sprintf(interp->result, "%s %s", "Wrong # of args. Must be 4 args,",
		"\"changeGroup name newDefType newDef\"");
	return TCL_ERROR;
    }

    /* Make sure no other group already has the new definition. */
    for (grpPtr = aWindow->groupList; grpPtr != NULL;
	    grpPtr = grpPtr->nextPtr) {
	if (strcmp(argv[3], grpPtr->rule) == 0) {
	    if (grpPtr->fileList == NULL && aWindow->hideEmptyGroupsP) {
		    sprintf(interp->result, "%s %s, %s",
		    "A group already has the definition", argv[3],
		    "but it may not be visible since no files match it.");
	    } else {
		sprintf(interp->result, "A group already has the definition %s",
			argv[3]);
	    }
	    return TCL_ERROR;
	}
    }

    /* find group to replace */
    if (aWindow->groupList == NULL) {
	sprintf(interp->result,
		"No groups have been defined in order to change one");
	return TCL_ERROR;
    }

    bPtr = NULL;
    for (grpPtr = aWindow->groupList; grpPtr != NULL;
	    grpPtr = grpPtr->nextPtr) {
	if (strcmp(argv[1], grpPtr->rule) == 0) {
	    break;
	}
	bPtr = grpPtr;
    }

    if (grpPtr == NULL) {
	sprintf(interp->result, "No group with that definition");
	return TCL_ERROR;
    }


    newGrpPtr = (FsflatGroup *) malloc(sizeof (FsflatGroup));

    /* test the new group's definition */
    if (GetNewGroup(aWindow, interp, argv + 2, newGrpPtr) != TCL_OK) {
	free(newGrpPtr);
	return TCL_ERROR;
    }

    newGrpPtr->nextPtr = grpPtr->nextPtr;

    FsflatGarbageGroup(aWindow, grpPtr);

    if (bPtr == NULL) {
	aWindow->groupList = newGrpPtr;
    } else {
	bPtr->nextPtr = newGrpPtr;
    }
    if (aWindow->dontDisplayChangesP == FALSE) {
	/* update display */
	if (aWindow->firstElement == UNINITIALIZED) {
	    aWindow->firstElement = 1;
	}
	FsflatSetPositions(aWindow);
    }
	
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * GetNewGroup --
 *
 *	Initialize and collect files for a new group.
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL errors if not.
 *
 * Side effects:
 *	Memory will be allocated for various things.
 *
 *----------------------------------------------------------------------
 */
int
GetNewGroup(aWindow, interp, argv, newGrpPtr)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    char		**argv;
    FsflatGroup		*newGrpPtr;
{
    int		procArgc;
    char	**procArgv;
    int		num;

    /* parse command arguments */
    if (strcmp("comparison", argv[0]) == 0) {	/* simple comp stuff */
	newGrpPtr->defType = COMPARISON;
	newGrpPtr->rule = Util_Strcpy((char *) NULL, argv[1]);
	/* test new grp def */
	if (Pattern_Match(newGrpPtr->rule, "x") < 0) {
	    sprintf(interp->result,
		    "The comparison rule {%s} contains an error",
		    newGrpPtr->rule);
	    free(newGrpPtr->rule);
	    return TCL_ERROR;
	}
    } else if (strcmp("proc", argv[0]) != 0) {	/* bad def */
	sprintf(interp->result, "Bad group definition type %s", argv[0]);
	return TCL_ERROR;
    } else {		/* TCL proc */
	newGrpPtr->defType = PROC;
	/* get name of proc */
	if (Tcl_SplitList(interp, argv[1], &procArgc, &procArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
	newGrpPtr->rule = Util_Strcpy((char *) NULL, procArgv[1]);
	if (Tcl_Eval(interp, argv[1], '\0', NULL) != TCL_OK) {
	    free(newGrpPtr->rule);
	    free(procArgv);
	    return TCL_ERROR;
	}
	/* Will this really free it?  Tcl man page says so... */
	free(procArgv);

	/* test new grp def */
	if (FsflatDoTclSelect(interp, newGrpPtr->rule, "x", &num) != TCL_OK) {
		free(newGrpPtr->rule);
		return TCL_ERROR;
	}
    }
    newGrpPtr->nextPtr = NULL;
    newGrpPtr->myColumn = UNINITIALIZED;
    newGrpPtr->headerWindow = UNINITIALIZED;
    newGrpPtr->x = UNINITIALIZED;
    newGrpPtr->y = UNINITIALIZED;
    newGrpPtr->width = 0;
    newGrpPtr->height = 0;
    newGrpPtr->fileList = NULL;
    newGrpPtr->groupBindings = NULL;
    newGrpPtr->selectedP = FALSE;
    newGrpPtr->highlightP = FALSE;
    /*
     * If this is too slow, then change this so it does not actually
     * gather the files?
     */
    if (FsflatGatherSingleGroup(aWindow, newGrpPtr) != TCL_OK) {
	/* eliminate group definition? */
	free(newGrpPtr->rule);
	return TCL_ERROR;
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatPrintTclError --
 *
 *	Print out a tcl interpreter error message.
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
FsflatPrintTclError(aWindow)
    FsflatWindow	*aWindow;
{
    char	c;

    /*
     * Capitalize first character of error message.
     */
    c = aWindow->interp->result[0];
    if ((c >= 'a') && (c <= 'z')) {
	aWindow->interp->result[0] += 'A' - 'a';
    }
    /*
     * output message - should call routine that uses 1-line window
     * if possible.
     */
    Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
	    aWindow->interp->result, NULL, TRUE, "Continue", (char *) NULL);
    /* replace lower case */
    aWindow->interp->result[0] = c;

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatDefineGroupCmd --
 *
 *	Add the definition of a new group.
 *	Currently, this only will add it to the end of the list of groups.
 *
 * Syntax:
 *	defineGroup defType def
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL errors if not.
 *
 * Side effects:
 *	A new group will be added.  It will not be displayed until something
 *	causes a redisplay.
 *
 *----------------------------------------------------------------------
 */
int
FsflatDefineGroupCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    FsflatGroup	*grpPtr;
    FsflatGroup	*newGrpPtr;

    if (argc != 3) {
	sprintf(interp->result, "%s %s", "Wrong # of args. Must be 3 args,",
		"\"defineGroup defType def\"");
	return TCL_ERROR;
    }

    for (grpPtr = aWindow->groupList; grpPtr != NULL;
	    grpPtr = grpPtr->nextPtr) {
	if (strcmp(argv[2], grpPtr->rule) == 0) {
	    if (grpPtr->fileList == NULL && aWindow->hideEmptyGroupsP) {
		    sprintf(interp->result, "%s %s, %s",
		    "A group already has the definition", argv[2],
		    "but it may not be visible since no files match it.");
	    } else {
		sprintf(interp->result, "A group already has the definition %s",
			argv[2]);
	    }
	    return TCL_ERROR;
	}
    }

    newGrpPtr = (FsflatGroup *) malloc(sizeof (FsflatGroup));

    /* test the new group's definition */
    if (GetNewGroup(aWindow, interp, argv + 1, newGrpPtr) != TCL_OK) {
	free(newGrpPtr);
	FsflatPrintTclError(aWindow);
	FsflatDoCmd(aWindow, "close");
	/*
	 * It seems weird to return with TCL_ERROR here, but if i don't,
	 * something will try to unmap the previously destroyed window.
	 * Strangely, nothing wrong seems to happen this way, such as
	 * something trying to print out a message in a deleted interpreter...
	 */
	return TCL_ERROR;
    }

    /* attach new group to end of list */
    if (aWindow->groupList == NULL) {
	aWindow->groupList = newGrpPtr;
    } else {
	for (grpPtr = aWindow->groupList; grpPtr->nextPtr != NULL;
		grpPtr = grpPtr->nextPtr) {
	    /* nothing */
	}
	grpPtr->nextPtr = newGrpPtr;
    }
    if (aWindow->dontDisplayChangesP == FALSE) {
	/* update display */
	if (aWindow->firstElement == UNINITIALIZED) {
	    aWindow->firstElement = 1;
	}
	FsflatSetPositions(aWindow);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatSelectionCmd --
 *
 *	Return the current value of the selection set in interp->result.
 *
 * Syntax:
 *	selection
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL_ERROR if any sort of
 *	error occurred.
 *
 * Side effects:
 *	None except memory allocation.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
FsflatSelectionCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    if (argc > 1) {
	sprintf(interp->result, "Too many args to selection command.");
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatSortFilesCmd --
 *
 *	Add or change the method of sorting directory entries.
 *
 * Syntax:
 *	changeSort [method]
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL_ERROR if any sort of
 *	error occurred.
 *
 * Side effects:
 *	The order of the files displayed will change if the new sorting
 *	method is different.  In this case, the user will be prompted
 *	to see if he wishes to make the new sorting method the default
 *	method for the directory.
 *
 *----------------------------------------------------------------------
 */
int
FsflatSortFilesCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    int	which;

    if (argc > 3) {
	sprintf(interp->result, "Too many args: 3 arg maximum to sort command");
	return TCL_ERROR;
    }
    if (argc == 1) {
	/*
	 * Get the sorting method from the user.  It would be nice if this
	 * could all be in one notifier or in a check-list like window, but
	 * Sx_Notify puts all the buttons in a single line at the top of the
	 * notifier and there are too many buttons to fit on the window here
	 * if the "reverse" options are included.  So for now it's done with
	 * 2 notifiers, one to get what to sort by and the next to say
	 * forwards or backwards.
	 */
	which = Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
		"Pick sorting method", NULL, TRUE, "Alpha", "AccessTime",
		"DataModifyTime", "DescriptorModifyTime", "Size",
		(char *) NULL);
	switch(which) {
	case 0:
	    aWindow->sortingInstructions = FSFLAT_ALPHA_SORT;
	    break;
	case 1:
	    aWindow->sortingInstructions = FSFLAT_ATIME_SORT;
	    break;
	case 2:
	    aWindow->sortingInstructions = FSFLAT_MTIME_SORT;
	    break;
	case 3:
	    aWindow->sortingInstructions = FSFLAT_DTIME_SORT;
	    break;
	case 4:
	    aWindow->sortingInstructions = FSFLAT_SIZE_SORT;
	    break;
	default:
	    sprintf(fsflatErrorMsg, "%s %s", "Something is wrong.",
		    "The sorthing method just entered is unrecognized.");
	    Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
		    fsflatErrorMsg, "Skip command", (char *) NULL);
	    return TCL_ERROR;
	}
	which = Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
		"Sort in forwards or reverse order?", NULL, TRUE,
		"Forwards", "Reverse", (char *) NULL);
	if (which == 1) {
	    aWindow->sortingInstructions |= FSFLAT_REVERSE_SORT;
	}
	FsflatSetSort(aWindow, NULL, NULL);
    } else if (argc == 2) {		/* parse command */
	FsflatSetSort(aWindow, argv[1], NULL);
    } else {
	FsflatSetSort(aWindow, argv[1], argv[2]);
    }
    /*
     * Now ask if they want to make this change be the default sort
     * method for this directory.
     */
    /* soon. */

    if (aWindow->dontDisplayChangesP == FALSE) {
	/* update display */
	if (aWindow->firstElement == UNINITIALIZED) {
	    aWindow->firstElement = 1;
	}
	FsflatSetPositions(aWindow);
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatSetSort --
 *
 *	Change the sorting information and resort files.
 *	If the reverse argument is non-null, do the reverse sort of the
 *	sortMethod argument.  If the sortMethod argument is NULL, then the
 *	aWindow->sortingInstructions field is already set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The order of the files displayed will change if the new sorting
 *	method is different.
 *
 *----------------------------------------------------------------------
 */
void
FsflatSetSort(aWindow, sortMethod, reverse)
    FsflatWindow	*aWindow;
    char		*sortMethod;
    char		*reverse;
{
    int		getAttrsP = FALSE;
    FsflatFile	*tmpPtr;
    FsflatGroup	*grpPtr;
    int		(*compareProc)();
    FsflatFile	**fileArray;
    int		i, arraySize;

    if (sortMethod != NULL) {
	aWindow->sortingInstructions = 0;
	if (reverse != NULL) {
	    aWindow->sortingInstructions |= FSFLAT_REVERSE_SORT;
	}
	if (strcmp(sortMethod, "alpha") == 0 ||
		strcmp(sortMethod, "Alpha") == 0) {
	    aWindow->sortingInstructions |= FSFLAT_ALPHA_SORT;
	}
	if (strcmp(sortMethod, "accessTime") == 0 ||
		strcmp(sortMethod, "AccessTime") == 0 ||
		strcmp(sortMethod, "atime") == 0 ||
		strcmp(sortMethod, "aTime") == 0 ||
		strcmp(sortMethod, "Atime") == 0) {
	    aWindow->sortingInstructions |= FSFLAT_ATIME_SORT;
	}
	if (strcmp(sortMethod, "modifyTime") == 0 ||
		strcmp(sortMethod, "ModifyTime") == 0 ||
		strcmp(sortMethod, "datamodtime") == 0 ||
		strcmp(sortMethod, "dataModTime") == 0 ||
		strcmp(sortMethod, "DataModTime") == 0 ||
		strcmp(sortMethod, "datamodifytime") == 0 ||
		strcmp(sortMethod, "dataModifyTime") == 0 ||
		strcmp(sortMethod, "DataModifyTime") == 0 ||
		strcmp(sortMethod, "mtime") == 0 ||
		strcmp(sortMethod, "mTime") == 0 ||
		strcmp(sortMethod, "Mtime") == 0) {
	    aWindow->sortingInstructions |= FSFLAT_MTIME_SORT;
	}
	if (strcmp(sortMethod, "descModifyTime") == 0 ||
		strcmp(sortMethod, "DescModifyTime") == 0 ||
		strcmp(sortMethod, "descmodifytime") == 0 ||
		strcmp(sortMethod, "descmodtime") == 0 ||
		strcmp(sortMethod, "descModTime") == 0 ||
		strcmp(sortMethod, "DescModTime") == 0 ||
		strcmp(sortMethod, "descriptormodifytime") == 0 ||
		strcmp(sortMethod, "descriptorModifyTime") == 0 ||
		strcmp(sortMethod, "DescriptorModifyTime") == 0 ||
		strcmp(sortMethod, "dtime") == 0 ||
		strcmp(sortMethod, "dTime") == 0 ||
		strcmp(sortMethod, "Dtime") == 0) {
	    aWindow->sortingInstructions |= FSFLAT_DTIME_SORT;
	}
	if (strcmp(sortMethod, "size") == 0 ||
		strcmp(sortMethod, "Size") == 0) {
	    aWindow->sortingInstructions |= FSFLAT_SIZE_SORT;
	}
    }

    /* Now sort the files -- get attr's if necessary */
    getAttrsP = FSFLAT_ATTR_NECESSARY_P;
    if (aWindow->groupList == NULL) {
	return;
    }
    /* need comparison function that takes ptrs to FsflatFiles */
    FsflatGetCompareProc(aWindow, &compareProc, TRUE);
    for (grpPtr = aWindow->groupList; grpPtr != NULL;
	    grpPtr = grpPtr->nextPtr) {
	/* if 0 or 1 files, don't sort */
	if (grpPtr->fileList == NULL || grpPtr->fileList->nextPtr == NULL) {
	    continue;
	}
	for (i = 0, tmpPtr = grpPtr->fileList; tmpPtr != NULL;
		tmpPtr = tmpPtr->nextPtr, i++) {
	    if (getAttrsP && tmpPtr->attrPtr == NULL) {
		tmpPtr->attrPtr = (struct stat *)
			malloc(sizeof (struct stat));
		if (lstat(tmpPtr->name, tmpPtr->attrPtr) != 0) {
		    /* skip this file */
		    sprintf(fsflatErrorMsg, "%s %s.",
			    "Couldn't get attributes for file", tmpPtr->name);
		    Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1,
			    0, fsflatErrorMsg, NULL, TRUE, "Continue",
			    (char *) NULL);
		    bzero((char *) tmpPtr->attrPtr, sizeof (struct stat));
		}
	    }
	}
	arraySize = i;
	fileArray = (FsflatFile **) malloc(arraySize *
		sizeof (FsflatFile *));
	for (tmpPtr = grpPtr->fileList, i = 0; tmpPtr != NULL;
		tmpPtr = tmpPtr->nextPtr, i++) {
	    fileArray[i] = tmpPtr;
	}
	qsort(fileArray, arraySize, sizeof (FsflatFile *), compareProc);
	for (i = 0; i < arraySize - 1; i++) {
	    fileArray[i]->nextPtr = fileArray[i+1];
	}
	fileArray[arraySize - 1]->nextPtr = NULL;
	grpPtr->fileList = fileArray[0];
	free(fileArray);
    }

    return;
}



/*
 *----------------------------------------------------------------------
 *
 * FsflatCloseCmd --
 *
 *	Close the window.  This should do some more cleaning up.
 *
 * Syntax:
 *	close
 *
 * Results:
 *	TCL_OK.
 *
 * Side effects:
 *	Closes the window, deletes the Tcl interpreter for the window, and
 *	frees up the window data structures.  If this was the last window,
 *	we may exit.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
FsflatCloseCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
    if (!MonClient_DeleteDir(aWindow->dir,
	    (ClientData) aWindow->surroundingWindow)) {
	/* what should I do here?  Does it matter? */
    }
    XDeleteContext(fsflatDisplay, aWindow->surroundingWindow,
	    fsflatWindowContext);
    /* DeleteHandlers? */
    FsflatGarbageCollect(aWindow);
    XDestroyWindow(fsflatDisplay, aWindow->surroundingWindow);
    Tcl_DeleteInterp(aWindow->interp);
    free(aWindow);
    fsflatWindowCount--;
    if (fsflatWindowCount <= 0) {
	exit(0);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatExecCmd --
 *
 *	Execute a command in an associated shell window.
 *
 * Syntax:
 *	exec command
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL_ERROR if an associated
 *	shell window could not be found or there was no shell command
 *	given.  Whether or not the command
 *	given to the shell executes happily is another matter.  Maybe I'll
 *	decide that its return status should be available, but I don't know
 *	how to do that...
 *
 * Side effects:
 *	Almost anything -- it depends what the shell command does.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
FsflatExecCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
    int		i;
    char	*command;
    int		length;
    int		result;

    length = argc;	/* spaces between command words + insert word */
    for (i = 0; i < argc; i++) {
	length += strlen(argv[i]);
    }
    length += strlen("insert");
    command = (char *) malloc(length + 1);	/* plus null char */
    strcpy(command, "insert");
    for (i = 0; i < argc; i++) {
	strcat(command, " ");
	strcat(command, argv[i]);
    }
    result = Tx_Command(fsflatDisplay, aWindow->txOutsideWindow, command);
    free(command);

    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatGroupBindCmd --
 *
 *	Add a command binding to a group.  This will bind a command
 *	to a particular button (or button combination).
 *
 * Syntax:
 *	groupbind groupname buttonMask command
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL errors if not.
 *
 * Side effects:
 *	A new command binding will be added to the group.
 *
 *----------------------------------------------------------------------
 */
int
FsflatGroupBindCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    FsflatGroup	*grpPtr;
    char	**buttonArgv;
    int		buttonArgc;
    int		button = 0;
    int		test;
    int		i;

    if (argc != 4) {
	sprintf(interp->result,
		"Wrong # of args to groupBind command, %s",
		"\"groupBind groupName buttonMask command\"");
	return TCL_ERROR;
    }
    for (grpPtr = aWindow->groupList; grpPtr != NULL;
	    grpPtr = grpPtr->nextPtr) {
	if (strcmp(grpPtr->rule, argv[1]) == 0) {
	    break;
	}
    }
    if (grpPtr == NULL) {
	sprintf(interp->result, "No such group %s", argv[1]);
	return TCL_ERROR;
    }

    if (Tcl_SplitList(interp, argv[2], &buttonArgc, &buttonArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < buttonArgc; i++) {
	test = FsflatWhichButton(buttonArgv[i]);
	if (test == 0) {
	    sprintf(interp->result, "Bad button name %s", argv[i]);
	    /* will this really free it?  Tcl man page says so... */
	    free(buttonArgv);
	    return TCL_ERROR;
	}
	button |= test;
    }
    /* will this really free it?  Tcl man page says so... */
    free(buttonArgv);
    FsflatAddGroupBinding(grpPtr, button, argv[3]);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatMenuCmd --
 *
 *	Create, delete, and modify menus.
 *
 * Syntax:
 *	menu append name leftText centerText rightText color cmd
 *	menu create name leftText centerText rightText color cmd leftText ...
 *	menu delete name
 *	menu modify name entryIndex leftText centerText rightText color cmd
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL_ERROR if any sort of
 *	error occurred.
 *
 * Side effects:
 *	The menu structure for the current window gets modified.
 *
 *----------------------------------------------------------------------
 */
int
FsflatMenuCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
#ifdef NOTDEF
    MxMenuInfo *miPtr;
#endif NOTDEF
    int length;
    Sx_MenuEntry entries[SX_MAX_MENU_ENTRIES];

    extern char *GetString();		/* Forward references */

    if (argc < 2) {
	sprintf(interp->result, "wrong # args: must be \"%.50s option [args]\"",
		argv[0]);
	return TCL_ERROR;
    }
    length = strlen(argv[1]);
    if (strncmp(argv[1], "append", length) == 0) {
	Window window;
	XFontStruct *fontPtr;
	int numEntries;
	unsigned long fg, bg;

	if (argc != 8) {
	    sprintf(interp->result, "wrong # args: should be \"%.50s append name left center right color cmd\"",
		    argv[0]);
	    return TCL_ERROR;
	}
	window = Sx_MenuGetWindow(fsflatDisplay, aWindow->menuBar, argv[2]);
	if (window == NULL) {
	    goto menuNameBad;
	}
	numEntries = Sx_MenuGetInfo(fsflatDisplay, window, entries, &fontPtr,
		&fg, &bg);
	if (numEntries >= SX_MAX_MENU_ENTRIES) {
	    return TCL_OK;
	}
	entries[numEntries].leftText = GetString(argv[3]);
	entries[numEntries].centerText = GetString(argv[4]);
	entries[numEntries].rightText = GetString(argv[5]);
	entries[numEntries].background =
		Util_StringToColor(fsflatDisplay, argv[6]);
/* should check to see if it returns -1 value!!!! */
	entries[numEntries].proc = FsflatMenuProc;
    /* Mx uses a structure with both command AND aWindow fields.  Should I? */
	entries[numEntries].clientData = (ClientData) Util_Strcpy(NULL,
		argv[7]);
	Sx_MenuCreate(fsflatDisplay, aWindow->menuBar, argv[2], numEntries+1,
		entries, fontPtr, fg, bg);
	return TCL_OK;
    }
    if (strncmp(argv[1], "create", length) == 0) {
	int numEntries, i, arg;

	numEntries = (argc - 3)/5;
	if ((argc-3) != (numEntries*5)) {
	    sprintf(interp->result, "wrong # args: should be \"%.50s create name [left center right color cmd] ...\"",
		    argv[0]);
	    return TCL_ERROR;
	}
	if (numEntries > SX_MAX_MENU_ENTRIES) {
	    sprintf(interp->result,
		    "can't create a menu with more than %d entries",
		    SX_MAX_MENU_ENTRIES);
	    return TCL_ERROR;
	}

	for (i = 0, arg = 3; i < numEntries; i++, arg += 5) {
	    entries[i].leftText = GetString(argv[arg]);
	    entries[i].centerText = GetString(argv[arg+1]);
	    entries[i].rightText = GetString(argv[arg+2]);
	    entries[i].background =
		    Util_StringToColor(fsflatDisplay, argv[arg+3]);
/* should check to see if it returns -1 value! */
	    entries[i].proc = FsflatMenuProc;
	    entries[i].clientData = (ClientData) Util_Strcpy(NULL, argv[arg+4]);
	}
	Sx_MenuCreate(fsflatDisplay, aWindow->menuBar, argv[2], numEntries,
		entries, aWindow->fontPtr, aWindow->menuForeground,
		aWindow->menuBackground);
	return TCL_OK;
    } else if (strncmp(argv[1], "delete",length) == 0) {
	Window window;
	int count;

	if (argc != 3) {
	    sprintf(interp->result,
		    "wrong # args: should be \"%.50s delete name\"",
		    argv[0]);
	    return TCL_ERROR;
	}
	window = Sx_MenuGetWindow(fsflatDisplay, aWindow->menuBar, argv[2]);
	if (window == NULL) {
	    goto menuNameBad;
	}
	count = Sx_MenuGetInfo(fsflatDisplay, window, entries,
		(XFontStruct **) NULL, (int *) NULL, (int *) NULL);
	for (count--; count >= 0; count--) {
	    free(entries[count].clientData);
	}
	XDestroyWindow(fsflatDisplay, window);
	return TCL_OK;
#ifdef NOTDEF
    } else if (strncmp(argv[1], "info", length) == 0) {
	Window window;
	int count, i;
	char *names[SX_MAX_MENUS];
	char *entryStrings[SX_MAX_MENU_ENTRIES];
	char *pieces[4];

	if (argc == 2) {
	    count = Sx_MenuGetNames(mxwPtr->display, mxwPtr->menuBar, names,
		    (Window *) NULL);
	    interp->result = Tcl_Merge(count, names);
	    interp->dynamic = 1;
	    return TCL_OK;
	}
	if (argc != 3) {
	    sprintf(interp->result,
		    "wrong # args: should be \"%.50s info [name]\"",
		    argv[0]);
	    return TCL_ERROR;
	}
	window = Sx_MenuGetWindow(mxwPtr->display, mxwPtr->menuBar, argv[2]);
	if (window == NULL) {
	    goto menuNameBad;
	}
	count = Sx_MenuGetInfo(mxwPtr->display, window, entries,
		(XFontStruct **) NULL, (unsigned long *) NULL,
		(unsigned long *) NULL);
	for (i = 0; i < count; i++) {
	    pieces[0] = entries[i].leftText;
	    if (pieces[0] == NULL) {
		pieces[0] = "";
	    }
	    pieces[1] = entries[i].centerText;
	    if (pieces[1] == NULL) {
		pieces[1] = "";
	    }
	    pieces[2] = entries[i].rightText;
	    if (pieces[2] == NULL) {
		pieces[2] = "";
	    }
	    pieces[3] = ((MxMenuInfo *) entries[i].clientData)->command;
	    if (pieces[3] == NULL) {
		pieces[3] = "";
	    }
	    entryStrings[i] = Tcl_Merge(4, pieces);
	}
	interp->result = Tcl_Merge(count, entryStrings);
	interp->dynamic = 1;
	for (i = 0; i < count; i++) {
	    free(entryStrings[i]);
	}
	return TCL_OK;
#endif NOTDEF
    } else if (strncmp(argv[1], "modify", length) == 0) {
	Window window;
	Sx_MenuEntry entry;
	int index;

	if (argc != 9) {
	    sprintf(interp->result, "wrong # args: should be \"%.50s modify name index left center right color cmd\"",
		    argv[0]);
	    return TCL_ERROR;
	}
	window = Sx_MenuGetWindow(fsflatDisplay, aWindow->menuBar, argv[2]);
	if (window == NULL) {
	    goto menuNameBad;
	}
	index = atoi(argv[3]);
	if ((index < 0) || (index >= Sx_MenuGetInfo(fsflatDisplay, window,
		entries, (XFontStruct **) NULL, (int *) NULL, (int *) NULL))) {
	    sprintf(interp->result,
		    "there's no entry %d in menu \"%.50s\"",
		    index, argv[2]);
	    return TCL_ERROR;
	}
	free(entries[index].clientData);
	entry.leftText = GetString(argv[4]);
	entry.centerText = GetString(argv[5]);
	entry.rightText = GetString(argv[6]);
/* should check to see if it returns -1 value! */
	entry.background = Util_StringToColor(fsflatDisplay, argv[7]);
	entry.proc = FsflatMenuProc;
	entry.clientData = (ClientData) Util_Strcpy(NULL, argv[8]);
	Sx_MenuReplaceEntry(fsflatDisplay, window, index, &entry);
	return TCL_OK;
    } else {
	sprintf(interp->result, "bad \"%.50s\" option: must be append, create, delete, or modify\"",
		argv[0]);
	return TCL_ERROR;
    }

    menuNameBad:
    sprintf(interp->result, "there's no menu named \"%.50s\".", argv[2]);
    return TCL_ERROR;
}



/*
 *----------------------------------------------------------------------
 *
 * FsflatOpenCmd --
 *
 *	Create another fsflat window.
 *	Eventually this will be create another Wish window, with an
 *	argument to say whether it should be a flat or tree window.  If
 *	the argument is not supplied, it should default to the type of
 *	window where the command was called from.
 *
 * Syntax:
 *	open directory
 *
 * Results:
 *	Returns TCL_OK if all went well, various TCL errors if not.
 *
 * Side effects:
 *	A new window should be created.
 *
 *----------------------------------------------------------------------
 */
int
FsflatOpenCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    Tcl_Interp		*interp;
    int			argc;
    char		**argv;
{
    if (argc > 2) {
	sprintf(interp->result, "%s %s", "Too many args to open command:",
		"open [directory]");
	return TCL_ERROR;
    }
    if (argc == 2) {
	if (FsflatCreate(aWindow, argv[1]) == NULL) {
	    return TCL_ERROR;
	}
    } else if (FsflatCreate(aWindow, NULL) == NULL) {
	return TCL_ERROR;
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatPatternCompareCmd --
 *
 *	Compare two strings.  In the interpreter, return 0 for a match,
 *	and < 0 for an error.  Return > 0 for failure to match.
 *
 * Syntax:
 *	pattern	string1 string2
 *
 * Results:
 *	TCL_OK if everything went okay, TCL_ERROR if not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
FsflatPatternCompareCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
    int	result;

    if (argc != 3) {
	sprintf(interp->result, "pattern command requires 3 args");
	return TCL_ERROR;
    }
    result = Pattern_Match(argv[1], argv[2]);
    sprintf(interp->result, "%d", result);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatQuitCmd --
 *
 *	Exit the program.
 *
 * Syntax:
 *	quit
 *
 * Results:
 *	Exits.
 *
 * Side effects:
 *	Exits.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
FsflatQuitCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
    /*
     * I should go through and call MonClient_DeleteDir for each remaining 
     * window.
     */
    exit(0);
}

#ifdef NOTDEF

/*
 *----------------------------------------------------------------------
 *
 * FsflatRestartCmd --
 *
 *	Resource the startup files and rebuild the display of the current
 *	directory.
 *
 * Syntax:
 *	restart
 *
 * Results:
 *	Returns TCL_OK if all went well, or TCL_ERROR if any sort of
 *	error occurred.
 *
 * Side effects:
 *	The display may change if the startup files have changed.
 *
 *----------------------------------------------------------------------
 */
int
FsflatRestartCmd(aWindow, interp, argc, argv)
    FsflatWindow	*aWindow;
    int			argc;
    char		**argv;
{
    if (argc != 1) {
	sprintf(interp->result, "too many args to restart command. 1 arg max");
	return TCL_ERROR;
    }
    FsflatGarbageCollect(aWindow);
    /* delete interpreter and start new one? */
    if (FsflatGatherNames(aWindow) != TCL_OK) {
	/* Fix here too. */
    }
    /* should it repick the size here if aWindow->pickSizeP is true? */
    aWindow->firstElement = 1;
    FsflatSetPositions(aWindow);
    /* FsflatRedraw will be called from event caused in FsflatSetPositions() */

    return TCL_OK;
}
#endif NOTDEF


/*
 *----------------------------------------------------------------------
 *
 * FsflatRedrawCmd --
 *
 *	Redraw the window.
 *
 * Syntax:
 *	redraw
 *
 * Results:
 *	TCL_OK.
 *
 * Side effects:
 *	Redraws the window.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
FsflatRedrawCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
    if (aWindow->dontDisplayChangesP == FALSE) {
	FsflatRedraw(aWindow);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatResizeCmd --
 *
 *	Resize the window.
 *
 * Syntax:
 *	redraw newheight newwidth
 *
 * Results:
 *	TCL_OK if everything goes well, TCL_ERROR if not.
 *
 * Side effects:
 *	Resizes the window.
 *
 *----------------------------------------------------------------------
 */
int
FsflatResizeCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
    int	height, width;
    char	*cptr;

    if (argc != 3) {
	sprintf(interp->result, "Wrong # of args to resize command: %s",
	    "\"resize newheight newwidth\"");
	return TCL_ERROR;
    }
    if ((height = strtol(argv[1], &cptr, 10)) == 0 && cptr == argv[1]) {
	sprintf(interp->result, "Bad height arg to resize command: %s",
	    argv[1]);
	return TCL_ERROR;
    }
    if ((width = strtol(argv[2], &cptr, 10)) == 0 && cptr == argv[2]) {
	sprintf(interp->result, "Bad width arg to resize command: %s", argv[2]);
	return TCL_ERROR;
    }
	
    FsflatSetWindowAndRowInfo(aWindow, height, width);

    if (aWindow->dontDisplayChangesP == FALSE) {
	/* update display */
	if (aWindow->firstElement == UNINITIALIZED) {
	    aWindow->firstElement = 1;
	}
	FsflatSetPositions(aWindow);
	/*
	 * FsflatRedraw will be called from event caused in FsflatSetPostions().
	 */
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatToggleSelEntryCmd --
 *
 *	Toggle the selection status of the given entry.
 *
 * Syntax:
 *	toggleSelectionEntry x y [wholeLine]
 *
 * Results:
 *	TCL_OK if everything went well.  TCL_ERROR if not.
 *
 * Side effects:
 *	Toggles the chosen entry.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
FsflatToggleSelEntryCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc;
    char	**argv;
{
    int		x, y;
    FsflatFile	*filePtr;
    FsflatGroup	*groupPtr = NULL;
    int	lineP = 0;
    char	*cptr;

    if (argc < 3 || argc > 4) {
	sprintf(aWindow->interp->result, "Wrong # of args to toggleSelection");
	return TCL_ERROR;
    }
    if ((x = strtol(argv[1], &cptr, 10)) == 0 && cptr == argv[1]) {
	sprintf(aWindow->interp->result, "Bad window x coordinate %s", argv[1]);
	return TCL_ERROR;
    }
    if ((y = strtol(argv[2], &cptr, 10)) == 0 && cptr == argv[2]) {
	sprintf(aWindow->interp->result, "Bad window y coordinate %s", argv[2]);
	return TCL_ERROR;
    }
    if (argc == 4) {
	if ((lineP = strtol(argv[3], &cptr, 10)) == 0 && cptr == argv[3]) {
	    sprintf(aWindow->interp->result, "Bad 3rd arg to toggleSelection");
	    return TCL_ERROR;
	}
    }
	
    filePtr = FsflatMapCoordsToFile(aWindow, x, y);
    if (filePtr == NULL && groupPtr == NULL) {
	sprintf(aWindow->interp->result,
		"No entry to select at those coordinates: %d %d", x, y);
	return TCL_ERROR;
    }
    if (filePtr != NULL) {
	FsflatChangeSelection(aWindow, (ClientData) filePtr, TRUE, lineP, TRUE);
    } else {
	FsflatChangeSelection(aWindow, (ClientData) groupPtr, FALSE, FALSE,
		TRUE);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FsflatToggleSelectionCmd --
 *
 *	Select or deselect given file.
 *
 * Syntax:
 *	toggleSelection x y [wholeLine]
 *
 * Results:
 *	TCL_OK if everything went well.  TCL_ERROR if not.
 *
 * Side effects:
 *	Changes the selection variable to be given file or empty.
 *
 *----------------------------------------------------------------------
 */
int
FsflatToggleSelectionCmd(aWindow, interp, argc, argv)
    register	FsflatWindow	*aWindow;
    Tcl_Interp	*interp;
    int		argc; char	**argv;
{
    int		x, y;
    FsflatFile	*filePtr = NULL;
    FsflatGroup	*groupPtr = NULL;		/* not used yet */
    int	lineP = 0;
    char	*cptr;

    if (argc < 3 || argc > 4) {
	sprintf(aWindow->interp->result, "Wrong # of args to toggleSelection");
	return TCL_ERROR;
    }
    if ((x = strtol(argv[1], &cptr, 10)) == 0 && cptr == argv[1]) {
	sprintf(aWindow->interp->result, "Bad window x coordinate %s", argv[1]);
	return TCL_ERROR;
    }
    if ((y = strtol(argv[2], &cptr, 10)) == 0 && cptr == argv[2]) {
	sprintf(aWindow->interp->result, "Bad window y coordinate %s", argv[2]);
	return TCL_ERROR;
    }
    if (argc == 4) {
	if ((lineP = strtol(argv[3], &cptr, 10)) == 0 && cptr == argv[3]) {
	    sprintf(aWindow->interp->result, "Bad 3rd arg to toggleSelection");
	    return TCL_ERROR;
	}
    }
	
    filePtr = FsflatMapCoordsToFile(aWindow, x, y);
    if (filePtr == NULL && groupPtr == NULL) {
	sprintf(aWindow->interp->result,
		"No entry to select at those coordinates: %d %d", x, y);
	return TCL_ERROR;
    }
    if (filePtr != NULL) {
	FsflatChangeSelection(aWindow, (ClientData) filePtr, TRUE, lineP,
		FALSE);
    } else {
	FsflatChangeSelection(aWindow, (ClientData) groupPtr, FALSE, FALSE,
		FALSE);
    }

    return TCL_OK;
}

void
FsflatCmdTableInit(cmdTablePtr, interpPtr, commands, clientData)
    Cmd_Table	*cmdTablePtr;
    Tcl_Interp	**interpPtr;
    CmdInfo	commands[];
    ClientData	clientData;
{
    CmdInfo	*cmd;
    int		i;

    *cmdTablePtr = Cmd_TableCreate();
    *interpPtr = Tcl_CreateInterp(); 
    for (cmd = commands; cmd->name != NULL; cmd++) {
	Tcl_CreateCommand(*interpPtr, cmd->name, cmd->proc, clientData,
		(void (*)()) NULL);
    }
    for (i = insertFirst; i <= insertLast; i++) {
	char	string[2];

	string[0] = i;
	string[1] = 0;
	Cmd_BindingCreate(*cmdTablePtr, string, "!@");
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatAddGroupBinding --
 *
 *	Add a command binding to a file group.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New command binding added to given group.
 *
 *----------------------------------------------------------------------
 */
void
FsflatAddGroupBinding(grpPtr, button, command)
    FsflatGroup		*grpPtr;
    int			button;			/* button binding */
    char		*command;
{
    FsflatGroupBinding	*bPtr;

    for (bPtr = grpPtr->groupBindings; bPtr != NULL; bPtr = bPtr->nextPtr) {
	if (bPtr->button == button) {
	    /* substitute command and return */
	    if (bPtr->command != NULL) {
		free(bPtr->command);
	    }
	    bPtr->command = Util_Strcpy(NULL, command);
	    return;
	}
    }
    /* Not there already */
    bPtr = (FsflatGroupBinding *) malloc(sizeof (FsflatGroupBinding));
    bPtr->button = button;
    bPtr->command = Util_Strcpy(NULL, command);
    bPtr->nextPtr = grpPtr->groupBindings;
    grpPtr->groupBindings = bPtr;

    return;
}

void
FsflatDeleteGroupBindings(grpPtr)
    FsflatGroup	*grpPtr;
{
    FsflatGroupBinding	*bPtr, *nPtr;

    for (bPtr = grpPtr->groupBindings; bPtr != NULL; ) {
	nPtr = bPtr->nextPtr;
	free(bPtr->command);
	free(bPtr);
	bPtr = nPtr;
    }
    grpPtr->groupBindings = NULL;

    return;
}

void
FsflatDeleteGroupBinding(grpPtr, button)
    FsflatGroup	*grpPtr;
    int		button;
{
    FsflatGroupBinding	*bPtr, *nPtr;

    if (grpPtr->groupBindings == NULL) {
	return;
    }
    if (grpPtr->groupBindings->button == button) {
	bPtr = grpPtr->groupBindings;
	grpPtr->groupBindings = grpPtr->groupBindings->nextPtr;
	free(bPtr->command);
	free(bPtr);

	return;
    }
    for (bPtr = grpPtr->groupBindings; bPtr->nextPtr != NULL;
	    bPtr = bPtr->nextPtr ) {
	nPtr = bPtr->nextPtr;
	if (nPtr->button == button) {
	    bPtr->nextPtr = nPtr->nextPtr;
	    free(nPtr->command);
	    free(nPtr);
	    return;
	}
    }

    return;
}



char *
FsflatGetGroupBinding(grpPtr, button)
    FsflatGroup	*grpPtr;
    int		button;
{
    FsflatGroupBinding	*bPtr;

    for (bPtr = grpPtr->groupBindings; bPtr != NULL; bPtr = bPtr->nextPtr) {
	if (bPtr->button == button) {
	    break;
	}
    }
    if (bPtr == NULL) {
	return NULL;
    }
    return bPtr->command;
}



/*
 *----------------------------------------------------------------------
 *
 * FsflatDoCmd --
 *
 *	Execute a given Tcl command in a given window, and display
 *	error information if the command doesn't complete successfully.
 *
 * Results:
 *	Returns the result code from the command: TCL_OK, etc.
 *
 * Side effects:
 *	Can be almost arbitrary, depending on the command.
 *
 *----------------------------------------------------------------------
 */
int
FsflatDoCmd(aWindow, command)
    FsflatWindow	*aWindow;
    char	*command;
{
    int		result;

    result = Tcl_Eval(aWindow->interp, command, 0, (char **) 0);
    if (result == TCL_OK) {
#ifdef NOTDEF
	if (*aWindow->interp->result != 0) {
/* this won't work with close command, interpreter and window structure gone! */
	    /*
	     * output message - should call routine that uses 1-line window
	     * if possible.
	     */
	}
#endif NOTDEF
	return result;
    }

    FsflatPrintTclError(aWindow);

    return result;
}


/*
 * Local procedure used to turn empty or "-" strings into NULLs.
 */
static char *
GetString(string)
    char *string;
{
    if ((string[0] == 0) || ((string[0] == '-') && (string[1] == 0))) {
	return NULL;
    }
    return string;
}

int
FsflatWhichButton(token)
    char	*token;
{

    if (strcmp(token, "Left") == 0 ||
	    strcmp(token, "left") == 0) {
	return FSFLAT_LEFT_BUTTON;
    }
    if (strcmp(token, "Middle") == 0 ||
	    strcmp(token, "middle") == 0) {
	return FSFLAT_MIDDLE_BUTTON;
    }
    if (strcmp(token, "Right") == 0 ||
	    strcmp(token, "right") == 0) {
	return FSFLAT_RIGHT_BUTTON;
    }
    if (strcmp(token, "Meta") == 0 ||
	    strcmp(token, "meta") == 0) {
	return FSFLAT_META_BUTTON;
    }
    if (strcmp(token, "Shift") == 0 ||
	    strcmp(token, "shift") == 0) {
	return FSFLAT_SHIFT_BUTTON;
    }
    return 0;
}
