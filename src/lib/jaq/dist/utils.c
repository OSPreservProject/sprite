/* 
 * utils.c--
 *
 *	Simple utility functions for Jaquith archive package.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Quote:
 *      "The question of whether a computer can think is no more
 *      interesting than the question of whether a submarine can swim."
 *      -- Dijkstra
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/utils.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

#if (defined(SYSV) && !defined(dynix))
extern char *getcwd();
#else
extern char *getwd();
#endif

/* file globals */
static char printBuf[T_MAXSTRINGLEN];

/*
 *----------------------------------------------------------------------
 *
 * Utils_Bailout--
 *
 *	print message in desired fashion and abort.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Kill's program.
 *
 *----------------------------------------------------------------------
 */

void
Utils_Bailout(msg, disposition)
    char *msg;                /* Death message */
    int disposition;          /* Indicator of message type */
{

    switch (disposition) {
    case BAIL_PRINT:
        fprintf(stderr,"%s\n", msg);
	break;
    case BAIL_PERROR:
        perror(msg);
	break;
    case BAIL_HERROR:
#ifdef sunos
        perror(msg);
#else
	herror(msg);
#endif
	break;
    default:
	fprintf(stderr,"!!Unexpected msg type (%d) in Bailout. msg was:\n",
		disposition);
        fprintf(stderr,"%s\n", msg);
	break;
    }
    exit(T_FAILURE);

}


