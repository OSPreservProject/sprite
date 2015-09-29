/*-
 * fontnames.c --
 *	Functions for finding and playing with font names.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * TODO:
 *	- Snarf compressed-font stuff from 4.2bsd filenames.c
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: /b/X/src/cmds/Xsprite/os/RCS/fontnames.c,v 1.5 89/08/03 17:14:47 brent Exp $ SPRITE (Berkeley)";
#endif lint

#include "spriteos.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>

#include "misc.h"
#include "resource.h"
#include "osstruct.h"
#include "opaque.h"

extern char *defaultFontPath;
static FontPathRec fontSearchList;  /* The current font search path */


/*-
 *-----------------------------------------------------------------------
 * SetDefaultFontPath --
 *	Initialize the default font path. Called from main(), so it takes
 *	a null-terminated string as an argument.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	fontSearchList is initialized.
 *
 *-----------------------------------------------------------------------
 */
void
SetDefaultFontPath (name)
    char    *name;
{
    int		len = strlen( name);

    fontSearchList.npaths = 1;
    fontSearchList.length = (int *)Xalloc(sizeof(int));
    fontSearchList.length[0] = len;
    fontSearchList.paths = (char **)Xalloc(sizeof(char *));
    fontSearchList.paths[0] = (char *)Xalloc(len + 1);
    bcopy(name, fontSearchList.paths[0], len);
    fontSearchList.paths[0][len] = '\0';        
}


/*-
 *-----------------------------------------------------------------------
 * FreeFontRecord --
 *	Free an array of font paths.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	All the paths are freed as well as the FontPathRec and should
 *	never again be referenced.
 *
 *-----------------------------------------------------------------------
 */
void
FreeFontRecord (pFP)
    FontPathPtr	  pFP;
{
    int i;

    for (i = 0; i < pFP->npaths; i++) {
        Xfree(pFP->paths[i]);
    }
    
    Xfree(pFP->paths);
    Xfree(pFP->length);
}

/*-
 *-----------------------------------------------------------------------
 * SetFontPath --
 *	Set the list of directories to search for fonts. If the number
 *	of paths is 0, the defaultFontPath is used.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	fontSearchList is altered.
 *
 *-----------------------------------------------------------------------
 */
void
SetFontPath (npaths, totalLength, countedStrings)
    unsigned	npaths;	    	    /* Number of directories */
    unsigned	totalLength;	    /* Total length of all paths */
    char 	*countedStrings;    /* An array of counted strings, each
				     * string longword aligned */
{
    int     	  i;
    char    	  *bufPtr = countedStrings;
    char    	  n;

    FreeFontRecord(&fontSearchList);
    if (npaths == 0) {
	SetDefaultFontPath(defaultFontPath);
    } else {
        fontSearchList.length = (int *)Xalloc(npaths * sizeof(int));
        fontSearchList.paths = (char **)Xalloc(npaths * sizeof(char *));

	for (i = 0; i < npaths; i++) {
	    n = *bufPtr++;
	    fontSearchList.length[i] = n;
	    fontSearchList.paths[i] = (char *) Xalloc(n + 1);
	    bcopy(bufPtr, fontSearchList.paths[i], n);
	    fontSearchList.paths[i][n] = '\0';
	    bufPtr += n;
	}
	fontSearchList.npaths = npaths;
    }
}

/*-
 *-----------------------------------------------------------------------
 * GetFontPath --
 *	Return the current search path for fonts.
 *
 * Results:
 *	A FontPathPtr pointing to the current font search path. This shoud
 *	not be touched.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
FontPathPtr
GetFontPath()
{
    return( & fontSearchList);
}

/*-
 *-----------------------------------------------------------------------
 * match --
 *	Function taken from the C-Shell to do filename pattern matching
 *	Refuses to match anything whose first character is a '.'
 *
 * Results:
 *	TRUE if the pattern matches the string. FALSE otherwise.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
static Bool
match (pat, string)
    char    *pat;	    /* Pattern to match against */
    char    *string;        /* Null-terminated string to match */
{
    if (string[0] == '.') {
        return FALSE;
    } else {
	return matchRecursive(pat, string);
    }
}

