/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific written prior permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1987 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ln.c	4.10 (Berkeley) 11/30/87";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include <sprite.h>
#include <fs.h>
#include <status.h>

static int	fflag,				/* undocumented force flag */
		sflag,				/* symbolic, not hard, link */
		rflag,				/* remote link for Sprite */
		(*linkf)();			/* system link call */

/*
 * Forward declarations to keep gcc happy:
 */

static int linkit(), usage(), remoteLink();

main(argc, argv)
	int	argc;
	char	**argv;
{
	extern int	optind;
	struct stat	buf;
	int	ch, exitval, link(), symlink();
	char	*sourcedir;

	while ((ch = getopt(argc, argv, "fsr")) != EOF)
		switch((char)ch) {
		case 'f':
			fflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case '?':
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	if (rflag && sflag) {
	    usage();
	}

	if (sflag) {
	    linkf = symlink;
	} else if (rflag) {
	    linkf = remoteLink;
	} else {
	    linkf = link;
	}

	switch(argc) {
	case 0:
		usage();
	case 1:				/* ln target */
		exit(linkit(argv[0], ".", 1));
	case 2:				/* ln target source */
		exit(linkit(argv[0], argv[1], 0));
	default:			/* ln target1 target2 directory */
		sourcedir = argv[argc - 1];
		if (stat(sourcedir, &buf)) {
			perror(sourcedir);
			exit(1);
		}
		if ((buf.st_mode & S_IFMT) != S_IFDIR)
			usage();
		for (exitval = 0; *argv != sourcedir; ++argv)
			exitval |= linkit(*argv, sourcedir, 1);
		exit(exitval);
	}
	/*NOTREACHED*/
}

static
 linkit(target, source, isdir)
	char	*target, *source;
	int	isdir;
{
	extern int	errno;
	struct stat	buf;
	char	path[MAXPATHLEN],
		*cp, *rindex(), *strcpy();

	if (!sflag && !rflag) {
		/* if target doesn't exist, quit now */
		if (stat(target, &buf)) {
			perror(target);
			return(1);
		}
		/* only symbolic links to directories, unless -f option used */
		if (!fflag && (buf.st_mode & S_IFMT) == S_IFDIR) {
			printf("%s is a directory.\n", target);
			return(1);
		}
	}

	/* if the source is a directory, append the target's name */
	if (isdir || !stat(source, &buf) && (buf.st_mode & S_IFMT) == S_IFDIR) {
		if (!(cp = rindex(target, '/')))
			cp = target;
		else
			++cp;
		(void)sprintf(path, "%s/%s", source, cp);
		source = path;
	}

	if ((*linkf)(target, source)) {
		perror(source);
		return(1);
	}
	return(0);
}

static
remoteLink(target, source)
    char *target;
    char *source;
{
    struct stat stb;
    ReturnStatus status;

    if (target[0] != '/') {
	fprintf(stderr, "Target \"%s\" should be an absolute path\n", target);
	return(1);
    }
    if (lstat(source, &stb) >= 0) {
	/*
	 * The link already exists.  Rewrite it if it's a remote link,
	 * otherwise complain.
	 */
	if ((stb.st_mode&S_IFMT) != S_IFRLNK) {
	    fprintf(stderr, "\"%s\" exists\n",source);
	    return(1);
	} else if (unlink(source) < 0) {
	    perror(target);
	    return(1);
	}
    }
    status = Fs_SymLink(target, source, 1);
    if (status != 0) {
	Stat_PrintMsg(status, "Fs_SymLink");
	return(1);
    }
    return(0);
}

static
usage()
{
	fputs("usage:\tln [-s | -r] targetname [sourcename]\n\tln [-s | -r] targetname1 targetname2 [... targetnameN] sourcedirectory\n\n", stderr);
	exit(1);
}
