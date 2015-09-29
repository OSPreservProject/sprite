#ifndef lint
static char rcsid[] = "$Id: scratch_entry.c,v 1.5 89/11/19 23:20:24 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Scratch Entry
 *
 * Removes the argument file from the Entries file.  A backup of the previous
 * Entries file is placed in Entries.backup.
 */

#include <sys/param.h>
#include "cvs.h"

Scratch_Entry(fname)
    char *fname;
{
    FILE *fpin, *fpout;
    char line[MAXLINELEN];
    char *cp, *cpend;

    rename_file(CVSADM_ENT, CVSADM_ENTBAK);
    fpin = open_file(CVSADM_ENTBAK, "r");
    fpout = open_file(CVSADM_ENT, "w+");
    while (fgets(line, sizeof(line), fpin) != NULL) {
	if ((cpend = rindex(line, '|')) && (cp = rindex(line, ' ')) &&
	    (cp++) && strncmp(fname, cp, MAX((cpend-cp), strlen(fname))) == 0)
	    continue;
	if (fputs(line, fpout) == EOF)
	    error(1, "cannot write file %s", CVSADM_ENT);
    }
    (void) fclose(fpin);
    (void) fclose(fpout);
}
