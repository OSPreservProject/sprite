/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getwd.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

/*
 * getwd() returns the pathname of the current working directory. On error
 * an error message is copied to pathname and null pointer is returned.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>

#include <sprite.h>
#include <fs.h>
#include <sysStats.h>

#define GETWDERR(s)	strcpy(pathname, (s));

static char *prepend();

char *strcpy();
static int pathsize;			/* pathname length */

static char *oldGetwd();

char *
getwd(pathname)
	char *pathname;
{
    char pathbuf[MAXPATHLEN];		/* temporary pathname buffer */
    char *pnptr = &pathbuf[(sizeof pathbuf)-1]; /* pathname pointer */
    char curdir[MAXPATHLEN];		/* current directory buffer */
    char *dptr = curdir;		/* directory pointer */
    char *prepend();			/* prepend dirname to pathname */
    int cdev;				/* current & root device number */
    int cino;				/* current & root inode number */
    int curServer;			/* current and root server ID */
    DIR *dirp;				/* directory stream */
    struct direct *dir;			/* directory entry struct */
    Fs_Attributes d;			/* file status struct */
    int fd;				/* Handle on current directory */
    Ioc_PrefixArgs iocPrefix;		/* Returned by ioctl. */
    ReturnStatus status;

    pathsize = 0;
    *pnptr = '\0';
    fd = open(".", O_RDONLY);
    if (fd < 0) {
	GETWDERR("getwd: can't open .");
    }
    status = Fs_IOControl(fd, IOC_PREFIX, 0, NULL, sizeof(Ioc_PrefixArgs), 
	    &iocPrefix);

    if (status != SUCCESS) {
	/*
	 * If the ioctl fails assume we are on an old kernel and do it the
	 * old way.
	 */
#if 0
	printf("Using old getwd -- \"%s\".\n", Stat_GetMsg(status));
#endif
	return oldGetwd(pathname);
    }
    close(fd);
    strcpy(dptr, "./");
    dptr += 2;
    if (Fs_GetAttributes(curdir, FS_ATTRIB_FILE, &d) != SUCCESS) {
	    GETWDERR("getwd: can't stat .");
	    return (NULL);
    }
    for (;;) {
	cino = d.fileNumber;
	cdev = d.domain;
	curServer = d.serverID;
	strcpy(dptr, "../");
	dptr += 3;
	if ((dirp = opendir(curdir)) == NULL) {
		GETWDERR("getwd: can't open ..");
		return (NULL);
	}
	Fs_GetAttributesID(dirp->dd_fd, &d);
	if (curServer == d.serverID && cdev == d.domain) {
	    /*
	     * Parent directory is in the same domain as the current point.
	     * Check against root loop and then scan parent looking for match.
	     */
	    if (cino == d.fileNumber) {
		/* reached root directory */
		closedir(dirp);
		break;
	    }
	    do {
		if ((dir = readdir(dirp)) == NULL) {
		    closedir(dirp);
		    GETWDERR("getwd: read error in ..");
		    return (NULL);
		}
	    } while (dir->d_ino != cino);
	    closedir(dirp);
	    pnptr = prepend("/", prepend(dir->d_name, pnptr));
	} else {
	    /*
	     * The parent directory is in a different domain.  This means that
	     * the current point should be the root of a domain and this
	     * host should have a prefix that corresponds to it. 
	     * We already have the prefix from the ioctl, so just break.
	     */
	    closedir(dirp);
	    break;
	}
    }
    if ((strcmp(iocPrefix.prefix, "/") != 0) || (*pnptr == 0)) {
	pnptr = prepend(iocPrefix.prefix, pnptr);
    }
    strcpy(pathname, pnptr);
    return (pathname);
}

/*
 * prepend() tacks a directory name onto the front of a pathname.
 */
static char *
prepend(dirname, pathname)
	register char *dirname;
	register char *pathname;
{
	register int i;			/* directory name size counter */

	for (i = 0; *dirname != '\0'; i++, dirname++)
		continue;
	if ((pathsize += i) < MAXPATHLEN)
		while (i-- > 0)
			*--pathname = *--dirname;
	return (pathname);
}

static char *
oldGetwd(pathname)
	char *pathname;
{
    char pathbuf[MAXPATHLEN];		/* temporary pathname buffer */
    char *pnptr = &pathbuf[(sizeof pathbuf)-1]; /* pathname pointer */
    char curdir[MAXPATHLEN];		/* current directory buffer */
    char *dptr = curdir;		/* directory pointer */
    char *prepend();			/* prepend dirname to pathname */
    int cdev, rdev;			/* current & root device number */
    int cino, rino;			/* current & root inode number */
    int curServer, rootServer;		/* current and root server ID */
    DIR *dirp;				/* directory stream */
    struct direct *dir;			/* directory entry struct */
    Fs_Attributes d;			/* file status struct */

    pathsize = 0;
    *pnptr = '\0';
    if (Fs_GetAttributes("/", FS_ATTRIB_FILE, &d) != SUCCESS) {
	    GETWDERR("getwd: can't stat /");
	    return (NULL);
    }
    rdev = d.domain;
    rino = d.fileNumber;
    rootServer = d.serverID;
    strcpy(dptr, "./");
    dptr += 2;
    if (Fs_GetAttributes(curdir, FS_ATTRIB_FILE, &d) != SUCCESS) {
	    GETWDERR("getwd: can't stat .");
	    return (NULL);
    }
    for (;;) {
	if (d.fileNumber == rino && d.domain == rdev
		&& d.serverID == rootServer)
		break;		/* reached root directory */
	cino = d.fileNumber;
	cdev = d.domain;
	curServer = d.serverID;
	strcpy(dptr, "../");
	dptr += 3;
	if ((dirp = opendir(curdir)) == NULL) {
		GETWDERR("getwd: can't open ..");
		return (NULL);
	}
	Fs_GetAttributesID(dirp->dd_fd, &d);
	if (curServer == d.serverID && cdev == d.domain) {
	    /*
	     * Parent directory is in the same domain as the current point.
	     * Check against root loop and then scan parent looking for match.
	     */
	    if (cino == d.fileNumber) {
		/* reached root directory */
		closedir(dirp);
		break;
	    }
	    do {
		if ((dir = readdir(dirp)) == NULL) {
		    closedir(dirp);
		    GETWDERR("getwd: read error in ..");
		    return (NULL);
		}
	    } while (dir->d_ino != cino);
	    closedir(dirp);
	    pnptr = prepend("/", prepend(dir->d_name, pnptr));
	} else {
	    /*
	     * The parent directory is in a different domain.  This means that
	     * the current point should be the root of a domain and this
	     * host should have a prefix that corresponds to it.  Scan the
	     * prefix table looking for a match.
	     */
	    register int index;
	    Fs_Prefix prefix;
	    closedir(dirp);
	    for (index=0 ; ; index++) {
		bzero((char *) &prefix, sizeof(Fs_Prefix));
		if (Sys_Stats(SYS_FS_PREFIX_STATS, index, (Address) &prefix)
			!= SUCCESS) {
		    sprintf(pathname,
			"getwd: missing prefix %s", pnptr);
		    return(NULL);
		}
		if (prefix.serverID == curServer &&
		    prefix.domain == cdev &&
		    prefix.fileNumber == cino) {
		    pnptr = prepend(prefix.prefix, pnptr);
		    goto done;
		}
	    }
	}
    }
done:
    if (*pnptr == '\0')		/* current dir == root dir */
	    strcpy(pathname, "/");
    else
	    strcpy(pathname, pnptr);
    return (pathname);
}
