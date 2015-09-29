#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getpass.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint
/* Taken from 4.3 BSD;  cannot be redistributed except to people with
 * proper AT&T source licenses. */

#include <stdio.h>
#include <signal.h>
#include <sgtty.h>

char *
getpass(prompt)
char *prompt;
{
	struct sgttyb ttyb;
	int flags;
	register char *p;
	register c;
	FILE *fi;
	static char pbuf[9];
	void (*sig)();
	extern char *getenv();

	p = getenv("TTY");
	if ((p != NULL) && ((fi = fopen(p, "r")) != NULL))
		setbuf(fi, (char *)NULL);
	else
		fi = stdin;
	sig = signal(SIGINT, SIG_IGN);
	ioctl(fileno(fi), TIOCGETP, (char *) &ttyb);
	flags = ttyb.sg_flags;
	ttyb.sg_flags &= ~ECHO;
	ioctl(fileno(fi), TIOCSETP, (char *) &ttyb);
	fprintf(stderr, "%s", prompt); fflush(stderr);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[8])
			*p++ = c;
	}
	*p = '\0';
	fprintf(stderr, "\n"); fflush(stderr);
	ttyb.sg_flags = flags;
	ioctl(fileno(fi), TIOCSETP, (char *) &ttyb);
	signal(SIGINT, sig);
	if (fi != stdin)
		fclose(fi);
	return(pbuf);
}
