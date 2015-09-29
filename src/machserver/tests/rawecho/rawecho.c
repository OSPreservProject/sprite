#include <sgtty.h>
#include <stdio.h>
#include <sys/file.h>

struct sgttyb ttyInfo;

void PutOne();

main()
{
    int tty;
    char buf[10];
    int charsRead;
    char *bufPtr;

    tty = open("/dev/console", O_RDONLY, 0666);
    if (tty < 0) {
	perror("can't open tty");
	exit(1);
    }
    if (gtty(tty, &ttyInfo) < 0) {
	perror("can't get tty info");
	exit(1);
    }
    ttyInfo.sg_flags |= RAW;
    if (stty(tty, &ttyInfo) < 0) {
	perror("can't set raw mode");
	exit(1);
    }

    while ((charsRead = read(tty, buf, sizeof(buf))) >= 0) {
	if (charsRead == 0) {
	    fprintf(stderr, ".");
	    sleep(1);
	    continue;
	}
	for (bufPtr = buf; bufPtr < buf+charsRead; bufPtr++) {
	    PutOne(*bufPtr);
	}
	if (buf[0] == '?') {
	    break;
	}
    }

    ttyInfo.sg_flags &= ~RAW;
    if (stty(tty, &ttyInfo) < 0) {
	perror("can't reset tty");
    }

    exit(0);
}

void
PutOne(ch)
    int ch;
{
    fprintf(stderr, " 0%o ", ch);
}
