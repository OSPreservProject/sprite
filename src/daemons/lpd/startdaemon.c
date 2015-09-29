/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)startdaemon.c	5.2 (Berkeley) 5/5/88";
#endif /* not lint */

/*
 * Tell the printer daemon that there are new files in the spool directory.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifndef sprite
#include <sys/un.h>
#else
#include <sys/file.h>
#endif
#include "lp.local.h"

static perr();

startdaemon(printer)
	char *printer;
{
#ifndef sprite
        struct sockaddr_un sun;
#endif
	register int s, n;
	char buf[BUFSIZ];

#ifdef sprite
	gethostname(buf, sizeof(buf));
	sprintf(pdevName, "/hosts/%s/dev/printer", buf);
	s = open (pdevName, O_WRONLY, 0);
	if (s < 0) {
	    perror(pdevName);
	    /*
	     * the daemon may be dead, so we attempt to start up a new one
	     */
	     switch (fork()) {

	     case 0:
	        fprintf(stderr, "attempting to restart lpd\n");
	        execl("/sprite/daemons.$MACHINE/lpd", "lpd", 0);
		perror("exec failed");
		return 0;

	     case -1:
	        perror("cannot fork");
		return 0;

	     default:
	        /* loop while we wait for the daemon to start up */
	        for (n = 60; --n >= 0;) {
		    sleep(1);
		    if ((s = open (pdevName, O_WRONLY, 0)) >= 0)
			break;
		}
		if (s < 0)
		    return 0;
		fprintf(stderr, "lpd restarted\n");
		break;
	     }
	}

	(void) sprintf(buf, "\1%s\n", printer);
	n = strlen(buf);
	if (write(s, buf, n) != n) {
		perr("write");
		(void) close(s);
		return(0);
	}
	(void) close(s);
	return(1);
#else
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perr("socket");
		return(0);
	}
	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, SOCKETNAME);
	if (connect(s, &sun, strlen(sun.sun_path) + 2) < 0) {
		perr("connect");
		(void) close(s);
		return(0);
	}
	(void) sprintf(buf, "\1%s\n", printer);
	n = strlen(buf);
	if (write(s, buf, n) != n) {
		perr("write");
		(void) close(s);
		return(0);
	}
	if (read(s, buf, 1) == 1) {
		if (buf[0] == '\0') {		/* everything is OK */
			(void) close(s);
			return(1);
		}
		putchar(buf[0]);
	}
	while ((n = read(s, buf, sizeof(buf))) > 0)
		fwrite(buf, 1, n, stdout);
	(void) close(s);
	return(0);
#endif
}

static
perr(msg)
	char *msg;
{
	extern char *name;
	extern int sys_nerr;
	extern char *sys_errlist[];
	extern int errno;

	printf("%s: %s: ", name, msg);
	fputs(errno < sys_nerr ? sys_errlist[errno] : "Unknown error" , stdout);
	putchar('\n');
}

