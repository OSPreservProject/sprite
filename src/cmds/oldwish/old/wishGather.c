/* 
 * fsflatGather.c --
 *
 *	Gathering and sorting file names.  Putting them in
 *	data structures.
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
static char rcsid[] = "$Header: fsflatGather.c,v 1.1 88/10/03 12:47:47 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include "sx.h"
#include "string.h"
#include "util.h"
#include "fsflatInt.h"

/* ClientData for scandir select proc's */
static	ClientData	scanData;

/* used before defined */
extern	Boolean	SelectFiles();
extern	int	AlphaForwards();
extern	int	AlphaReverse();
extern	int	AtimeForwards();
extern	int	AtimeReverse();
extern	int	CtimeForwards();
extern	int	CtimeReverse();
extern	int	MtimeForwards();
extern	int	MtimeReverse();
extern	int	DtimeForwards();
extern	int	DtimeReverse();
extern	int	SizeForwards();
extern	int	SizeReverse();
extern	int	AtimeSort();
extern	int	MtimeSort();
extern	int	DtimeSort();
extern	int	SizeSort();


/*
 *----------------------------------------------------------------------
 *
 * FsflatGatherNames --
 *
 *	Using scandir() and a bunch of rules and call-back thingies,
 *	gather the appropriate names into the appropriate places.
 *
 * Results:
 *	TCL_OK if all went well.  Some TCL error if not.
 *
 * Side effects:
 *	The names are gathered and stuff is allocated.
 *
 *----------------------------------------------------------------------
 */
