#ifndef lint
static char rcsid[] = "$Id: find_names.c,v 1.11 89/11/19 23:20:02 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Find Names
 *
 *	Writes all the pertinent file names, both from the administration
 *	and from the repository to the argc/argv arguments.
 *
 *	The names should be freed by callin free_names() when they are no
 *	longer needed.
 *
 *	Note that Find_Names() honors the administration Entries.Static file
 *	to indicate that the Repository should not be searched for new files.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <dirent.h>
#include "cvs.h"

Find_Names(pargc, argv, which)
    int *pargc;
    char *argv[];
    enum ftype which;
{
    char dir[MAXPATHLEN], line[MAXLINELEN];
    FILE *fpin;
    char *cp;

    *pargc = 0;
    argv[0] = NULL;			/* Assume none */
    if (which == MOD) {
	fpin = open_file(CVSADM_MOD, "r");
	/*
	 * Parse the Mod file, and calling addname() for each line.
	 */
	while (fgets(line, sizeof(line), fpin) != NULL) {
	    if ((cp = rindex(line, '\n')) != NULL)
		*cp = '\0';
	    *cp = '\0';
	    addname(pargc, argv, line);
	}
	(void) fclose(fpin);
    } else {
	fpin = open_file(CVSADM_ENT, "r");
	/*
	 * Only scan for ,v files if Entries.Static does not exist
	 */
	if (!isfile(CVSADM_ENTSTAT)) {
	    if (find_rcs(Repository, pargc, argv) != 0)
		error(1, "cannot open directory %s", Repository);
	    if (which == ALLPLUSATTIC) {
		(void) sprintf(dir, "%s/%s", Repository, CVSATTIC);
		(void) find_rcs(dir, pargc, argv);
	    }
	}
	/*
	 * Parse the Entries file, and calling addname() for each one.
	 */
	while (fgets(line, sizeof(line), fpin) != NULL) {
	    if ((cp = rindex(line, '|')) == NULL)
		continue;
	    *cp = '\0';
	    if ((cp = rindex(line, ' ')) == NULL)
		continue;
	    cp++;
	    addname(pargc, argv, cp);
	}
	(void) fclose(fpin);
    }
    /*
     * And finally, sort the names so that they look reasonable
     * as they are processed (there *is* order in the world)
     */
    qsort((char *)&argv[0], *pargc, sizeof(argv[0]), ppstrcmp);
}

/*
 * Finds all the ,v files in the argument directory, and adds them to the
 * argv list.  Returns 0 for success and non-zero if the argument
 * directory cannot be opened.
 */
static
find_rcs(dir, pargc, argv)
    char *dir;
    int *pargc;
    char *argv[];
{
    char *cp, line[50];
    struct dirent *dp;
    DIR *dirp;

    if ((dirp = opendir(dir)) == NULL)
	return (1);
    (void) sprintf(line, ".*%s$", RCSEXT);
    if ((cp = re_comp(line)) != NULL)
	error(0, "%s", cp);
    while ((dp = readdir(dirp)) != NULL) {
	if (re_exec(dp->d_name)) {
	    /* strip the ,v */
	    *rindex(dp->d_name, ',') = '\0';
	    addname(pargc, argv, dp->d_name);
	}
    }
    (void) closedir(dirp);
    return (0);
}

/*
 * addname() adds a name to the argv array, only if it is not in
 * the array already
 */
static
addname(pargc, argv, name)
    int *pargc;
    char *argv[];
    char *name;
{
    register int i;

    for (i = 0; i < *pargc; i++) {
	if (strcmp(argv[i], name) == 0)
	    return;
    }
    (*pargc)++;
    argv[i] = xmalloc(strlen(name) + 1);
    (void) strcpy(argv[i], name);
}