/*
 *----------------------------------------------------------------------
 *
 * Utils_GetWorkingDir --
 *
 *	Figure out where the hell we are
 *
 * Results:
 *	Returns a ptr to current working directory and its length
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Utils_GetWorkingDir(path, pathLenPtr)
    char *path;               /* receiving path */
    int *pathLenPtr;          /* ptr to receiving length */
{
    if (GETWD(path, pathLenPtr) == NULL) {
	*path = '\0';
	*pathLenPtr = 0;
    } else {
	*pathLenPtr = strlen(path);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Utils_MakeFullPath
 *
 *	Expand simple filename to absolute pathname
 *
 * Results:
 *	Returns a ptr to the full pathname.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Utils_MakeFullPath(relPath)
    char *relPath;               /* simple file name (we think) */
{
    char root[T_MAXPATHLEN];
    int len = sizeof(root);

    if (*relPath == '/') {
	return Str_Dup(relPath);
    }

    Utils_GetWorkingDir(root, &len);

    if (*relPath) {
	return Str_Cat(3, root, "/", relPath);
    } else {
	return Str_Dup(root);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Utils_CheckName --
 *
 *	Validate a filename
 *
 * Results:
 *	T_SUCCESS or T_FAILURE
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
Utils_CheckName(name, noWhiteSpace)
    char *name;               /* filename to validate */
    int noWhiteSpace;         /* 1== whitespace not allowed */
{
    if ((name == NULL) || (!*name)) {
	return T_FAILURE;
    }

    while (*name) {
	if ((isspace(*name) && noWhiteSpace) ||
	    (!(isprint(*name)))) {
	    return (T_FAILURE);
	}
	name++;
    }

    return(T_SUCCESS);
    
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_ReadLine --
 *
 *	Get a line from config file
 *
 * Results:
 *	Ptr to buffer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Utils_ReadLine(stream, stripFlag)
    FILE *stream;             /* input stream from fopen */
    int stripFlag;            /* remove trailing '\n' */
{
    int len;
    static char buf[T_MAXLINELEN];

    while (fgets(buf, sizeof(buf), stream) != NULL) {
	if ((*buf == COMMENT_CHAR) || (*buf == '\n')) {
	    continue;
	}
	len = strlen(buf)-1;
	if (buf[len] != '\n') {
	    Utils_Bailout("ReadLine: Line too long.\n", BAIL_PRINT);
	}
	if (stripFlag) {
	    buf[len] = '\0';
	}
	return buf;
    }
    return NULL;
    
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_StringHashProc --
 *
 *	Default hashing function on strings.
 *
 * Results:
 *	Integer key value.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      Default hashing function is to run over the string,
 *      multiplying the current total by 9 and adding the new character.
 *      Algorithm stolen directly from Ousterhout's tcl code.
 *
 *----------------------------------------------------------------------
 */

int
Utils_StringHashProc(key, keyLen, size)
    Hash_Key key;             /* key to be hashed */
    int keyLen;               /* size of key */
    int size;                 /* size of table */
{
    unsigned char *keyString = (unsigned char *)key;
    int val = 0;
    register int letter;

    if ((size > 100000) || (size < 1)) {
	sprintf(printBuf, "StringHashProc: Bad table size %d\n", size);	
    }

    if (keyLen > 8) {
	keyLen = 8;
    }

    while (keyLen--) {
	letter = *keyString++;
	val = (val << 3) + letter;
    }

    if (val < 0) {
	sprintf(printBuf, "StringHashProc: Bad key %d\n", val);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    return (val % size);
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_IntegerHashProc --
 *
 *	Default hashing function on integers.
 *
 * Results:
 *	Integer key value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Utils_IntegerHashProc(key, keyLen, size)
    Hash_Key key;             /* key to be hashed */
    int keyLen;               /* size of key */
    int size;                 /* size of table */
{
    int *keyInt = (int *)key;

    if ((size > 100000) || (size < 1)) {
	sprintf(printBuf, "IntegerHashProc: Bad table size %d\n", size);
    }

    return ((*keyInt) % size);
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_CvtInteger --
 *
 *      Convert a string to an integer in a given range.
 *
 * Results:
 *	Return Code.
 *
 * Side effects:
 *	none.
 *
 * Note: 
 *      string is interpreted as hex if it begins "0x"
 *                               oct if it begins "0"
 *                           and dec otherwise
 *
 *----------------------------------------------------------------------
 */

int
Utils_CvtInteger(string, low, high, valPtr)
    char *string;             /* ascii number to convert */
    int low;                  /* lowest acceptable value */
    int high;                 /* highest acceptable value */
    int *valPtr;              /* converted value */
{
    char *endPtr;

    *valPtr = strtol(string, &endPtr, 0);

    if ((*endPtr != '\0') || (*valPtr < low) || (*valPtr > high)) {
	return T_FAILURE;
    }

    return T_SUCCESS;
	
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_GetLoginByUid --
 *
 *	Convert user id into login string name
 *
 * Results:
 *      user name.
 *
 * Side effects:
 *	Allocates string space.
 *
 *----------------------------------------------------------------------
 */

char *
Utils_GetLoginByUid(uid)
    int uid;                  /* user id */
{
    struct passwd *entryPtr;
    char dummy = '\0';
    uid_t uidShort = (uid_t) uid;

    entryPtr = getpwuid(uidShort);

    if (entryPtr == (struct passwd *)NULL) {
	return (Str_Dup(&dummy));
    }

    return (Str_Dup(entryPtr->pw_name));

}


/*
 *----------------------------------------------------------------------
 *
 * Utils_GetGroupByGid --
 *
 *	Convert group id into group string name
 *
 * Results:
 *      user name.
 *
 * Side effects:
 *	Allocates string space.
 *
 *----------------------------------------------------------------------
 */

char *
Utils_GetGroupByGid(gid)
    int gid;                  /* group id */
{
    struct group *entryPtr;
    char dummy = '\0';
    gid_t gidShort = (gid_t) gid;

    entryPtr = getgrgid(gidShort);

    if (entryPtr == (struct group *)NULL) {
	return (Str_Dup(&dummy));
    }

    return (Str_Dup(entryPtr->gr_name));

}



/*
 *----------------------------------------------------------------------
 *
 * Utils_GetUidByLogin --
 *
 *	Convert a username string to a user id
 *
 * Results:
 *	userid
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Utils_GetUidByLogin(login)
    char *login;              /* login name */
{
    struct passwd *entryPtr;

    entryPtr = getpwnam(login);

    if (entryPtr == (struct passwd *)NULL) {
	return T_FAILURE;
    }

    return entryPtr->pw_uid;

}

/*
 *----------------------------------------------------------------------
 *
 * Utils_GetGidByGroup --
 *
 *	Convert a group name string to a group id
 *
 * Results:
 *	groupid
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Utils_GetGidByGroup(group)
    char *group;              /* group name */
{
    struct group *entryPtr;

    entryPtr = getgrnam(group);

    if (entryPtr == (struct group *)NULL) {
	return T_FAILURE;
    }

    return entryPtr->gr_gid;

}


/*
 *----------------------------------------------------------------------
 *
 * Utils_MakeErrorMsg --
 *
 *	Interpret error code from server
 *
 * Results:
 *	static (semi) descriptive character string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Utils_MakeErrorMsg(status, syserr)
    int status;               /* Jaquith error number. See jaquith.h */
    int syserr;               /* errno */
{
    static char *msgList[] = {
	"",
	"Bad message version. Recompile application?\n",
	"Bad message format. Application error?\n",
	"Unknown command. Application error?\n",
	"Permission denied.\n",
	"I/O operation failed.\n",
	"Indexing operation failed.\n",
	"Buffering operation failed.\n",
	"Meta-buffering operation failed.\n",
	"Robot operation failed.\n",
	"Volume unknown.\n",
	"Exec failed.\n",
	"No such archive. Use 'jstat -archlist' for an archive list.\n"
	};

    if ((status < 0) || (status > T_MAXERR)) {
	sprintf(printBuf, "MakeErrorMsg: unknown type. Status %d\n",
		status);
    } else {
	strcpy(printBuf, msgList[status]);
    }

    if (syserr != 0) {
	if ((syserr < 0) || (syserr > sys_nerr)) {
	    sprintf(printBuf, "MakeErrorMsg: unknown type. Errno %d\n",	    
		    syserr);
	} else {
	    strcat(printBuf, sys_errlist[syserr]);
	    strcat(printBuf, "\n");
	}

    }

    return printBuf;
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_SendMail --
 *
 *	Send mail routine
 *
 * Results:
 *      none.
 *
 * Side effects:
 *	Sends msg through mail system
 *
 *----------------------------------------------------------------------
 */

int
Utils_SendMail(recipient, msg, type)
    char *recipient;          /* email name of recipient */
    char *msg;                /* text of message */
    char *type;               /* message severity */
{
    sprintf(printBuf,
	    "echo \"Automagic Jaquith %s:\n%s\n\" | mail -s \"Jaquith %s\" %s\n",
		type, msg, type, recipient);
    return (system(printBuf));

}


/*
 *----------------------------------------------------------------------
 *
 * Utils_FreeFileStat --
 *
 *	Release space for a T_FileStat
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates space.
 *
 *----------------------------------------------------------------------
 */

void
Utils_FreeFileStat(statInfoPtr, memFlag)
    T_FileStat *statInfoPtr;  /* file info to be freed */
    int memFlag;              /* release containing block, too */
{

    MEM_FREE("FreeFile", statInfoPtr->fileName);
    MEM_FREE("FreeFile", statInfoPtr->linkName);
    MEM_FREE("FreeFile", statInfoPtr->uname);
    MEM_FREE("FreeFile", statInfoPtr->gname);
    MEM_FREE("FreeFile", statInfoPtr->abstract);
    MEM_FREE("FreeFile", statInfoPtr->fileList);
    if (memFlag) {
	MEM_FREE("FreeFile", statInfoPtr);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Utils_CopyFileStat --
 *
 *	Allocate space for a T_FileStat
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls MEM_ALLOC.
 *
 *----------------------------------------------------------------------
 */

T_FileStat *
Utils_CopyFileStat(statInfoPtr)
    T_FileStat *statInfoPtr;  /* source file data */
{
    T_FileStat *newInfoPtr;

    newInfoPtr = (T_FileStat *)MEM_ALLOC("CopyFileStat", sizeof(T_FileStat));
    *newInfoPtr = *statInfoPtr;
    newInfoPtr->fileName = Str_Dup(statInfoPtr->fileName);
    newInfoPtr->linkName = Str_Dup(statInfoPtr->linkName);
    newInfoPtr->uname    = Str_Dup(statInfoPtr->uname);
    newInfoPtr->gname    = Str_Dup(statInfoPtr->gname);
    newInfoPtr->abstract = Str_Dup(statInfoPtr->abstract);
    newInfoPtr->fileList = Str_Dup(statInfoPtr->fileList);

    return newInfoPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_GetOk --
 *
 *	Get confirmation from user.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Utils_GetOk(msg)
    char *msg;                /* Text message */
{
    char response[T_MAXLINELEN];
    int ok = 0;
    int notOk = 0; 

    while ((!ok) && (!notOk)) {
	fputs(msg, stderr);
	fflush(stderr);
	gets(response);
	if ((ok=Str_Match(response, "{y,Y,yes,Yes,YES}")) ||
	    (notOk=Str_Match(response, "{n,N,no,NO}"))) {
	    return ok;
	}
    }
    return ok;    /* just to make compiler happy */
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_GetInteger -- 
 *
 *	Read an integer from user
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Program terminates of user enters nothing.
 *
 *----------------------------------------------------------------------
 */

int
Utils_GetInteger(msg, low, high)
    char *msg;                /* Text message */
{
    int i;
    char answer[T_MAXLINELEN];

    while (1) {
	fprintf(stderr, msg);
	gets(answer);
	if (!*answer) {
	    exit(-1);
	}
	if (Utils_CvtInteger(answer, low, high, &i) == T_SUCCESS) {
	    return i;
	}
    }
}