int
FsflatGatherNames(aWindow)
    FsflatWindow	*aWindow;
{
    int		nitems;
    struct	direct	**namelist;
    int		(*compareProc)();
    int		i;
    FsflatFile	*tmpPtr, *backTmpPtr;
    FsflatGroup	*grpPtr;
    int		result;
    Boolean	getAttrsP = FALSE;
    struct	stat	attrs;

    if (aWindow->groupList == NULL) {
	/* get it out of the file */
#ifdef NOTDEF
	Sx_Panic(fsflatDisplay,
		"FsflatGatherNames: Should source .wish file, but can't.");
#endif NOTDEF
#ifdef NOTDEF
	FsflatReadRules(aWindow, name);
#endif NOTDEF
    }
    if (aWindow->groupList == NULL) {
	/*
	 * There wasn't a file, select everything (or should the default
	 * be to select nothing?
	 */
	aWindow->groupList = (FsflatGroup *) malloc(sizeof (FsflatGroup));
	aWindow->groupList->myColumn = -1;
	aWindow->groupList->headerWindow = UNINITIALIZED;
	aWindow->groupList->x = aWindow->groupList->y = -1;
	aWindow->groupList->width = aWindow->groupList->height = -1;
	aWindow->groupList->entry_x = aWindow->groupList->entry_y =
		aWindow->groupList->entry_width = -1;
	aWindow->groupList->fileList = NULL;
	aWindow->groupList->defType = COMPARISON;
	aWindow->groupList->rule = Util_Strcpy(NULL, "*");
	aWindow->groupList->groupBindings = NULL;
	aWindow->groupList->length = -1;
	aWindow->groupList->selectedP = FALSE;
	aWindow->groupList->highlightP = FALSE;
	aWindow->groupList->nextPtr = NULL;

	aWindow->numElements = UNINITIALIZED;
	aWindow->numGroups = 1;
	aWindow->numHiddenGroups = 0;
    }
    if (aWindow->numElements >= 0) {
	/*
	 * For now, this means this has already been set up, since there's
	 * no garbage collection yet.
	 */
	return TCL_OK;
    }
    FsflatGetCompareProc(aWindow, &compareProc, FALSE);
    getAttrsP = FSFLAT_ATTR_NECESSARY_P; 

    scanData = (ClientData) aWindow;
    nitems = scandir(aWindow->dir, &namelist, SelectFiles, compareProc);
    /*
     * I use Stat_GetMsg here.  Am I doing it correctly with errno?
     */
    if (nitems < 0) {
	sprintf(aWindow->interp->result, "%s %s %s %s.  %s %s",
		"Directory scan failed in", aWindow->dir, "with error",
		Stat_GetMsg(errno),
		"Either the directory doesn't exist, or it",
		"is unreadable, or we've run out of memory");
	return TCL_ERROR;
    }
    /*
     * For each file, run through all the groups putting a copy of the file
     * into the group if it matches the rule for that group.  This allows
     * one file to show up in all groups that can select it.  If files should
     * only show up in one place, then skip to the next file after we've
     * found one group for it.  Right now, I use the first policy.  Maybe
     * this should be an option.
     *
     * Since the files are in proper sorted order already and I run through
     * them in order, they will come out in proper sorted order inside the
     * groups.
     *
     * NOTE:  This may be way too slow to use Pattern_Match() in 2 passes
     * like this.
     */
    aWindow->numElements = 0;		/* since it's initialized to -1 */
    aWindow->numHiddenGroups = 0;
    for (i = 0; i < nitems; i++) {
	if (getAttrsP) {
	    /*
	     * If the files were sorted by something involving an attribute,
	     * then there should be a way in the sort routines to cache that
	     * info so we don't do a second GetAttributes()'s here...
	     * This means dealing with the garbage collection aspects too.
	     */
	    if (lstat(namelist[i]->d_name, &attrs)
		    != 0) {
		char	buffer[MAXPATHLEN];
		sprintf(fsflatErrorMsg, "%s %s.  %s %s.  %s %s.",
			"Couldn't get attributes for file",
			namelist[i]->d_name,
			"Error was",
			Stat_GetMsg(errno),
			"Current dir is",
			getwd(buffer) == NULL ? "NULL" : buffer);
		Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
			fsflatErrorMsg,
			NULL, TRUE, "Skip offending file", (char *) NULL);
		continue;
	    }
	}
	for (grpPtr = aWindow->groupList; grpPtr != NULL;
		grpPtr = grpPtr->nextPtr) {
	    result = Pattern_Match(grpPtr->rule, namelist[i]->d_name);
	    if (result == 0) {		/* it matched */
		tmpPtr = (FsflatFile *) malloc(sizeof (FsflatFile));
		tmpPtr->name = Util_Strcpy(NULL, namelist[i]->d_name);
		if (getAttrsP) {
		    tmpPtr->attrPtr = (struct	stat *)
			    malloc(sizeof (struct	stat));
		    *(tmpPtr->attrPtr) = attrs;
		} else {
		    tmpPtr->attrPtr = NULL;
		}
		tmpPtr->length = -1;
		tmpPtr->x = tmpPtr->y = -1;
		tmpPtr->myColumn = -1;
		tmpPtr->selectedP = FALSE;
		tmpPtr->lineP = FALSE;
		tmpPtr->highlightP = FALSE;
		tmpPtr->myGroupPtr = grpPtr;
		tmpPtr->nextPtr = NULL;
		if (grpPtr->fileList == NULL) {
		    grpPtr->fileList = tmpPtr;
		} else {
		    /* add file to end of list */
		    for (backTmpPtr = grpPtr->fileList;
			    backTmpPtr->nextPtr != NULL;
			    backTmpPtr = backTmpPtr->nextPtr) {
			/* do nothing */
		    }
		    backTmpPtr->nextPtr = tmpPtr;
		}
		aWindow->numElements++;
	    } else if (result < 0) {
		/*
		 * I check the rules as they are created, or the following
		 * would cause a billion notifiers!  This is just for extreme
		 * caution...
		 */
		sprintf(fsflatErrorMsg, "The rule %s contains an error.",
		    grpPtr->rule);
		Sx_Notify(fsflatDisplay, aWindow->displayWindow, -1, -1, 0,
			fsflatErrorMsg, NULL,
			TRUE, "Continue", NULL);
	    }
	}
    }
    if (aWindow->hideEmptyGroupsP) {
	for (grpPtr = aWindow->groupList; grpPtr != NULL;
		grpPtr = grpPtr->nextPtr) {
	    if (grpPtr->fileList == NULL) {
		aWindow->numHiddenGroups++;
	    }
	}
    }
    /*
     * total number of things that could be displayed is the number of files
     * plus the number of visible group headers plus the number of spaces
     * between visible groups.
     */
    aWindow->totalDisplayEntries = aWindow->numElements +
	    (2 * (aWindow->numGroups - aWindow->numHiddenGroups)) - 1;
    free_scandir(nitems, &namelist);

    return TCL_OK;
}

