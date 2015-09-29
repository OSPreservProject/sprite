#include "framebuf.h"

int     Radius = 200;
int     Steps = 10;
int     StWidth;
int     Xc = 500;
int     Yc = 400;

main (argc, argv)
char  **argv; {
    register    x;
    while (argc >= 3) {
	x = atoi (argv[2]);
	if (x <= 0) {
	    printf ("Bad numeric value: %s (for %s)\n", argv[2], argv[1]);
	    exit (1);
	}
	if (strcmp (argv[1], "-r") == 0)
	    Radius = x;
	else
	    if (strcmp (argv[1], "-s") == 0)
		Steps = x;
	    else
		if (strcmp (argv[1], "-x") == 0)
		    Xc = x;
		else
		    if (strcmp (argv[1], "-y") == 0)
			Yc = x;
		    else {
			printf ("Bad option: %s\nUse one of -s, -r, -x or -y\n",
				argv[1]);
			exit (1);
		    }
	argc -= 2;
	argv += 2;
    }
    StWidth = Radius / Steps;
    if (GXfind ())
	exit (1);
    GXfunction = GXset;
    ROPds (0, 0, 1024, 1024);
    GXfunction = GXclear;
    for (x = Radius; x >= -Radius; x--) {
	register    slot = ((x + Radius) % StWidth) * 3 / StWidth;
	register    r,
	            r2;
	if (slot == 2)
	    continue;
	r = sqrt (Radius * Radius - x * x);
	r2 = slot ? r : x >= 0 ? Radius / 2 - r : r - Radius / 2;
	if (-r2 > r || r2 > r)
	    continue;
	vec (Xc - x, Yc - r, Xc - x, Yc + r2);
    }
}

sqrt (x)
register    x; {
    register    v,
                i;
    v = 0;
    i = 1024;
    while (i != 0) {
	if ((v + i) * (v + i) <= x)
	    v += i;
	i >>= 1;
    }
    return (v);
}
