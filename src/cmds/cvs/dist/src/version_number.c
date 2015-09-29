#ifndef lint
static char rcsid[] = "$Id: version_number.c,v 1.16.1.2 91/01/29 07:20:26 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Version Number
 *
 *	Returns the requested version number of the RCS file, satisfying tags
 *	and walking branches, if necessary.
 *
 *	"rcs" is the full pathname to the ,v file which is read to determine
 *	the current head version number for the RCS file.  Things are
 *	complicated by needing to walk branches and symbolic tags.  The
 *	algorithm used here for branches is not quite the same as that
 *	included with RCS, but should work 99.999% of the time (or more).
 *
 *	The result is placed in "vers"; null-string if error.
 */

#include <ctype.h>
#include "cvs.h"

Version_Number(rcs, tag, date, vers)
    char *rcs;
    char *tag;
    char *date;
    char *vers;
{
    FILE *fpin;
    char rev[50];

    vers[0] = rev[0] = '\0';		/* Assume failure */
    /*
     * Open the RCS file.  If it failed, just return, as the version
     * is already nulled.
     */
    if ((fpin = fopen(rcs, "r")) == NULL)
	return;
    get_version(fpin, tag, date, rev, vers);
    if (vers[0] == '\0' && tag[0] != '\0') {
	if (!isdigit(tag[0])) {
	    char *cp, temp[MAXLINELEN];
	    extern char update_dir[];

	    if (CVSroot[0] != '\0' &&
		strncmp(rcs, CVSroot, strlen(CVSroot)) == 0) {
		cp = rcs + strlen(CVSroot) + 1;
	    } else {
		if ((cp = index(rcs, '/')) == NULL && update_dir[0] != '\0') {
		    (void) sprintf(temp, "%s/%s", update_dir, rcs);
		    cp = temp;
		} else {
		    cp = rcs;
		}
	    }
	    if (force_tag_match) {
		if (!quiet)
		    warn(0, "tag %s undefined in %s; ignored", tag, cp);
	    } else {
		if (!quiet)
		    warn(0, "tag %s undefined in %s; using %s", tag, cp, rev);
		(void) strcpy(vers, rev);
	    }
	} else {
	    if (!force_tag_match)
		(void) strcpy(vers, rev);
	}
    }
    (void) fclose(fpin);
}

/*
 * The main driver for doing the work of getting the required revision
 * number.  Returns computed revision in "rev" or "vers" depending
 * on whether the "tag" could be satisfied or not.
 */
static
get_version(fp, tag, date, rev, vers)
    FILE *fp;
    char *tag;
    char *date;
    char *rev;
    char *vers;
{
    char line[MAXLINELEN];
    char *cp;
    int symtag_matched = 0;

    /*
     * Scan to find the current "head" setting,
     * which really isn't the head if the RCS file is using a branch
     * for the head, sigh.
     *
     * Assumption here is that the "head" line is always first.
     */
    if (fgets(line, sizeof(line), fp) == NULL)
	return;
    if (strncmp(line, RCSHEAD, sizeof(RCSHEAD) - 1) != 0 ||
	!isspace(line[sizeof(RCSHEAD) - 1]) ||
	(cp = rindex(line, ';')) == NULL)
	return;
    *cp = '\0';				/* strip the ';' */
    if ((cp = rindex(line, ' ')) == NULL &&
	(cp = rindex(line, '\t')) == NULL)
	return;
    cp++;
    (void) strcpy(rev, cp);
    /*
     * The "rev" string now contains the value of the RCS "head".
     * Read the next line to find out if we should walk the branches.
     *
     * Assumption here is that "branch" is always on the second line
     * of the RCS file.  If a "branch" line does not exist, we assume
     * it is an old format RCS file, and blow it off.
     */
    if (fgets(line, sizeof(line), fp) == NULL)
	return;
    if (strncmp(line, RCSBRANCH, sizeof(RCSBRANCH) - 1) == 0 &&
	isspace(line[sizeof(RCSBRANCH) - 1]) &&
	(cp = rindex(line, ';')) != NULL) {
	*cp = '\0';			/* strip the ';' */
	if ((cp = rindex(line, ' ')) == NULL &&
	    (cp = rindex(line, '\t')) == NULL)
	    return;
	cp++;
	if (*cp != NULL)
	    (void) strcpy(rev, cp);
    }
    /*
     * "rev" now contains either the "head" value, or the "branch"
     * value (if it was set).  If we're looking for a particular symbolic
     * or numeric tag, we must find the symbol, and then do
     * branch completion as usual, if necessary.
     */
    if (date[0] != '\0') {
	get_date(fp, date, rev, vers);
	return;
    }
    if (tag[0] != '\0') {
	/* return of 0 means we found an exact match, or there was an error */
	if ((symtag_matched = get_tag(fp, tag, rev, vers)) == 0)
	    return;
    }
    /*
     * "rev" now contains either the "head" value, or the tag value,
     * or the "branch" value.  get_branch() will fill in "rev" with
     * the highest numbered branch off "rev", if necessary.
     */
    get_branch(fp, rev);
    if (tag[0] == '\0' || isdigit(tag[0]) || symtag_matched < 0) {
	if ((numdots(rev) & 1) != 0)
	    (void) strcpy(vers, rev);
    }
}