typedef	struct	{
    FsflatWindow	*aWindow;
    FsflatGroup		*grpPtr;
} GatherInfo;


/*
 *----------------------------------------------------------------------
 *
 * FsflatGatherSingleGroup --
 *
 *	Using scandir() and a bunch of rules and call-back thingies,
 *	gather the appropriate names into the appropriate places.
 *
 * Results:
 *	TCL_OK if all went well.  Some TCL error if not.
 *
 * Side effects:
 *	The names are gathered and stuff is allocated.
 *
 *----------------------------------------------------------------------
 */
int
FsflatGatherSingleGroup(aWindow, grpPtr)
    FsflatWindow	*aWindow;
    FsflatGroup		*grpPtr;
{
    Boolean	getAttrsP = FALSE;
    int		nitems;
    struct	direct	**namelist;
    int		(*compareProc)();
    struct	stat	attrs;
    FsflatFile	*tmpPtr, *backTmpPtr;
    extern	Boolean	SelectGroupFiles();	/* forward reference */
    int		i;
    GatherInfo	gatherInfo;

    FsflatGetCompareProc(aWindow, &compareProc, FALSE);
    getAttrsP = FSFLAT_ATTR_NECESSARY_P; 

    gatherInfo.aWindow = aWindow;
    gatherInfo.grpPtr = grpPtr;
    scanData = (ClientData) &gatherInfo;
    nitems = scandir(aWindow->dir, &namelist, SelectGroupFiles, compareProc);
    if (nitems < 0) {
	sprintf(aWindow->interp->result, "%s %s %s %s.  %s %s",
		"Directory scan failed in", aWindow->dir, "with error",
		Stat_GetMsg(errno),
		"Either the direcotry doesn't exist, or it",
		"is unreadable, or we've run out of memory");
	return TCL_ERROR;
    }
    /* since they're initialized to -1 */
    if (aWindow->numElements == UNINITIALIZED) {
	aWindow->numElements = 0;
    }
    if (aWindow->numHiddenGroups == UNINITIALIZED) {
	aWindow->numHiddenGroups = 0;
    }
    for (i = 0; i < nitems; i++) {
	if (getAttrsP) {
	    /*
	     * If the files were sorted by something involving an attribute,
	     * then there should be a way in the sort routines to cache that
	     * info so we don't do a second GetAttributes()'s here...
	     * This means dealing with the garbage collection aspects too.
	     */
	    if (lstat(namelist[i]->d_name, &attrs)
		    != 0) {
		char	buffer[MAXPATHLEN];
		sprintf(fsflatErrorMsg, "%s %s.  %s %s.  %s %s.",
			"Couldn't get attributes for file",
			namelist[i]->d_name,
			"Error was",
			Stat_GetMsg(errno),
			"Current dir is",
			getwd(buffer) == NULL ? "NULL" : buffer);
		Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
			fsflatErrorMsg,
			NULL, TRUE, "Leave attributes blank", (char *) NULL);
		bzero(&attrs, sizeof (struct	stat));;
	    }
	}
	tmpPtr = (FsflatFile *) malloc(sizeof (FsflatFile));
	tmpPtr->name = Util_Strcpy(NULL, namelist[i]->d_name);
	if (getAttrsP) {
	    tmpPtr->attrPtr = (struct	stat *)
		    malloc(sizeof (struct	stat));
	    *(tmpPtr->attrPtr) = attrs;
	} else {
		tmpPtr->attrPtr = NULL;
	}
	tmpPtr->length = -1;
	tmpPtr->x = tmpPtr->y = -1;
	tmpPtr->myColumn = -1;
	tmpPtr->selectedP = FALSE;
	tmpPtr->lineP = FALSE;
	tmpPtr->highlightP = FALSE;
	tmpPtr->myGroupPtr = grpPtr;
	tmpPtr->nextPtr = NULL;
	if (grpPtr->fileList == NULL) {
	    grpPtr->fileList = tmpPtr;
	} else {
	    /* add file to end of list */
	    for (backTmpPtr = grpPtr->fileList;
		    backTmpPtr->nextPtr != NULL;
		    backTmpPtr = backTmpPtr->nextPtr) {
		/* do nothing */
	    }
	    backTmpPtr->nextPtr = tmpPtr;
	}
	aWindow->numElements++;
    }
    if (aWindow->numGroups == UNINITIALIZED) {
	aWindow->numGroups = 1;
	aWindow->numHiddenGroups = 0;
    } else {
	aWindow->numGroups++;
    }
    if (aWindow->hideEmptyGroupsP) {
	if (grpPtr->fileList == NULL) {
	    aWindow->numHiddenGroups++;
	}
    }
    /*
     * Total number of things that could be displayed is the number of files
     * plus the number of visible group headers plus the number of spaces
     * between visible groups.
     */
    aWindow->totalDisplayEntries = aWindow->numElements +
	    (2 * (aWindow->numGroups - aWindow->numHiddenGroups)) - 1;
    free_scandir(nitems, &namelist);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * SelectFiles --
 *
 *	Routine passed to scandir(). 
 *
 * Results:
 *	Return TRUE or FALSE depending
 *	upon whether or not there's a group with a rule that would
 *	select this the given file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
