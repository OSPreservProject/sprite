/* 
 * man.c --
 *
 *	This file contains the "man" program for Sprite.  See the man
 *	page for details on how it works.
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
static char rcsid[] = "$Header: /sprite/src/cmds/man/RCS/man.c,v 1.10 91/08/15 23:13:46 ouster Exp Locker: shirriff $ SPRITE (Berkeley)";
#endif not lint

#include <ctype.h>
#include <errno.h>
#include <option.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/dir.h>

/*
 * Name of default configuration file:
 */

#ifndef CONFIG_FILE
#define CONFIG_FILE "/sprite/lib/man/config"
#endif

/*
 * Information related to command-line options:
 */

int typeset = 0;		/* Non-zero means print on typesetter
				 * instead of on terminal. */
int noMore = 0;			/* Non-zero means don't filter output through
				 * the "more" program. */
char *sectionName = NULL;	/* Name of section to search in. */
char *configFile = CONFIG_FILE;	/* Configuration file that describes where
				 * man pages are located. */
int reformat = 0;		/* Non-zero means reformat man page even if
				 * formatted copy appears to be up-to-date. */
int makeIndex = 0;		/* Non-zero means generate index from args
				 * rather than printing man pages. */
int keywordLookup = 0;		/* Non-zero means "man -k": look for
				 * keywords. */
int where = 0;			/* Non-zero means say where man page is. */
int doAll = 0;			/* Non-zero means check all directories. */

Option optionArray[] = {
    {OPT_TRUE, "a", (char *) &doAll,
	    "Check all man directories (slower)"},
    {OPT_STRING, "c", (char *) &configFile,
	    "Name of configuration file (default: /sprite/lib/man/config)"},
    {OPT_TRUE, "f", (char *) &keywordLookup,
	    "Identical to \"-k\" (provided for UNIX compatibility)"},
    {OPT_TRUE, "i", (char *) &makeIndex,
	    "Generate index from file name arguments"},
    {OPT_TRUE, "k", (char *) &keywordLookup,
	    "Print index information for keyword arguments"},
    {OPT_TRUE, "r", (char *) &reformat,
	    "Force man page to be reformatted, even if up-to-date"},
    {OPT_STRING, "s", (char *) &sectionName,
	    "Section name in which to search for man page(s)"},
    {OPT_TRUE, "t", (char *) &typeset,
	    "Print man page on typesetter instead of on terminal"},
    {OPT_TRUE, "w", (char *) &where,
	    "Print where the man page was found"},
    {OPT_TRUE, "", (char *) &noMore,
	    "Don't filter output through \"more\" program"},
};

/*
 * One of the data structures built up by this program is the one
 * that describes the directories containing man page sources, and
 * the corresponding directories containing pre-formatted man pages.
 */

typedef struct {
    char *sourceDir;		/* Directory holding man page sources. */
    char *fmtDir;		/* Directory holding formatted entries,
				 * by same name. */
    char *sectionName;		/* Preferred name for this section of the
				 * manual. */
    int allflag;		/* 1 if we check for everything, not just
				 * foo.man */
} ManDir;

#define MAX_DIRS 100
ManDir dirs[MAX_DIRS];
int numDirs;			/* Number of valid entries in dirs. */

/*
 * The data structure below is used to hold all the index information
 * associated with a manual entry.
 */

#define MAX_NAMES 100
#define MAX_KEYWORDS 100
#define NAME_CHARS 1000
#define KEYWORD_CHARS 1000
#define SYNOPSIS_CHARS 100
#define FILE_CHARS 100

