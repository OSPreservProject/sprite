/***********************************************************
Copyright 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* $XConsortium: fontdir.c,v 1.9 88/10/11 15:05:11 rws Exp $ */

#include <stdio.h>
#include <sys/param.h>

#include <X11/X.h>

#include "fontos.h"
#include "fontfilestr.h"
#include "fontdirstr.h"


#define  XK_LATIN1
#include <X11/keysymdef.h>

/***====================================================================***/

static char *
fdMakeCopy(orig)
char	*orig;
{
char	*copy = (char *) fosAlloc(strlen(orig) + 1);

    if	(copy)
	strcpy(copy, orig);
    return copy;
}

/***====================================================================***/

static	Bool
fdNameIsWildcard(name)
register char	*name;
{

    while (*name) {
	if ((*name == XK_asterisk) || (*name == XK_question))
	    return TRUE;
	name++;
    }
    return(FALSE);
}

/***====================================================================***/

/*
 * The value returned is either the entry that matched, or in the case
 * that 'found' is false, where in the table the entry should be inserted.
 */

static int 
fdFindNormalNameIndex(table, fontName, found)
FontTablePtr	 table;
char		*fontName;
Bool		*found;
{
register int	 left, right, center, result;

    *found = FALSE;

/*
 * binary search with invariant:
 *	legal search space is in [left .. right - 1];
 */

    left = 0;
    right = table->used;
    while (left < right) {
	center = (left + right) / 2;
	result = strcmp(fontName, table->fonts[center].fontName);
	if (result == 0) {
	    *found = TRUE;
	    return center;
	}
	if (result < 0)
	    right = center;
	else
	    left = center + 1;
    }
    return left;
}

/***====================================================================***/

static int 
fdFindWildNameIndex(table, fontName, firstWild, found)
FontTablePtr	 table;
char		*fontName, *firstWild;
Bool		*found;
{
char	stub[MAXPATHLEN];
int	low, high, i;
Bool	ignore;

    *found = FALSE;
    if (firstWild == fontName) {
	low = 0;
	high = table->used;
    } else {
	strncpy(stub, fontName, firstWild - fontName);
	stub[firstWild - fontName] = '\0';
	low = fdFindNormalNameIndex(table, stub, &ignore);
	stub[firstWild - fontName -1]++;
	high = fdFindNormalNameIndex(table, stub, &ignore);
    }
    for (i = low; i < high; i++) {
	if (fdMatch(fontName, table->fonts[i].fontName)) {
	    *found = TRUE;
	    return i;
	}
    }
    return low;		/* should not be used */
}

/***====================================================================***/

static	int 
fdFindNameIndex(table, fontName, found)
FontTablePtr	 table;
char		*fontName;
Bool		*found;
{
    register char *wildChar;

    for (wildChar = fontName; *wildChar; wildChar++) {
	if ((*wildChar == XK_asterisk) || (*wildChar == XK_question))
	    return fdFindWildNameIndex(table, fontName, wildChar, found);
    }
    return fdFindNormalNameIndex(table, fontName, found);
}

/***====================================================================***/

TypedFontFilePtr
fdFindFontFile(table, fontName)
FontTablePtr	 table;
char		*fontName;
{
int	index,found;

    if ((table==NullFontTable)||(fontName==NULL))
	return(NullTypedFontFile);
    index= fdFindNameIndex(table, fontName, &found);
    if (found)	return(table->fonts[index].pTFF);
    else	return(NullTypedFontFile);
}

/***====================================================================***/

/*
 * This will overwrite a previous entry for the same name. This means that if
 * multiple files have the same font name contained within them, then the last
 * will win.
 */