SelectFiles(entryPtr)
    struct	direct	*entryPtr;
{
    FsflatWindow	*aWindow;
    FsflatGroup		*grpPtr;
    int			result;

    aWindow = (FsflatWindow *) scanData;
    for (grpPtr = aWindow->groupList; grpPtr != NULL;
	    grpPtr = grpPtr->nextPtr) {
	result = Pattern_Match(grpPtr->rule, entryPtr->d_name);
	if (result == 0) {
	    break;
	} else if (result < 0) {
	    /*
	     * I must check the rules as they are created, or the following
	     * would cause a billion notifiers!
	     */
	    sprintf(fsflatErrorMsg, "The rule %s contains an error.",
		grpPtr->rule);
	    Sx_Notify(fsflatDisplay, aWindow->displayWindow, -1, -1, 0,
		    fsflatErrorMsg, NULL, TRUE, "Continue", NULL);
	}
    }
    if (grpPtr == NULL) {
	return FALSE;
    }
    return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * SelectGroupFiles --
 *
 *	Routine passed to scandir() for selecting files for a single group. 
 *
 * Results:
 *	Return TRUE or FALSE depending
 *	upon whether or not the given group rule will select the given file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
SelectGroupFiles(entryPtr)
    struct	direct	*entryPtr;
{
    FsflatGroup		*grpPtr;
    int			result;
    FsflatWindow	*aWindow;

