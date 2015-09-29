#ifndef lint
static char rcsid[] = "$Id: create_admin.c,v 1.7 89/11/19 23:19:55 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Create Administration.
 *
 *	Creates a CVS administration directory based on the argument
 *	repository; the "Entries" file is prefilled from the "initrecord"
 *	argument.
 */

#include <sys/param.h>
#include "cvs.h"

Create_Admin(repository, initrecord)
    register char *repository;
    register char *initrecord;
{
    register FILE *fout;
    char *cp;

    if (!isdir(repository))
	error(0, "there is no repository %s", repository);
    if (!isfile(initrecord))
	error(0, "there is no file %s", initrecord);
    if (isfile(CVSADM))
	error(0, "there is a version here already");
    make_directory(CVSADM);
    fout = open_file(CVSADM_REP, "w+");
    cp = repository;
    if (CVSroot != NULL) {
	char path[MAXPATHLEN];

	(void) sprintf(path, "%s/", CVSroot);
	if (strncmp(repository, path, strlen(path)) == 0)
	    cp = repository + strlen(path);
    }
    if (fprintf(fout, "%s\n", cp) == EOF)
	error(1, "write to %s failed", CVSADM_REP);
    (void) fclose(fout);
    copy_file(initrecord, CVSADM_ENT);
}
