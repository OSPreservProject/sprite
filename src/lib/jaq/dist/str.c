/* 
 * str.c--
 *
 *	String manipulation facilities for the Jaquith server.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/str.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"


/*
 *----------------------------------------------------------------------
 *
 * Str_Match --
 *
 *	See if a particular string matches a particular pattern.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and
 *	0 otherwise.  The matching operation permits the following
 *	special characters in the pattern: *?\[] (see the manual
 *	entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 * Note: This wass stolen straight from the tcl distribution,
 *       where it's called Tcl_StringMatch. Then curly support was added
 *
 *----------------------------------------------------------------------
 */

int
Str_Match(string, pattern)
    register char *string;	/* String. */
    register char *pattern;	/* Pattern, which may contain
				 * special characters. */
{
    char c2;

    while (1) {
	/* See if we're at the end of both the pattern and the string.
	 * If so, we succeeded.  If we're at the end of the pattern
	 * but not at the end of the string, we failed.
	 */
	
	if (*pattern == 0) {
	    if (*string == 0) {
		return 1;
	    } else {
		return 0;
	    }
	}
	if ((*string == 0) && (*pattern != '*')) {
	    return 0;
	}

	/* If the next pattern character is '{', match any item
	 * in the set until the next '}'.
	 */
	
	if (*pattern == '{') {
	    int patLen;
	    int remLen;
	    int curlyDepth;
	    char *recursePat;
	    int recurseLen = 0;
	    char *endPat;
	    char *lastCurly;

	    curlyDepth = 0;
	    for (lastCurly=pattern; *lastCurly != '\0'; lastCurly++) {
		if (*lastCurly == '\\') {
		    lastCurly++;
		} else if (*lastCurly == '{') {
		    curlyDepth++;
		} else if ((*lastCurly == '}') && (--curlyDepth < 1)) {
		    break;
		}
	    }
	    if (curlyDepth != 0) {
		return 0;
	    }
	    remLen = strlen(lastCurly);
	    recurseLen = 1024;
	    recursePat = (char *)MEM_ALLOC("Str_Match",recurseLen*sizeof(char));
	    for (endPat= ++pattern; endPat <= lastCurly; endPat++) {
		if (*endPat == '\\') {
		    endPat++;
		} else if (*endPat == '{') {
		    curlyDepth++;
		} else if ((*endPat == '}') && (curlyDepth > 0)) {
		    curlyDepth--;
		} else if (((*endPat == '}') || (*endPat == ',')) &&
			   (curlyDepth == 0)) {
		    patLen = endPat-pattern;
		    if ((patLen+remLen) > recurseLen) {
			MEM_FREE("Str_Match", recursePat);
			recurseLen = patLen + remLen;
			recursePat = (char *)MEM_ALLOC("Str_Match",
						      recurseLen*sizeof(char));
		    }
		    strncpy(recursePat, pattern, patLen);
		    strcpy(recursePat+patLen, lastCurly+1);
		    if (Str_Match(string, recursePat)) {
			MEM_FREE("Str_Match", recursePat);
			return 1;
		    }
		    pattern = endPat+1;
		}
	    }
	    MEM_FREE("Str_Match", recursePat);
	}

	/* Check for a "*" as the next pattern character.  It matches
	 * any substring.  We handle this by calling ourselves
	 * recursively for each postfix of string, until either we
	 * match or we reach the end of the string.
	 */
	
	if (*pattern == '*') {
	    pattern += 1;
	    if (*pattern == 0) {
		return 1;
	    }
	    while (*string != 0) {
		if (Str_Match(string, pattern)) {
		    return 1;
		}
		string += 1;
	    }
	    return 0;
	}
    
	/* Check for a "?" as the next pattern character.  It matches
	 * any single character.
	 */

	if (*pattern == '?') {
	    goto thisCharOK;
	}

	/* Check for a "[" as the next pattern character.  It is followed
	 * by a list of characters that are acceptable, or by a range
	 * (two characters separated by "-").
	 */
	
	if (*pattern == '[') {
	    pattern += 1;
	    while (1) {
		if ((*pattern == ']') || (*pattern == 0)) {
		    return 0;
		}
		if (*pattern == *string) {
		    break;
		}
		if (pattern[1] == '-') {
		    c2 = pattern[2];
		    if (c2 == 0) {
			return 0;
		    }
		    if ((*pattern <= *string) && (c2 >= *string)) {
			break;
		    }
		    if ((*pattern >= *string) && (c2 <= *string)) {
			break;
		    }
		    pattern += 2;
		}
		pattern += 1;
	    }
	    while ((*pattern != ']') && (*pattern != 0)) {
		pattern += 1;
	    }
	    goto thisCharOK;
	}
    
	/* If the next pattern character is '\', just strip off the '\'
	 * so we do exact matching on the character that follows.
	 */
	
	if (*pattern == '\\') {
	    pattern += 1;
	    if (*pattern == 0) {
		return 0;
	    }
	}

	/* There's no special character.  Just make sure that the next
	 * characters of each string match.
	 */
	
	if (*pattern != *string) {
	    return 0;
	}

	thisCharOK: pattern += 1;
	string += 1;
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Str_Split --
 *
 *	Split a string into parts.
 *
 * Results:
 *      Array of parts and number of parts.
 *
 * Side effects:
 *	none.
 *
 * Note:
 *      Parts is parts. List is NULL terminated.
 *
 *----------------------------------------------------------------------
 */

char **
Str_Split(pathArg, splitChar, partCntPtr, elide, insidePtr)
    char *pathArg;            /* pathname to parse */
    char splitChar;           /* separator */
    int *partCntPtr;          /* number of components */
    int elide;                /* 1 == merge multiple split chars */
    char **insidePtr;         /* ptr to interior space */
{
    char *src;
    char *path;
    int copying;
    int partCnt;
    char **workParts;
    char *start = NULL;

    *insidePtr = path = (char *)MEM_ALLOC("Str_Split",
					  (strlen(pathArg)+1)*sizeof(char));
    strcpy(path, pathArg);

    for (src=path,partCnt=0,copying=0,start=NULL; *src; src++) {
	if (*src == splitChar) {
	    if ((copying) || (!elide)) {
		copying = 0;
		partCnt++;
	    }
	} else {
	    if (!copying) {
		copying = 1;
	    }
	}
    }
    if ((copying) || (!elide)) {
	partCnt++;
    }

    workParts = (char **)MEM_ALLOC("Str_Split", (partCnt+1)*sizeof(char *));

    for (src=path,partCnt=0,copying=0,start=NULL; *src; src++) {
	if (*src == splitChar) {
	    if ((copying) || (!elide)) {
		copying = 0;
		if (start) {
		    workParts[partCnt++] = start;
		    start = NULL;
		} else {
		    workParts[partCnt++] = src;
		}
		*src = '\0';
	    }
	} else {
	    if (!copying) {
		start = src;
		copying = 1;
	    }
	}
    }
    if ((copying) || (!elide)) {
	if (start) {
	    workParts[partCnt++] = start;
	} else {
	    workParts[partCnt++] = src;
	}
    }

    workParts[partCnt] = NULL;
    *partCntPtr = partCnt;

    return workParts;

}


/*
 *----------------------------------------------------------------------
 *
 * Str_Unquote --
 *
 *	Remove '\' preceding metacharacters in string
 *
 * Results:
 *	New string with '\' inserted.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
Str_Unquote(src)
    char *src;                /* string to be quoted */
{
    char *dest;
    char *ptr;

    for (ptr=src,dest=src; *ptr; ptr++,dest++) {
	if (*ptr == '\\') {
	    ptr++;
	}
	*dest = *ptr;
    }
    *dest = '\0';

    return T_SUCCESS;

}


/*
 *----------------------------------------------------------------------
 *
 * Str_Quote --
 *
 *	Quote metacharacters in string
 *
 * Results:
 *	Updates string in place.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */


char *
Str_Quote(src, metachars)
    char *src;                /* string to be quoted */
    char *metachars;          /* list of chars to be quoted */
{
    int len;
    char *newPtr;
    char *ptr;

    for (ptr=src,len=0; *ptr; ptr++,len++) {
	if (STRCHR(metachars, *ptr) != NULL) {
	    len++;
	}
    }

    newPtr = MEM_ALLOC("Str_Quote", (len+1)*sizeof(char));

    for (ptr=src,len=0; *ptr; ptr++,len++) {
	if (STRCHR(metachars, *ptr) != NULL) {
	    *(newPtr+len++) = '\\';
	}
	*(newPtr+len) = *ptr;
    }
    *(newPtr+len) = '\0';

    return newPtr;

}



/*
 *----------------------------------------------------------------------
 *
 * Str_StripDots
 *
 *	Remove '.' and '..' references in path;
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
Str_StripDots(src)
    char *src;                /* pathname to be updated in place */
{
    register char *path = src;
    register char *work = src;
    char *work1;
    char *work2;
    char *work3;

    while (*work) {
	while (*work == '/') {
	    work1 = work+1;
	    work2 = work+2;
	    work3 = work+3;
	    if ((*work == '/') && (*work1 == '.') &&
		((*work2 == '/') || (*work2 == '\0'))) {
		work = work2;
	    } else if ((*work == '/') &&
		       (*work1 == '.') && (*work2 == '.') &&
		       ((*work3 == '/') || (*work3 == '\0'))) {
		while ((*--path != '/') && (path >= src)) {
		    ;
		}
		if (path >= src) {
		    work = work3;
		} else {
		    fprintf(stderr,"bad path: %s\n", src);
		    return T_FAILURE;
		}
	    } else {
		break;
	    }
	}
	*path++ = *work++;
    } 
    *path = '\0';
    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Str_Dup --
 *
 *	Safe string duplicator.
 *
 * Results:
 *	Copy of string made using our allocator.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Str_Dup(string)
    char *string;             /* string to be duplicated */
{
    char *newString;

    newString = (char *)MEM_ALLOC("Str_Dup",
				  (strlen(string)+1)*sizeof(char));
    strcpy(newString, string);
    return newString;

}


/*
 *----------------------------------------------------------------------
 *
 * Str_Cat --
 *
 *	Safe string concatenator.
 *
 * Results:
 *	Concatenation of string parts made using our allocator.
 *
 * Side effects:
 *	None.
 *
 * Notes:
 *      Argument list is: argument count, strings to be concatenated.
 *
 *----------------------------------------------------------------------
 */

char *
Str_Cat(va_alist)
    va_dcl
{
    int argCnt;
    int len;
    va_list argPtr;
    char *curPtr;
    char *bufPtr;
    char *curArg;
    int i;

    va_start(argPtr);
    argCnt = va_arg(argPtr, int);
    for (i=0,len=0; i<argCnt; i++) {
	curArg = va_arg(argPtr, char *);
	len += strlen(curArg);
    }
    va_end(argPtr);

    bufPtr = MEM_ALLOC("Str_Cat", (len+1)*sizeof(char));

    va_start(argPtr);
    argCnt = va_arg(argPtr, int);
    for (i=0,curPtr=bufPtr; i<argCnt; i++) {
	curArg = va_arg(argPtr, char *);
	while (*curArg) {
	    *curPtr++ = *curArg++;
	} 
    }
    va_end(argPtr);

    *(bufPtr+len) = '\0';
    return bufPtr;

}

#ifndef HASSTRTOK

/*
 *----------------------------------------------------------------------
 *
 * strtok --
 *
 *      Split a string up into tokens
 *
 * Results:
 *      If the first argument is non-NULL then a pointer to the
 *      first token in the string is returned.  Otherwise the
 *      next token of the previous string is returned.  If there
 *      are no more tokens, NULL is returned.
 *
 * Side effects:
 *      Overwrites the delimiting character at the end of each token
 *      with '\0'.
 *
 *----------------------------------------------------------------------
 */

char *
strtok(s, delim)
    char *s;            /* string to search for tokens */
    const char *delim;  /* delimiting characters */
{
    static char *lasts;
    register int ch;

    if (s == 0)
        s = lasts;
    do {
        if ((ch = *s++) == '\0')
            return 0;
    } while (STRCHR(delim, ch));
    --s;
    lasts = s + strcspn(s, delim);
    if (*lasts != 0)
        *lasts++ = 0;
    return s;
}

#endif
