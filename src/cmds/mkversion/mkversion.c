/* 
 * mkversion.c --
 *
 *	Output a string to be used as "version.h" describing the current
 *	working directory and date/time.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /a/newcmds/mkversion/RCS/mkversion.c,v 1.4 88/11/13 12:02:47 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "sys/param.h"
#include "sys/time.h"
#include "time.h"
#include "option.h"

int printDir = 0;
int printDate = 1;
char *prog = NULL;

Option optionArray[] = {
    {OPT_TRUE, "d",  (char *) &printDir,
	"Print the current working directory (TRUE)"},
    {OPT_FALSE, "t",  (char *) &printDate,
	"Don't print the date/time-stamp (FALSE)"},
    {OPT_STRING, "p",  (char *) &prog,
	"Output program name STRING (following the directory, if applicable)"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

main(argc, argv)
    int		argc;
    char	*argv[];
{
    struct	timeval	curTime;
    struct	timezone	local;
    char 	pathName[MAXPATHLEN];

    (void) Opt_Parse(argc, argv, optionArray, numOptions, OPT_ALLOW_CLUSTERING);

    printf("#define VERSION \"");
    
    if (printDir) {
	if (getwd(pathName) == NULL) {
	    fprintf(stderr, "Error in getwd: '%s'\n", pathName);
	} else {
	    printf("%s", pathName);
	}
    }
    if (prog) {
	if (printDir) {
	    printf("/");
	}
	printf("%s", prog);
    }
    if (printDir || prog) {
	printf(" ");
    }
    if (printDate) {
	char *date;
	int numDateChars = 1;
	extern char *asctime();

	gettimeofday(&curTime, &local);
	date = asctime(localtime(&curTime.tv_sec));
	/*
	 *  ctime format is ugly:
	 *  "Sat Aug 10 10:30:01 1987\n\0"
	 *   012345678901234567890123 4 5
	 *
	 * Make it look like Time_ToAscii by picking substrings.
	 * Get rid of the leading space in the date by checking for ' '
	 * and printing 1 or 2 chars, starting at the first non-blank.
	 */

	if (date[8] != ' ') {
	    numDateChars++;
	}
	printf("(%.*s %.3s %.2s %.8s)", numDateChars,
		 &date[10-numDateChars], &date[4], &date[22], &date[11]);
    }
    printf("\"\n");
    exit(0);
}
