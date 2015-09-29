/* 
 * update.c --
 *
 *	A smart copy program that preserves date stamps and only
 *	copies if files are out-of-date.
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
static char rcsid[] = "$Header: /sprite/src/cmds/update/RCS/update.c,v 1.33 92/10/30 14:01:08 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <a.out.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <limits.h>
#ifdef sprite
#include <fs.h>
#endif
#include <grp.h>
#include <pwd.h>
#include "regexp.h"
#include "option.h"

#ifndef HASSTRERROR
char *strerror();
#endif

#if (defined(sunos) || defined(OSF1))
#define DirObject struct dirent
#else
#define DirObject struct direct
#endif

/*
 * Library imports:
 */

extern long lseek();

/*
 * Variables and tables used to parse and identify switch values:
 */

static int strip = 0;
static int move = 0;
static char *backupDir = (char *)0;
static int backupAge = 14;
static char *modeString = (char *)0;
static int newMode = -1;
static char *ownerName = (char *)0;
static int newOwner = -1;
static int preserveOwnership = 0;
static char *groupName = (char *)0;
static int newGroup = -1;
static int force = 0;
static int noLinks = 0;
static int copyLinks = 1;
static int setTimes = 1;
static int quietMode = 0;
static int verifyMode = 0;
static int niceMode = 0;
static int PruneOpt();
static int ignoreLinks = 0;

static Option optionArray[] = {
    {OPT_STRING, "b", (char *) &backupDir,
	    "Next argument contains name of backup directory"},
    {OPT_INT, "B", (char *) &backupAge,
	    "Next argument contains age (in days) needed to cause backup to overwrite older backup"},
    {OPT_TRUE, "f", (char *) &force,
	    "Force: always update, regardless of time stamps"},
    {OPT_STRING, "g", (char *) &groupName,
	    "Next argument contains name of group for destination file(s)"},
    {OPT_FALSE, "l", (char *) &copyLinks,
	    "Copy files referenced by symbolic links\n\t\tDefault: copy symbolic links as symbolic links"},
    {OPT_STRING, "m", (char *) &modeString,
	    "Next argument contains new protection bits for file(s)"},
    {OPT_TRUE, "M", (char *) &move, "Move instead of copy"},
    {OPT_TRUE, "n", (char *) &niceMode, "Be nice about potential errors"},
    {OPT_STRING, "o", (char *) &ownerName,
	    "Next argument contains name of owner for destination file(s)"},
    {OPT_TRUE, "O", (char *) &preserveOwnership,
	    "Preserve ownership of files"},
    {OPT_TRUE, "q", (char *) &quietMode,
	    "Quiet mode:  only print error messages"},
    {OPT_TRUE, "s", (char *) &strip, "Strip destination (not supported on ds3100/hpux"},
    {OPT_FALSE, "t", (char *) &setTimes, "Use current time for file access times\n\t\tDefault: set target times to match source"},
    {OPT_TRUE, "v", (char *) &verifyMode,
	    "Verify mode:  print messages, but don't modify anything"},
    {OPT_FUNC, "p", (char *) PruneOpt,
	    "Prune sub-trees defined by given regular expression"}, 
    {OPT_TRUE, "i", (char *) &ignoreLinks,
	    "Ignore symbolic links completely"}, 
};

/*
 * Miscellaneous global variables:
 */

static int realUid;		/* The user id of the caller (which is not
				 * our effective user id, since we're running
				 * setuid. */

#define MAX_PRUNE	25
static int prune = 0;		/* Number of regular expressions to prune. */
static struct {
    regexp	*exp;		/* compiled expression */
    char	*expString;	/* expression string */
} pruneArray[MAX_PRUNE]; /* Regular expressions to prune. */

#ifdef sprite
#define MAKELINK(x,y,z) Fs_SymLink((x),(y),((z)==S_IFRLNK))
#define MAKEMSG(x) Stat_GetMsg(x)
#define GETATTRS(x,y) Fs_GetAttributes((x),FALSE,(y))
#else
typedef int ReturnStatus;
#define SUCCESS 0
#define MAKELINK(x,y,z) symlink((x),(y))
#define MAKEMSG(x) strerror(x)
#define GETATTRS(x,y) stat((x),(y))
#endif
/*
 * Forward references to procedures declared later in this file:
 */

static int Update();
static int UpdateDir();
static int Copy();
static void CheckGroup();
static void PrintUsageAndExit();
static int SetAttributes();
static int CreateDirectory();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for "update".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Files get copied, etc.
 *
 *----------------------------------------------------------------------
 */
void
main(argc, argv)
    int argc;
    char *argv[];
{
    char *destFile;
    char *term;
    struct stat srcAttr, destAttr;
    register int argIndex;
    int numErrors = 0;
    char *userName;

    /*
     * Suck up command line options and check protection.
     */

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray),
	    OPT_ALLOW_CLUSTERING);