typedef struct {
    char fileName[FILE_CHARS];		/* Base name of file containing man
					 * page (everything up to "."). */
    char *names[MAX_NAMES+1];		/* Names of procedures or programs
					 * described by this entry.  These
					 * fields come from the "NAME" manual
					 * section.  Terminated by a NULL
					 * pointer. */
    char synopsis[SYNOPSIS_CHARS];	/* Short description of the entry.
					 * Comes from the part of the "NAME"
					 * section that follows the dash. */
    char *keywords[MAX_KEYWORDS+1];	/* Keywords associated with this
					 * manual entry.  Comes from the
					 * "KEYWORD" section of the entry, if
					 * there is one.  Terminated by a
					 * NULL pointer. */
    char nameBuffer[NAME_CHARS];	/* Storage space for names. */
    char keywordBuffer[KEYWORD_CHARS];	/* Storage space for keywords. */
} IndexEntry;

char * strcasestr();

/*
 * Commands to use for formatting and printing manual pages.  The %s's
 * in these commands get filled in with particular file names in the
 * code below.
 */

#define FORMAT			"nroff -man -Tcrt %s > %s"
#define FORMAT_PRINT		"nroff -man -Tcrt %s | %s -s"
#define FORMAT_PRINT_NO_MORE	"nroff -man -Tcrt %s"
#define PRINT			"%s -s %s"
#define PRINT_NO_MORE		"cat %s"
#define TYPESET			"ditroff -man %s"

/*
 *----------------------------------------------------------------------
 *
 * NextLine --
 *
 *	Read the next line of a given file, skipping comment lines.
 *	Break the line up into fields separated by white space.
 *
 * Results:
 *	The return value is the number of fields in the line (i.e. the
 *	number of elements of argv that are now valid).  If EOF was
 *	encountered, then the return value is -1.  The fields pointed
 *	to by argv are allocated in static storage, so they'll only
 *	be valid up until the next call to this procedure.
 *
 * Side effects:
 *	The argv array is modified.  If the line contains more fields than
 *	are permitted by maxArgs, then an error message is output on stderr
 *	and the extra fields are ignored.
 *
 *----------------------------------------------------------------------
 */

