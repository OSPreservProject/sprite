#include "stdio.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "font.h"
#include "font.v0.h"


main (argc, argv)
char  **argv; {
    char   *dir = "/user/jag/v1fonts";
    while (--argc >= 1) {
	argv++;
	if (strcmp (*argv, "-d") == 0) {
	    dir = *++argv;
	    argc--;
	}
	else {
	    char    buf[100];
	    register char  *p,
	                   *n;
	    p = n = *argv;
	    while (*p)
		if (*p++ == '/')
		    n = p;
	    sprintf (buf, "%s/%s", dir, n);
	    cvtfont (*argv, buf);
	}
    }
}

cvtfont (in, out)
char   *in,
       *out; {
    struct v0_font *v0f;
    int     fd;
    struct stat st;
    FILE * outf;
    fd = open (in, 0);
    if (fd < 0) {
	printf ("Can't read %s\n", in);
	return;
    }
    printf ("%s -> %s\n", in, out);
    fstat (fd, &st);
    v0f = (struct v0_font  *) malloc (st.st_size + 1);
    if (read (fd, v0f, st.st_size + 1) != st.st_size) {
	printf ("  Error reading %s\n", in);
	close (fd);
	return;
    }
    close (fd);
    ExplodeV0Font (v0f);
    ComputeBoundingBoxes ();
    ImplodeFont ();
}
