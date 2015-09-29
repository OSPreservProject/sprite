/* rdate --- get the date from the date server on a specified host */
/*           and reset the local system date/time */

/* Copyright (c) 1990, W. Keith Pyle, Austin, Texas */

#include "rdate.h"

extern int optind;
extern int opterr;

void change_time();

/* ------------------------------------------------------------------------- */

main(argc, argv)

int argc;
char *argv[];

{
	int adjust;
	int argno;
	int display_all;
	int display_only;
	int flag;
	int socket_fd;
	int success;

	unsigned long tod;

	adjust = FALSE;
	display_all = FALSE;
	display_only = FALSE;
	opterr = 0;
	success = FALSE;

	while ((flag = getopt(argc, argv, "adD")) != EOF) {

		switch (flag) {

			case 'a':

#ifdef HAS_ADJTIME
				adjust = TRUE;
#else
				(void)fprintf(stderr, "-a not supported on this system\n");
				exit(1);
#endif
				break;
			
			case 'd':

				display_only = TRUE;
				break;
			
			case 'D':

				display_only = TRUE;
				display_all = TRUE;
				break;
			
			default:

				(void)fprintf(stderr, "%s: invalid argument: %c\n", argv[0],
					flag);
				exit(1);
		}
	}

	/* Was a host specified? */

	if ((argc - optind) < 1) {

		(void)fprintf(stderr, "usage: %s [-ad] host [host] ...\n", argv[0]);
		exit(2);
	}

	/* Try the hosts in order until one of them responds */

	for (argno = optind ; argno < argc ; argno++) {

		/* Open the timed port on the host specified by the argument */

		if ((socket_fd = open_port(argv[argno], TIME_PORT)) < 0)
			continue;

		/* Get the time value */

		if (read_socket(socket_fd, (char *)&tod, sizeof(int)) == sizeof(int)) {

			success = TRUE;

			/* Convert tod to host byte order and correct it to Unix time */
			/* (timed returns seconds since 00:00:00 1 January 1900 GMT) */

			tod = ntohl(tod);
			tod -= 2208988800;

			if (!display_only)
				change_time(tod, adjust);
			
			/* Display the value and where we got it */

			(void)printf("%s%s: %s",
				display_only ? "" : (adjust ? "adjusted to " : "set to "),
				argv[argno], ctime((time_t *)&tod));

			if (!display_all)
				break;
		}
		
		(void)close(socket_fd);
	}

	/* Was an attempt successful? */

	if (!success) {

		(void)fprintf(stderr, "couldn't get time from any listed host\n");
		exit(1);
	}

	/* We're done */

	(void)close(socket_fd);
	exit(0);
	/* NOTREACHED */
}

/* ------------------------------------------------------------------------- */

void
change_time(tod, adjust)

unsigned long tod;
int adjust;

{
#ifndef SYSV
	struct timeval timeval;

	/* Put it in the timeval structure for settimeofday */

	timeval.tv_sec = tod;
	timeval.tv_usec = 0;
#endif

	/* Are we to adjust the time or just set it? */

	if (adjust) {

#ifdef HAS_ADJTIME
		struct timeval currtime;

		/* Get the local time and determine the adjustment */

		if (gettimeofday(&currtime, (struct timezone *)NULL) < 0) {

			perror("couldn't get local time of day");
			exit(1);
		}

		timeval.tv_sec -= currtime.tv_sec;
		timeval.tv_usec -= currtime.tv_usec;

		/* Adjust it */

		if (adjtime(&timeval, (struct timeval *)NULL) < 0) {

			perror("couldn't adjust time");
			exit(1);
		}
#endif
	}

	/* Set the time of day, but leave the timezone alone */

#ifndef SYSV
	else if (settimeofday(&timeval, (struct timezone *)NULL) < 0) {
#else
	else if (stime(&tod) < 0) {
#endif

		perror("couldn't set time of day");
		exit(1);
	}

	return;
}