/*
 * We were requested to find a particular symbolic or numeric revision.
 * So, scan for the "symbols" line in the RCS file, or for the first
 * match of a line if the tag is numeric.
 *
 * Would really like to use strtok() here, but callers in update() are
 * already using it.
 *
 * Return value of 0 means to use "vers" as it stands, while a return value
 * of 1 means to use "rev", but to check for possible branch completions
 * to find the head of a branch.
 */
static
get_tag(fp, tag, rev, vers)
    FILE *fp;
    char *tag;
    char *rev;
    char *vers;
{
    char line[MAXLINELEN];
    char *cp, *cprev;

    if (isdigit(tag[0])) {
	while (tag[strlen(tag)-1] == '.')
	    tag[strlen(tag)-1] = '\0';	/* strip trailing dots */
	if ((numdots(tag) & 1) == 0) {
	    (void) strcpy(rev, tag);
	    return (1);			/* let get_branch() figure it out */
	}
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
	if (strncmp(line, RCSDESC, sizeof(RCSDESC) - 1) == 0) {
	    rewind(fp);
	    return (1);			/* use head */
	}
	/*
	 * For numeric tags, the RCS file contains the revision
	 * number all by itself on a single line, so we check for
	 * that match here.
	 */
	if (isdigit(tag[0])) {
	    if ((cp = rindex(line, '\n')) != NULL)
		*cp = '\0';
	    if (strcmp(tag, line) == 0) {
		(void) strcpy(vers, line);
		return (0);		/* a match for a numeric tag */
	    }
	    continue;
	}
	if (strncmp(line, RCSSYMBOL, sizeof(RCSSYMBOL)-1)!=0 ||
	    !isspace(line[sizeof(RCSSYMBOL) - 1]))
	    continue;
	/*
	 * A rather ugly loop to process the "symbols" line.  I would
	 * really rather use strtok(), but other above me already are,
	 * and strtok() blows up in this case.
	 */
	for (cp = line + sizeof(RCSSYMBOL);  cp;  ) {
	    while (isspace(*cp))
		cp++;
	    if (*cp == ';')
		break;
	    if (!*cp) {
    		if (fgets(line, sizeof(line), fp) == NULL)
		    return 0;
		cp = line;
		continue;
	    }
	    /* symbols and revisions are separated by a colon */
	    if ((cprev = index(cp, ':')) == NULL) {
		while (*cp && !isspace(*cp) && *cp!=';')
		    cp++;
		continue;
	    }
	    *cprev++ = '\0';
	    /*
	     * "cp" points to the NULL-terminated symbolic name;
	     * "cprev" points to the revision, which must be NULL-terminated;
	     */
	    if (strcmp(tag, cp) == 0) {
		if ((cp = index(cprev, ' ')) == NULL
		    && (cp = index(cprev, ';')) == NULL
		    && (cp = index(cprev, '\n')) == NULL)
		    continue;
		*cp = '\0';
		(void) strcpy(rev, cprev);
		return (-1);		/* look for branches off rev */
	    } else {
		while (!isspace(*cp) && *cp!=';')
		    cp++;
	    }
	}
	return (1);
    }
    return (0);
}

/*
 * Decides if we should determine the highest branch number, and
 * returns it in "rev".  This is only done if there are
 * an even number of dots ('.') in the revision number passed in.
 */
