#ifndef lint
static char rcsid[] = "$Id: mkmodules.c,v 1.9 89/11/19 23:20:10 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * mkmodules
 *
 *	Re-build the modules database for the CVS system.  Accepts one
 *	argument, which is the directory that the modules,v file lives in.
 */

#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>
#include <ndbm.h>
#include <ctype.h>
#include "cvs.h"

char *progname;

char prog[MAXPROGLEN];
char *Rcsbin = RCSBIN_DFLT;

main(argc, argv)
    int argc;
    char *argv[];
{
    extern char *getenv();
    char temp[50];
    char *cp;

    /*
     * Just save the last component of the path for error messages
     */
    if ((progname = rindex(argv[0], '/')) == NULL)
	progname = argv[0];
    else
	progname++;

    if (argc != 2)
	mkmodules_usage();
    
    if ((cp = getenv(RCSBIN_ENV)) != NULL)
	Rcsbin = cp;

    if (chdir(argv[1]) < 0)
	error(1, "cannot chdir to %s", argv[1]);
    /*
     * First, do the work necessary to update the "modules" database.
     */
    make_tempfile(CVSMODULE_TMP, temp);
    if (checkout_file(CVSMODULE_FILE, temp) == 0) {
	write_dbmfile(temp);
	rename_dbmfile(temp);
    }
    (void) unlink(temp);
    /*
     * Now, check out the "loginfo" file, so that it is always up-to-date
     * in the CVSROOT.adm directory.
     */
    make_tempfile(CVSLOGINFO_TMP, temp);
    if (checkout_file(CVSLOGINFO_FILE, temp) == 0)
	rename_loginfo(temp);
    (void) unlink(temp);
    exit(0);
}

static
make_tempfile(file, temp)
    char *file;
    char *temp;
{
    int fd;

    (void) strcpy(temp, file);
    if ((fd = mkstemp(temp)) < 0)
	error(1, "cannot create temporary file %s", temp);
    (void) close(fd);
}

static
checkout_file(file, temp)
    char *file;
    char *temp;
{
    (void) sprintf(prog, "%s/%s -q -p %s > %s", Rcsbin, RCS_CO, file, temp);
    if (system(prog) != 0) {
	warn(0, "failed to check out %s file", file);
	return (1);
    }
    return (0);
}

static
write_dbmfile(temp)
    char *temp;
{
    char line[DBLKSIZ], value[DBLKSIZ];
    FILE *fp;
    DBM *db;
    char *cp, *vp;
    datum key, val;
    int len, cont, err = 0;

    fp = open_file(temp, "r");
    if ((db = dbm_open(temp, O_RDWR|O_CREAT|O_TRUNC, 0666)) == NULL)
	error(1, "cannot open dbm file %s for creation", temp);
    for (cont = 0; fgets(line, sizeof(line), fp) != NULL; ) {
	if ((cp = rindex(line, '\n')) != NULL)
	    *cp = '\0';			/* strip the newline */
	/*
	 * Add the line to the value, at the end if this is a continuation
	 * line; otherwise at the beginning, but only after any trailing
	 * backslash is removed.
	 */
	vp = value;
	if (cont)
	    vp += strlen(value);
	/*
	 * See if the line we read is a continuation line, and strip the
	 * backslash if so.
	 */
	len = strlen(line);
	if (len > 0)
	    cp = &line[len-1];
	else
	    cp = line;
	if (*cp == '\\') {
	    cont = 1;
	    *cp = '\0';
	} else {
	    cont = 0;
	}
	(void) strcpy(vp, line);
	if (value[0] == '#')
	    continue;			/* comment line */
	vp = value;
	while (*vp && isspace(*vp))
	    vp++;
	if (*vp == '\0')
	    continue;			/* empty line */
	/*
	 * If this was not a continuation line, add the entry to the database
	 */
	if (!cont) {
	    key.dptr = vp;
	    while (*vp && !isspace(*vp))
		vp++;
	    key.dsize = vp - key.dptr;
	    *vp++ = '\0';		/* NULL terminate the key */
	    while (*vp && isspace(*vp))
		vp++;			/* skip whitespace to value */
	    if (*vp == '\0') {
		warn(0, "warning: NULL value for key '%s'", key.dptr);
		continue;
	    }
	    val.dptr = vp;
	    val.dsize = strlen(vp);
	    if (dbm_store(db, key, val, DBM_INSERT) == 1) {
		warn(0, "duplicate key found for '%s'", key.dptr);
		err++;
	    }
	}
    }
    dbm_close(db);
    (void) fclose(fp);
    (void) unlink(temp);
    if (err) {
	char dotdir[50], dotpag[50];

	(void) sprintf(dotdir, "%s.dir", temp);
	(void) sprintf(dotpag, "%s.pag", temp);
	(void) unlink(dotdir);
	(void) unlink(dotpag);
	error(0, "DBM creation failed; correct above errors");
    }
}

static
rename_dbmfile(temp)
    char *temp;
{
    char newdir[50], newpag[50];
    char dotdir[50], dotpag[50];
    char bakdir[50], bakpag[50];

    (void) signal(SIGHUP, SIG_IGN);	/* don't mess with me... */
    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGTERM, SIG_IGN);

    (void) sprintf(dotdir, "%s.dir", CVSMODULE_FILE);
    (void) sprintf(dotpag, "%s.pag", CVSMODULE_FILE);
    (void) sprintf(bakdir, "%s%s.dir", BAKPREFIX, CVSMODULE_FILE);
    (void) sprintf(bakpag, "%s%s.pag", BAKPREFIX, CVSMODULE_FILE);
    (void) sprintf(newdir, "%s.dir", temp);
    (void) sprintf(newpag, "%s.pag", temp);

    (void) chmod(newdir, 0666);
    (void) chmod(newpag, 0666);

    (void) unlink(bakdir);		/* rm .#modules.dir .#modules.pag */
    (void) unlink(bakpag);
    (void) rename(dotdir, bakdir);	/* mv modules.dir .#modules.dir */
    (void) rename(dotpag, bakpag);	/* mv modules.pag .#modules.pag */
    (void) rename(newdir, dotdir);	/* mv "temp".dir modules.dir */
    (void) rename(newpag, dotpag);	/* mv "temp".pag modules.pag */
}

static
rename_loginfo(temp)
    char *temp;
{
    char bak[50];

    if (chmod(temp, 0666) < 0)			/* chmod 666 "temp" */
	warn(1, "warning: cannot chmod %s", temp);
    (void) sprintf(bak, "%s%s", BAKPREFIX, CVSLOGINFO_FILE);
    (void) unlink(bak);				/* rm .#loginfo */
    (void) rename(CVSLOGINFO_FILE, bak);	/* mv loginfo .#loginfo */
    (void) rename(temp, CVSLOGINFO_FILE);	/* mv "temp" loginfo */
}

/*
 * For error() only
 */
void
Lock_Cleanup(sig)
{
#ifdef lint
    sig = sig;
#endif lint
}

static
mkmodules_usage()
{
    (void) fprintf(stderr, "Usage: %s modules-directory\n", progname);
    exit(1);
}
