/*
**  GETOPT PROGRAM AND LIBRARY ROUTINE
**
**  I wrote main() and AT&T wrote getopt() and we both put our efforts into
**  the public domain via mod.sources.
**	Rich $alz
**	Mirror Systems
**	(mirror!rs, rs@mirror.TMC.COM)
**	August 10, 1986
*/

/* $Header: /sprite/src/cmds/getopt/RCS/getopt.c,v 1.2 90/10/29 13:19:34 kupfer Exp $ */

#include <stdio.h>


#ifndef INDEX
#define INDEX index
#endif


extern char	*INDEX();
extern int	 optind;
extern char	*optarg;


main(ac, av)
    register int	 ac;
    register char 	*av[];
{
    register char 	*flags;
    register int	 c;

    /* Check usage. */
    if (ac < 2) {
	fprintf(stderr, "usage: %s flag-specification arg-list\n", av[0]);
	exit(2);
    }

    /* Play games; remember the flags (first argument), then splice
       them out so it looks like a "standard" command vector. */
    flags = av[1];
    av[1] = av[0];
    av++;
    ac--;

    /* Print flags. */
    while ((c = getopt(ac, av, flags)) != EOF) {
	if (c == '?')
	    exit(1);
	/* We assume that shells collapse multiple spaces in `` expansion. */
	printf("-%c %s ", c, INDEX(flags, c)[1] == ':' ? optarg : "");
    }

    /* End of flags; print rest of options. */
    printf("-- ");
    for (av += optind; *av; av++)
	printf("%s ", *av);

    printf("\n");
    exit(0);
}
