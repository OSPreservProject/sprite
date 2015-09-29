/* 
 * adduser.c --
 *
 *	Add a user to /etc/passwd, create a home directory,
 *      create a .project file, add them to the sprite-users
 *      mailing list, and send them mail to inform them that
 *      their account is ready.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/adduser/RCS/adduser.c,v 1.13 91/12/16 12:12:40 kupfer Exp $";
#endif /* not lint */

#include "common.h"
#include <sprite.h>
#include <assert.h>
#include <bstring.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* This is where we keep a prototype home directory. */
#define NEWUSER_DIR	"/user1/newuser"

/* This is the host that has the master UID database. */
#define DATABASE_HOST	"thalm"

#if 0
/* XXX MAIL_TMP doesn't allow for concurrent addusers */
#define MAIL_TMP	"/tmp/mail.adduser"

#define MAIL_MSG	mail_message

static CONST char mail_message[] = "\
Your Sprite account is ready.\n\
If you have any problems or questions\n\
let me know.\n\n\
Your passwd is < enter passwd >\n";
#endif /* 0 */

static char whoami[0x100];

/* Forward references: */
static void add2MailingList _ARGS_((CONST char *list, CONST char *name));
static void cleanup _ARGS_((int sig));
static int  copy _ARGS_((CONST char *from, CONST char *to, int mode));
static void createHomeDirectory _ARGS_((CONST char *dir, CONST char *username,
	       CONST char *project, CONST char *forward, int uid, int gid));
static char *getHomeDir _ARGS_((void));
static char *getPasswdEntryFromDataBase _ARGS_((void));
static char *getPasswdEntryInteractive _ARGS_((CONST struct passwd *pwd));
static char *getPasswdEntryFromUser _ARGS_((void));
static void insertPasswdEntry _ARGS_((CONST struct passwd *newpw));
static void makeLinktoHomeDir _ARGS_((CONST char *homedir,
				      CONST char *username));
static int  parsePasswdEntry _ARGS_((char *p, struct passwd *ps));
static void printPasswdEntry _ARGS_((CONST struct passwd *pwd,
				     CONST char *dir));
static int  sed _ARGS_((CONST char *filename, CONST char *expr, int mode));
static char *xmalloc _ARGS_((int n));

/*
 * Flags to keep track of what the program has done, so we can back out
 * semi-gracefully if the program is interrupted.
 */