    aWindow = ((GatherInfo *) scanData)->aWindow;
    grpPtr = ((GatherInfo *) scanData)->grpPtr;
    if (grpPtr->defType == COMPARISON) {
	result = Pattern_Match(grpPtr->rule, entryPtr->d_name);
	if (result == 0) {
	    return TRUE;
	} else if (result < 0) {
	    sprintf(fsflatErrorMsg, "The rule %s contains an error.",
		grpPtr->rule);
	    Sx_Notify(fsflatDisplay, aWindow->displayWindow, -1, -1, 0,
		    fsflatErrorMsg, NULL, TRUE, "Continue", NULL);
	}
	return FALSE;
    }
    if (grpPtr->defType != PROC) {
	Sx_Notify(fsflatDisplay, aWindow->displayWindow, -1, -1, 0,
		"unknown type of rule definition", NULL,
		TRUE, "Continue", NULL);
	return FALSE;
    }
    if (FsflatDoTclSelect(aWindow->interp, grpPtr->rule, entryPtr->d_name,
	    &result) != TCL_OK) {
	Sx_Notify(fsflatDisplay, aWindow->displayWindow, -1, -1, 0,
		aWindow->interp->result, NULL, TRUE, "Continue", NULL);
	return FALSE;
    }
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatDoTclSelect --
 *
 *	Execute the tcl rule procName on the fileName arg and determine
 *	whether the file matches the rule.
 *
 * Results:
 *	TCL_OK if things work ok, and TCL_ERROR if not.  If the routine
 *	returns TCL_OK, then boolean value
 *	determining whether the file matches the rule is returned in the
 *	selectValPtr parameter.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
FsflatDoTclSelect(interp, procName, arg, selectValPtr)
    Tcl_Interp		*interp;
    char		*procName;
    char		*arg;
    int			*selectValPtr;
{
    char	*buffer;
    char	*cPtr;

    if (procName == NULL || arg == NULL) {
	strcpy(interp->result,
		"Null procedure name or argument for file selection");
	return TCL_ERROR;
    }
    buffer = (char *) malloc(strlen(procName) + strlen(arg) + 2);
    sprintf(buffer, "%s %s", procName, arg);
    if (Tcl_Eval(interp, buffer, '\0', NULL) != TCL_OK) {
	sprintf(interp->result,
		"Something is wrong with the rule definition %s.", procName);
	free(buffer);
	return TCL_ERROR;
    }
    free(buffer);
    if ((*selectValPtr = strtol(interp->result, &cPtr, 10)) == 0
	    && cPtr == interp->result) {
	sprintf(interp->result, "Bad proc definition %s", procName);
	return TCL_ERROR;
    }
    if (*selectValPtr < 0) {
	sprintf(interp->result, "proc %s returned an error value", procName);
	return TCL_ERROR;
    }
    /* reverse the meaning, since pattern match returns 0 for success... */
    if (*selectValPtr == 0) {
	*selectValPtr = 1;
    } else {
	*selectValPtr = 0;
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * AlphaForwards --
 *
 *	Compare two files for alphabetical order.
 *
 * Results:
 *	Negative if the first is before the second.  Zero if they are the
 *	same.  Positive if the first is after the second.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
AlphaForwards(first, second)
    struct	direct	**first, **second;
{
    return alphasort(first, second);
}


/*
 *----------------------------------------------------------------------
 *
 * AlphaReverse --
 *
 *	Compare two files for reverse alphabetical order.
 *
 * Results:
 *	Negative if the first is before the second.  Zero if they are the
 *	same.  Positive if the first is after the second.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
AlphaReverse(first, second)
    struct	direct	**first, **second;
{
    return (-alphasort(first, second));
}


int
AtimeForwards(first, second)
    struct	direct	**first, **second;
{
    return AtimeSort(first, second);
}

int
AtimeReverse(first, second)
    struct	direct	**first, **second;
{
    return (-AtimeSort(first, second));
}

#ifdef NOTDEF
int
CtimeForwards(first, second)
    struct	direct	**first, **second;
{
    return CtimeSort(first, second);
}

int
CtimeReverse(first, second)
    struct	direct	**first, **second;
{
    return (-CtimeSort(first, second));
}
#endif /* NOTDEF */

int
MtimeForwards(first, second)
    struct	direct	**first, **second;
{
    return MtimeSort(first, second);
}

int
MtimeReverse(first, second)
    struct	direct	**first, **second;
{
    return (-MtimeSort(first, second));
}

int
DtimeForwards(first, second)
    struct	direct	**first, **second;
{
    return DtimeSort(first, second);
}

int
DtimeReverse(first, second)
    struct	direct	**first, **second;
{
    return (-DtimeSort(first, second));
}

int
SizeForwards(first, second)
    struct	direct	**first, **second;
{
    return SizeSort(first, second);
}

int
SizeReverse(first, second)
    struct	direct	**first, **second;
{
    return (-SizeSort(first, second));
}

static int
FileAlphaSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return strcmp((*file1)->name, (*file2)->name);
}

static int
FileAlphaReverseSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return (-strcmp((*file1)->name, (*file2)->name));
}

static int
AtimeCompare(attrs1, attrs2)
    struct	stat	*attrs1;
    struct	stat	*attrs2;
{
    if (attrs1->st_atime > attrs2->st_atime) {
	return -1;
    }
    if (attrs1->st_atime< attrs2->st_atime) {
	return 1;
    }
#ifdef NOTDEF
    /*
     * What to do about this?  Sprite gives microseconds as well as
     * seconds...
     */
    if (attrs1->st_atime.microseconds > attrs2->st_atime.microseconds) {
	return -1;
    }
    if (attrs1->st_atime.microseconds < attrs2->st_atime.microseconds) {
	return 1;
    }
#endif /* NOTDEF */
    return 0;
}

static int
FileAtimeSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return AtimeCompare((*file1)->attrPtr, (*file2)->attrPtr);
}

static int
FileAtimeReverseSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return (-AtimeCompare((*file1)->attrPtr, (*file2)->attrPtr));
}

