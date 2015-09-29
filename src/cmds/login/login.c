/*
 * login.c --
 *
 *	A program to prompt for a user to login and execute a shell
 *	for the person.  It can either operate in one-shot mode, or
 *	iterate waiting for the person to log out, then repeat the
 *	whole process.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: /sprite/src/cmds/login/RCS/login.c,v 1.24 92/04/22 14:04:00 kupfer Exp $ SPRITE (Berkeley)";
#endif /* lint */

#include <errno.h>
#include <fs.h>
#include <option.h>
#include <pwd.h>
#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysStats.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <time.h>
#include <ulog.h>
#include <host.h>
#include <syslog.h>

/*
 * Library imports:
 */

extern char *getpass(), *crypt(), *ttyname();
extern struct tm *localtime();

/*
 * Information about command-line options:
 */

int	singleLogin =		1;
int	shouldTimeOut =		1;
int	useUserLog =		1;
int     portID =		-1;    
char	*location =		(char *) "";
char	*remoteUser = 		(char *) NULL;
char	*device = 		(char *) NULL;
int	failures;

#define ERROR_LOG	"/sprite/admin/loginFailures"

Option	options[] = {
    OPT_FALSE, "l", (char *) &useUserLog,
	    "Don't log login/logout in user log",
    OPT_INT, "P", (char *) &portID, "Port number to use for user log",
    OPT_FALSE, "r", (char *) &singleLogin,
	    "Prompt for a new login upon exit",
    OPT_FALSE, "t", (char *) &shouldTimeOut,
	    "Don't time out, even for single logins",
    OPT_STRING, "d", (char *) &device,
	    "Device on which to perform login",
    OPT_STRING, "h", (char *) &location,
	    "Location of user to be recorded in user log and to be used for authentication",
    OPT_STRING, "R", (char *) &remoteUser,
	    "Name of user on remote machine, for authentication",
};

/*
 * Array of possible shells to execute. The first entry is reserved for
 * the shell listed for the user in the password file.
 */
char *shells[] = {
    (char *) 0,	  	/* For user's shell */
    "/sprite/cmds.$MACHINE/csh",
    "/sprite/cmds/csh",
    "/local/csh",
    "/boot/cmds/csh",
    "/sprite/cmds/sh",
    (char *) 0
};



/*
 *----------------------------------------------------------------------
 *
 * OpenDevice --
 *
 *	Open the device we were given and set it up to be used for
 *	all standard streams.
 *
 * Results:
 *	Returns 0 if all went well, -1 if an error occurred.
 *
 * Side effects:
 *	Streams 0, 1, and 2 are opened to the device, and the device's
 *	name is placed in the "TTY" variable.  If an error occurs then
 *
 *
 *----------------------------------------------------------------------
 */

