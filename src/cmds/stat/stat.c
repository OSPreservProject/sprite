/* 
 * stat.c --
 *
 *	Get the complete attributes of a file
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/stat/RCS/stat.c,v 1.4 89/01/29 16:09:07 rab Exp Locker: shirriff $";
#endif /* not lint */

#include <sprite.h>
#include <fs.h>
#include <stdio.h>
#include <option.h>
#include <sys/types.h>
#include <time.h>

/*
 * Default values for parameters
 */
Boolean shortForm = FALSE;
Boolean link = FALSE;
Boolean showDay = FALSE;
Boolean timing = FALSE;
int	numTimes = 1000;

Option optionArray[] = {
    OPT_TRUE, "s", (Address)&shortForm, "Output a short form of the attributes",
    OPT_TRUE, "l", (Address)&link, "Get attributes of the link, not the target",
    OPT_TRUE, "d", (Address)&showDay, "Include day of the week in dates",
    OPT_TRUE, "t", (Address)&timing, "Time N Fs_GetAttributes calls",
    OPT_INT,  "n", (Address)&numTimes, "Number of repititions (with -t)",
};
int numOptions = sizeof(optionArray) / sizeof(Option);

int localOffset;
char * PrintDate();

/*
 * main --
 *	Collect arguments, then call Stat() for each file.
 */
void
main(argc, argv)
    int argc;
    char **argv;
{
    register ReturnStatus status;
    register int i;
    register char *fileName;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, OPT_ALLOW_CLUSTERING);

    if (argc < 2) {
	fprintf(stderr, "usage: %s filename [filename ...]\n", 
			argv[0]);
	exit(0);
    }
    Sys_GetTimeOfDay(NULL, &localOffset, NULL);
    localOffset = (localOffset * 60) + 3600;

    if (!timing) {
	for (i=1 ; i<argc ; i++) {
	    fileName = argv[i];

	    status = Stat(fileName, link);
	    if (status != SUCCESS) {
		fprintf(stderr, "%s: ", fileName);
		Stat_PrintMsg(status, "");
	    }
	}
    } else {
	Time before, after;
	Fs_Attributes attrs;

	Sys_GetTimeOfDay(&before, NULL, NULL);
	for (i=0 ; i<numTimes ; i++) {
	    status = Fs_GetAttributes(argv[1], link, &attrs);
	    if (status != SUCCESS) {
		fprintf(stderr, "%s: ", argv[1]);
		Stat_PrintMsg(status, "");
		break;
	    }
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	if (i > 0) {
	    Time_Subtract(after, before, &after);
	    printf("%d Fs_GetAttributes of %s at %dus each\n", i, argv[1],
		(after.seconds * 1000000 + after.microseconds) / i);
	}
    }
    exit(status);
}

/*
 * Stat --
 *	Get the attributes of a file and print them.
 *
 * Results:
 *	The status code from Fs_GetAttributes
 *
 * Side Effects:
 *	None.
 */
ReturnStatus
Stat(fileName, link)
    char *fileName;	/* File to get the attributes of */
    Boolean link;	/* If TRUE the attributes of a link file are printed,
			 * otherwise the attributes of the target of the link 
			 * are printed */
{
    ReturnStatus status;
    Fs_Attributes attrs;

    status = Fs_GetAttributes(fileName, link, &attrs);
    if (status != SUCCESS) {
	return(status);
    }

    PrintFileType(attrs.type);
    PrintFilePermissions(attrs.permissions);
    printf("%2d  ID=(%d,%d) ", attrs.numLinks, attrs.uid, attrs.gid);
    if (shortForm) {
	/*
	 * Print a 1-line summary of the attributes.
	 */
	if (attrs.type == FS_DEVICE) {
	    printf("%5d%5d ", attrs.devType, attrs.devUnit);
	} else {
	    printf("%10d ", attrs.size);
	}
	printf("%s %s\n", PrintDate(attrs.dataModifyTime), fileName);
    } else {
	/*
	 * Print the complete attributes.
	 */
	printf("%7d bytes  %s\n", attrs.size, fileName);

	printf("%7s %7s %10s", "Server", "Domain", "File #");
	if (attrs.type == FS_DEVICE) {
	    printf("  Device: %7s %7s %7s", "Server", "Type", "Unit");
	}
	printf("\n");

	printf("%7d %7d %10d", 
			attrs.serverID, attrs.domain, attrs.fileNumber);
	if (attrs.type == FS_DEVICE) {
	    printf("          %7d %7d %7d", 
		attrs.devServerID, attrs.devType, attrs.devUnit);
	}
	printf("\n");

	printf("Version %d	UserType 0x%x\n",
		attrs.version, attrs.userType);

	printf("Created:         %s\n", PrintDate(attrs.createTime));
	printf("Data modified:   %s\n", PrintDate(attrs.dataModifyTime));
	printf("Descr. modified: %s\n", PrintDate(attrs.descModifyTime));
	printf("Last accessed:   %s\n", PrintDate(attrs.accessTime));
    }
    return(SUCCESS);
}

/*
 * PrintFileType --
 *	Print a character to represent a file's type.
 */
PrintFileType(type)
    int type;
{
    char c;
    switch(type) {
	case FS_FILE:
	    c = '-';
	    break;
	case FS_DIRECTORY:
	    c = 'd';
	    break;
	case FS_SYMBOLIC_LINK:
	    c = 'l';
	    break;
	case FS_REMOTE_LINK:
	    c = 'r';
	    break;
	case FS_DEVICE:
	case FS_REMOTE_DEVICE:
	    c = 'D';
	    break;
	case FS_NAMED_PIPE:
	    c = 'p';
	    break;
	case FS_PSEUDO_DEV:
	    c = 'x';
	    break;
	default:
	    printf("(%d)", type);
	    return;
    }
    printf("%c", c);
}
/*
 * PrintFilePermissions --
 *	Print out characters to represent the permission bits of a file.
 */
PrintFilePermissions(permissions)
    int permissions;
{
    char c;
    register int i;
    register int bits;

    if (permissions & FS_SET_UID) {
	printf("u");
    } else if (permissions & FS_SET_GID) {
	printf("g");
    } else {
	printf("-");
    }
    for (i=0 ; i<3 ; i++) {
	bits = (permissions >> ((2-i)*3)) & 0x7;
	if (bits & FS_WORLD_READ) { c = 'r'; } else { c = '-'; }
	printf("%c", c);
	if (bits & FS_WORLD_WRITE) { c = 'w'; } else { c = '-'; }
	printf("%c", c);
	if (bits & FS_WORLD_EXEC) { c = 'x'; } else { c = '-'; }
	printf("%c", c);
    }
}
/*
 * PrintDate --
 *	Print a date in ascii format
 */

char *
PrintDate(time)
    Time time;
{
    time_t t;
    char *string;

    t = time.seconds;
    string = asctime(localtime(&t));        
    string[strlen(string) - 1] = '\0';      /* remove trailing '\n' */
    if (showDay) {
	return(string);
    } else {
	return(&string[4]);
    }
}
