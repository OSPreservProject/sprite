#ifndef lint
static char rcsid[] = "$Id: version_ts.c,v 1.1 91/07/11 18:05:50 jhh Exp Locker: jhh $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Version and Time Stamp
 *
 *	Sets the following global variables:
 *	VN_User		version # of the RCS file the user file derives from;
 *			may also be:
 *				empty:		no entry for user file
 *				0:		user file is new
 *				-$VN_User:	user file is to be removed
 *	VN_Rcs		version # of active RCS file
 *				is empty for absent RCS file
 *	TS_User		present time stamp of the user file
 *				is empty for absent user file
 *	TS_Rcs		time stamp of the lastest check-out of the RCS file.
 *
 *	The syntax of an entry is
 *		<version-number>|<time-stamp>|
 *	and the time-stamp currently includes the file change and modify 
 *	times as well as the User file name.
 */

#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <utmp.h>
#include "cvs.h"

/*
 * "rcs" is the full pathname to the ,v file; "user" is the name of
 * the local file.
 */
Version_TS(rcs, tag, user)
    char *rcs;
    char *tag;
    char *user;
{
    FILE *fpin;
    char line[MAXLINELEN];
    char *cp;
    int found = 0;

    /*
     * Get RCS version number in VN_Rcs
     */
    Version_Number(rcs, tag, Date, VN_Rcs);
    time_stamp(user, TS_User);		/* get time-stamp in TS_User */
    /*
     * Now read through the "Entries" file to find the
     * version number of the user file, and the time-stamp
     * of the RCS file
     */
    fpin = open_file(CVSADM_ENT, "r");
    while (fgets(line, sizeof(line), fpin) != NULL) {
	if ((cp = rindex(line, '|')) == NULL)
	    continue;
	*cp = '\0';
	if ((cp = rindex(line, ' ')) == NULL)
	    continue;
	cp++;
	if (strcmp(user, cp) == 0) {
	    found = 1;
	    break;
	}
    }
    if (found) {
	if ((cp = index(line, '|')) != NULL) {
	    *cp++ = '\0';
	    (void) strcpy(VN_User, line);
	    (void) strcpy(TS_Rcs, cp);
	} else {
	    VN_User[0] = '\0';
	    TS_Rcs[0] = '\0';
	}
    } else {
	VN_User[0] = '\0';
	TS_Rcs[0] = '\0';
    }
    (void) fclose(fpin);
}

/* Some UNIX distributions don't include these in their stat.h */
#ifndef S_IWRITE
#define	S_IWRITE	0000200		/* write permission, owner */
#endif !S_IWRITE
#ifndef S_IWGRP
#define	S_IWGRP		0000020		/* write permission, grougroup */
#endif !S_IWGRP
#ifndef S_IWOTH
#define	S_IWOTH		0000002		/* write permission, other */
#endif !S_IWOTH

/*
 * Gets the time-stamp for the file "file" and puts it in the already
 * allocated string "ts".
 *
 * As a side effect, if the user wants writable files and the file
 * currently has no write bits on, the file is made writable now.
 */
static
time_stamp(file, ts)
    char *file;
    char *ts;
{
    struct stat sb;
    char *ctime();
    char *cp;

    if (lstat(file, &sb) < 0) {
		ts[0] = '\0';
    } else {
	if (cvswrite == TRUE &&
	    (sb.st_mode & (S_IWRITE|S_IWGRP|S_IWOTH)) == 0) {
	    xchmod(file, 1);
	    (void) stat(file, &sb);
	}
	cp = ctime(&sb.st_ctime);
	cp[24] = ' ';
	(void) strcpy(ts, cp);
	cp = ctime(&sb.st_mtime);
	cp[24] = ' ';
	(void) strcat(ts, cp);
	(void) strcat(ts, file);
    }
}
