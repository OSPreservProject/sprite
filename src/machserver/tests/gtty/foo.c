/* 
 * Short test program to verify that cross-host ioctl's work on the 
 * console. 
 */

#include <stdio.h>
#include <sys/file.h>
#include <sgtty.h>

main(argc, argv)
    int argc;
    char *argv[];
{
    int dev;			/* device to get attributes for */
    struct sgttyb info;		/* attributes */

    if (argc == 1) {
	dev = fileno(stdout);
    } else {
	dev = open(argv[1], O_RDONLY, 0);
	if (dev < 0) {
	    perror(argv[1]);
	    exit(1);
	}
    }

    if (gtty(dev, &info) < 0) {
	perror("can't get device attributes");
	exit(1);
    }

    printf("input speed: %d, erase: 0x%x, kill: 0x%x\n",
	   info.sg_ispeed, info.sg_erase, info.sg_kill);
    printf("flags: 0%o\n", info.sg_flags);
}