static
get_branch(fp, rev)
    FILE *fp;
    char *rev;
{
    char line[MAXLINELEN];
    char branch[50];
    char *cp;
    int len, dots = numdots(rev);

    if ((dots & 1) != 0)
	return;
    (void) sprintf(branch, "%s.", rev);
    len = strlen(branch);
    while (fgets(line, sizeof(line), fp) != NULL) {
	if (strncmp(line, RCSDESC, sizeof(RCSDESC) - 1) == 0)
	    return;
	if (isdigit(line[0])) {
	    if ((cp = rindex(line, '\n')) != NULL)
		*cp = '\0';		/* strip the newline */
	    if (numdots(line) == dots+1 &&
		strncmp(line, branch, len) == 0) {
		if ((numdots(rev) & 1) == 0 || strcmp(rev, line) <= 0)
		    (void) strcpy(rev, line);
	    }
	}
    }
}

/*
 * Look up a the most recent revision, based on the supplied date.
 * But do some funky stuff if necessary to follow any vendor branch.
 */
static
get_date(fp, date, rev, version)
    FILE *fp;
    char *date;
    char *rev;
    char *version;
{
    char *cp;

    if ((numdots(rev) & 1) == 0) {
	/*
	 * A branch is the head, so get the revision from the branch
	 * specified in "rev".  If we didn't get a match, try the trunk.
	 */
	get_branch_date(fp, date, rev, version);
	if (version[0] == '\0') {
	    if ((cp = index(rev, '.')) != NULL)
		*cp = '\0';
	    rewind(fp);
	    get_branch_date(fp, date, rev, version);
	}
    } else {
	/*
	 * The trunk is the head.  Get the revision from the trunk and
	 * see if it evaluates to 1.1.  If so, walk the 1.1.1 branch looking
	 * for a match and return that; if not, just return 1.1.
	 */
	if ((cp = rindex(rev, '.')) != NULL)
	    *cp = '\0';			/* turn the revision into a branch */
	get_branch_date(fp, date, rev, version);
	if (strcmp(version, "1.1") == 0) {
	    rewind(fp);
	    get_branch_date(fp, date, "1.1.1", version);
	}
    }
}

static
get_branch_date(fp, date, rev, version)
    FILE *fp;
    char *date;
    char *rev;
    char *version;
{
    char line[MAXLINELEN], last_rev[50], curdate[50], date_dot[50];
    int date_dots, date_dotlen;
    char *cp, *semi;

    last_rev[0] = '\0';
    curdate[0] = '\0';
    (void) sprintf(date_dot, "%s.", rev);
    date_dotlen = strlen(date_dot);
    date_dots = numdots(rev);
    while (fgets(line, sizeof(line), fp) != NULL) {
	if (strncmp(line, RCSDESC, sizeof(RCSDESC) - 1) == 0)
	    return;
	if (isdigit(line[0])) {
	    if ((cp = rindex(line, '\n')) != NULL)
		*cp = '\0';		/* strip the newline */
	    if ((date_dots == 0 || strncmp(date_dot, line, date_dotlen) == 0) &&
		numdots(line) == date_dots+1)
		(void) strcpy(last_rev, line);
	    else
		last_rev[0] = '\0';
	    continue;
	}
	if (strncmp(line, RCSDATE, sizeof(RCSDATE) - 1) == 0 &&
	    isspace(line[sizeof(RCSDATE) - 1]) &&
	    last_rev[0] != '\0') {
	    for (cp = line; *cp && !isspace(*cp); cp++)
		;
	    while (*cp && isspace(*cp))
		cp++;
	    if (*cp && (semi = index(cp, ';')) != NULL) {
		*semi = '\0';		/* strip the semicolon */
		if (datecmp(cp, date) <= 0 && datecmp(cp, curdate) >= 0) {
		    (void) strcpy(curdate, cp);
		    (void) strcpy(version, last_rev);
		}
	    }
	}
    }
}

/*
 * Compare two dates in RCS format.
 * Beware the change in format on January 1, 2000,
 * when years go from 2-digit to full format.
 */
int
datecmp(date1, date2)
    char *date1, *date2;
{
    int length_diff = strlen(date1) - strlen(date2);
    return length_diff ? length_diff : strcmp(date1, date2);
}