int
NextLine(file, maxArgs, argv)
    FILE *file;			/* File from which to read. */
    int maxArgs;		/* Number of entries in argv. */
    char **argv;		/* Array to fill in with pointers to the
				 * fields of the line. */
{
#define MAX_CHARS 200
    static char buffer[MAX_CHARS];
    register char *p;
    int i;

    while (1) {
	if (fgets(buffer, MAX_CHARS, file) == NULL) {
	    return -1;
	}
	for (p = buffer; ; p++) {
	    if (isspace(*p)) {
		continue;
	    }
	    if ((*p != '#') && (*p != 0)) {
		goto gotLine;
	    }
	    break;
	}
    }

    /*
     * Break the line up into fields.
     */

    gotLine:
    for (i = 0, p = buffer; ; i++) {
	while (isspace(*p)) {
	    p++;
	}
	if (*p == 0) {
	    return i;
	}
	if (i >= maxArgs) {
	    break;
	}
	argv[i] = p;
	while (!isspace(*p)) {
	    p++;
	}
	*p = 0;
	p++;
    }

    /*
     * Ran out of space to store field info.
     */

    fprintf(stderr,
	    "More than %d fields in config file line;  extras ignored.\n",
	    maxArgs);
    return maxArgs;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadConfig --
 *
 *	This procedure reads in the configuration file given by
 *	name, and builds a list of all the directories that match
 *	the given section.
 *
 * Results:
 *	Zero is returned if all went well, -1 if there was an error.
 *
 * Side effects:
 *	The dirs data structure is created.  If an error occurred, then
 *	an error message is printed.
 *
 *----------------------------------------------------------------------
 */

int
ReadConfig(name, section)
    char *name;			/* Name of config file. */
    char *section;		/* Only consider directories that match
				 * this string;  if NULL, consider
				 * everything. */
{
#define MAX_FIELDS 50
    char *argv[MAX_FIELDS];
    FILE *f;
    int j, argc;
    int allflag;

    f = fopen(name, "r");
    if (f == NULL) {
	fprintf(stderr, "Couldn't open \"%s\": %s.\n", name, strerror(errno));
	return -1;
    }

    for (numDirs = 0; numDirs < MAX_DIRS; ) {
	allflag=0;
	argc = NextLine(f, MAX_FIELDS, argv);
	if (argc < 0) {
	    fclose(f);
	    return 0;
	}
	for (j = 2; j < argc; j++) {
	    if ((section != NULL) && (strcmp(argv[j], section) == 0)) {
		goto makeEntry;
	    }
	    if (strcmp(argv[j], "ALL")==0) {
		allflag = 1;
	    }
	}
	if (section != NULL) {
	    continue;
	}
	if (allflag==1 && doAll==0) {
	    continue;
	}

	/*
	 * This line matched the section name;  add an entry to dirs.
	 */

	makeEntry:
	dirs[numDirs].sourceDir =
		(char *) malloc((unsigned) (strlen(argv[0]) + 1));
	strcpy(dirs[numDirs].sourceDir, argv[0]);
	dirs[numDirs].fmtDir =
		(char *) malloc((unsigned) (strlen(argv[1]) + 1));
	strcpy(dirs[numDirs].fmtDir, argv[1]);
	dirs[numDirs].sectionName =
		(char *) malloc((unsigned) (strlen(argv[2]) + 1));
	strcpy(dirs[numDirs].sectionName, argv[2]);
	dirs[numDirs].allflag = allflag;
	numDirs++;
    }

    fprintf(stderr, "Too many lines in \"%s\": ignoring extras.\n", name);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FindSection --
 *
 *	Advance a file just past the header line for a given section.
 *
 * Results:
 *	Returns 0 if the given section was found, -1 if EOF was reached
 *	before the desired section.
 *
 * Side effects:
 *	Characters are read from file until a line is found in the form
 *	".SH section".  The line and its terminating newline are read and
 *	discarded.
 *
 *----------------------------------------------------------------------
 */

int
FindSection(file, section)
    FILE *file;			/* File to read. */
    char *section;		/* Section to search for. */
{
#define LINE_LENGTH 200
    char line[LINE_LENGTH];
    register char *p1, *p2;

    while (fgets(line, LINE_LENGTH, file) != NULL) {
	if ((line[0] != '.') || (line[1] != 'S') || (line[2] != 'H')) {
	    continue;
	}
	for (p1 = &line[3]; isspace(*p1); p1++) {
	    /* Skip white space. */
	}
	for (p2 = section; ; p1++, p2++) {
	    if ((*p2 == 0) && ((*p1 == 0) || (isspace(*p1)))) {
		return 0;
	    }
	    if (*p2 != *p1) {
		break;
	    }
	}
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * IndexFromMan --
 *
 *	Read the source file for a manual entry and generate an
 *	index entry for it.
 *
 * Results:
 *	Normally zero is returned, but if an error occurs then a
 *	no-zero value is returned.  *indexPtr is filled in with
 *	information describing this man page.
 *
 * Side effects:
 *	If an error occurs in reading the entry, then an error
 *	message gets printed.
 *
 *----------------------------------------------------------------------
 */

int
IndexFromMan(fileName, indexPtr)
    char *fileName;			/* Name of file containing man page. */
    register IndexEntry *indexPtr;	/* Pointer to index entry. */
{
    register FILE *file;
    register char *p;
    register int c;
    char *limit;
    int index;

    file = fopen(fileName, "r");
    if (file == NULL) {
	fprintf(stderr, "Couldn't open \"%s\": %s.\n", fileName,
		strerror(errno));
	return -1;
    }

    /*
     * Parse off the root of the file name.
     */

    for (p = fileName; (*p != '.') && (*p != 0); p++) {
	/* Null loop body. */
    }
    if ((p-fileName) > FILE_CHARS) {
	fprintf(stderr, "File name \"%s\" too long.\n", fileName);
	goto error;
    }
    strncpy(indexPtr->fileName, fileName, (p-fileName));
    indexPtr->fileName[p-fileName] = 0;

    /*
     * Parse off the names (a bunch of strings, all but the last of which
     * are terminated by commas, with the last terminated by space).
     */

    if (FindSection(file, "NAME") != 0) {
	fprintf(stderr, "Couldn't find \"NAME\" section in \"%s\".\n",
		fileName);
	goto error;
    }
    c = getc(file);

    /*
     * Skip any troff commands at the beginning of the section.
     */

    while (c == '.') {
	while ((c != '\n') && (c != EOF)) {
	    c = getc(file);
	}
	c = getc(file);
    }

    /*
     * Parse off the procedure names.
     */

    p = indexPtr->nameBuffer;
    limit = &indexPtr->nameBuffer[NAME_CHARS-1];
    for (index = 0; index < MAX_NAMES; ) {
	while (isspace(c)) {
	    c = getc(file);
	}
	indexPtr->names[index] = p;
	while (!isspace(c) && (c != ',') && (p < limit)) {
	    *p = c;
	    p++;
	    c = getc(file);
	}
	if (p >= limit) {
	    break;
	}
	if (indexPtr->names[index] != p) {
	    *p = 0;
	    p++;
	    index++;
	}
	if (c != ',') {
	    break;
	}
	c = getc(file);
    }
    if (c == EOF) {
	fprintf(stderr, "Unexpected end-of-file in NAME section of \"%s\".\n",
		fileName);
	goto error;
    }
    if ((index == MAX_NAMES) || (p >= limit)) {
	fprintf(stderr, "Too many names in \"%s\";  skipped the extras.\n",
		fileName);
    }
    indexPtr->names[index] = 0;

    /*
     * Skip up to and through a hyphen and any following space, then
     * use the rest of the line as a synopsis.
     */

    while ((c != '-') && (c != EOF)) {
	c = getc(file);
    }
    for (c = getc(file); isspace(c); c = getc(file)) {
	/* Null loop body. */
    }
    ungetc(c, file);
    fgets(indexPtr->synopsis, SYNOPSIS_CHARS, file);
    for (p = indexPtr->synopsis; *p != 0; p++) {
	if (*p == '\n') {
	    *p = 0;
	    break;
	}
    }

    /*
     * Skip to the keywords section and parse off the keywords in a
     * fashion similar to the names, except that (a) it's OK not to have
     * a KEYWORDS section, (b) it's OK to have space in a keyword, and
     * (c) the last keyword is terminated by newline.
     */

    if (FindSection(file, "KEYWORDS") != 0) {
	indexPtr->keywords[0] = NULL;
	goto done;
    }
    c = getc(file);
    p = indexPtr->keywordBuffer;
    limit = &indexPtr->keywordBuffer[KEYWORD_CHARS-1];
    for (index = 0; index < MAX_KEYWORDS; ) {
	while (isspace(c)) {
	    c = getc(file);
	}
	indexPtr->keywords[index] = p;
	while ((c != ',') && (c != '\n') && (c != EOF) && (p < limit)) {
	    *p = c;
	    p++;
	    c = getc(file);
	}
	if (p >= limit) {
	    break;
	}
	if (indexPtr->keywords[index] != p) {
	    *p = 0;
	    p++;
	    index++;
	}
	if (c == EOF) {
	    fprintf(stderr,
		    "Unexpected end-of-file in KEYWORDS section of \"%s\".\n",
		    fileName);
	    goto error;
	}
	if (c == '\n') {
	    break;
	}
	c = getc(file);
    }
    if ((index == MAX_NAMES) || (p >= limit)) {
	fprintf(stderr, "Too many names in \"%s\";  skipped the extras.\n",
		fileName);
    }
    indexPtr->keywords[index] = 0;

    done:
    fclose(file);
    return 0;

    error:
    fclose(file);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintIndex --
 *
 *	Read manual pages and print index entries on standard output.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets printed on standard output.  Error messages
 *	may appear on stderr.
 *
 *----------------------------------------------------------------------
 */

void
PrintIndex(argc, argv)
    int argc;			/* Number of files to read. */
    char **argv;		/* Names of files to read. */
{
    IndexEntry index;
    int i;
    char **p;

    for (i = 0; i < argc; i++) {
	if (IndexFromMan(argv[i], &index) != 0) {
	    continue;
	}
	printf("%s\n", index.fileName);
	for (p = index.names; *p != 0; p++) {
	    printf("%s, ", *p);
	}
	putchar('\n');
	printf("%s\n", index.synopsis);
	for (p = index.keywords; *p != 0; p++) {
	    printf("%s, ", *p);
	}
	putchar('\n');
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReadNextIndex --
 *
 *	Given a handle for an index file created by PrintIndex, read
 *	the next index entry from the file.
 *
 * Results:
 *	Zero is returned if all went well;  otherwise -1 is returned
 *	and an error message is printed on stderr.  The fields in
 *	*indexPtr will be filled in with information describing this
 *	index entry.
 *
 * Side effects:
 *	The position in file is advanced.
 *
 *----------------------------------------------------------------------
 */

int
ReadNextIndex(file, indexPtr)
    FILE *file;			/* File to read. */
    IndexEntry *indexPtr;	/* Entry to fill in. */
{
    register char *p;
    int i;

    /*
     * Read in the file name line.
     */

    if (fgets(indexPtr->fileName, FILE_CHARS, file) == NULL) {
	return -1;
    }
    for (p = indexPtr->fileName; *p != '\n'; p++) {
	if (*p == 0) {
	    fprintf(stderr, "Filename line for \"%s\" too long.\n",
		    indexPtr->fileName);
	    return -1;
	}
    }
    *p = 0;

    /*
     * Read in and parse the keyword line.
     */

    if (fgets(indexPtr->nameBuffer, NAME_CHARS, file) == NULL) {
	fprintf(stderr, "End-of-file in name line for \"%s\".\n",
		indexPtr->fileName);
	return -1;
    }
    for (i = 0, p = indexPtr->nameBuffer; i < MAX_NAMES; ) {
	while (isspace(*p)) {
	    p++;
	}
	indexPtr->names[i] = p;
	while ((*p != ',') && (*p != 0)) {
	    p++;
	}
	if (p != indexPtr->names[i]) {
	    i++;
	}
	if (*p == 0) {
	    break;
	}
	*p = 0;
	p++;
    }
    indexPtr->names[i] = 0;
    if ((p[-1] != '\n') || (i == MAX_NAMES)) {
	fprintf(stderr, "Too many names for \"%s\".\n", indexPtr->fileName);
	return -1;
    }

    /*
     * Read in the synopsis line;  there's no parsing to do.
     */

    if (fgets(indexPtr->synopsis, SYNOPSIS_CHARS, file) == NULL) {
	fprintf(stderr, "End-of-file in synopsis line for \"%s\".\n",
		indexPtr->fileName);
	return -1;
    }
    for (p = indexPtr->synopsis; *p != '\n'; p++) {
	if (*p == 0) {
	    fprintf(stderr, "Synopsis line for \"%s\" too long.\n",
		    indexPtr->fileName);
	    return -1;
	}
    }
    *p = 0;

    /*
     * Read in and parse the keywords line.
     */

    if (fgets(indexPtr->keywordBuffer, KEYWORD_CHARS, file) == NULL) {
	fprintf(stderr, "End-of-file in keywords line for \"%s\".\n",
		indexPtr->fileName);
	return -1;
    }
    for (i = 0, p = indexPtr->keywordBuffer; i < MAX_KEYWORDS;) {
	while (isspace(*p)) {
	    p++;
	}
	indexPtr->keywords[i] = p;
	while ((*p != ',') && (*p != 0)) {
	    p++;
	}
	if (p != indexPtr->keywords[i]) {
	    i++;
	}
	if (*p == 0) {
	    break;
	}
	*p = 0;
	p++;
    }
    indexPtr->keywords[i] = 0;
    if ((p[-1] != '\n') || (i == MAX_KEYWORDS)) {
	fprintf(stderr, "Too many keywords for \"%s\".\n", indexPtr->fileName);
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * PrettyPrintIndex --
 *
 *	Print an index entry in human-readable form on standard output.
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
PrettyPrintIndex(indexPtr, section)
    IndexEntry *indexPtr;	/* Entry to print. */
    char *section;		/* Name to use for manual section. */
{
#define COLS_FOR_NAMES 24
    int i, numCols;

    numCols = 0;
    for (i = 0; indexPtr->names[i] != 0; i++) {
	if (i != 0) {
	    fputs(", ", stdout);
	    numCols += 2;
	}
	fputs(indexPtr->names[i], stdout);
	numCols += strlen(indexPtr->names[i]);
    }
    numCols += printf(" (%s) ", section);
    if (numCols < COLS_FOR_NAMES) {
	printf("%*c", COLS_FOR_NAMES-numCols, ' ');
    }
    printf("- %s\n", indexPtr->synopsis);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintByKeyword --
 *
 *	Given a keyword, locate the index entries (if any) corresponding
 *	to that keyword and print out each matching index entry.
 *
 * Results:
 *	Information is printed for each index entry that contains
 *	the keyword as a substring of the entry's name, synopsis, or
 *	keyword fields.  If no matching entry was found, then an
 *	error message is printed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
PrintByKeyword(keyword)
    char *keyword;			/* String to search for. */
{
    int i, foundMatch;
    FILE *f;
    char **p;
    char indexName[400];
    IndexEntry index;

    foundMatch = 0;
    for (i = 0; i < numDirs; i++) {
	sprintf(indexName, "%.350s/index", dirs[i].sourceDir);
	f = fopen(indexName, "r");
	if (f == NULL) {
	    continue;
	}
	while (ReadNextIndex(f, &index) == 0) {
	    for (p = index.names; *p != 0; p++) {
		if (strcasestr(*p, keyword) != 0) {
		    goto found;
		}
	    }
	    if (strcasestr(index.synopsis, keyword) != 0) {
		goto found;
	    }
	    for (p = index.keywords; *p != 0; p++) {
		if (strcasestr(*p, keyword) != 0) {
		    goto found;
		}
	    }
	    continue;

	    found:
	    PrettyPrintIndex(&index, dirs[i].sectionName);
	    foundMatch = 1;
	}
	fclose(f);
    }
    if (!foundMatch) {
	fprintf(stderr,
		"Couldn't find any manual entries related to \"%s\".\n",
		keyword);
    }
}

#define BUFLEN 10
char suffixBuf[BUFLEN];
/*
 *----------------------------------------------------------------------
 *
 * search --
 *
 *	Search for a file dir/name.suffix, where suffix is unknown.
 *
 * Results:
 *	0 for success.
 *	Returns suffix.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
search(dir, name, suffix)
    char *dir;
    char *name;
    char **suffix;
{
    DIR *dirp;
    struct direct *direct;
    int len = strlen(name);
    int status = 1;

    dirp = opendir(dir);
    if (dir==NULL) {
	fprintf(stderr,"man: couldn't open %s\n", dir);
	return 1;
    }

    while (1) {
	direct = readdir(dirp);
	if (direct==NULL) {
	    break;
	}
	if (strncmp(name,direct->d_name,len)==0 &&
		direct->d_name[len]=='.') {
	    strncpy(suffixBuf, direct->d_name+len+1, BUFLEN);
	    suffixBuf[BUFLEN]='\0';
	    *suffix = suffixBuf;
	    status = 0;
	    break;
	}

    }

    closedir(dirp);
    return status;
    
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program, which runs the whole show.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read the man page for details.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char **argv;
{
    int i, result;
    char srcName[500], fmtName[500];
    char command[1100];
    char *progName = argv[0];
    char *pager;
    struct stat srcStat, fmtStat;
    IndexEntry index;
    char *suffix;

    pager = getenv("PAGER");
    if(pager == 0) pager = "more";

    /*
     * Process command-line options and figure out what section to
     * look in.
     */

    result = 0;
    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if ((sectionName == NULL) && (argc > 1) && (isdigit(argv[1][0]))) {
	sectionName = argv[1];
	argc--;
	argv++;
    }

    /*
     * If we're just generating an index, do it here and quit.
     */

    if (makeIndex) {
	PrintIndex(argc-1, &argv[1]);
	exit(0);
    }

    /*
     * Read in the configuration file, and make sure that there is at
     * least one place to look for the desired manual page.
     */

    if (ReadConfig(configFile, sectionName) != 0) {
	exit(1);
    }
    if (numDirs == 0) {
	if (sectionName != NULL) {
	    fprintf(stderr, "No manual section named \"%s\".\n", sectionName);
	} else {
	    fprintf(stderr, "The config file (%s) is empty!\n", configFile);
	}
	exit(1);
    }

    if (argc == 1) {
	fprintf(stderr, "Usage:  %s [options] entryName entryName ...\n",
		progName);
	exit(1);
    }

    /*
     * Loop over all of the named man pages, processing each one separately.
     */

    for (argv++ ; argc > 1; argc--, argv++) {
	/*
	 * Handle special case of keyword lookup.
	 */

	if (keywordLookup) {
	    PrintByKeyword(argv[0]);
	    continue;
	}

	/*
	 * Search for the desired entry in two passes.  In the first pass,
	 * look in each of the available directories for a file named
	 * "foo.man" where foo is the entry's name.  If this doesn't work,
	 * then the second pass reads the index files in each of the
	 * available directories, checking for an entry with the desired
	 * name.
	 * If allflag is set for the directory, we then do two more pases:
	 * In the third pass, we look for a file named "foo.section".
	 * Finally, we look for "foo.*"
	 */
    
	for (i = 0; i < numDirs; i++) {
	    sprintf(srcName, "%.350s/%.100s.man", dirs[i].sourceDir, argv[0]);
	    if (access(srcName, R_OK) == 0) {
		sprintf(fmtName, "%.350s/%.100s.man", dirs[i].fmtDir, argv[0]);
		goto gotEntry;
	    }
	}
	for (i = 0; i < numDirs; i++) {
	    FILE *f;
	    char **p;

	    sprintf(srcName, "%.350s/index", dirs[i].sourceDir);
	    f = fopen(srcName, "r");
	    if (f == NULL) {
		continue;
	    }
	    while (ReadNextIndex(f, &index) == 0) {
		for (p = index.names; *p != 0; p++) {
		    if (strcmp(*p, argv[0]) == 0) {
			sprintf(srcName, "%.350s/%.100s.man",
				dirs[i].sourceDir, index.fileName);
			if (access(srcName, R_OK) == 0) {
			    sprintf(fmtName, "%.350s/%.100s.man",
				    dirs[i].fmtDir, index.fileName);
			    fclose(f);
			    goto gotEntry;
			}
		    }
		}
	    }
	    fclose(f);
	}

	if (strchr(argv[0], '.') != NULL) {
	    sprintf(srcName, "./%.100s", argv[0]);
	    if (access(srcName, R_OK) == 0) {
		sprintf(fmtName, "/tmp/%.100s", argv[0]);
		i = 0;
		goto gotEntry;
	    }
	}

	for (i = 0; i < numDirs; i++) {
	    if (dirs[i].allflag && dirs[i].sectionName) {
		sprintf(srcName, "%.350s/%.100s.%s", dirs[i].sourceDir,
			argv[0], dirs[i].sectionName);
		if (access(srcName, R_OK) == 0) {
		    sprintf(fmtName, "%.350s/%.100s.%s", dirs[i].fmtDir,
			    argv[0], dirs[i].sectionName);
		    goto gotEntry;
		}
	    }
	}
	for (i = 0; i < numDirs; i++) {
	    if (dirs[i].allflag &&
		    search(dirs[i].sourceDir, argv[0],&suffix)==0) {
		sprintf(srcName, "%.350s/%.100s.%s", dirs[i].sourceDir, argv[0],
			suffix);
		sprintf(fmtName, "%.350s/%.100s.%s", dirs[i].fmtDir, argv[0],
		        suffix);
		goto gotEntry;
	    }
	}

	/*
	 * Couldn't find the manual entry.
	 */

	if (sectionName == NULL) {
	    fprintf(stderr, "No manual entry for \"%s\".\n", argv[0]);
	} else {
	    fprintf(stderr,
		    "No manual entry for \"%s\" in section \"%s\".\n",
		    argv[0], sectionName);
	}
	result = 1;
	continue;

	/*
	 * If the entry is to be typeset, just do it.
	 */

	gotEntry:

	if (where) {
	    printf("Man page found in %s\n", fmtName);
	    continue;
	}

	if (typeset) {
	    sprintf(command, TYPESET, srcName);
	    if (system(command) != 0) {
		printf("Error in typesetting \"%s\" man page.\n", argv[0]);
		result = 1;
	    }
	    continue;
	}

	/*
	 * See if there is an up-to-date formatted version of the page.
	 * If not, then regenerate it.  (If the formatted directory is
	 * "-" that means no formatted version of the man page is to be
	 * kept)
	 */

	if (strcmp(dirs[i].fmtDir, "-") == 0) {
	    goto couldntFormat;
	}

	if ((stat(srcName, &srcStat) != 0) || (stat(fmtName, &fmtStat) != 0)
		|| (srcStat.st_mtime > fmtStat.st_mtime) || reformat) {
	    printf("Reformatting manual entry.  Please wait...\n");
	    sprintf(command, FORMAT, srcName, fmtName);
	    if (system(command) != 0) {
		unlink(fmtName);
		goto couldntFormat;
	    }

	    /*
	     * Reprotect the formatted file so that anyone can overwrite
	     * it later.
	     */

	    chmod(fmtName, 0666);
	}

	/*
	 * Print the formatted version of the man page.
	 */

	if (noMore) {
	    sprintf(command, PRINT_NO_MORE, fmtName);
	} else {
	    sprintf(command, PRINT, pager, fmtName);
	}
	if (system(command) == 0) {
	    continue;
	}

	/*
	 * We get here if it wasn't possible to format and/or print the
	 * man page.  Try one last desperation move:  print and format
	 * in a single command.
	 */

	couldntFormat:
	if (noMore) {
	    sprintf(command, FORMAT_PRINT_NO_MORE, srcName);
	} else {
	    sprintf(command, FORMAT_PRINT, srcName, pager);
	}
	(void) system(command);
    }
    exit(result);
}


/*
 *----------------------------------------------------------------------
 *
 * strcasestr --
 *
 *	Locate the first instance of a substring in a string, ignoring
 *	case.  This is the code for strstr, very slightly modified.
 *
 * Results:
 *	If string contains substring, the return value is the
 *	location of the first matching instance of substring
 *	in string.  If string doesn't contain substring, the
 *	return value is 0.  Matching is done on an exact
 *	character-for-character basis with no wildcards or special
 *	characters.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define makeupper(x) (islower(x)?toupper(x):(x))

char *
strcasestr(string, substring)
    register char *string;	/* String to search. */
    char *substring;		/* Substring to try to find in string. */
{
    register char *a, *b;

    /* First scan quickly through the two strings looking for a
     * single-character match.  When it's found, then compare the
     * rest of the substring.
     */

    b = substring;
    if (*b == 0) {
	return string;
    }
    for ( ; *string != 0; string += 1) {
	if (makeupper(*string) != makeupper(*b)) {
	    continue;
	}
	a = string;
	while (1) {
	    if (*b == 0) {
		return string;
	    }
	    if (makeupper(*a) != makeupper(*b)) {
		a++;
		b++;
		break;
	    }
	    a++;
	    b++;
	}
	b = substring;
    }
    return (char *) 0;
}
