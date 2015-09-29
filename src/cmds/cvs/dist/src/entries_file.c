#ifndef lint
static char rcsid[] = "$Id: entries_file.c,v 1.6 89/11/19 23:20:00 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Entries file to Files file
 *
 * Creates the file Files containing the names that comprise
 * the project, from the Entries file.
 */

#include "cvs.h"

Entries2Files()
{
    register FILE *fpin, *fpout;
    register int num = 0;
    register char *cp;
    char line[MAXLINELEN];
    char *fnames[MAXFILEPERDIR];

    fpin = open_file(CVSADM_ENT, "r");
    fpout = open_file(CVSADM_FILE, "w+");
    while (fgets(line, sizeof(line), fpin) != NULL) {
	if ((cp = rindex(line, '|')) == NULL)
	    continue;
	*cp = '\0';
	if ((cp = rindex(line, ' ')) == NULL)
	    continue;
	cp++;
	fnames[num] = xmalloc(strlen(cp) + 1);
	(void) strcpy(fnames[num++], cp);
	if (num >= MAXFILEPERDIR)
	    error(0, "more than %d files is too many", MAXFILEPERDIR);
    }
    if (num) {
	qsort((char *)&fnames[0], num, sizeof(fnames[0]), ppstrcmp_files);
	while (num--) {
	    if (fprintf(fpout, "%s\n", fnames[num]) == EOF)
		error(1, "cannot write %s", CVSADM_FILE);
	    free(fnames[num]);
	}
    }
    (void) fclose(fpin);
    (void) fclose(fpout);
}