#if (defined(ds3100) || defined(__mips) || defined(hpux))
    if (strip) {
	strip = 0;
    }
#endif
    realUid = getuid();
    if (preserveOwnership ||
	(ownerName != (char *) 0) || (groupName != (char *) 0)) {
	register struct passwd *pwPtr;

	pwPtr = getpwuid(realUid);
	if (pwPtr == (struct passwd *) 0) {
	    fprintf(stderr,
		    "Couldn't find password entry for user %d.\n",
		    realUid);
	    exit(1);
	}
	userName = malloc((unsigned) (strlen(pwPtr->pw_name) + 1));
	strcpy(userName, pwPtr->pw_name);
	setpwent();
    }
    if (preserveOwnership) {
	if (ownerName != (char *)0) {
	    fprintf(stderr, "Ignoring -o option in favor of -O\n");
	    ownerName = (char *)0;
	}
	if (groupName != (char *)0) {
	    fprintf(stderr, "Ignoring -g option in favor of -O\n");
	    groupName = (char *)0;
	}
	/*
	 * Don't allow ownership preservation unless:
	 *	1. Caller is root -or-
	 *	2. Caller is in the wheel group.
	 */
	if (realUid != 0) {
	    CheckGroup("wheel", userName);
	    realUid = 0;
	}
    }

    if (ownerName != (char *)0) {
	register struct passwd *pwPtr;
	pwPtr = getpwnam(ownerName);
	if (pwPtr == (struct passwd *)0) {
	    fprintf(stderr, "Unknown user \"%s\".\n", ownerName);
	    exit(1);
	}
	newOwner = pwPtr->pw_uid;

	/*
	 * Don't allow a change of owner unless one of three things is true:
	 *     1. Caller is root.
	 *     2. Target uid is root, and the caller is in the "wheel" group.
	 *     3. There's a group name by the same name as the target uid,
	 *	  and the caller is in the group.
	 */

	if ((realUid != 0) && (realUid != newOwner)) {
	    register struct group *grPtr;
	    char *groupName;
	    int i;

	    if (newOwner == 0) {
		groupName = "wheel";
	    } else {
		groupName = ownerName;
	    }
	    CheckGroup(groupName, userName);
	}
	realUid = newOwner;
    }
    if (setuid(realUid) != 0) {
	fprintf(stderr, "Couldn't change user id: %s\n", strerror(errno));
	exit(1);
    }

    if (groupName != (char *) 0) {
	register struct group *grPtr;
	grPtr = getgrnam(groupName);
	if (grPtr == (struct group *)0) {
	    fprintf(stderr, "Unknown group \"%s\".\n", groupName);
	    exit(1);
	}
	newGroup = grPtr->gr_gid;

	/*
	 * Don't allow a change of group unless either
	 *     1. Caller is root.
	 *     2. Caller is in the group being changed to.
	 */

	if (realUid != 0) {
	    int i;

	    for (i = 0; ; i++) {
		if (grPtr->gr_mem[i] == NULL) {
		    fprintf(stderr,
			    "Sorry, but you're not in the \"%s\" group.\n",
			    groupName);
		    exit(1);
		}
		if (strcmp(grPtr->gr_mem[i], userName) == 0) {
		    break;
		}
	    }
	}
	endgrent();
    }
    if (modeString != (char *)0) {
	newMode = strtoul(modeString, &term, 8);
	if ((term == modeString) || (*term != 0)) {
	    fprintf(stderr, "Bad mode value \"%s\": should be octal integer\n",
		    modeString);
	    exit(1);
	}
    }

    /*
     * Convert backupAge into the date we need in seconds, instead of days
     * prior to the current time.
     */
    if (backupDir && backupAge > 0) {
	backupAge = time(0) - backupAge * 60 * 60 * 24;
    }
    /*
     * Check for a reasonable number of arguments (normally at least 2
     * in addition to the command name).
     */

    if (argc < 3) {
	if (argc == 2) {
	    destFile = argv[1];
	    if ((stat(destFile, &destAttr) == 0)
		    && ((destAttr.st_mode & S_IFMT) == S_IFDIR)) {
		/*
		 * Exit cleanly if there is a target directory but no
		 * sources.  This situation arises occasionally in makefiles.
		 */
		fprintf(stderr, "No sources for \"%s\".\n", destFile);
		exit(0);
	    }
	}
	PrintUsageAndExit(argv);
    }

    /*
     * Determine the initial case.  There are two:
     * 	% update {src1 ... srcN} destDir
     *  % update src dest
     *
     * The tricky thing is how to decide when to treat the destination
     * as a directory (and thus put things INSIDE it) and when to treat
     * it as a file (and thus REPLACE it).  We always work in REPLACE
     * mode unless the destination is a directory and either a) there's
     * more than one source or b) the single source isn't a directory.
     */

    destFile = argv[argc-1];
    if (stat(destFile, &destAttr) != 0) {
	if (errno == ENOENT) {
	    if (argc == 3) {
		/*
		 * update src dest - src can be file or directory
		 */
		numErrors = Update(argv[1], argv[2]);
		exit(numErrors);
	    }

	    /*
	     * Create the destination directory.
	     */

	    if (!quietMode) {
		fprintf(stderr, "Installing: %s\n", destFile);
	    }
	    if (!verifyMode) {
		if (mkdir(destFile, 0777) != 0) {
		    fprintf(stderr,
			"Couldn't create destination directory \"%s\": %s.\n",
			destFile, strerror(errno));
		    exit(1);
		}
		if (stat(destFile, &destAttr) != 0) {
		    fprintf(stderr,
			    "Couldn't stat \"%s\" after creating it: %s.\n",
			    destFile, strerror(errno));
		    exit(1);
		}
		if (SetAttributes(destFile, &destAttr, 0) != 0) {
		    exit(1);
		}
	    }

	    /*
	     * Fall through to the code to put things inside the
	     * destination directory.
	     */
	    goto putInside;
	}
	fprintf(stderr, "Couldn't access \"%s\": %s.\n", destFile,
		strerror(errno));
	exit(1);
    } else {
	if ((destAttr.st_mode & S_IFMT) == S_IFDIR) {
	    if (argc == 3) {
		int result;
		if (copyLinks) {
		    result = lstat(argv[1], &srcAttr);
		} else {
		    result = stat(argv[1], &srcAttr);
		}
		if (result != 0) {
		    fprintf(stderr, "Couldn't access \"%s\": %s.\n", argv[1],
			    strerror(errno));
		    exit(1);
		}
		if ((srcAttr.st_mode & S_IFMT) == S_IFDIR) {
		    /*
		     * Update dir1 dir2
		     */
		    numErrors = Update(argv[1], argv[2]);
		    exit(numErrors);
		}
	    }
	} else if (argc == 3) {
	    /*
	     * Update file1 file2
	     */
	    numErrors = Update(argv[1], argv[2]);
	    exit(numErrors);
	} else {
	    PrintUsageAndExit(argv);
	}
    }

    /*
     * The above cases handled all the replacements.  At this point there
     * are one or more files to be put INSIDE the destination directory:
     *
     * % update src1 ... srcN destDir
     */

    putInside:

    for (argIndex = 1; argIndex < argc - 1 ; argIndex++) {
	numErrors += UpdateDir("", argv[argIndex], destFile);
    }

    if (numErrors == 0) {
	exit(0);
    }
    exit(1);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintUsageAndExit --
 *
 *	Print a usage message and exit the process.
 *
 * Results:
 *	Never returns.
 *
 * Side effects:
 *	Stuff gets printed on stderr.
 *
 *----------------------------------------------------------------------
 */