int
OpenDevice(device)
    char    *device;		/* Name of device file to open. */
{
    int id;

    /*
     * Make sure that the caller has access rights for the device
     * (we're running set-user-id, so open will succeed).
     */

    if (access(device, R_OK|W_OK) != 0) {
	(void) fprintf(stderr, "Login couldn't open \"%s\": %s\n", device,
		strerror(errno));
	return -1;
    }
    id = open(device, O_RDWR, 0);
    if (id < 0) {
	(void) fprintf(stderr, "Login couldn't open \"%s\" to std files: %s\n",
		device, strerror(errno));
	return -1;
    }
    if ((dup2(id, 0) == -1) || (dup2(id, 1) == -1)
	    || (dup2(id, 2) == -1)) {
	(void) fprintf(stderr, "Login couldn't dup \"%s\" to std files: %s\n",
		device, strerror(errno));
	return -1;
    }
    (void) close(id);
    setenv("TTY", device);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FindShell --
 *
 *	Look around the filesystem for a shell program.
 *
 * Results:
 *	If successful, the return value is the name of a shell to use.
 *	If unsuccessful, NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
FindShell()
{
    register int i;

    for (i = 0; shells[i] != (char *)NULL; i++) {
	if (access(shells[i], X_OK) == 0) {
	    return shells[i];
	}
	(void) fprintf(stderr, "Login couldn't use shell \"%s\": %s\n",
		shells[i], strerror(errno));
    }
    return (char *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DumpFile --
 *	Copy data from a stream to stdout.
 *
 * Results:
 *	Returns 0 if all went well, -1 if an I/O error occurred.
 *
 * Side Effects:
 *	StreamID is closed upon completion.
 *
 *----------------------------------------------------------------------
 */
int
DumpFile(streamID)
    int streamID;		/* ID for an open file. */
{
#define BUFFER_SIZE 1000
    char buffer[BUFFER_SIZE];
    int bytesRead;

    while (1) {
	bytesRead = read(streamID, buffer, BUFFER_SIZE);
	if (bytesRead == 0) {
	    (void) close(streamID);
	    return 0;
	}
	if (bytesRead < 0) {
	    return -1;
	}
	(void) fwrite(buffer, bytesRead, 1, stdout);
	if (ferror(stdout)) {
	    return -1;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FindDualFile --
 *
 *	Look for a file in /etc and /hosts/`hostname`. If it is there,
 *	dump its contents to stdout.  
 *
 * Results:
 *	0 if neither file is found, 1 if found.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
FindDualFile(file, hostName, msg)
    char *file;			/* Name of file to look for. */
    char *hostName;		/* Name of host to use for a host-specific
				 * message file.  NULL means don't look for
				 * a host-specific message file. */
    char *msg;			/* message to print before dumping file */
{
    int		id;
    char	buf[1024];
    int		foundIt = 0;

    
    (void) sprintf(buf, "/etc/%.900s", file);
    id = open(buf, O_RDONLY, 0);
    if (id >= 0) {
	if (msg) {
	    puts(msg);
	    fflush(stdout);
	}
	if (DumpFile(id) < 0) {
	    (void) fprintf(stderr, "Couldn't print \"%s\": %s\n", buf,
		    strerror(errno));
	}
	foundIt = 1;
    }
    if (hostName != NULL) {
	(void) sprintf(buf, "/hosts/%.50s/%.900s", hostName, file);
	id = open(buf, O_RDONLY, 0);
	if (id >= 0) {
	    if (msg && !foundIt) {
		puts(msg);
		fflush(stdout);
	    }
	    if (DumpFile(id) < 0) {
		(void) fprintf(stderr, "Couldn't print \"%s\": %s\n",
			buf, strerror(errno));
	    }
	    foundIt = 1;
	}
    }
    return(foundIt);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintMotd --
 *
 *	Print the message-of-the-day to stdout.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
PrintMotd(hostName)
    char *hostName;		/* Name of host to use for a host-specific
				 * message file.  NULL means don't look for
				 * a host-specific message file. */
{
    int		id;
    char	version[128];

    if (Sys_Stats(SYS_GET_VERSION_STRING, sizeof(version), version) == 0) {
	(void) printf("Sprite %s\n", version);
    } else {
	(void) printf("Sprite\n");
    }
    (void) FindDualFile("motd", hostName, (char *) NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * AlarmHandler --
 *
 *	Routine to service a SIGALRM signal.  An alarm means the login
 *	has timed out, so login will exit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process exits.
 *
 *----------------------------------------------------------------------
 */
static int
AlarmHandler()
{
    fprintf(stderr, "Login timed out after 60 seconds.\n");
    exit(1);
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for "login".
 *
 * Results:
 *	See the man page.
 *
 * Side effects:
 *	See the man page.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int  argc;
    char *argv[];
{
#define HOST_NAME_SIZE 256
#define NAME_SIZE 80
    static struct sigvec	siginfo = {SIG_IGN, 0, 0};
    char			*argArray[2];
    char			hostName[HOST_NAME_SIZE];
    int				pid, myPid, child;
    struct passwd		*pwdPtr;
    char			name[NAME_SIZE];
    char			*password, *shell;
    struct stat			stdinStat, mailStat;
    union wait			status;
    int				oldGroup;
    Ulog_Data   		*dataPtr;
    int				needPass;
    char			*userName;
    struct itimerval		itimer;
    int				amRoot;
    int				cnt;
    char			tbuf[MAXPATHLEN + 2];
    struct passwd		*oldPwdPtr;

    tbuf[0] = '\0';



    /*
     * Parse arguments.
     */

    argc = Opt_Parse(argc, argv, options, Opt_Number(options),
		     OPT_ALLOW_CLUSTERING);
    if (argc > 2) {
	(void) fprintf(stderr, "Usage: %s [options] [userName]\n", argv[0]);
	exit(1);
    }
    if (getuid() == 0) {
	amRoot = 1;
    } else {
	amRoot = 0;
	singleLogin = 0;
	if ((device != (char *) NULL) && (access(device, R_OK|W_OK) != 0)) {
	    (void) fprintf(stderr, "%s can't access \"%s\": %s\n",
		    argv[0], device, strerror(errno));
	    exit(1);
	}
	if (portID != -1 || remoteUser != (char *) NULL ||
	    useUserLog != 1 || *location != 0) {
	    char *tty;
	    
	    (void) fprintf(stderr, "%s: must be root to override defaults.  (uid = %d, portID = %d, remoteUser = %s, useUserLog = %d, location = %s)\n",
			   argv[0], getuid(), portID, remoteUser, useUserLog,
			   location);
	    exit(1);
	}
    }
    if (!singleLogin) {
	shouldTimeOut = 0;
    }

    if (argc > 1) {
	userName = argv[1];
    } else {
	userName = NULL;
    }
    /*
     * Ignore signals.
     */

    (void) sigvec(SIGHUP, &siginfo, (struct sigvec *) NULL);
    (void) sigvec(SIGINT, &siginfo, (struct sigvec *) NULL);
    (void) sigvec(SIGQUIT, &siginfo, (struct sigvec *) NULL);
    (void) sigvec(SIGTERM, &siginfo, (struct sigvec *) NULL);
    (void) sigvec(SIGTSTP, &siginfo, (struct sigvec *) NULL);

    if (gethostname(hostName, HOST_NAME_SIZE) != 0) {
	(void) fprintf(stderr, "%s couldn't get hostname: %s\n",
		argv[0], strerror(errno));
	exit(1);
    }

    
    /*
     * If a device has been specified, open the device as our
     * standard descriptors.  If
     * it's the console, set the portID if it hasn't been set already.
     */

    if (device != (char *) NULL) {
	if (OpenDevice(device) != 0) {
	    exit(1);
	}
	if (portID == -1 && strcmp(device, "/dev/console") == 0) {
	    portID = ULOG_LOC_CONSOLE;
	}
    }

    /*
     * If we're supposed to do multiple logins, then we're being
     * invoked as part of a boot sequence.  Fork off and let a child
     * do the work, so whoever called us can keep going.
     */

    if (!singleLogin) {
	pid = fork();
	if (pid == -1) {
	    (void) fprintf(stderr, "%s couldn't fork: %s\n",
		    argv[0], strerror(errno));
	    exit(1);
	} else if (pid > 0) {
	    exit(0);
	}
    }
    
    argArray[0] = "-csh";
    argArray[1] = 0;

    /*
     * Start a new process group and make it the controlling one for
     * the device.  Get the device attributes for later use.
     */

    myPid = getpid();
    if (setpgrp(myPid, myPid) == -1) {
	(void) fprintf(stderr, "%s couldn't set  its process group: %s\n",
		argv[0], strerror(errno));
	exit(1);
    }
    if (ioctl(0, TIOCGPGRP, (char *) &oldGroup) == -1) {
	(void) fprintf(stderr, "%s couldn't get process group for terminal: %s\n",
		argv[0], strerror(errno));
	oldGroup = -1;
    }
    if (fstat(0, &stdinStat) != 0) {
	(void) fprintf(stderr, "%s couldn't stat stdin: %s\n", argv[0],
		strerror(errno));
	exit(1);
    }

    while (1) {
	if (ioctl(0, TIOCSPGRP, (char *) &myPid) == -1) {
	    (void) fprintf(stderr,
			   "%s couldn't set process group for terminal: %s\n",
			   argv[0], strerror(errno));
	}
	(void) ioctl(0, TIOCFLUSH, (char *) 0);
	if (userName == (char *) NULL) {
	    (void) printf("\nWelcome to Sprite (%s)\n\n", hostName);
	}

	if (shouldTimeOut) {
	    timerclear(&itimer.it_interval);
	    itimer.it_value.tv_sec = 60;
	    itimer.it_value.tv_usec = 0;
	    (void) signal(SIGALRM, AlarmHandler);
	    if (setitimer(ITIMER_REAL, &itimer,
			  (struct itimerval *) NULL) < 0) {
		syslog(LOG_ERR,
		       "%s: unable to set interval timer.\n", argv[0]);
	    }
	}
		
		
	/*
	 * If we weren't given a user to login as, prompt for one from
	 * the device. If the name we get isn't a valid login, print
	 * a message and continue prompting. The final name is left
	 * in 'userName'
	 */
	cnt = 0;
	failures = 0;
	do {
	    pwdPtr = (struct passwd *) NULL;
	    if (userName == (char *) NULL) {
		char *p;

		(void) printf("Login: ");
		if (fgets(name, NAME_SIZE, stdin) == NULL || name[0] == '\n') {
		    if (feof(stdin)) {
			badlogin(tbuf, hostName);
		    }
		    if (feof(stdin) && singleLogin) {
			exit(0);
		    }
		    clearerr(stdin);
		    continue;
		}
		userName = name;
		for (p = name; *p != 0; p++) {
		    if (*p == '\n') {
			*p = 0;
			break;
		    }
		}
	    }

	    pwdPtr = getpwnam(userName);

	    needPass = 1;
	    if ((*location != '\0') &&
		amRoot &&
		(pwdPtr != (struct passwd *)NULL)) {
		if (ruserok(location, pwdPtr->pw_uid == 0, remoteUser,
			    userName) == 0) {
		    needPass = 0;
		}
	    }
		
	    if (needPass && ((pwdPtr == (struct passwd *)NULL) ||
			     ((pwdPtr->pw_passwd != (char *)NULL) &&
			      (*pwdPtr->pw_passwd != '\0')))) {
		password = getpass("Password: ");
	    }

	    /*
	     * Note if trying multiple user names;
	     * log failures for previous user name,
	     * but don't bother logging one failure
	     * for nonexistent name (mistyped username).
	     */
	    if (failures && strcmp(tbuf, userName)) {
		    if (failures > (oldPwdPtr ? 0 : 1)) {
			    badlogin(tbuf, hostName);
		    }
		    failures = 0;
	    }
	    (void)strcpy(tbuf, userName);
	    oldPwdPtr = pwdPtr;

	    if (pwdPtr == (struct passwd *) NULL || (needPass &&
			pwdPtr->pw_passwd && (*pwdPtr->pw_passwd != '\0'))) {
		char *encrypted;

		if (pwdPtr != (struct passwd *) NULL) {
		    encrypted = crypt(password, pwdPtr->pw_passwd);
		    if (strcmp(pwdPtr->pw_passwd, encrypted) != 0) {
			userName = (char *) NULL;
			pwdPtr = (struct passwd *) NULL;
		    }
		}
		if (pwdPtr == (struct passwd *) NULL) {
		    (void) printf("Login incorrect.\n");
		    failures++;
		    notelogin(tbuf, hostName);
		    userName = (char *) NULL;
		    /* we allow 10 tries, but after 3 we start backing off */
		    if (++cnt > 3) {
			    if (cnt >= 10) {
				    badlogin(userName, hostName);
				    (void)ioctl(0, TIOCHPCL, (struct sgttyb *)NULL);
				    sleepexit(1);
			    }
			    sleep((u_int)((cnt - 3) * 5));
		    }
			
		}
	    }
	} while (pwdPtr == (struct passwd *) NULL);
	notelogin(tbuf, hostName);

	if (shouldTimeOut) {
	    timerclear(&itimer.it_value);
	    if (setitimer(ITIMER_REAL, &itimer,
			  (struct itimerval *) NULL) < 0) {
		syslog(LOG_ERR,
		       "%s: unable to clear interval timer.\n", argv[0]);
	    }
	    (void) signal(SIGALRM, SIG_DFL);
	}

	/*
	 * If not root, check for logins being disabled on this
	 * host or on the system in general.
	 */
	if (pwdPtr->pw_uid != 0) {
	    if (FindDualFile("nologin", hostName,
			     "Logins are currently disabled:")) {
		if (singleLogin) {
		    exit(0);
		}
		userName = (char *) NULL;
		continue;
	    }
	}
	    
	dataPtr = Ulog_LastLogin(pwdPtr->pw_uid);
	if (dataPtr != (Ulog_Data *) NULL) {
	    if (dataPtr->updated != 0) {
		char *timeStr;
		char *asctime();
		char lastHost[HOST_NAME_SIZE];
		Host_Entry *hostPtr;

		timeStr = asctime(localtime((time_t *) &dataPtr->updated));
		hostPtr = Host_ByID(dataPtr->hostID);
		if (hostPtr == (Host_Entry *) NULL) {
		    syslog(LOG_ERR,
			   "%s: unable to obtain host information for host %d.\n",
			  argv[0], dataPtr->hostID);
		} else {
		    (void) strncpy(lastHost, hostPtr->name, HOST_NAME_SIZE);
		    (void) printf("Last login %.19s on %s\n", timeStr, lastHost);
		    (void) fflush(stdout);
		}
	    }
	}
	    
	if (useUserLog) {
	    if (portID == -1) {
		(void) fprintf(stderr,
			"Warning: can't determine which port to record login into.\n");
	    } else {
		/*
		 * Record the login in the user database.  EINVAL means the
		 * portID exceeds the maximum, and a syslog entry will be
		 * generated by the ULog routine.
		 */
		if (Ulog_RecordLogin(pwdPtr->pw_uid, location, portID) != 0) {
		    (void) fprintf(stderr, "%s couldn't record login: %s\n",
			    argv[0], strerror(errno));
		}
	    }
	}

	pid = fork();
	if (pid == -1) {
	    (void) fprintf(stderr, "%s couldn't fork shell: %s\n",
		    argv[0], strerror(errno));
	    exit(1);
	}
	
	if (pid == 0) {
	    char mailFile[MAXPATHLEN];

	    /*
	     * Set up a bit of the shell's environment.  The rest of it
	     * is set up by the shell (like PATH).
	     */

	    setenv("USER", userName);
	    setenv("HOME", pwdPtr->pw_dir);

	    /*
	     * Set our state the way the user will want it:
	     *	  change tty ownership to this process (unless this is
	     * 		a generic device that could potentially be shared
	     *		by many users)
	     *	  change to the user's home directory
	     *	  initialize the list of groups
	     *	  set both uids to the user's uid.
	     */

	    if (stdinStat.st_devServerID != FS_LOCALHOST_ID) {
		if (fchown(0, pwdPtr->pw_uid, pwdPtr->pw_gid) != 0) {
		    (void) fprintf(stderr, "%s couldn't set stdin owner: %s\n",
			    argv[0], strerror(errno));
		}
		if (fchmod(0, 0644) != 0) {
		    (void) fprintf(stderr,
			    "%s couldn't set stdin permissions: %s\n",
			    argv[0], strerror(errno));
		}
	    }
	    if (chdir(pwdPtr->pw_dir) != 0) {
		(void) fprintf(stderr, "%s couldn't cd to \"%s\": %s\n",
			argv[0], pwdPtr->pw_dir, strerror(errno));
	    }
	    if (initgroups(userName, pwdPtr->pw_gid) == -1) {
		(void) fprintf(stderr, "%s couldn't set group ids: %s\n",
			argv[0], strerror(errno));
	    }
	    if (setreuid(pwdPtr->pw_uid, pwdPtr->pw_uid) != 0) {
		(void) fprintf(stderr, "%s couldn't set user ids: %s\n",
			argv[0], strerror(errno));
	    }

	    PrintMotd(hostName);
	    (void) sprintf(mailFile, "/sprite/spool/mail/%s", pwdPtr->pw_name);
	    if ((stat(mailFile, &mailStat) == 0) && (mailStat.st_size != 0)) {
		(void) fprintf(stderr, "You have %smail.\n",
			(mailStat.st_mtime > mailStat.st_atime) ? "new " : "");
	    }

	    /*
	     * Look for a shell to execute. Stuff the user's shell into
	     * the shells array first so FindShell will check it first,
	     * then call FindShell to locate a shell to execute.
	     */
	    if (pwdPtr->pw_shell != (char *) NULL) {
		shells[0] = pwdPtr->pw_shell;
		setenv("SHELL", pwdPtr->pw_shell);
	    } else {
		shells[0] = "";
	    }
	    shell = FindShell();
	    if (shell == NULL) {
		(void) fprintf(stderr, "%s couldn't find shell to execute.\n",
			argv[0]);
		exit(1);
	    }
	    (void) execv(shell, argArray);
	    (void) fprintf(stderr, "%s couldn't exec \"%s\": %s\n",
		    argv[0], shell, strerror(errno));
	    exit(1);
	}

	/*
	 * Wait for the child to exit.
	 */

	while (1) {
	    child = wait(&status);
	    if (child != -1) {
		if ((status.w_termsig != 0) || (status.w_retcode != 0)) {
		    (void) fprintf(stderr, "Shell 0x%x died: termsig=%d, status=%d\n",
			    child, status.w_termsig, status.w_retcode);
		}
		break;
	    }
	    if (errno != EINTR) {
		(void) fprintf(stderr, "%s couldn't wait for shell to terminate: %s\n",
			argv[0], strerror(errno));
		break;
	    }
	}

	/*
	 * Reset terminal characteristics and remove the user from the
	 * login database.
	 */

	if (stdinStat.st_serverID != FS_LOCALHOST_ID) {
	    if (fchown(0, (int) stdinStat.st_uid,
		       (int) stdinStat.st_gid) != 0) {
		(void) fprintf(stderr, "%s couldn't reset stdin owner: %s\n",
			argv[0], strerror(errno));
	    }
	    if (fchmod(0, stdinStat.st_mode) != 0) {
		(void) fprintf(stderr,
			"%s couldn't reset stdin permissions: %s\n",
			argv[0], strerror(errno));
	    }
	}
	if (useUserLog && portID != -1 && portID < ULOG_MAX_PORTS) {
	    if (Ulog_RecordLogout(pwdPtr->pw_uid, portID) != 0) {
		(void) fprintf(stderr, "%s couldn't record logout: %s\n",
			argv[0], strerror(errno));
	    }
	}
	if (singleLogin) {
	    if ((oldGroup != -1)
		    && (ioctl(0, TIOCSPGRP, (char *) &oldGroup) == -1)) {
		(void) fprintf(stderr,
			"%s couldn't reset process group for terminal: %s\n",
			argv[0], strerror(errno));
	    }
	    exit(0);
	}
	userName = (char *) NULL;
	name[0] = 0;
    }
}

badlogin(name, hostName)
        char *name;
	char *hostName;
{
	int pid, w, status;
	char *remoteUserPtr;
	char buffer[100];
	time_t now;
	char *t;
	FILE *logFile;
	char *tty, *ttyn;
        if (failures == 0)
                return;
	time(&now);
	t = asctime(localtime(&now));
	t[24] = '\0';	/* Remove the newline. */
	ttyn = ttyname(0);
	if (ttyn == NULL || *ttyn == '\0') {
	    ttyn = "/dev/tty??";
	}
	if (tty = rindex(ttyn,'/')) {
	    ++tty;
	} else {
	    tty = ttyn;
	}
	if (hostName == NULL) {
	    hostName = "";
	}
	logFile = fopen(ERROR_LOG, "a");
	if (logFile) {
	    if (chmod(ERROR_LOG,0600)) {
		perror("Chmod failed\n");
		logFile = (FILE *)NULL;
	    }
	}

        if (location && *location) {
		remoteUserPtr = remoteUser ? remoteUser : "?anonymous?";
                syslog(LOG_NOTICE, "%d LOGIN FAILURE%s FROM %s@%s to %s@%s",
                    failures, failures > 1 ? "S" : "", remoteUser,location,
		    name, hostName, t);
		if (logFile) {
		    fprintf(logFile,"%d LOGIN FAILURE%s FROM %s@%s to %s@%s at %s\n",
			    failures, failures > 1 ? "S" : "", remoteUser,
			    location, name, hostName, t);
		    fflush(logFile);
		    /*
		     * Do a finger at the evil host.
		     */
		    if (!(pid=vfork())) {
			close(1);
			dup(fileno(logFile));
			sprintf(buffer,"@%.90s",location);
			execl("/sprite/cmds/finger","finger",buffer,NULL);
			exit(-1);
		    }
		    while ((w=wait(&status))!=pid && w != -1){}
		}
        } else if (strcmp(tty,"console")) {
		/*
		 * If logging in from console, why print message?
		 */
                syslog(LOG_NOTICE, "%d LOGIN FAILURE%s ON %s to %s@%s",
                    failures, failures > 1 ? "S" : "", tty, name, hostName);
		if (logFile) {
		    fprintf(logFile, "%d LOGIN FAILURE%s ON %s to %s@%s at %s\n",
			failures, failures > 1 ? "S" : "", tty, name, hostName,
			t);
		}
	}
	failures = 0;
	if (logFile) {
	    fclose(logFile);
	}
}

notelogin(name, hostName)
        char *name;
	char *hostName;
{
	int pid, w, status;
	char *remoteUserPtr;
	char buffer[100];
	time_t now;
	char *t;
	FILE *logFile;
	char *tty, *ttyn;
	time(&now);
	t = asctime(localtime(&now));
	t[24] = '\0';	/* Remove the newline. */
	ttyn = ttyname(0);
	if (ttyn == NULL || *ttyn == '\0') {
	    ttyn = "/dev/tty??";
	}
	if (tty = rindex(ttyn,'/')) {
	    ++tty;
	} else {
	    tty = ttyn;
	}
	if (hostName == NULL) {
	    hostName = "";
	}
	logFile = fopen(ERROR_LOG, "a");
	if (logFile) {
	    if (chmod(ERROR_LOG,0600)) {
		perror("Chmod failed\n");
		logFile = (FILE *)NULL;
	    }
	}

        if (location && *location) {
		remoteUserPtr = remoteUser ? remoteUser : "?anonymous?";
		if (logFile) {
		    fprintf(logFile,"%d LOGIN FROM %s@%s to %s@%s at %s\n",
			    failures, remoteUser,
			    location, name, hostName, t);
		    fflush(logFile);
		}
        } else {
		if (logFile) {
		    fprintf(logFile, "%d LOGIN ON %s to %s@%s at %s\n",
			failures, tty, name, hostName,
			t);
		}
	}
	failures = 0;
	if (logFile) {
	    fclose(logFile);
	}
}

sleepexit(eval)
        int eval;
{
        sleep((u_int)5);
	/*
        exit(eval);
	*/
}
