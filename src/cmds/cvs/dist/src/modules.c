#ifndef lint
static char rcsid[] = "$Id: modules.c,v 1.14.1.1 91/01/29 07:17:32 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Modules
 *
 *	Functions for accessing the modules file.
 *
 *	The modules file supports basically three formats of lines:
 *		key [options] directory files...
 *		key [options] directory
 *		key -a aliases...
 *
 *	The -a option allows an aliasing step in the parsing of the modules
 *	file.  The "aliases" listed on a line following the -a are
 *	processed one-by-one, as if they were specified as arguments on the
 *	command line.
 */

#include <sys/param.h>
#include <sys/file.h>
#include <ndbm.h>
#include "cvs.h"

extern int update_build_dirs;

int run_module_prog = 1;		/* run -i/-o/-t prog by default */

/*
 * Open the modules file, and die if the CVSROOT environment variable
 * was not set.  If the modules file does not exist, that's fine, and
 * a warning message is displayed and a NULL is returned.
 */
DBM *
open_module()
{
    char mfile[MAXPATHLEN];
    DBM *db;

    if (CVSroot == NULL) {
	(void) fprintf(stderr, "%s: must set the CVSROOT environment variable\n",
		       progname);
	error(0, "or specify the '-d' option to %s", progname);
    }
    (void) sprintf(mfile, "%s/%s", CVSroot, CVSROOTADM_MODULES);
    if ((db = dbm_open(mfile, O_RDONLY, 0666)) == NULL)
	warn(0, "warning: cannot open modules file %s", mfile);
    return (db);
}

/*
 * Close the modules file, if the open succeeded, that is
 */
close_module(db)
    DBM *db;
{
    if (db != NULL) {
	dbm_close(db);
    }
}

/*
 * This is the recursive function that processes a module name.
 * If tag is set, just tag files, otherwise, we were called to check files
 * out.
 */
