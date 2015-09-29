/* 
 * ld_front.c --
 *
 *	Front end for loader.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/ld/RCS/ld_front.c,v 1.2 90/08/06 21:07:08 rab Exp Locker: rab $";
#endif /* not lint */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <assert.h>
#ifndef __STDC__
#define const
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE    1
#endif

#ifdef sun3
static const char *hostMachine = "sun3";
#else
#ifdef sun4
static const char *hostMachine = "sun4";
#else
#ifdef ds3100
static const char *hostMachine = "ds3100";
#else
#ifdef symm
static const char *hostMachine = "symm";
#else
NO DEFAULT TARGET MACHINE TYPE DEFINED
#endif
#endif
#endif
#endif

/*
 *----------------------------------------------------------------------
 *
 * main --
 *      Search through the command line arguments looking for an argument
 *      of the form -mfoo, where `foo' is a machine type.  If found, delete
 *      it from the list of arguments, and use it to determine which
 *      backend to exec.  If there is no -m then default to the type
 *      of the host machine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Execs machine dependent loader backend.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int argc;
    char **argv;
{
    extern int errno;
    int i;
    int j;
    const char *targetMachine;
    char path[MAXPATHLEN];

    targetMachine = hostMachine;
    for (i = 1; i < argc; ++i) {
	if (argv[i][0] == '-' && argv[i][1] == 'm') {
	    if (argv[i][2] == '\0') {
		targetMachine = argv[i + 1];
		if (targetMachine == NULL) {
		    (void) fprintf(stderr, "No target machine specified\n");
		    exit(EXIT_FAILURE);
		}
		for (j = i; j < argc - 1; ++j) {
		    argv[j] = argv[j + 2];
		}
		argc -= 2;
		assert(argv[argc] == NULL);
	    } else {
		targetMachine = argv[i] + 2;
		for (j = i; j < argc; ++j) {
		    argv[j] = argv[j + 1];
		}
		--argc;
		assert(argv[argc] == NULL);
	    }
	    break;
	}
    }
    assert(argc >= 1);
    (void) sprintf(path,
	"/sprite/lib/gcc/%s.md/ld.%s", hostMachine, targetMachine);
    argv[0] = path;
    (void) execv(path, argv);
    (void) fprintf(stderr, "Can't exec ``%s'': %s\n", path, strerror(errno));
    exit(EXIT_FAILURE);
}