static Bool
matchRecursive(pat, string)
    char *pat;			/* Pattern to match against */
    char *string;		/* Null-terminated string to match */
{
    int ip;

    for (ip = 0; pat[ip] != '\0'; ip++) {
	if (pat[ip] == '?' || pat[ip] == string[0]) {
	    string++;
	} else if (pat[ip] == '*') {
	    while (!matchRecursive (&pat[ip+1], string)) {
		if (*string == '\0') {
		    /*
		     * If we've hit the end of the string without finding
		     * a match, the pattern doesn't match.
		     */
		    return FALSE;
		}
		string++;
	    }
	} else {
	    return FALSE;
	}
    }
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * ExpandFontName --
 *	Given a font pattern, expand it to the absolute path of the font
 *
 * Results:
 *	The length of the returned path.
 *
 * Side Effects:
 *	ppPathname is set to point at the full path of the font.
 *
 *-----------------------------------------------------------------------
 */
int
ExpandFontName (ppPathname, lenfname, fname)
    char	**ppPathname;	/* OUT: full path of the font */
    unsigned	lenfname;   	/* Length of fname */
    char	*fname;	    	/* Font name to find */
{
    int		  in;
    char	  *fullname = NULL;
    char	  *lowername;
    register char *psch, *pdch;
    register int  i;

    /*
     * Allocate space for fname converted to lowercase and with .snf tacked
     * on to the end, if needed.
     */
    lowername = (char *) Xalloc(lenfname + 5);
    
    /*
     * Copy all characters of fname to lowerfname, converting them to
     * lowercase as needed. When done, null-terminate lowername.
     */
    for (psch = fname, pdch = lowername, i = lenfname;
	 i != 0;
	 i--, psch++, pdch++) {
	     if (isupper (*psch)) {
		 *pdch = (*psch - 'A') + 'a';
	     } else {
		 *pdch = *psch;
	     }
    }
    *pdch = '\0';

    /*
     * Now check that the last four characters of the name are ".snf"
     * If they aren't, tack the suffix onto the name and note the name's
     * increased length in lenfname.
     */
    if (*--pdch != 'f' || *--pdch != 'n' || *--pdch != 's' || *--pdch != '.') {
	strcat (lowername, ".snf");
	lenfname += 4;
    }
    
    if (lowername[0] == '/') {
	/*
	 * If the given name is absolute, check for read access for the
	 * file.
	 */
	if (access (lowername, R_OK) == 0) {
	    *ppPathname = lowername;
	    return lenfname;
	}
    } else {
	int 	  n,
		  ifp;

	for (ifp = 0; ifp < fontSearchList.npaths; ifp++) {
	    n = fontSearchList.length[ifp] + lenfname + 1;
	    fullname = (char *) Xrealloc (fullname,  n);

	    strcpy (fullname, fontSearchList.paths[ifp]);
	    strncat (fullname, lowername, lenfname);
	    fullname[n-1] = '\0';
	    if (access (fullname, R_OK) == 0) {
		*ppPathname = fullname;
		Xfree(lowername);
		return (n);
	    } else {
		fprintf(stderr, "%s: %s\n", fullname, strerror(errno));
	    }
	}
    }
    
    Xfree(lowername);
    Xfree(fullname);
    *ppPathname = NULL;
    return 0;
}

/*-
 *-----------------------------------------------------------------------
 * SearchDirectory --
 *	Search a directory for files matching the given pattern and
 *	return their paths in the given FontPath list, up to the given
 *	number of files.
 *
 * Results:
 *	The FontPath pointed to by pFP.
 *
 * Side Effects:
 *	*pFP is altered.
 *
 *-----------------------------------------------------------------------
 */
static void
SearchDirectory(dname, pat, pFP, limit)
    char    	  *dname;	/* Directory to search */
    char    	  *pat;	    	/* Pattern to match */
    FontPathPtr   pFP;	    	/* Font path list to fill in */
    int     	  limit;    	/* Maximum number of matches */
{
    DIR *	  dir;
    struct direct	  *ent;
    register int  i;

    dir = opendir (dname);
    i = pFP->npaths;
    while ((ent = readdir (dir)) != NULL)   {
	if (match (pat, ent->d_name)){  
	    pFP->length = (int *)Xrealloc(pFP->length, (i+1)*sizeof(int));
	    pFP->paths = (char **)Xrealloc(pFP->paths, (i+1)*sizeof(char *));	    
	    pFP->length[i] = ent->d_namlen;
	    pFP->paths[i] = (char *) Xalloc(ent->d_namlen);
	    bcopy(ent->d_name, pFP->paths[i], ent->d_namlen);

	    pFP->npaths += 1;
	    i += 1;

	    if (i + 1 == limit) {
		break;
	    }
	}
    }
    closedir (dir);
}

/*-
 *-----------------------------------------------------------------------
 * ExpandFontPathPattern --
 *	Expand a font pattern into a FontPath of matching names. This
 *	function is only called in response to a ListFonts protocol request.
 *	See match() for a description of the pattern.
 *
 * Results:
 *	The FontPathRec of all matching fonts.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
FontPathPtr
ExpandFontNamePattern (lenpat, Ppattern, maxNames)
    unsigned	lenpat;	    	    /* length of the pattern */
    char	*Ppattern;  	    /* Protocol pattern */
    unsigned	maxNames;   	    /* Maximum number of names to return */
{
    char	*pattern;   	    /* Null-terminated pattern */
    int		i;	    	    /* Index into array of font paths */
    FontPathPtr	fpr;	    	    /* Array of font paths */


    /*
     * Initialize font path
     */
    fpr = (FontPathPtr) Xalloc (sizeof(FontPathRec));
    fpr->npaths = 0;
    fpr->length = (int *)NULL;
    fpr->paths = (char **)NULL;

    /*
     * make a pattern which is guaranteed NULL-terminated
     */
    pattern = (char *) ALLOCATE_LOCAL (lenpat+1+4);
    strncpy(pattern, Ppattern, lenpat);
    pattern[lenpat] = '\0';
    strcat (pattern, ".snf");

    /*
     * find last '/' in npattern, if any
     */
    for (i = lenpat-1; i >= 0; i--) {
	if (pattern[i] == '/') {
	    break;
	}
    }

    if (i >= 0) {		/* pattern contains its own dir prefix */
	pattern[i] = '\0';	/* break pattern at the last path separator */
        SearchDirectory(pattern, &pattern[i+1], fpr, maxNames);
    } else {
        /*
	 * for each prefix in the font path list
	 */
	for (i = 0; i < fontSearchList.npaths; i++) {
	    SearchDirectory (fontSearchList.paths[i], pattern, fpr, maxNames);
	    if (fpr->npaths == maxNames) {
	        break;
	    }
	}
    }

    /*
     * Strip off the .snf from the end of each font name since the client
     * wants real font names, not file names.
     */
    for (i = 0; i < fpr->npaths; i++) {
	fpr->length[i] -= 4;
    }
    
    DEALLOCATE_LOCAL(pattern);
    return fpr;
}