enum Step { NOT_DONE_YET, DONE, SKIP_THIS_STEP };
static enum Step create_ptmp_file      = NOT_DONE_YET;
static enum Step move_ptmp_to_passwd   = NOT_DONE_YET;
static enum Step create_home_dir       = NOT_DONE_YET;
static enum Step edit_mailaliases_file = NOT_DONE_YET;
static enum Step make_symlink          = NOT_DONE_YET;
static char real_homedir[MAXPATHLEN];
static char sym_homedir[MAXPATHLEN];
static char project[BUFFER_SIZE];
static char forward[BUFFER_SIZE];


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Get account information from the user, or from the ucb database.
 *      Use the information to create an account.
 *
 * Results:
 *	0 exit status if no errors.  Non-zero if there were
 *      problems.
 *
 * Side effects:
 *	Account is created.  If the program is interrupted, it is
 *      supposed to be able to back out with out changing anything.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int argc;
    char **argv;
{
    struct passwd passwdStruct;
    int c;
    char *entry;
    char *dir;
    char *username;
    struct passwd *pwd;
    char from[MAXPATHLEN], to[MAXPATHLEN], *tend, *fend;
    int fd;

    if ((pwd = getpwuid(getuid())) == NULL) {
	fprintf(stderr, "getpwuid failed: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    strcpy(whoami, pwd->pw_name);
#ifndef TEST
    /* 
     * Do any necessary security checks and setup.
     */
    SecurityCheck();
#endif
    signal(SIGINT, cleanup);
    printf(
     "Enter 1 if you already have a /etc/passwd file from another machine.\n");
    printf("Enter 2 if you want to fetch an entry from the ucb data base.\n");
    printf("Enter 3 if you want to enter the information interactively.\n");
    printf("Enter q to quit\n");
    for (;;) {
	printf("Please choose 1, 2 or 3: ");
	c = raw_getchar();
	printf("\n");
	switch(c) {
	case '1':  entry = getPasswdEntryFromUser();        break;
	case '2':  entry = getPasswdEntryFromDataBase();    break;
	case '3':  entry = getPasswdEntryInteractive(NULL);    break;
	case 'q':  case 'Q':    exit(EXIT_FAILURE);
	default:   continue;
	}
	break;
    }
    for (;;) {
	while (parsePasswdEntry(entry, &passwdStruct) == 0) {
	    fprintf(stderr, "Entry is mangled, please correct it.\n");
	    c = '1';
	    entry = getPasswdEntryInteractive(&passwdStruct);
	}
	getString("", "Project    ", project);
	for (;;) {
	    getString("", "Forward    ", forward);
	    if (*forward && strchr(forward, '@') == NULL) {
		fprintf(stderr,
		   "Improperly formed address, should be ``user@machine''\n");
		continue;
	    }
	    break;
	}
	dir = getHomeDir();
	printPasswdEntry(&passwdStruct, dir);
	if (strcmp(passwdStruct.pw_passwd,"*")==0) {
	    fprintf(stderr,"WARNING: passwd is '*' - login disabled\n");
	}
	if (yes("Is this correct?")) {
	    break;
	}
	entry = getPasswdEntryInteractive(&passwdStruct);
    }
    sprintf(real_homedir, "%s/%s", dir, passwdStruct.pw_name);
    username = passwdStruct.pw_name;
    insertPasswdEntry(&passwdStruct);
    makeLinktoHomeDir(real_homedir, username);
    if (create_ptmp_file == DONE) {

        if (makedb(PTMP_FILE)) {
	    (void)fprintf(stderr, "adduser: mkpasswd failed.\n");
	    cleanup(0);
        }

        /*
         * possible race; have to rename four files, and someone could slip
         * in between them.  LOCK_EX and rename the ``passwd.dir'' file first
         * so that getpwent(3) can't slip in; the lock should never fail and
         * it's unclear what to do if it does.  Rename ``ptmp'' last so that
         * passwd/vipw/chpass can't slip in.
         */
        fend = strcpy(from, PTMP_FILE) + strlen(PTMP_FILE);
        tend = strcpy(to, PASSWD_FILE) + strlen(PASSWD_FILE);
        bcopy(".dir", fend, 5);
        bcopy(".dir", tend, 5);
        if ((fd = open(from, O_RDONLY, 0)) >= 0) {
	    (void)flock(fd, LOCK_EX);
	}
        /* here we go... */
        (void)rename(from, to);
        bcopy(".pag", fend, 5);
        bcopy(".pag", tend, 5);
        (void)rename(from, to);
        bcopy(".orig", fend, 6);
        (void)rename(PASSWD_FILE, PASSWD_BAK);
        (void)rename(MASTER_PASSWD_FILE, MASTER_BAK);
        (void)rename(from, PASSWD_FILE);
        (void)rename(PTMP_FILE, MASTER_PASSWD_FILE);
        /* done! */

	move_ptmp_to_passwd = DONE;
    }
    createHomeDirectory(real_homedir, username, project, forward,
	passwdStruct.pw_uid, passwdStruct.pw_gid);
    add2MailingList("sprite-users", username);
#if 0    
    if (yes("do you want to send mail?")) {
	sendmsg(username);
    }
#endif    
    printf("Done creating account for %s\n", username);
    exit(EXIT_SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * parseGecos --
 *
 *	Parse the gecos field of a passwd entry.
 *
 * Results:
 *	Returns the next comma-sperated field.
 *
 * Side effects:
 *	Overwrites the commas with null bytes.
 *
 *----------------------------------------------------------------------
 */

static char *
parseGecos(p)
    char *p;
{

    while (*p) {
	if (*p++ == ',') {
	    p[-1] = '\0';
	    return p;
	}
    }
    return p;
}


/*
 *----------------------------------------------------------------------
 *
 * printPasswdEntry --
 *
 *	Pick apart a passwd entry and print it out.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints the passwd information to stdout.
 *
 *----------------------------------------------------------------------
 */
static void
printPasswdEntry(pwd, dir)
    CONST struct passwd *pwd;
    CONST char *dir;
{
    CONST struct group *gwd;
    char *group, *fullname, *office, *phone, *home_phone;
    char *gecos;

    if ((gwd = getgrgid(pwd->pw_gid)) == NULL) {
	group = "Invalid Group";
    } else {
	group = gwd->gr_name;
    }
    gecos = xmalloc(strlen(pwd->pw_gecos));
    strcpy(gecos, pwd->pw_gecos);
    fullname = gecos;
    office = parseGecos(fullname);
    phone = parseGecos(office);
    home_phone = parseGecos(phone);
    printf("\n");
    printf("Login name: %s\n", pwd->pw_name);
    printf("Full name:  %s\n", fullname);
    printf("Passwd:     %s\n", pwd->pw_passwd);
    printf("Uid:        %d\n", pwd->pw_uid);
    printf("Group:      %s\n", group);
    printf("Office:     %s\n", office);
    printf("Phone       %s\n", phone);
    printf("Home Phone: %s\n", home_phone);
    printf("Directory:  %s -> %s/%s\n", pwd->pw_dir, dir, pwd->pw_name);
    printf("Shell:      %s\n", pwd->pw_shell);
    printf("Project:    %s\n", project);
    printf("Forward:    %s\n", forward);
    printf("\n");
    free(gecos);
    return;
}

#if 0

/*
 *----------------------------------------------------------------------
 *
 * sendmail --
 *
 *	Send a mail message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Mail is sent.
 *
 *----------------------------------------------------------------------
 */
static void
sendmsg(username)
    CONST char *username;
{
    char buf[0x100];
    CONST char *editor;
    int tmp;

    if ((tmp = open(MAIL_TMP, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
	fprintf(stderr, "Cannot open %s: %s\n", MAIL_TMP, strerror(errno));
        return;
    }
    if (write(tmp, MAIL_MSG, strlen(MAIL_MSG)) != strlen(MAIL_MSG)) {
	fprintf(stderr, "Error writing %s: %s\n", MAIL_TMP, strerror(errno));
        return;
    }
    close(tmp);
    if ((editor = getenv("EDITOR")) == 0) {
	editor = "vi";
    }
    if (strcmp(editor, "mx") == 0) {
	(void)sprintf(buf, "mx -D %s", MAIL_TMP);
    } else {
	(void)sprintf(buf, "%s %s", editor, MAIL_TMP);
    }
    if (system(buf)) {
	fprintf(stderr, "Not sending mail: %s\n", strerror(errno));
    } else if (yes("Do you want to mail the message?")) {
	sprintf(buf, "mail %s -c %s -s \"Sprite account\" < %s",
	    username, whoami, MAIL_TMP);
	if (system(buf)) {
	    fprintf(stderr, "Problem sending mail: %s\n", strerror(errno));
	}
    }
    unlink(MAIL_TMP);
    return;
}
#endif /* 0 */


/*
 *----------------------------------------------------------------------
 *
 * makeLinktoHomeDir --
 *
 *	Makes a link from /users/username to the real home directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A symbolic link is created.
 *
 *----------------------------------------------------------------------
 */
static void
makeLinktoHomeDir(homedir, username)
    CONST char *homedir;
    CONST char *username;
{

    if (!strncmp(USER_DIR, homedir, strlen(USER_DIR))) {
	return;
    }
    sprintf(sym_homedir, "%s/%s", USER_DIR, username);
    if (symlink(homedir, sym_homedir)) {
	if (errno != EEXIST) {
	    fprintf(stderr, "Cannot create link %s -> %s\n",
		sym_homedir, homedir);
	}
	make_symlink = SKIP_THIS_STEP;
    } else {
	make_symlink = DONE;
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * add2MailingList --
 *
 *	Adds a user to a mailing list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the sendmail aliases file.
 *
 *----------------------------------------------------------------------
 */
static void
add2MailingList(list, name)
    CONST char *list;
    CONST char *name;
{
    char *expr;
    int status;			/* exit status from subprocesses */
    char logMsg[4096];		/* log message for "ci" */

    printf("Adding %s to \"%s\" mailing list\n", name, list);
    sprintf(logMsg, "-madduser: add %s to \"%s\".", name, list);

    status = rcsCheckOut(ALIASES);
    if (status < 0) {
	cleanup(0);
	/* NOTREACHED */
    } else if (status != 0) {
	goto error;
    }

    /* 
     * Edit a copy of the aliases file and then rename it to be the 
     * real one. 
     */
    if (copy(ALIASES, ALIASES_TMP, 0644)) {
	goto error;
    }
    expr = xmalloc(strlen(list) + strlen(name) + 20);
    sprintf(expr, "/^%s:/s/, *$/, %s,/", list, name);
    if (sed(ALIASES_TMP, expr, 0644)) {
	fprintf(stderr, "Cannot change mail alias file\n");
	free(expr);
	goto error;
    }
    free(expr);

    /* XXX - error checking? */
    unlink(ALIASES_BAK);
    rename(ALIASES, ALIASES_BAK);
    rename(ALIASES_TMP, ALIASES);

    /* 
     * Check the aliases file back in.  If there's a problem, say that 
     * we succeeded anyway (because we did edit the aliases).
     */
    status = rcsCheckIn(ALIASES, logMsg);
    if (status == 0) {
	printf("Added %s to %s mailing list.\n", name, list);
    } else {
	fprintf(stderr, "\nPlease check the aliases file.\n");
	fprintf(stderr, "(Make sure that `%s' was added to `%s'\n",
	       name, list);
	fprintf(stderr, "and that the file was checked in.)\n\n");
    }
    edit_mailaliases_file = DONE;
    return;

 error:
    fprintf(stderr, "\nWarning: unable to add `%s' to the list `%s'.\n",
	    name, list);
    fprintf(stderr, "You'll have to edit the aliases file by hand.\n\n");
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * sed --
 *
 *	Filter a file through sed.
 *
 * Results:
 *	Returns the exit status from sed.  0 if successful,
 *      non-zero if there were problems.
 *
 * Side effects:
 *	File is changed, a backup file is created.
 *
 *----------------------------------------------------------------------
 */
static int
sed(filename, expr, mode)
    CONST char *filename;
    CONST char *expr;
    int mode;
{
    char tmp[MAXPATHLEN];
    char old[MAXPATHLEN];
    int child;
    int w;
    int in, out;
    union wait ws;

    sprintf(tmp, "%s.adduser", filename);
    sprintf(old, "%s.old", filename);
    if ((in = open(filename, O_RDONLY)) < 0) {
	fprintf(stderr, "Cannot open %s: %s\n", filename, strerror(errno));
	return 1;
    }
    if ((out = open(tmp, O_WRONLY|O_CREAT|O_EXCL, mode)) < 0) {
	fprintf(stderr, "Cannot open %s: %s\n", tmp, strerror(errno));
        return 1;
    }
    switch (child = fork()) {

    case -1:
	fprintf(stderr, "Fork failed: %s\n", strerror(errno));
	cleanup(0);

    case 0:
	dup2(in, 0);
	dup2(out, 1);
	execlp("sed", "sed", expr, NULL);
	fprintf(stderr, "Cannot exec sed: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

    default:
	while ((w = wait(&ws)) > 0 && w != child) {
	    continue;
	}
	if (ws.w_retcode) {
	    return ws.w_retcode;
	}
	break;
    }
    unlink(old);
    if (rename(filename, old)) {
	fprintf(stderr, "Cannot rename %s as %s: %s\n",
	    filename, old, strerror(errno));
	return 1;
    }
    if (rename(tmp, filename)) {
	fprintf(stderr, "Cannot rename %s as %s: %s\n",
	    tmp, filename, strerror(errno));
	return 1;
    }
    unlink(old);
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * createHomeDirectory --
 *
 *	Create a directory and copy the prototype new user directory
 *      there.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates directory.
 *
 *----------------------------------------------------------------------
 */
static void
createHomeDirectory(dir, username, project, forward, uid, gid)
    CONST char *dir;
    CONST char *username;
    CONST char *forward;
    CONST char *project;
    int uid;
    int gid;
{
    int child;
    int w;
    char cshrc[MAXPATHLEN];
    char expr[BUFFER_SIZE + 20];
    char projf[MAXPATHLEN];
    char forwf[MAXPATHLEN];
    union wait ws;
    int fd;

    printf("Creating home directory: %s\n", dir);
    if (mkdir(dir, 0775) != 0) {
	fprintf(stderr, "Cannot create %s: %s\n", dir, strerror(errno));
	if (yes("Do you want to continue?")) {
	    create_home_dir  = SKIP_THIS_STEP;
	    return;
	} else {
	    cleanup(0);
	}
    }
    create_home_dir = DONE;
#ifndef TEST
    if (chown(dir, uid, gid)) {
	fprintf(stderr, "Can't change ownership of %s: %s\n",
	    dir, strerror(errno));
	cleanup(0);
    }
#endif
    switch (child = fork()) {

    case -1:
	fprintf(stderr, "Fork failed: %s\n", strerror(errno));
	cleanup(0);

    case 0:
#ifdef TEST
	execlp("update", "update", NEWUSER_DIR, dir, NULL);
#else
	execlp("update", "update", "-o", username, NEWUSER_DIR, dir, NULL);
#endif
	fprintf(stderr, "Cannot exec update\n");
	cleanup(0);

    default:
	while ((w = wait(&ws)) > 0 && w != child) {
	    continue;
	}
	if (ws.w_retcode) {
	    if (yes("Update failed, do you want to continue?") == 0) {
		cleanup(0);
	    }
	}
	break;
    }
    sprintf(cshrc, "%s/.cshrc", dir);
    sprintf(expr, "s/newuser/%s/g", username);
    if (sed(cshrc, expr, 0644)) {
	fprintf(stderr, "Cannot edit %s\n", cshrc);
    }
#ifndef TEST
    if (chown(cshrc, uid, gid)) {
	fprintf(stderr, "Warning: Can't change ownership of %s: %s\n",
	    cshrc, strerror(errno));
    }
#endif
    sprintf(projf, "%s/.project", dir);
    if ((fd = open(projf, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0) {
	fprintf(stderr, "Cannot open %s: %s\n", projf, strerror(errno));
	return;
    }
    strcat(project, "\n");
    if (write(fd, project, strlen(project)) != strlen(project)) {
	fprintf(stderr, "Error writing %s: %s\n", projf, strerror(errno));
    }
    close(fd);
    sprintf(forwf, "%s/.forward", dir);
    if (*forward) {
	if ((fd = open(forwf, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0) {
	    fprintf(stderr, "Cannot open %s: %s\n", forwf, strerror(errno));
	    return;
	}
	strcat(forward, "\n");
	if (write(fd, forward, strlen(forward)) != strlen(forward)) {
	    fprintf(stderr, "Error writing %s: %s\n", forwf, strerror(errno));
	}
    } else {
	unlink(forwf);
    }
    close(fd);
    return;
}


/*
 *----------------------------------------------------------------------
 *
 *  insertPasswdEntry --
 *
 *	Inserts a passwd entry into the password file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies /etc/passwd.  Creates a backup file.
 *
 *----------------------------------------------------------------------
 */

static void
insertPasswdEntry(newpwd)
    CONST struct passwd *newpwd;
{
    int fd;
    FILE *ptmp;
    struct passwd *pwd;
    char buf1[16], buf2[16];

    if ((fd = open(PTMP_FILE, O_RDWR|O_CREAT|O_EXCL, 0600)) < 0) {
	fprintf(stderr, "Cannot open %s: %s\n", PTMP_FILE, strerror(errno));
	cleanup(0);
    }
    ptmp = fdopen(fd, "w");
#ifndef TEST
    if (getgrnam("wheel") == (struct group *)NULL) {
	fprintf(stderr,"Warning: no wheel group\n");
    } else {
	fchown(fd, -1, getgrnam("wheel")->gr_gid);
    }
#endif

    create_ptmp_file = DONE;
    while ((pwd = getpwent()) != NULL) {
	if (strcmp(pwd->pw_name, newpwd->pw_name) == 0) {
	    fprintf(stderr,
		"There is already an entry in /etc/passwd for %s\n",
		newpwd->pw_name);
	    if (yes("Do you want to replace it?")) {
		continue;
	    } else if (yes("Do you want to continue?")) {
		unlink(PTMP_FILE);
		create_ptmp_file = SKIP_THIS_STEP;
		printf("%s left unchanged\n", MASTER_PASSWD_FILE);
		return;
	    } else {
		cleanup(0);
	    }
	}
	if (pwd->pw_change==0) {
	    *buf1 = '\0';
	} else {
	    sprintf(buf1,"%l",pwd->pw_change);
	}
	if (pwd->pw_expire==0) {
	    *buf2 = '\0';
	} else {
	    sprintf(buf2,"%l",pwd->pw_expire);
	}
	fprintf(ptmp, "%s:%s:%d:%d:%s:%s:%s:%s:%s:%s\n",
	    pwd->pw_name,
	    pwd->pw_passwd,
	    pwd->pw_uid,
	    pwd->pw_gid,
	    pwd->pw_class,
	    buf1,
	    buf2,
	    pwd->pw_gecos,
	    pwd->pw_dir,
	    pwd->pw_shell);
    }
    endpwent();
    if (newpwd->pw_change==0) {
	*buf1 = '\0';
    } else {
	sprintf(buf1,"%l",newpwd->pw_change);
    }
    if (newpwd->pw_expire==0) {
	*buf2 = '\0';
    } else {
	sprintf(buf2,"%l",newpwd->pw_expire);
    }
    fprintf(ptmp, "%s:%s:%d:%d:%s:%s:%s:%s:%s:%s\n",
	newpwd->pw_name,
	newpwd->pw_passwd,
	newpwd->pw_uid,
	newpwd->pw_gid,
	newpwd->pw_class,
	buf1,
	buf2,
	newpwd->pw_gecos,
	newpwd->pw_dir,
	newpwd->pw_shell);
     fclose(ptmp);
     return;
}


/*
 *----------------------------------------------------------------------
 *
 * getPasswdEntryFromUser --
 *
 *	Prompt the user for a string.
 *
 * Results:
 *	Returns the string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static char *
getPasswdEntryFromUser()
{
    static char buf[BUFFER_SIZE];

    getString("", "Enter the /etc/passwd line", buf);
    return buf;
}


/*
 *----------------------------------------------------------------------
 *
 * getPasswdEntryFromDataBase --
 *
 *	Gets a passwd file entry from the ucb global database.
 *
 * Results:
 *	Returns then entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static char *
getPasswdEntryFromDataBase()    
{
    static char lastname[BUFFER_SIZE];
    static char group[BUFFER_SIZE];
    int child;
    int pipefd[2];
    int w;
    static char entry[BUFFER_SIZE];
    union wait ws;

    for (;;) {
	getString(":", "Enter user's last name", lastname);
	for (;;) {
	    getString(":", "Enter user's group", group);
	    if (getgrnam(group)) {
		break;
	    }
	    fprintf(stderr,
		"%s is not a valid group (not in /etc/group).\n", group);
	    fprintf(stderr, "Pleas try again\n");
	}
	printf("lastname is %s, group is %s\n", lastname, group);
	if (yes("Is this correct?")) {
	    break;
	}
    }
    printf("Fetching passwd entry from database on %s.\n", DATABASE_HOST);
    printf("This will take a minute or two.  Please be patient....\n");
    pipe(pipefd);
    switch (child = fork()) {

    case -1:
	fprintf(stderr, "Cannot fork: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

    case 0: /* child */
        close(pipefd[0]);
	dup2(pipefd[1], 1);
	execlp("rsh", "rsh", "allspice", "-l", "root", "rsh", DATABASE_HOST,
	       "-l", "account", "=bin/mkpwent", USER_DIR, lastname, group,
	       NULL);
	fprintf(stderr, "Cannot exec rsh: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

    default: /* parent */
	close(pipefd[1]);
        if ((w = read(pipefd[0], entry, sizeof(entry))) < 0) {
	    fprintf(stderr, "Read from pipe failed: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	if (entry[strlen(entry) - 1] == '\n') {
	    entry[strlen(entry) - 1] = '\0';
	}
	close(pipefd[0]);

	while ((w = wait(&ws)) > 0 && w != child) {
	    continue;
	}
	if (ws.w_retcode) {
	    fprintf(stderr, "Could't fetch entry from %s\n", DATABASE_HOST);
	    fprintf(stderr, "Make sure your machine is listed in /.rhosts\n");
	    cleanup(0);
	}
	break;
    }
    return entry;
}


/*
 *----------------------------------------------------------------------
 *
 * getPasswdEntryInteractive --
 *
 *	description.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static char *
getPasswdEntryInteractive(pwd)
    CONST struct passwd *pwd;
{
    static char username[BUFFER_SIZE];
    static char userid[BUFFER_SIZE];
    static char groupid[BUFFER_SIZE];
    static char fullname[BUFFER_SIZE];
    static char office[BUFFER_SIZE];
    static char phone[BUFFER_SIZE];
    static char home_phone[BUFFER_SIZE];
    static char info[BUFFER_SIZE];
    static char entry[BUFFER_SIZE];
    static char passwd[BUFFER_SIZE];

    char *shell;
    struct group *gp;

    if (pwd) {
	char *gecos, *t1, *t2;

	strcpy(username, pwd->pw_name);
	strcpy(passwd, pwd->pw_passwd);
	sprintf(userid, "%d", pwd->pw_uid);
	sprintf(groupid, "%d", pwd->pw_gid);
	gecos = xmalloc(strlen(pwd->pw_gecos));
	strcpy(gecos, pwd->pw_gecos);
	t1 = parseGecos(gecos);
	strcpy(fullname, gecos);
	t2 = parseGecos(t1);
	strcpy(office, t1);
	t1 = parseGecos(t2);
	strcpy(phone, t2);
	strcpy(home_phone, t1);
    }
    getString(":", "new users login name", username);
    getPasswd(passwd);
    getNumber("user id", userid);
    for (;;) {
	getString(":", "group id", groupid);
	if (isdigit(*groupid)) {
	    gp = getgrgid(atoi(groupid));
	} else {
	    gp = getgrnam(groupid);
	}
	if (gp == NULL) {
	    fprintf(stderr, "%s is not a valid group\n", groupid);
	    fprintf(stderr, "Please try again\n");
	    continue;
	}
	break;
    }
    getString(":,", "full name", fullname);
    getString(":,", "office", office);
    getString(":,", "work phone number", phone);
    getString(",:", "home phone number", home_phone);
    shell = getShell();
    sprintf(info, "%s,%s,%s,%s", fullname, office, phone, home_phone);
    sprintf(entry, "%s:%s:%s:%d:%s:%s/%s:%s",
        username, passwd, userid, gp->gr_gid, info, USER_DIR, username, shell);
    return entry;
}

static char *
getHomeDir()
{
    static char dir[BUFFER_SIZE];
    int c;

    printf("Where would you like to put the home directory?\n");
    printf("Enter 1 for /user1    (misc accounts)\n");
    printf("Enter 2 for /user6    (spriters)\n");
    printf("Enter 3 for /user3    (on cory cluster)\n");
    printf("Enter 4 for /user4    (raid hardware people)\n");
    printf("Enter 5 for /pcs      (mic group)\n");
    printf("Enter 6 for /postdev  (postgres people)\n");
    printf("Enter 7 for somewhere else\n");
    for (;;) {
        printf("Please choose one of the above: ");
	c = raw_getchar();
	printf("\n");
	switch (c) {

	case '1':
	    strcpy(dir, "/user1");
	    break;

	case '2': 
	    strcpy(dir, "/user6");
	    break;

	case '3':
	    strcpy(dir, "/user3");
	    break;

	case '4':
	    strcpy(dir, "/user4");
	    break;

	case '5':
	    strcpy(dir, "/pcs");
	    break;

	case '6':
	    strcpy(dir, "/postdev");
	    break;

	case '7':
	    for (;;) {
		getString("*?[]", "Please enter the directory", dir);
		if (*dir != '/') {
		    fprintf(stderr, "Path must start at root.");
		    continue;
		}
		if (strlen(dir) > 1 && dir[strlen(dir) - 1] == '/') {
		    dir[strlen(dir) - 1] = '\0';
		}
		break;
	    }
	    break;

	default:
	    continue;
	}
	break;
    }
    return dir;
}

static char *
pwskip(p)
    char *p;
{
    while (*p && *p != ':') {
	++p;
    }
    if (*p) {
	*p++ = 0;
    }
    return p;
}

static int
parsePasswdEntry(p, ps)
    char *p;
    struct passwd *ps;
{
    char *uid, *gid;
    static char homedir[MAXPATHLEN];

    ps->pw_name = p;
    p = pwskip(p);
    ps->pw_passwd = p;
    uid = p = pwskip(p);
    ps->pw_uid = atoi(p);
    gid = p = pwskip(p);
    ps->pw_gid = atoi(p);
    p = pwskip(p);
    ps->pw_class = "";
    ps->pw_change = 0;
    ps->pw_expire = 0;
    ps->pw_gecos = p;
    p = pwskip(p);
    ps->pw_dir = p;
    p = pwskip(p);
    ps->pw_shell = p;
    if (strlen(ps->pw_name) == 0) {
	fprintf(stderr, "Null username\n");
	return 0;
    }
    if (*ps->pw_passwd != '*' && strlen(ps->pw_passwd) != 13) {
	fprintf(stderr, "Invalid encrypted password: \"%s\"\n", ps->pw_passwd);
	return 0;
    }
#if 0    
    while (strlen(ps->pw_passwd) != 13) {
	if (*ps->pw_passwd == '\0' || strcmp(ps->pw_passwd, "*") == 0) {
	    printf("Please enter a password\n");
	    ps->pw_passwd = getPasswd();
	    continue;
	}
	fprintf(stderr, "Invalid encrypted passwd\n");
	return 0;
    }
#endif
    if (checkNumber(uid) == 0) {
	fprintf(stderr, "Non digit in uid field\n");
	return 0;
    }
    if (checkNumber(gid) == 0) {
	fprintf(stderr, "Non digit in gid field\n");
	return 0;
    }
    sprintf(homedir, "%s/%s", USER_DIR, ps->pw_name);
    ps->pw_dir = homedir;
    if (*ps->pw_shell != '/') {
	fprintf(stderr, "Bad shell: %s\n", ps->pw_shell);
	return 0;
    }
    return 1;
}

static char *
xmalloc(n)
    int n;
{
    char *p;

    if ((p = malloc(n)) == NULL) {
	fprintf(stderr, "Malloc failed: %s\n", strerror(errno));
	cleanup(0);
    }
    return p;
}


/*
 *----------------------------------------------------------------------
 *
 * copy --
 *
 *	Create a copy of a file.
 *
 * Results:
 *	Returns 0 if there was no error, non-zero if there was an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
copy(from, to, mode)
    CONST char *from;
    CONST char *to;
    int mode;			/* the permissions for the new file */
{
    char *buf;
    int in, out;
    long in_len;

    if ((in = open(from, O_RDONLY)) < 0) {
	fprintf(stderr, "Cannot open %s: %s\n", from, strerror(errno));
	return 1;
    }
    if ((out = open(to, O_WRONLY|O_CREAT|O_EXCL, mode)) < 0) {
	fprintf(stderr, "Cannot open %s: %s\n", to, strerror(errno));
	return 1;
    }
    in_len = lseek(in, 0, L_XTND);
    lseek(in, 0, L_SET);
    buf = xmalloc(in_len);
    if (read(in, buf, in_len) != in_len) {
	fprintf(stderr, "Error reading %s: %s\n", to, strerror(errno));
	return 1;
    }
    close(in);
    if (write(out, buf, in_len) != in_len) {
	fprintf(stderr, "Error writing %s: %s\n", to, strerror(errno));
	return 1;
    }
    free(buf);
    return 0;
}

static void    
cleanup(sig)
    int sig;			/* signal number (ignored) */
{
    fprintf(stderr, "\nCleaning up ...\n");
    if (move_ptmp_to_passwd == DONE) {
	if (rename(MASTER_PASSWD_FILE, PTMP_FILE)) {
	    fprintf(stderr, "Cannot rename %s to %s: %s\n",
		MASTER_PASSWD_FILE, PTMP_FILE, strerror(errno));
	} else if (rename(MASTER_BAK, MASTER_PASSWD_FILE)) {
	    fprintf(stderr, "Cannot rename %s to %s: %s\n",
		MASTER_BAK, MASTER_PASSWD_FILE, strerror(errno));
	} else if (rename(PASSWD_BAK, PASSWD_FILE)) {
	    fprintf(stderr, "Cannot rename %s to %s: %s\n",
		PASSWD_BAK, PASSWD_FILE, strerror(errno));
	} else {
	    unlink(PTMP_FILE);
	}
    } else if (create_ptmp_file == DONE) {
	if (unlink(PTMP_FILE)) {
	    fprintf(stderr, "Cannot unlink %s: %s\n", PTMP_FILE,
		    strerror(errno));
	}
    }
    if (edit_mailaliases_file == DONE) {
	if (unlink(ALIASES_TMP)) {
	    fprintf(stderr, "Cannot unlink %s: %s\n",
		ALIASES_TMP, strerror(errno));
	}
    }
    if (create_home_dir == DONE) {
	fprintf(stderr, "Deleteing %s/* ...\n", real_homedir);
	if (fork() == 0) {
	    execlp("rm", "rm", "-rf", real_homedir, NULL);
	}
    }
    if (make_symlink == DONE) {
	if (unlink(sym_homedir)) {
	    fprintf(stderr, "Cannot unlink %s: %s\n",
		sym_homedir, strerror(errno));
	}
    }
    exit(EXIT_FAILURE);
}

