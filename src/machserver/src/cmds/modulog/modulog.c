/*
 * modulog --
 *
 *	This program is invoked at boot time to invalidate the user log
 *	entry for the current host.  It can also be used to change the
 *	user log manually in other ways; see the man page.
 */

#include <stdio.h>
#include <host.h>
#include <ulog.h>
#include <db.h>
#include <syslog.h>
#include <option.h>
#include <sys/time.h>
#include <pwd.h>
#include <string.h>

extern int errno;
extern char *sys_errlist[];

int port = 0;
char *user = NULL;
char *location = NULL;
int clearAll = 0;
int invalidate = 0;

Option optionArray[] = {
	{OPT_INT, "p", (char *)&port,
	     "Port to modify (default is console)."},
	{OPT_STRING, "u", (char *)&user,
	     "User to \"log in\" (default is current user)."},
	{OPT_TRUE, "i", (char *)&invalidate,
		 "Invalidate the  log entry for the specified port."},
	{OPT_STRING, "l", (char *)&location,
		 "Create a log entry for the specified port and user at this location."},
	{OPT_TRUE, "C", (char *)&clearAll,
		 "Clear all entries for current host."},
};
static int numOptions = sizeof(optionArray) / sizeof(Option);

#define BUFSIZE 256

main(argc, argv)
    int argc;
    char **argv;
{
    int		 	status;
    int 		i;
    Host_Entry		*entryPtr;
    struct passwd	*pwdPtr;
    char		hostname[BUFSIZE];
    int			userID;	

    argc = Opt_Parse(argc, argv, optionArray, numOptions,
		       OPT_ALLOW_CLUSTERING);


    if (port < 0 || port > ULOG_MAX_PORTS) {
	(void) fprintf(stderr, "Invalid port: %d.\n", port);
	exit(1);
    }

    if (gethostname(hostname, sizeof(hostname)) < 0) {
	syslog(LOG_ERR, "error getting hostname.\n");
	perror("gethostname");
	exit(1);
    }
    entryPtr = Host_ByName(hostname);
    if (entryPtr == (Host_Entry *) NULL) {
	syslog(LOG_ERR, "Error getting host information for '%s'.\n",
	       hostname);
	exit(1);
    }

    if (user != NULL) {
	pwdPtr = getpwnam(user);
	if (pwdPtr == (struct passwd *) NULL) {
	    (void) fprintf(stderr, "Invalid user: %s.\n", user);
	    exit(1);
	}
	userID = pwdPtr->pw_uid;
    } else {
	userID = getuid();
    }

    
    if (invalidate) {
	if (Ulog_RecordLogout(0, port) < 0) {
	    perror("Error in Ulog_RecordLogout");
	}
    } else if (location) {
	if (Ulog_RecordLogin(userID, location, port) < 0) {
	    perror("Error in Ulog_RecordLogin");
	}
    } else {
	if (Ulog_ClearLogins() < 0) {
	    perror("Error in Ulog_ClearLogins");
	}
    }
    exit(0);
}






