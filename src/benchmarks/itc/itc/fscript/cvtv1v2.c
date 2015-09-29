#include "font.v1.h"
#include "font.h"
#include "sys/types.h"
#include "sys/stat.h"

char *f;
int size = 0;

struct font SF;
#define firstpart (((int) &SF.NIcons) - (int) &SF)

cvt(name) {
    int fd = open(name, 0);
    struct stat st;
    static short len = 128;
    if (fd<0 || fstat(fd, &st)<0) {
	printf ("%s:	Can't open\n", name);
	return;
    };
    if (size<=st.st_size) {
	if (f) free(f);
	f = (char *) malloc(size = st.st_size*3/2);
    }
    if (read (fd, f, st.st_size+1) != st.st_size) {
	printf ("%s:	Couldn't read\n", name);
	close(fd);
    }
    close(fd);
    if (((struct v1_font *) f)->magic != v1_FONTMAGIC) {
	printf ("%s:	Not a version 1 font\n", name);
	return;
    }
    ((struct font *) f)->magic = FONTMAGIC;
    fd = creat (name, 0666);
    write (fd, f, firstpart);
    write (fd, &len, 2);
    write (fd, f+firstpart, st.st_size-firstpart);
    close (fd);
    printf (" %s:	Done.\n", name);
}

main (argc, argv)
char **argv; {
    while (--argc>0) cvt(*++argv);
}