static	int
AtimeSort(d1, d2)
    struct	direct	**d1, **d2;
{
    struct	stat	attrs1;
    struct	stat	attrs2;

    if (lstat((*d1)->d_name, &attrs1) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d1)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    if (lstat((*d2)->d_name, &attrs2) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d2)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    return AtimeCompare(&attrs1, &attrs2);
 
}

#ifdef NOTDEF
static int
CtimeCompare(attrs1, attrs2)
    struct	stat	*attrs1;
    struct	stat	*attrs2;
{
    if (attrs1->createTime.seconds > attrs2->createTime.seconds) {
	return -1;
    }
    if (attrs1->createTime.seconds < attrs2->createTime.seconds) {
	return 1;
    }
    if (attrs1->createTime.microseconds > attrs2->createTime.microseconds) {
	return -1;
    }
    if (attrs1->createTime.microseconds < attrs2->createTime.microseconds) {
	return 1;
    }
    return 0;
}

static int
FileCtimeSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return CtimeCompare((*file1)->attrPtr, (*file2)->attrPtr);
}

static int
FileCtimeReverseSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return (-CtimeCompare((*file1)->attrPtr, (*file2)->attrPtr));
}

static	int
CtimeSort(d1, d2)
    struct	direct	**d1, **d2;
{
    struct	stat	attrs1;
    struct	stat	attrs2;

    if (lstat((*d1)->d_name, &attrs1) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d1)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    if (lstat((*d2)->d_name, &attrs2) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d2)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    return CtimeCompare(&attrs1, &attrs2);
}
#endif /* NOTDEF */

static int
MtimeCompare(attrs1, attrs2)
    struct	stat	*attrs1;
    struct	stat	*attrs2;
{
    if (attrs1->st_mtime > attrs2->st_mtime) {
	return -1;
    }
    if (attrs1->st_mtime < attrs2->st_mtime) {
	return 1;
    }
#ifdef NOTDEF
    if (attrs1->dataModifyTime.microseconds >
	    attrs2->dataModifyTime.microseconds) {
	return -1;
    }
    if (attrs1->dataModifyTime.microseconds <
	    attrs2->dataModifyTime.microseconds) {
	return 1;
    }
#endif /* NOTDEF */
    return 0;
}

static int
FileMtimeSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return MtimeCompare((*file1)->attrPtr, (*file2)->attrPtr);
}

static int
FileMtimeReverseSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return (-MtimeCompare((*file1)->attrPtr, (*file2)->attrPtr));
}

static	int
MtimeSort(d1, d2)
    struct	direct	**d1, **d2;
{
    struct	stat	attrs1;
    struct	stat	attrs2;

    if (lstat((*d1)->d_name, &attrs1) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d1)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    if (lstat((*d2)->d_name, &attrs2) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d2)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    return MtimeCompare(&attrs1, &attrs2);
}

static int
DtimeCompare(attrs1, attrs2)
    struct	stat	*attrs1;
    struct	stat	*attrs2;
{
    if (attrs1->st_ctime > attrs2->st_ctime) {
	return -1;
    }
    if (attrs1->st_ctime < attrs2->st_ctime) {
	return 1;
    }
#ifdef NOTDEF
    if (attrs1->st_ctime.microseconds > attrs2->st_ctime.microseconds) {
	return -1;
    }
    if (attrs1->st_ctime.microseconds < attrs2->st_ctime.microseconds) {
	return 1;
    }
#endif /* NOTDEF */
    return 0;
}

static int
FileDtimeSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return DtimeCompare((*file1)->attrPtr, (*file2)->attrPtr);
}

static int
FileDtimeReverseSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return (-DtimeCompare((*file1)->attrPtr, (*file2)->attrPtr));
}

static	int
DtimeSort(d1, d2)
    struct	direct	**d1, **d2;
{
    struct	stat	attrs1;
    struct	stat	attrs2;

