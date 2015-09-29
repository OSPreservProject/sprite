# include	<stdio.h>
# include	<ctype.h>
# include	<sys/param.h>
# include	<sys/stat.h>
# include	<strings.h>

extern	char	*getenv();

static	char	*NextPart();	/* next name in the PATH */

static	char	privatefullpath[MAXPATHLEN];

/*
 *---------------------------------------------------------------------
 *
 * Whence --
 *
 *	Using argv[0], figure out from where this program was called.
 *	Copy the answer to a safe place and return a ptr to the answer.
 *
 * Results:
 *	A pointer to the pathname of the directory whence this program was
 *	called.  NULL if we aren't successful.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------
 */
char *
Whence(fullpath, argv_zero)
char	*fullpath;	/* must be MAXPATHLEN long or NULL */
char	*argv_zero;	/* argv[0] passed in. */
{
    char	*p, *q;
    char	*last_slash;
    char	*path;
    char	*argv0;
    int		status1, status2;
    int		found = 0;

    if (fullpath == NULL) {
	fullpath = privatefullpath;
    }
    argv0 = argv_zero;
    path = getenv("PATH");
    if (argv0 == NULL || path == NULL || *argv0 == '\0' || *path == '\0') {
	fullpath[0] = '\0';
	return NULL;
    }
    if (*argv0 == '/') {
	(void) strcpy(fullpath, argv0);
    } else {
	for (p = path, q = NextPart(&p, argv0, fullpath); q != NULL;
		q = NextPart(&p, argv0, fullpath)) {
	    if ((status1 = is_a_file(q)) && (status2 = is_executable(q))) {
		found = 1;
		break;
	    } else if (status2 == -1) {
		fullpath[0] = '\0';
		return NULL;
	    }
	}
	if (!found) {
	    fullpath[0] = '\0';
	    return NULL;
	}
    }
    if ((last_slash = rindex(fullpath, '/')) != NULL) {
	*(last_slash) = '\0';
    }

    return fullpath;
}

static	char	name_buf[MAXPATHLEN];
static	struct	stat	stat_buf;

int
is_a_file(name)
char	*name;
{
    if (strcmp(name_buf, name) != 0) {
	(void) strcpy(name_buf, name);
	if (stat(name, &stat_buf) != 0) {
	    return 0;
	}
    }
    
    if ((stat_buf.st_mode & S_IFMT) == S_IFREG) {
	return 1;
    }
    return 0;
}

int
is_executable(name)
char	*name;
{
    if (strcmp(name_buf, name) != 0) {
	(void) strcpy(name_buf, name);
	if (stat(name, &stat_buf) != 0) {
	    return -1;
	}
    }
    
    if ((stat_buf.st_mode & S_IEXEC) == S_IEXEC) {
	return 1;
    }
    return 0;
}


/*
 *	NextPart - get next name fromm the path
 */
static	char *
NextPart(ppath, argv0, fullpath)
char	**ppath;	/* Pointer to the next entry in the ppath */
char	*argv0;		/* name of the file	*/
char	*fullpath;	/* path to put return info into */
{
    register	char	*cptr;
    int	wdLen;

    /*
     * Skip leading blanks and colons.	Then make sure that there's
     * another entry in the ppath.
     */
    while (**ppath && (isspace(**ppath) || (**ppath == ':'))) {
	(*ppath)++;
    }
    if (**ppath == '\0') {
	return (NULL);
    }

    /*
     * Expand "." as a ppath.
     */
    if ((*ppath)[0] == '.' && ((*ppath)[1] == ':' || (*ppath)[1] == '\0') ) {
	getwd(fullpath);
	(*ppath)++;
	wdLen = strlen(fullpath);
	fullpath[wdLen] = '/';
	strncpy(fullpath+wdLen+1, argv0, sizeof (argv0));
	return (fullpath);
    }
    /*
     * Grab the next directory name and terminate it with a slash if
     * there isn't one there already.
     */
    for (cptr = fullpath; **ppath && !(isspace(**ppath) || (**ppath == ':'));
	    cptr++, (*ppath)++) {
	*cptr = **ppath;
    }
    if (cptr[-1] != '/') {
	*cptr++ = '/';
    }
    strncpy(cptr, argv0, strlen(argv0));
    cptr[strlen(argv0)] = '\0';
    return (fullpath);
}