int 
fdAddFont(table, fontName, pTFF)
FontTablePtr	 	 table;
char			*fontName;
TypedFontFilePtr	 pTFF;
{
int		i;
Bool		found;

    if ((table==NullFontTable)||(fontName==NULL)||(pTFF==NullTypedFontFile))
	return(BadValue);

    if (fdNameIsWildcard(fontName))
	return(BadValue);

    i = fdFindNormalNameIndex (table, fontName, &found);
    if (!found) {				/* else just overwrite entry */
	if (table->size == table->used) {
	    table->size *= 2;
	    /* If this realloc fails and frees table->fonts in the process, */
	    /* there'll be hell to pay.  Maybe this should be an alloc and  */
	    /* a copy... */
	    table->fonts = (FontNamePtr)fosRealloc (
		    (unsigned char *)table->fonts, 
		    sizeof (FontNameRec) * (table->size));
	    if (table->fonts==NULL)
		return(BadAlloc);
	}
	if (i < table->used) {
	    register int j;

	    for (j = table->used; j > i; j--) {
		table->fonts[j] = table->fonts[j-1];     /* struct copy */
	    }
	}
	table->used++;
	table->fonts[i].fontName = fdMakeCopy (fontName);
	if (table->fonts[i].fontName==NULL)
	    return(BadAlloc);
    }
    else  if (tffDecRefCount(table->fonts[i].pTFF)<1) {
	tffCloseFile(table->fonts[i].pTFF,TRUE);
    }
    table->fonts[i].pTFF= 	pTFF;
    tffIncRefCount(pTFF);
    return Success;
}

/***====================================================================***/

FontTablePtr
fdCreateFontTable(osContext,size)
pointer	 osContext;
int	 size;
{
FontTablePtr	table;

    if (size<1)
	return(NullFontTable);

    table = 		(FontTablePtr)fosAlloc(sizeof(FontTableRec));
    if (table==NullFontTable)
	return(NullFontTable);

    if (osContext)
	 table->osContext=	(pointer)fdMakeCopy((char *)osContext);
    else table->osContext=	NULL;
    table->fonts= 	(FontNamePtr)fosAlloc(sizeof(FontNameRec)*size);
    if (table->fonts==NULL) {
	fosFree(table);
	return(NullFontTable);
    }
    table->size=  	size;
    table->used= 	0;
    table->pPriv= 	NULL;
    return table;
}

/***====================================================================***/

void 
fdFreeFontTable(table)
FontTablePtr	table;
{
TypedFontFilePtr	pFile;
int			i;

    if (table==NullFontTable)
	return;

    for (i = 0; i < table->used; i++) {
	fosFree((unsigned char *)table->fonts[i].fontName);
	if (tffDecRefCount(table->fonts[i].pTFF)<1) {
	    tffCloseFile(table->fonts[i].pTFF,TRUE);
	}
    }
    fosFree((unsigned char *)table->osContext);
    fosFree((unsigned char *)table->fonts);
    fosFree((unsigned char *)table);
    return;
}

/***====================================================================***/

Bool 
fdMatch( pat, string)
register char	*pat;
register char	*string;
{
    if (pat==string)			return(TRUE);
    if ((pat==NULL)||(string==NULL))	return(FALSE);

    for (; *pat != '\0'; pat++, string++)
    {
        if (*pat == XK_asterisk)
	{
	    pat++;
	    if (*pat == '\0')
		return TRUE;
	    while (!fdMatch(pat, string))
	    {
		if (*string++ == '\0') 
		    return FALSE;
	    }
	    return TRUE;
	}
        else if (*string == '\0')
            return FALSE;
	else if ((*pat != XK_question) && (*pat != *string))
	    return FALSE;
    }
    return (*string == '\0');
}

/***====================================================================***/

/* 
 * Make the each of the file names an automatic alias for each of the files 
 * in pNames.  This assumes that all file names are of the form 
 *     <root>.<extension>.
 * This really should be rewritten -- as it stands, it's modifying the list
 * of files  as it iterates through them.  Sloppy.
 */

Bool
fdAddFileNameAliases(table)
FontTablePtr	  table;
{
int		 i;
char		 copy[MAXPATHLEN];
TypedFontFilePtr pTFF;
char		*tmp;
extern		char *rindex();

    if (table==NullFontTable)
	return FALSE;

    for (i = 0; (i < table->used); i++) {
	pTFF= table->fonts[i].pTFF;
	strcpy(copy, tffName(pTFF));

	if ((tmp=rindex(copy,'.'))==NULL)	continue;
	*tmp=	NUL;

	if ((tmp=rindex(copy,'/'))==NULL)	tmp=	copy;
	else					tmp++;

	fosCopyISOLatin1Lowered ((unsigned char *)tmp, (unsigned char *)tmp,
			      	strlen(tmp));

	if (!fdFindFontFile(table, tmp)) {
	    if (fdAddFont(table, tmp, pTFF)==BadAlloc)
		return FALSE;
	}
    }
    return TRUE;
}