    if (lstat((*d1)->d_name, &attrs1) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d1)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    if (lstat((*d2)->d_name, &attrs2) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d2)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    return DtimeCompare(&attrs1, &attrs2);
}

static int
SizeCompare(attrs1, attrs2)
    struct	stat	*attrs1;
    struct	stat	*attrs2;
{
    if (attrs1->st_size > attrs2->st_size) {
	return 1;
    }
    if (attrs1->st_size < attrs2->st_size) {
	return -1;
    }
    return 0;
}

static int
FileSizeSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return SizeCompare((*file1)->attrPtr, (*file2)->attrPtr);
}

static int
FileSizeReverseSort(file1, file2)
    FsflatFile	**file1;
    FsflatFile	**file2;
{
    return (-SizeCompare((*file1)->attrPtr, (*file2)->attrPtr));
}

static	int
SizeSort(d1, d2)
    struct	direct	**d1, **d2;
{
    struct	stat	attrs1;
    struct	stat	attrs2;

    if (lstat((*d1)->d_name, &attrs1) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d1)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    if (lstat((*d2)->d_name, &attrs2) != 0) {
	sprintf(fsflatErrorMsg, "Couldn't get attributes for %s.",
		(*d2)->d_name);
	/* Is panic the right thing? */
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    return SizeCompare(&attrs1, &attrs2);
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatGetCompareProc --
 *
 *	Figure out what the comparison routine is and return it in
 *	the compareProc parameter.
 *	If fileProcP is true, return a function that takes pointers
 *	to FsflatFiles.  Otherwise it takes struct	direct's.
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
FsflatGetCompareProc(aWindow, compareProcPtr, fileProcP)
    FsflatWindow	*aWindow;
    int	(**compareProcPtr)();
    Boolean		fileProcP;	/* which comp func type */
{
    if (aWindow->sortingInstructions == 0) {
	aWindow->sortingInstructions = FSFLAT_ALPHA_SORT;	/* default */
    }
    if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_ALPHA_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileAlphaReverseSort;
	    } else {
		*compareProcPtr = AlphaReverse;
	    }
	} else if (aWindow->sortingInstructions & FSFLAT_ATIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileAtimeReverseSort;
	    } else {
		*compareProcPtr = AtimeReverse;
	    }
#ifdef NOTDEF
	} else if (aWindow->sortingInstructions & FSFLAT_CTIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileCtimeReverseSort;
	    } else {
		*compareProcPtr = CtimeReverse;
	    }
#endif NOTDEF
	} else if (aWindow->sortingInstructions & FSFLAT_MTIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileMtimeReverseSort;
	    } else {
		*compareProcPtr = MtimeReverse;
	    }
	} else if (aWindow->sortingInstructions & FSFLAT_DTIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileDtimeReverseSort;
	    } else {
		*compareProcPtr = DtimeReverse;
	    }
	} else if (aWindow->sortingInstructions & FSFLAT_SIZE_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileSizeReverseSort;
	    } else {
		*compareProcPtr = SizeReverse;
	    }
	}
    } else {
	if (aWindow->sortingInstructions & FSFLAT_ALPHA_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileAlphaSort;
	    } else {
		*compareProcPtr = AlphaForwards;
	    }
	} else if (aWindow->sortingInstructions & FSFLAT_ATIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileAtimeSort;
	    } else {
		*compareProcPtr = AtimeForwards;
	    }
#ifdef NOTDEF
	} else if (aWindow->sortingInstructions & FSFLAT_CTIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileCtimeSort;
	    } else {
		*compareProcPtr = CtimeForwards;
	    }
#endif /* NOTDEF */
	} else if (aWindow->sortingInstructions & FSFLAT_MTIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileMtimeSort;
	    } else {
		*compareProcPtr = MtimeForwards;
	    }
	} else if (aWindow->sortingInstructions & FSFLAT_DTIME_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileDtimeSort;
	    } else {
		*compareProcPtr = DtimeForwards;
	    }
	} else if (aWindow->sortingInstructions & FSFLAT_SIZE_SORT) {
	    if (fileProcP) {
		*compareProcPtr = FileSizeSort;
	    } else {
		*compareProcPtr = SizeForwards;
	    }
	}
    }

    return;
}
