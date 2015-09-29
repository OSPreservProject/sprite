/* 
 * chmagic.c --
 *
 *	Change the magic number of a BSD format demand paged executable.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <a.out.h>
#include <sys/exec.h>
#include <sys/file.h>

extern int errno;

static int forceFlag;

static int changeMagic _ARGS_ ((int magic, const char *filename));
static int getMagic _ARGS_ ((const char *magicString));
static void usage _ARGS_ ((void));

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int argc;
    char **argv;
{
    int i;
    int magic;
    int status;

    if (argc < 3) {
	usage();
    }
    if (strcmp(argv[1], "-f") == 0) {
	forceFlag = 1;
	++argv;
	--argc;
    }
    magic = getMagic(argv[1]);
    status = EXIT_SUCCESS;
    for (i = 2; i < argc; ++i) {
	if (changeMagic(magic, argv[i])) {
	    status = EXIT_FAILURE;
	}
    }
    exit(status);
}

static int
getMagic(magicString)
    const char *magicString;
{
    int i;
    int base;
    char *endPtr;
    int val;
    static struct {
	const char *string;
	int val;
    } tab[] = {
	{ "OMAGIC",           OMAGIC },
	{ "NMAGIC",           NMAGIC },
	{ "ZMAGIC",           ZMAGIC },
	{ "SPRITE_ZMAGIC",    SPRITE_ZMAGIC },
	{ "UNIX_ZMAGIC",      UNIX_ZMAGIC },
    };

    /*
     * See if the magicString specifies on of the magic symbols
     */
    for (i = 0; i < sizeof(tab)/sizeof(*tab); ++i) {
	if (strcmp(magicString, tab[i].string) == 0) {
	    return tab[i].val;
	}
    }
    val = strtoul(magicString, &endPtr, 0);
    if (*endPtr != '\0') {
	usage();
    }
    return val;
}

static int
changeMagic(magic, filename)
    int magic;
    const char *filename;
{
    int fd;
    struct exec buf;
    int r;

    if ((fd = open(filename, O_RDWR)) < 0) {
	(void) fprintf(stderr, "Cannot open %s: %s\n",
	    filename, strerror(errno));
	return 1;
    }

    if ((r = read(fd, (char *) &buf, sizeof(buf))) != sizeof(buf)) {
	if (r < 0) {
	    (void) fprintf(stderr, "Error reading %s: %s\n",
		filename, (r < 0) ? strerror(errno) : "file too small");
	}
	return 1;
    }
    if (forceFlag == 0 && N_BADMAG(buf)) {
	(void) fprintf(stderr, "Bad magic number for %s: 0x%x\n",
	    filename, buf.a_magic);
	return 1;
    }
    buf.a_magic = magic;
    if (lseek(fd, 0L, L_SET) != 0) {
	(void) fprintf(stderr, "Error seeking %s: %s\n",
	    filename, strerror(errno));
	return 1;
    }
    if (write(fd, (char *) &buf, sizeof(buf)) != sizeof(buf)) {
	(void) fprintf(stderr, "Error writing %s: %s\n",
	    filename, strerror(errno));
	return 1;
    }
    close(fd);
    return 0;
}

static void
usage()
{

    (void) fprintf(stderr, "usage: chmagic [-f] MAGIC file1 ...\n");
    exit(EXIT_FAILURE);
}

