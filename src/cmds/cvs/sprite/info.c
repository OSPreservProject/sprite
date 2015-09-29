#include "cvs.h"

info(argc, argv)
    int argc;
    char *argv[];
{
    int c, err = 0;

    if (argc == -1)
	info_usage();
    optind = 1;
    while ((c = getopt(argc, argv, "fr:D:")) != -1) {
	switch (c) {
	case 'Q':
	    really_quiet = 1;
	    /* FALL THROUGH */
	case 'q':
	    quiet = 1;
	    break;
	case 'f':
	    force_tag_match = 1;
	    break;
	case 'r':
	    (void) strcpy(Tag, optarg);
	    break;
	case 'D':
	    Make_Date(optarg, Date);
	    break;
	case '?':
	default:
	    info_usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if (!isdir(CVSADM)) {
	if (!quiet) {
	    warn(0, "warning: no %s directory found", CVSADM);
	}
    }
    Name_Repository();
    Reader_Lock();
    if (argc <= 0) {
	if (force_tag_match && (Tag[0] != '\0' || Date[0] != '\0'))
	    Find_Names(&fileargc, fileargv, ALLPLUSATTIC);
	else
	    Find_Names(&fileargc, fileargv, ALL);
	argc = fileargc;
	argv = fileargv;
    }
    (void) Collect_Sets(argc, argv);
    free_names(&fileargc, fileargv);
    info_process_lists();
    Lock_Cleanup(0);
}
/*
 * Process the lists created by Collect_Sets().
 */
static
info_process_lists()
{
    char *cp;
    int err = 0;
    /*
     * Olist is the "needs checking out" list.
     */
    for (cp = strtok(Olist, " \t"); cp; cp = strtok((char *)NULL, " \t")) {
	printf("U %s\n", cp);
    }
    /*
     * Mlist is the "modified, needs checking in" list.
     */
    for (cp = strtok(Mlist, " \t"); cp; cp = strtok((char *)NULL, " \t")) {
	printf("M %s\n", cp);
    }
    /*
     * Alist is the "to be added" list.
     */
    for (cp = strtok(Alist, " \t"); cp; cp = strtok((char *)NULL, " \t")) {
	printf("A %s\n", cp);
    }
    /*
     * Rlist is the "to be removed" list.
     */
    for (cp = strtok(Rlist, " \t"); cp; cp = strtok((char *)NULL, " \t")) {
	printf("R %s\n", cp);
    }
    /*
     * Glist is the "modified, needs merging" list.
     */
    for (cp = strtok(Glist, " \t"); cp; cp = strtok((char *)NULL, " \t")) {
	printf("C %s\n", cp);
    }
    /*
     * Wlist is the "needs to be removed" list.
     */
    for (cp = strtok(Wlist, " \t"); cp; cp = strtok((char *)NULL, " \t")) {
	printf("D %s\n", cp);
    }
}


static
info_usage()
{
    (void) fprintf(stderr,
		   "Usage: %s %s [-f] [-r tag|-D date] [files...]\n",
		   progname, command);
    exit(1);
}