static void
PrintUsageAndExit(argv)
    char *argv[];
{
    fprintf(stderr, "Usage: %s srcFile destFile\n", argv[0]);
    fprintf(stderr, "       %s file1 file2 ... destDir\n", argv[0]);
    Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
    exit(1);
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateDir --
 *
 *	Update a file inside a directory.  This creates the name of the
 *	target file from that of the source file and the destination
 *	directory.  Then Update is called to do the actual updating.
 *
 * Results:
 *	The return value is the number of errors encountered.
 *
 * Side effects:
 *	Files get created or replaced.
 *
 *----------------------------------------------------------------------
 */

static int
UpdateDir(srcDir, srcFile, destDir)
    char *srcDir;		/* Directory containing srcFile. */
    char *srcFile;		/* Source file name relative to srcDir. */
    char *destDir;		/* Destination directory name. */
{
    register char *charPtr;
    char newDest[MAXPATHLEN];
    char newSrc[MAXPATHLEN];

    /*
     * Set up the source file name.  Append the file to the directory name.
     */
    strcpy(newSrc, srcDir);
    if (*srcDir != '\0') {
	strcat(newSrc, "/");
    }
    strcat(newSrc, srcFile);

    /*
     * Set up the destination name.  We take the last component of the
     * filename here.  This handles command lines like
     * % update a/b/c dest
     * which will update dest/c from a/b/c.
     * When we are called recusively srcFile only has one component.
     */
    strcpy(newDest, destDir);
    charPtr = strrchr(srcFile, '/');
    if (charPtr == (char *)NULL) {
	strcat(newDest, "/");
	strcat(newDest, srcFile);
    } else {
	strcat(newDest, charPtr);
    }
    return (Update(newSrc, newDest));
}

/*
 *----------------------------------------------------------------------
 *
 * Update --
 *
 *	Update a file.  This checks the date stamps on the destination
 * 	file and copies the file data if needed, or just updates the
 *	destination file's attributes, or does nothing if the file
 *	is up-to-date.
 *
 * Results:
 *	The return value is the number of errors encountered.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
Update(srcFile, destFile)
    char *srcFile;		/* Name of source file. */
    char *destFile;		/* Name of file that should be made
				 * identical to source file. */
{
    int result;	
    struct stat srcAttr, destAttr, backupAttr;
    char *lastSlash;
    char tmpFileName[MAXPATHLEN];
    static char *typeNames[] = {
	"0",			"010000",		"character special",
	"030000",		"directory",		"050000",
	"block special",	"070000",		"regular file",
	"0110000",		"symbolic link",	"0130000",
	"socket",		"pseudo-device",	"remote link",
	"0170000"
    };
    int backup;
    int	i;

    if (prune) {
	lastSlash = strrchr(srcFile, '/');
	if (lastSlash == NULL) {
	    lastSlash = srcFile;
	} else {
	    lastSlash += 1;
	}
	for (i = 0; i < prune; i++) {
	    if (regexec(pruneArray[i].exp, lastSlash)) {
		if (!quietMode) {
		    fprintf(stderr, "Pruning:    %s { %s }\n", destFile,
			pruneArray[i].expString);
		}
		return 0;
	    }
	}
    }
    if (copyLinks || ignoreLinks) {
	result = lstat(srcFile, &srcAttr);
    } else {
	result = stat(srcFile, &srcAttr);
    }
    if (result != 0) {
	fprintf(stderr, "Couldn't find \"%s\": %s.\n",
		srcFile, strerror(errno));
	return 1;
    }

    if (((srcAttr.st_mode & S_IFMT) == S_IFLNK) && ignoreLinks) {
	if (!quietMode) {
	    fprintf(stderr, "Ignoring link: %s\n", srcFile);
	}
	return 0;
    }

    if ((lstat(destFile, &destAttr) != 0) && (errno = ENOENT)) {
	/*
	 * OK to create new file.
	 */
	if ((srcAttr.st_mode & S_IFMT) != S_IFDIR) {
	    if (!quietMode) {
		fprintf(stderr, "Installing: %s\n", destFile);
	    }
	    if (verifyMode) {
		return 0;
	    }
	    return Copy(srcFile, &srcAttr, destFile, 0);
	} else {
	    /*
	     * Make the target directory and then fall through to
	     * the code below which recursively updates it.
	     */
	    if (!quietMode) {
		fprintf(stderr, "Installing: %s\n", destFile);
	    }
	    if (!verifyMode) {
		if (mkdir(destFile, (int) (srcAttr.st_mode & 0777)) != 0) {
		    fprintf(stderr, "Couldn't create directory \"%s\": %s.\n",
			    destFile, strerror(errno));
		    return 1;
		}
		if (SetAttributes(destFile, &srcAttr, 0) != 0) {
		    return(1);
		}
	    }
	    destAttr = srcAttr;
	}
    }
    if ((destAttr.st_mode & S_IFMT) != (srcAttr.st_mode & S_IFMT)) {
	fprintf(stderr,  "Type of \"%s\" (%s) differs from \"%s\" (%s).\n",
		srcFile, typeNames[(srcAttr.st_mode & S_IFMT) >> 12],
		destFile, typeNames[(destAttr.st_mode & S_IFMT) >> 12]);

	if (niceMode) {
	    return 0;
	}
	/*
	 * Don't let the user get confused by failing to copy a link to a link
	 * because of type mismatch.  This may easily happen if something
	 * is installed as a link and then later the "copyLinks" flag is
	 * disabled.
	 */
	if (!copyLinks && ((destAttr.st_mode & S_IFMT) == S_IFLNK) &&
	    ((srcAttr.st_mode & S_IFMT) == S_IFREG)) {
	    if (lstat(srcFile, &srcAttr) == 0 &&
		((srcAttr.st_mode & S_IFMT) == S_IFLNK)) {
		fprintf(stderr, "\tNote: following a link to a regular file due to \"-l\" option.\n");
	    }
	}
	return(1);
    }
    if ((srcAttr.st_mode & S_IFMT) == S_IFDIR) {
	DIR *dirStream;
	DirObject *dirEntryPtr;
	int numErrors = 0;

	/*
	 * See if we can just rename the directory.
	 */

	if (move && !verifyMode && !strip) {
	    if (rename(srcFile, destFile) == 0) {
		return 0;
	    }
	}

	/*
	 * Recursively update the target directory, which already exists.
	 *	Read all names from srcFile directory, and call
	 *	UpdateDir(eachName, destFile) to update each one.
	 */

	dirStream = opendir(srcFile);
	if (dirStream == (DIR *)NULL) {
	    fprintf(stderr, "Can't read source directory \"%s\".\n",
			    srcFile);
	    return(1);
	}
	dirEntryPtr = readdir(dirStream);
	while (dirEntryPtr != (DirObject *)NULL) {
	    if ((dirEntryPtr->d_namlen == 1 &&
		 dirEntryPtr->d_name[0] == '.') ||
		(dirEntryPtr->d_namlen == 2 &&
		 dirEntryPtr->d_name[0] == '.' &&
		 dirEntryPtr->d_name[1] == '.')) {
		/* Don't do "." or ".." */ ;
	    } else {
		numErrors += UpdateDir(srcFile, dirEntryPtr->d_name,
			destFile);
	    }
	    dirEntryPtr = readdir(dirStream);
	}
	closedir(dirStream);

	/*
	 * If moving, must delete source directory.
	 */

	if (move && !verifyMode && (rmdir(srcFile) != 0)) {
	    fprintf(stderr,
		    "Couldn't remove directory \"%s\" during move: %s\n",
		    srcFile, strerror(errno));
	    numErrors++;
	}
	return(numErrors);
     }
     if (force || (destAttr.st_mtime < srcAttr.st_mtime)) {
	/*
	 * Target file has to be updated.
	 */

	if (!quietMode) {
	    fprintf(stderr, "Updating: %s\n", destFile);
	}
	if (verifyMode) {
	    return 0;
	}

	/*
	 * If no backup directory, then rename the file, copy a new
	 * version in, and remove the old renamed target.
	 */

	if (backupDir == (char *) 0) {
	    sprintf(tmpFileName, "%sXXX", destFile);

	    if (rename(destFile, tmpFileName) != 0) {
		fprintf(stderr, "Couldn't rename \"%s\" to \"%s\": %s.\n",
			destFile, tmpFileName, strerror(errno));
		return(1);
	    }
	    if (Copy(srcFile, &srcAttr, destFile, 0) == 0) {
		if (unlink(tmpFileName) != 0) {
		    fprintf(stderr,
			    "Couldn't remove renamed old version \"%s\": %s.\n",
			    tmpFileName, strerror(errno));
		    return 1;
		}
	    } else {
		if (rename(tmpFileName, destFile) != 0) {
		    fprintf(stderr,
			"Couldn't restore original \"%s\":  see \"%s\".\n",
			destFile, tmpFileName);
		}
		return 1;
	    }
	    return 0;
	}

	/*
	 * There's a backup directory.  Copy the file to it, then
	 * copy in new version.
	 */

	lastSlash = strrchr(destFile, '/');
	if (lastSlash == NULL) {
	    lastSlash = destFile;
	} else {
	    lastSlash += 1;
	}
	sprintf(tmpFileName, "%s/%s", backupDir, lastSlash);

	backup = 1;
	if (backupAge > 0 && stat(tmpFileName, &backupAttr) == 0) {

	    if (destAttr.st_mtime > backupAge) {
		if (!quietMode) {
		    fprintf(stderr,
			    "\tWarning: target too recent, so not overwriting backup copy.\n");
		}
		backup = 0;
	    }
	}
	if (backup && Copy(destFile, &destAttr, tmpFileName, 1) != 0) {
	    fprintf(stderr,
		    "Couldn't copy \"%s\" to backup dir \"%s\".\n",
		    destFile, backupDir);
	    return 1;
	}
	unlink(destFile);
	return Copy(srcFile, &srcAttr, destFile, 0);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Copy --
 *
 *	Copy a file.  For regular files this copies the file data.
 *	For other special files a new file of the same type as
 *	the original is created.
 *
 * Results:
 *	0 is returned if all went well, and 1 is returned if an
 *	error occurred.
 *
 * Side effects:
 *	srcFile gets copied to destFile.
 *
 *----------------------------------------------------------------------
 */

static int
Copy(srcFile, srcAttrPtr, destFile, backup)
    char *srcFile;			/* Name of source file. */
    struct stat *srcAttrPtr;		/* Attributes of source file (used to
					 * save stat calls). */
    char *destFile;			/* Name of destination file. */
    int backup;				/* Non-zero means this is a backup
					 * copy being made;  as a result,
					 * attribute changes are handled
					 * differently. */
{
#define BUFSIZE 8192
    static char buffer[BUFSIZE];

    /*
     * See if we can just rename the file.
     */
    if (move && (realUid == srcAttrPtr->st_uid) && !strip) {
	if (rename(srcFile, destFile) == 0) {
	    return 0;
	}
    }

    switch (srcAttrPtr->st_mode & S_IFMT) {
	case S_IFREG: {
	    int result = 0;
	    int amountRead, bytesCopied;
	    int maxSize = INT_MAX;
	    int srcID, destID;
	    int stripping = 0;

	    srcID = open(srcFile, O_RDONLY, 0); 
	    if (srcID < 0) {
		fprintf(stderr, "Couldn't open \"%s\": %s.\n",
			srcFile, strerror(errno));
		return 1;
	    }
	    (void) unlink(destFile);
	    destID = open(destFile, O_WRONLY|O_CREAT|O_TRUNC,
		    srcAttrPtr->st_mode & 0777);
	    if (destID < 0) {
		char pathname[MAXPATHLEN];
		char *s;
		int realErrno;

		/*
		 * Some of the subdirectories in the path may
		 * not exist.  Try and create them, and if
		 * successful, try to open the file again.  Make sure
		 * that if we don't successfully create subdirectories and
		 * then redo the open, we return the errno corresponding
		 * to the open, not the failed mkdir.
		 */

		realErrno = errno;
		strcpy(pathname, destFile);
		if ((s = strrchr(pathname, '/')) != NULL) {
		    *s = '\0';
		    if (CreateDirectory(pathname) == 0) {
			destID = open(destFile, O_WRONLY|O_CREAT|O_TRUNC,
			    srcAttrPtr->st_mode & 0777);
		    } else {
			errno = realErrno;
		    }
		}
		if (destID < 0) {
		    fprintf(stderr, "Couldn't create \"%s\": %s.\n",
			destFile, strerror(errno));
		    return 1;
		}
	    }
#if (!defined(ds3100) && !defined(__mips) && !defined(hpux))
	    if (strip) {
		struct exec hdr;
		if ((read(srcID, (char *) &hdr, sizeof(hdr)) != sizeof(hdr))
			|| N_BADMAG(hdr)) {
		    fprintf(stderr,
			    "\"%s\" isn't a binary executable;  can't strip.\n",
			    srcFile);
		} else {
		    maxSize = N_TXTOFF(hdr) + hdr.a_text + hdr.a_data;
		    stripping = 1;
		}
		lseek(srcID, (long) 0, L_SET);
	    }
#endif

	    for (bytesCopied = 0; bytesCopied < maxSize;
		    bytesCopied += amountRead) {
		int wanted;

		wanted = maxSize - bytesCopied;
		if (wanted > BUFSIZE) {
		    wanted = BUFSIZE;
		}
		amountRead = read(srcID, buffer, wanted);
		if (amountRead < 0) {
		    fprintf(stderr, "Read error on \"%s\": %s.\n",
			    srcFile, strerror(errno));
		    result = 1;
		    break;
		}
		if (amountRead == 0) {
		    break;
		}

#if (!defined(ds3100) && !defined(__mips) && !defined(hpux))
		/*
		 * If we're stripping, clear out the symbol table size
		 * in the file's header.
		 */

		if ((bytesCopied == 0) && stripping) {
		    struct exec *execPtr = (struct exec *) buffer;

		    execPtr->a_syms = 0;
		    execPtr->a_trsize = 0;
		    execPtr->a_drsize = 0;
		}
#endif
		if (write(destID, buffer, amountRead) != amountRead) {
		    fprintf(stderr, "Write error on \"%s\": %s.\n",
			    destFile, strerror(errno));
		    result = 1;
		    break;
		}
	    }
	    close(srcID);
	    close(destID);
	    if (result != 0) {
		return result;
	    }
	    break;
	}
	case S_IFDIR: {
	    fprintf(stderr, "Internal error:  Copy called with directory.\n");
	    return 1;
	}
	case S_IFLNK:
#ifdef sprite
	case S_IFRLNK:
#endif
	    {
	    char targetName[MAXPATHLEN];
	    int length;
	    ReturnStatus	status;

	    length = readlink(srcFile, targetName, MAXPATHLEN);
	    if (length < 0) {
		fprintf(stderr, "Couldn't read value of link \"%s\": %s.\n",
			srcFile, strerror(errno));
		return 1;
	    }
	    targetName[length] = 0;
	    status = MAKELINK(targetName, destFile, 
			      (srcAttrPtr->st_mode & S_IFMT));
	    if (status != SUCCESS) {
		char pathname[MAXPATHLEN];
		char *s;
		int realStatus;

		/*
		 * Some of the subdirectories in the path may
		 * not exist.  Try and create them, and if
		 * successful, try to open the file again.  Make sure
		 * that if we don't successfully create subdirectories and
		 * then redo the open, we return the errno corresponding
		 * to the open, not the failed mkdir.
		 */

		realStatus = status;
		strcpy(pathname, destFile);
		if ((s = strrchr(pathname, '/')) != NULL) {
		    *s = '\0';
		    if (CreateDirectory(pathname) == 0) {
			status = MAKELINK(targetName, destFile, 
			    (srcAttrPtr->st_mode & S_IFMT));
		    } else {
			status = realStatus;
		    }
		}
		if (status != SUCCESS) {
		    fprintf(stderr,
			    "Couldn't create link at \"%s\": %s.\n",
			    destFile, MAKEMSG(status));
		    return 1;
		}
	    }
	    break;
	}
	case S_IFBLK:
	case S_IFCHR: {
	    ReturnStatus	status;
#ifdef sprite
	    Fs_Attributes 	attrs;
	    Fs_Device		device;
#else
	    struct stat attrs;
#endif

	    status = GETATTRS(srcFile, &attrs);
	    if (status != SUCCESS) {
		fprintf(stderr, "Unable to get attributes of \"%s\": %s\n",
		    srcFile, MAKEMSG(status));
		return 1;
	    }
#ifdef sprite
	    device.type = attrs.devType;
	    device.unit = attrs.devUnit;
	    device.serverID = attrs.devServerID;
	    device.data = (ClientData) 0;
	    status = Fs_MakeDevice(destFile, &device, 
				   srcAttrPtr->st_mode & 0777);
#else
	    status = mknod(destFile, srcAttrPtr->st_mode,
			   srcAttrPtr->st_dev);
#endif
	    if (status != SUCCESS) {
		fprintf(stderr, "Unable to create device \"%s\": %s\n",
		    destFile, MAKEMSG(status));
		return 1;
	    }
	    break;
	}
	case S_IFIFO: {
	    fprintf(stderr, "\"%s\" is a fifo; don't know how to update.\n",
		    srcFile);
	    return 1;
	}
#ifdef sprite
	case S_IFPDEV: {
	    fprintf(stderr,
		    "\"%s\" is a pseudo-device; don't know how to update.\n",
		    srcFile);
	    return 1;
	}
#endif
	default: {
	    fprintf(stderr,
		    "\"%s\" has type 0x%x; don't know how to update.\n",
		    srcFile, srcAttrPtr->st_mode & S_IFMT);
	    return 1;
	}
    }
    if (SetAttributes(destFile, srcAttrPtr, backup) != 0) {
	return 1;
    }
    if (move) {
	if (unlink(srcFile) != 0) {
	    fprintf(stderr,
		    "Couldn't remove source file \"%s\" during move: %s.\n",
		    srcFile, strerror(errno));
	    return 1;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckGroup --
 *
 *	Verify that a user is in a particular group.  This is called to
 *	check against the wrong users attempting to use update to
 *	change file ownership.
 *
 * Results:
 *	Returns if the user is ok, otherwise this exits.
 *
 * Side effects:
 *	Suicide if the user isn't in the group.
 *
 *----------------------------------------------------------------------
 */

static void
CheckGroup(groupName, userName)
    char *groupName;		/* group name */
    char *userName;		/* user name */
{
    register struct group *grPtr;
    int i;

    grPtr = getgrnam(groupName);
    if (grPtr == (struct group *) 0) {
	fprintf(stderr, "Couldn't find \"%s\" group to check owner change.\n",
		groupName);
	exit(1);
    }
    for (i = 0; ; i++) {
	if (grPtr->gr_mem[i] == NULL) {
	    fprintf(stderr,
		    "Can't change owners: you're not in the \"%s\" group.\n",
		    groupName);
	    exit(1);
	}
	if (strcmp(grPtr->gr_mem[i], userName) == 0) {
	    break;
	}
    }
    endgrent();
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * SetAttributes --
 *
 *	Set attributes of target file, taking into account command line
 *	options that specify group and new mode.
 *
 * Results:
 *	Returns 0 if all went well, 1 if there was an error.
 *
 * Side effects:
 *	Times, protection, and group may get changed for fileName.
 *
 *----------------------------------------------------------------------
 */

static int
SetAttributes(fileName, attrPtr, backup)
    char *fileName;			/* Name of file to change. */
    struct stat *attrPtr;		/* Attributes of source file, which
					 * are used (along with command-line
					 * options) to set fileName. */
    int backup;				/* Non-zero means that this is a
					 * backup copy whose attributes are
					 * being set;  ignore command-line
					 * options and just copy relevant
					 * attributes from the source. */

{
#ifdef SYSV
    struct utimbuf times;
#else
    struct timeval times[2];
#endif

    /*
     * Preserve ownership if requested (or if this is a backup copy).
     */

    if (preserveOwnership && !backup) {
	if (chown(fileName, attrPtr->st_uid, attrPtr->st_gid) != 0) {
	    fprintf(stderr, "Couldn't set owner for \"%s\": %s.\n",
		    fileName, strerror(errno));
	    return 1;
	}
    }

    /*
     * Set group if a specific one was requested.
     */

    if ((newGroup >= 0) && !backup) {
	if (chown(fileName, -1, newGroup) != 0) {
	    fprintf(stderr, "Couldn't set group id for \"%s\": %s.\n",
		    fileName, strerror(errno));
	    return 1;
	}
    }

    /*
     * Don't set permissions or times for symbolic links, since they'll
     * end up affecting the target of the link.
     */

    if ((attrPtr->st_mode & S_IFMT) == S_IFLNK) {
	return 0;
    }

    /*
     * Set permissions (but only if the source isn't a symbolic link;  if
     * it's a link, then we'd be setting the permissions of the link's
     * target).
     */

    if ((newMode >= 0) && !backup) {
	if (chmod(fileName, newMode) != 0) {
	    fprintf(stderr,
		    "Couldn't set permissions of \"%s\" to 0%o: %s.\n",
		    fileName, newMode, strerror(errno));
	    return 1;
	}
    } else {
	/* 
	 * Set the file permissions.  When the file was created the
	 * permissions were modified by umask, so we have to set them
	 * here.
	 */
	if (chmod(fileName, attrPtr->st_mode & 0777) != 0) {
	    fprintf(stderr,
		    "Couldn't set permissions for \"%s\": %s.\n",
		    fileName, strerror(errno));
	    return 1;
	}
	/*
	 * Preserve the set-user-id bit if a new mode wasn't given, and if
	 * the set-user-id bit was set in the source file, and if the file's
	 * new owner is the same as its previous owner.
	 */
    
	if ((attrPtr->st_mode & (S_ISUID)) &&
		(preserveOwnership || (realUid == attrPtr->st_uid))) {
	    if (chmod(fileName, attrPtr->st_mode & 04777) != 0) {
		fprintf(stderr,
			"Couldn't set set-user-id for \"%s\": %s.\n",
			fileName, strerror(errno));
		return 1;
	    }
	}
    }

    /*
     * Set times.
     */

    if ((setTimes) || backup) {
#ifdef SYSV
	times.actime = attrPtr->st_atime;
	times.modtime = attrPtr->st_mtime;
	if (utime(fileName, &times) != 0) {
#else
	times[0].tv_usec = times[1].tv_usec = 0;
	times[0].tv_sec = attrPtr->st_atime;
	times[1].tv_sec = attrPtr->st_mtime;
	if (utimes(fileName, times) != 0) {	    
#endif
	    fprintf(stderr, "Couldn't set times of \"%s\": %s.\n",
		    fileName, strerror(errno));
	    return 1;
	}
    }

#ifdef sprite
    /*
     * Set the advisory file type if it's set in the source file.
     */
    if (attrPtr->st_userType != S_TYPE_UNDEFINED) {
	if (setfiletype(fileName, attrPtr->st_userType) != 0) {
	    fprintf(stderr, "Couldn't set type of \"%s\": %s.\n",
		    fileName, strerror(errno));
	}
    }
#endif

    return 0;
}

static int
CreateDirectory(dir)
    char *dir;
{
    char pathname[MAXPATHLEN];
    char *s;

    /*
     * Try to create the directory
     */
    if (mkdir(dir, 0777) == 0) {
	return 0;
    }

    /*
     * Couldn't create the directory.
     * Try to create the parent directory, and then try again.
     */
    strcpy(pathname, dir);
    if ((s = strrchr(pathname, '/')) == NULL) {
	return 1;
    }
    *s = '\0';
    if (CreateDirectory(pathname) == 0) {
	return mkdir(dir, 0777);
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * PruneOpt --
 *
 *	Process a "-p" option.  Take the regular expression that follows
 *	and put it in the array of trees to prune..
 *
 * Results:
 *	1 (-p option requires an argument).
 *
 * Side effects:
 *	The argument to the option is put in the regular expression array..
 *
 *----------------------------------------------------------------------
 */

static int
PruneOpt(optionPtr, exprString)
    char	*optionPtr; 	/* Current option. */
    char	*exprString;	/* The regular expression. */
{
    regexp	*expPtr;

    if ((exprString == NULL) || (*exprString == '-')) {
	fprintf(stderr, "Warning: \"-%s\" option needs an argument\n", 
	    optionPtr);
	return 1;
    }
    expPtr = regcomp(exprString);
    if (expPtr == NULL) {
	fprintf(stderr, "Warning: \"%s\" is not a regular expression.\n", 
	    exprString);
	return 1;
    }
    if (prune >= MAX_PRUNE) {
	fprintf(stderr, "Warning: too many prune options.\n");
	return 1;
    }
    pruneArray[prune].exp = expPtr;
    pruneArray[prune].expString = exprString;
    prune++;
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * regerror --
 *
 *	This routine is called by the regular expression library when
 *	something goes wrong.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed out.
 *
 *----------------------------------------------------------------------
 */

void
regerror(msg)
    char	*msg;	/* The error message. */
{
    fprintf(stderr, "Error in regular expression library: %s\n", msg);
    exit(1);
}


#ifdef NEEDSTRERROR

/*
 *----------------------------------------------------------------------
 *
 * strerror --
 *
 *      Simplistic message generator for systems that
 *	don't have the real thing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
strerror(error)
    int error;
{
    extern char *sys_errlist[];
    extern int sys_nerr;
    static char badMsg[100];

    if (error > sys_nerr) {
	sprintf(badMsg, "Unknown errno value: %d\n", error);
	return badMsg;
    } else {
	return sys_errlist[error];
    }
}

#endif