do_module(db, mname, m_type, msg)
    DBM *db;
    char *mname;
    enum mtype m_type;
    char *msg;
{
    char *checkin_prog = NULL, *checkout_prog = NULL, *tag_prog = NULL;
    char cwd[MAXPATHLEN], file[MAXPATHLEN];
    char *moduleargv[MAXFILEPERDIR];
    int moduleargc, just_file;
    datum key, val;
    char *cp, **argv;
    int c, alias = 0, argc, err = 0;

    just_file = FALSE;
    update_build_dirs = FALSE;
    /*
     * Look the argument module name up in the database; if we found it, use
     * it, otherwise, see if the file or directory exists relative to the
     * root directory (CVSroot).  If there is a directory there, it is
     * extracted or tagged recursively; if there is a file there, it is
     * extracted or tagged individually; if there is no file or directory
     * there, we are done.
     */
    key.dptr = mname;
    key.dsize = strlen(key.dptr);
    if (db != NULL)
	val = dbm_fetch(db, key);
    else
	val.dptr = NULL;
    if (val.dptr != NULL) {
	val.dptr[val.dsize] = '\0';
    } else {
	/*
	 * Need to determine if the argument module name is a directory
	 * or a file relative to the CVS root and set update_build_dirs
	 * and just_file accordingly
	 */
	update_build_dirs = TRUE;
	(void) sprintf(file, "%s/%s", CVSroot, key.dptr);
	if (!isdir(file)) {
	    (void) strcat(file, RCSEXT);
	    if (!isfile(file)) {
		warn(0, "cannot find '%s' - ignored", key.dptr);
		err++;
		return (err);
	    } else {
		update_build_dirs = FALSE;
		just_file = TRUE;
	    }
	}
	val = key;
    }
    /*
     * If just extracting or tagging files, need to munge the
     * passed in module name to look like an actual module entry.
     */
    if (just_file == TRUE) {
	if ((cp = rindex(key.dptr, '/')) != NULL) {
	    *cp++ = '\0';
	} else {
	    cp = key.dptr;
	    key.dptr = ".";
	}
	(void) sprintf(file, "%s %s %s", key.dptr, key.dptr, cp);
    } else {
	(void) sprintf(file, "%s %s", key.dptr, val.dptr);
    }
    line2argv(&moduleargc, moduleargv, file);
    argc = moduleargc;
    argv = moduleargv;
    if (getwd(cwd) == NULL)
	error(0, "cannot get current working directory: %s", cwd);
    optind = 1;
    while ((c = getopt(argc, argv, CVSMODULE_OPTS)) != -1) {
	switch (c) {
	case 'a':
	    alias = 1;
	    break;
	case 'i':
	    checkin_prog = optarg;
	    break;
	case 'o':
	    checkout_prog = optarg;
	    break;
	case 't':
	    tag_prog = optarg;
	    break;
	case '?':
	    warn(0, "modules file has invalid option for key %s value %s",
		 key.dptr, val.dptr);
	    err++;
	    return (err);
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc == 0) {
	warn(0, "modules file missing directory for key %s value %s",
	     key.dptr, val.dptr);
	err++;
	return (err);
    }
    if (alias) {
	register int i;

	for (i = 0; i < argc; i++) {
	    if (!quiet)
		printf("%s %s: %s %s\n", progname, command, msg, argv[i]);
	    err += do_module(db, argv[i], m_type, msg);
	}
	return (err);
    }
    err += process_module(argc, argv, m_type, &key);
    if (err == 0 && run_module_prog) {
	if (m_type == CHECKOUT && checkin_prog !=  NULL) {
	    FILE *fp = open_file(CVSADM_CIPROG, "w+");
	    (void) fprintf(fp, "%s\n", checkin_prog);
	    (void) fclose(fp);
	}
    }
    if (chdir(cwd) < 0)
	error(1, "failed chdir to %s!", cwd);
    if (err == 0 && run_module_prog) {
	if ((m_type == TAG && tag_prog != NULL) ||
	    (m_type == CHECKOUT && checkout_prog != NULL)) {
	    (void) sprintf(prog, "%s %s",
			   m_type == TAG ? tag_prog : checkout_prog, key.dptr);
	    if (!quiet)
		printf("%s %s: Executing '%s'\n", progname, command, prog);
	    err += system(prog);
	}
    }
    free_names(&moduleargc, moduleargv);
    return (err);
}

static
process_module(argc, argv, m_type, keyp)
    int argc;
    char *argv[];
    enum mtype m_type;
    datum *keyp;
{
    register int i, just_file;
    int err = 0;

    just_file = argc > 1;
    if (!just_file && update_build_dirs == FALSE && argc == 1)
	update_build_dirs = TRUE;
    if (m_type == TAG || m_type == PATCH) {
	(void) sprintf(Repository, "%s/%s", CVSroot, argv[0]);
	if (chdir(Repository) < 0) {
	    warn(1, "cannot chdir to %s", Repository);
	    err++;
	    return (err);
	}
    } else {
	if (Build_Dirs_and_chdir(keyp->dptr) != 0) {
	    warn(0, "ignoring module %s", keyp->dptr);
	    err++;
	    return (err);
	}
	(void) sprintf(Repository, "%s/%s", CVSroot, argv[0]);
	if (!isdir(CVSADM)) {
	    FILE *fp;

	    Create_Admin(Repository, DFLT_RECORD);
	    if (just_file == TRUE) {
		fp = open_file(CVSADM_ENTSTAT, "w+");
		(void) fclose(fp);
	    }
	} else {
	    char file[MAXPATHLEN];

	    (void) strcpy(file, Repository);
	    Name_Repository();
	    if (strcmp(Repository, file) != 0) {
		warn(0, "existing repository %s does not match %s",
		     Repository, file);
		err++;
		return (err);
	    }
	}
    }
    if (update_build_dirs == TRUE) {
	extern char update_dir[];

	(void) strcpy(update_dir, keyp->dptr);
	if (m_type == CHECKOUT)
	    err += update(0, (char **)0);
	else if (m_type == TAG)
	    err += tagit((char *)0);
	else if (m_type == PATCH)
	    err += patched((char *)0);
	else
	    error(0, "impossible module type %d", (int)m_type);
	update_dir[0] = '\0';
	return (err);
    }
    argc--;
    argv++;
    for (i = 0; i < argc; i++) {
	char line[MAXLINELEN];

	(void) strcpy(User, argv[i]);
	(void) sprintf(Rcs, "%s/%s%s", Repository, User, RCSEXT);
	if (m_type == CHECKOUT) {
	    Version_TS(Rcs, Tag, User);
	    if (TS_User[0] == '\0') {
		(void) sprintf(line, "Initial %s", User);
		Register(User, VN_Rcs, line);
	    }
	} else if (m_type == TAG) {
	    err += tagit(Rcs);
	} else if (m_type == PATCH) {
	    err += patched(Rcs);
	} else {
	    error(0, "impossible module type %d", (int)m_type);
	}
    }
    if (m_type == CHECKOUT)
	err += update(++argc, --argv);
    return (err);
}

cat_module()
{
    FILE *fp;
    DBM *db;
    datum key, val;

    if ((db = open_module()) == NULL)
	error(0, "failed to cat the modules file");
    if ((fp = popen(SORT, "w")) == NULL)
	fp = stdout;
    for (key = dbm_firstkey(db); key.dptr != NULL; key = dbm_nextkey(db)) {
	key.dptr[key.dsize] = '\0';
	(void) fprintf(fp, "%-20s", key.dptr);
	val = dbm_fetch(db, key);
	if (val.dptr != NULL) {
	    val.dptr[val.dsize] = '\0';
	    (void) fprintf(fp, " %s\n", val.dptr);
	} else {
	    (void) fprintf(fp, "\n");
	}
    }
    if (fp != stdout)
	(void) pclose(fp);
}
