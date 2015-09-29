/************************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
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

************************************************************************/

/* $XConsortium: osfonts.c,v 1.25 89/06/16 17:03:00 keith Exp $ */

#include <stdio.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>

#include <X11/X.h>
#include <X11/Xproto.h>

#include "font.h"
#include "fontstr.h"
#include "fonttype.h"
#include "fosfile.h"
#include "fontlib.h"
#include "fontfilestr.h"
#include "fontdirstr.h"
#include "fontpathstr.h"

char	*defaultFontPath;
int	 defaultFontDensity = 75;
Bool	 mallocedFontPath;


/* maintained for benefit of GetFontPath */
static FontPathPtr searchList = (FontPathPtr)NULL;

/***====================================================================***/

static FontPathPtr
fpCreateFontPathRecord(size)
    unsigned	size;
{
    FontPathPtr fp;    

    fp = (FontPathPtr)fosAlloc(sizeof(FontPathRec));
    if (fp) {
	fp->npaths = 0;
	fp->size = size;
	fp->length = (int *)fosAlloc(size * sizeof(int));
	fp->paths = (char **)fosAlloc(size * sizeof(char *));
	fp->osPrivate = (pointer *)fosAlloc(size * sizeof(pointer));
	if (!fp->length || !fp->paths || !fp->osPrivate) {
	    fosFree(fp->length);
	    fosFree(fp->paths);
	    fosFree(fp->osPrivate);
	    fosFree(fp);
	    return(NullFontPath);
	}
    }
    return fp;
}

/***====================================================================***/

static int
fpAddFontPathElement(path, element, length, fontDir)
    FontPathPtr path;
    char *element;
    int  length;
    Bool fontDir;
{
    int index = path->npaths;
    FontTablePtr table;
    char *nelt;
    int status;

    if (fontDir) {
	status = fpReadFontDirectory(element, &table, path);
	if (status != Success)
	    return status;
    }
    nelt = (char *)fosAlloc(length + 1);
    if (!nelt)
	return BadAlloc;
    if (index >= path->size)
    {
	int	  size=	path->size << 1;
	int	 *pLengths;
	char	**pPaths;
	pointer	 *ppPriv;

	pLengths= (int *)fosRealloc(path->length, size*sizeof(int));
	pPaths= (char **)fosRealloc(path->paths, size*sizeof(char *));
	ppPriv=	 (pointer *)fosRealloc(path->osPrivate, size * sizeof(pointer));
	if (pLengths && pPaths && ppPriv)
	{
	    path->size = size;
	    path->length = pLengths;
	    path->paths = pPaths;
	    path->osPrivate = ppPriv;
	}
	else
	{
	    fosFree(pLengths);
	    fosFree(pPaths);
	    fosFree(ppPriv);
	    return BadAlloc;
	}
    }
    path->length[index] = length;
    path->paths[index] = nelt;
    strncpy(nelt, element, length);
    nelt[length] = '\0';
    if (fontDir)	path->osPrivate[index] = (pointer)table;
    else		path->osPrivate[index] = NULL;
    path->npaths++;
    return Success;
}

/***====================================================================***/

void
fpFreeFontPath(path)
    FontPathPtr path;
{
    int     i, j;
    EncodedFontPtr font;
    FontTablePtr table;

    if (path) {
	for (i = 0; i < path->npaths; i++) {
	    table = (FontTablePtr) path->osPrivate[i];
	    if (table) {
		fdFreeFontTable(table);
		path->osPrivate[i]=	NULL;
	    }
	    fosFree(path->paths[i]);
	}
	fosFree(path->paths);
	fosFree(path->length);
	fosFree(path->osPrivate);
	fosFree(path);
    }
    return;
}

/***====================================================================***/

/*
 * Font paths are not smashed to lower case. (Note "/usr/lib/X11/fonts")
 *
 * Allow initial font path to have names separated by spaces tabs or commas
 */
int
fpSetDefaultFontPath(name)
    char *	name;
{
    register char *start, *end;
    char dirName[MAXPATHLEN];
    char expandedDirName[MAXPATHLEN];
    FontPathPtr path;
    int status;

    path = fpCreateFontPathRecord(3);
    if (!path)
	return BadAlloc;
    end = name;
    for (;;) {
	start = end;
	while ((*start == ' ') || (*start == '\t') || (*start == ','))
	    start++;
	if (*start == '\0')
	    break;
	end = start;
	while ((*end != ' ') && (*end != '\t') && (*end != ',') &&
		(*end != '\0'))
	   end++;
	strncpy(dirName, start, end - start);
	dirName[end - start] = '\0';
	if (dirName[strlen(dirName) - 1] != '/')
	    strcat(dirName, "/");
        sprintf(expandedDirName, dirName, defaultFontDensity);
	status = fpAddFontPathElement(path,
		 expandedDirName,
		 strlen (expandedDirName),
		 TRUE);
/* 9/11/89 (ef) -- this was smashing the whole path if one directory was */
/*	bad (checking only for status==Success) which seems wrong.       */
/*	Now it only frees the path if it has an allocation problem, which */
/*	seems better							 */
	if ((status != Success) && (status != BadValue))
	{
	    fpFreeFontPath(path);
	    return status;
	}
    }
    fpFreeFontPath(searchList);
    searchList = path;
    return Success;
}

/***====================================================================***/

int
fpSetFontPath(npaths, countedStrings, pError)
    unsigned	npaths;
    char	*countedStrings;
    int		*pError;
{
    int i;
    unsigned char * bufPtr = (unsigned char *)countedStrings;
    char dirName[MAXPATHLEN];
    unsigned int n;
    FontPathPtr path;
    int status;

    if (npaths == 0)
	return fpSetDefaultFontPath(defaultFontPath); /* this frees old paths */

    path = fpCreateFontPathRecord(npaths);
    if (!path)
	return BadAlloc;
    for (i=0; i<npaths; i++) {
	n = (unsigned int)(*bufPtr++);
	strncpy(dirName, (char *) bufPtr, (int) n);
	dirName[n] = '\0';
	if (dirName[n - 1] != '/')
	    strcat(dirName, "/");
	status = fpAddFontPathElement(path, dirName, strlen (dirName), TRUE);
/* 9/11/89 (ef) -- this was smashing the whole path if one directory was */
/*	bad (checking only for status==Success) which seems wrong.       */
/*	Now it only frees the path if it has an allocation problem, which */
/*	seems better							 */
	if ((status != Success)&&(status != BadValue))
	{
	    fpFreeFontPath(path);
	    if (pError)
		*pError=	i;
	    return status;
	}
	bufPtr += n;
    }
    fpFreeFontPath(searchList);
    searchList = path;
    return Success;
}

/***====================================================================***/

FontPathPtr
fpGetFontPath()
{
    return(searchList);
}

/***====================================================================***/

int
fpReadFontDirectory(directory, pTable, path)
char		*directory;
FontTablePtr	*pTable;
FontPathPtr	 path;
{
char			file_name[MAXPATHLEN];
char			font_name[MAXPATHLEN];
char			dir_file[MAXPATHLEN];
char			BUF[BUFSIZ];
FILE			*file;
int			count, i, status;
TypedFontFilePtr	pTFF;
FontTablePtr 		matchTable;
FontTablePtr 		table = NullFontTable;

    strcpy(dir_file, directory);
    if (directory[strlen(directory) - 1] != '/')
 	strcat(dir_file, "/");
    strcat(dir_file, FontDirFile);
    file = fopen(dir_file, "r");
    if (file)
    {
	setbuf (file, BUF);
	count = fscanf(file, "%d\n", &i);
	if ((count == EOF) || (count != 1)) {
	    fclose(file);
	    return BadValue;
	}
	table = fdCreateFontTable(directory, count+10);
	if (table==NullFontTable) {
	    fclose(file);
	    return(BadAlloc);
	}
	for (;;) {
	    count = fscanf(file, "%s %[^\n]\n", file_name, font_name);
	    if (count == EOF)
		break;
	    if (count != 2) {
	        fclose(file);
		fdFreeFontTable(table);
		return BadValue;
	    }
	    if ((tffFindFileType(file_name) >= 0) &&
		(!fpFindFontFile (path, font_name))) {
		pTFF=	tffCreate(file_name,(pointer)directory);
		if (!pTFF) {
		    fclose(file);
		    fdFreeFontTable(table);
		    return( BadAlloc );
		}
		if ((i= fdAddFont(table, font_name, pTFF))!=Success) {
		    if (i==BadAlloc) {
			fclose(file);
			fdFreeFontTable(table);
			return ( i );
		    }
		    else if (i==BadValue) {
			tffCloseFile(pTFF,TRUE);
		    }
		}
	    }
	}
	fclose(file);
    }
    else if (errno != ENOENT)
    {
	return BadValue;
    }

#ifndef LEAVE_OUT_POSTSCRIPT
    if (table) {
	strcpy(dir_file,directory);
	if (directory[strlen(directory) - 1]!='/')
	    strcat(dir_file,"/");
	strcat(dir_file,"psfonts.dir");
	file = fopen(dir_file,"r");
	if (file != NULL) {
	    ReadPSFontDir(file,table);
	    fclose(file);
	}
    }
#endif

    status = fpReadFontAlias(directory, FALSE, &table, path);
/* 9/11/89 (ef) -- Should a problem with aliases trash the whole directory? */
/*		   I don't think so...                                      */
#ifdef notdef
    if (status != Success)
    {
	fdFreeFontTable(table);
	return status;
    }
#endif

    *pTable = table;
    return Success;
}

/***====================================================================***/

/*
 * parse the font.aliases file.  Format is:
 *
 * alias font-name
 *
 * To imbed white-space in an alias name, enclose it like "font name" 
 * in double quotes.  \ escapes and character, so
 * "font name \"With Double Quotes\" \\ and \\ back-slashes"
 * works just fine.
 */

/*
 * token types
 */

static int	lexAlias (), lexc ();

# define NAME		0
# define NEWLINE	1
# define DONE		2
# define EALLOC		3

int
fpReadFontAlias(directory, isFile, ptable, path)
char      	*directory;
Bool		 isFile;
FontTablePtr	*ptable;
FontPathPtr	 path;
{
    char alias[MAXPATHLEN];
    char font_name[MAXPATHLEN];
    char alias_file[MAXPATHLEN];
    char buf[BUFSIZ];
    FILE *file;
    int i;
    FontTablePtr matchTable;
    FontTablePtr table;
    int	token;
    Bool found;
    char *lexToken;
    int status = Success;
    TypedFontFilePtr	pTFF;

    table = *ptable;
    strcpy(alias_file, directory);
    if (!isFile)
    {
	if (directory[strlen(directory) - 1] != '/')
	    strcat(alias_file, "/");
	strcat(alias_file, AliasFile);
    }
    file = fopen(alias_file, "r");
    if (!file)
	return ((errno == ENOENT) ? Success : BadValue);
    setbuf (file, buf);

    if (!table) {
	*ptable = table = fdCreateFontTable ((pointer)directory, 10);
	if (!table)
	    return BadAlloc;
    }

    while (status == Success) {
	token = lexAlias (file, &lexToken);
	switch (token) {
	case NEWLINE:
	    break;
	case DONE:
	    fclose (file);
	    return Success;
	case EALLOC:
	    status = BadAlloc;
	    break;
	case NAME:
	    strcpy (alias, lexToken);
	    token = lexAlias (file, &lexToken);
	    switch (token) {
	    case NEWLINE:
		if (strcmp (alias, "FILE_NAMES_ALIASES"))
		    status = BadValue;
		else if (!fdAddFileNameAliases(table))
		    status = BadAlloc;
		break;
	    case DONE:
		status = BadValue;
		break;
	    case EALLOC:
		status = BadAlloc;
		break;
	    case NAME:
		fosCopyISOLatin1Lowered ((unsigned char *)alias,
				      (unsigned char *)alias,
				      strlen (alias));
		fosCopyISOLatin1Lowered ((unsigned char *)font_name,
				      (unsigned char *)lexToken,
				      strlen (lexToken));
		if ((fdFindFontFile(table,alias)==NullTypedFontFile)&&
		    (fpFindFontFile(path, alias)==NullTypedFontFile)) {
		    pTFF= fpFindFontFile(path,font_name);
		    if  (pTFF==NullTypedFontFile) 
			pTFF= fdFindFontFile(table,font_name);
		    if (pTFF!=NullTypedFontFile)
				fdAddFont(table,alias,pTFF);
		    else	status = BadValue;
		}
		break;
	    }
	}
    }
    fclose(file);
    return status;
}

# define QUOTE		0
# define WHITE		1
# define NORMAL		2
# define END		3
# define NL		4

static int	charClass;

static int
lexAlias (file, lexToken)
FILE	*file;
char	**lexToken;
{
	int		c;
	char		*t;
	enum state { Begin, Normal, Quoted } state;
	int		count;

	static char	*tokenBuf = (char *)NULL;
	static int	tokenSize = 0;

	t = tokenBuf;
	count = 0;
	state = Begin;
	for (;;) {
		if (count == tokenSize) {
		    int nsize;
		    char *nbuf;

		    nsize = tokenSize ? (tokenSize << 1) : 64;
		    nbuf = (char *)fosRealloc(tokenBuf, nsize);
		    if (!nbuf)
			return EALLOC;
		    tokenBuf = nbuf;
		    tokenSize = nsize;
		    t = tokenBuf + count;
		}
		c = lexc (file);
		switch (charClass) {
		case QUOTE:
			switch (state) {
			case Begin:
			case Normal:
				state = Quoted;
				break;
			case Quoted:
				state = Normal;
				break;
			}
			break;
		case WHITE:
			switch (state) {
			case Begin:
				continue;
			case Normal:
				*t = '\0';
				*lexToken = tokenBuf;
				return NAME;
			case Quoted:
				break;
			}
			/* fall through */
		case NORMAL:
			switch (state) {
			case Begin:
				state = Normal;
			}
			*t++ = c;
			++count;
			break;
		case END:
		case NL:
			switch (state) {
			case Begin:
				*lexToken = (char *)NULL;
				return charClass == END ? DONE : NEWLINE;
			default:
				*t = '\0';
				*lexToken = tokenBuf;
				ungetc (c, file);
				return NAME;
			}
		}
	}
}

static int
lexc (file)
FILE	*file;
{
	int	c;
	c = getc (file);
	switch (c) {
	case EOF:
		charClass = END;
		break;
	case '\\':
		c = getc (file);
		if (c == EOF)
			charClass = END;
		else
			charClass = NORMAL;
		break;
	case '"':
		charClass = QUOTE;
		break;
	case ' ':
	case '\t':
		charClass = WHITE;
		break;
	case '\n':
		charClass = NL;
		break;
	default:
		charClass = NORMAL;
		break;
	}
	return c;
}

/***====================================================================***/

static Bool
fpSearchDirectory(index, pat, fontList, limit)
    int index;
    char *pat;
    FontPathPtr fontList;
    unsigned limit;
{
    int i;
    FontTablePtr table;

    if (!searchList)
	return TRUE;
    table = (FontTablePtr)searchList->osPrivate[index];
    if (!table)
	return TRUE;
    /*
     * XXX
     * should do better than linear scan. (This is still better than
     * reading the disk.)
     */
    for (i = 0; i < table->used; i++) {
	if (fdMatch(pat, table->fonts[i].fontName)) {
	    if (fpAddFontPathElement(fontList, table->fonts[i].fontName,
				   strlen(table->fonts[i].fontName), FALSE))
		return FALSE;
	    if (fontList->npaths >= limit)
		break;
	}
    }
    return TRUE;
}

/***====================================================================***/

/*******************************************************************
 *  ExpandFontNamePattern
 *
 *	Returns a FontPathPtr with at most max-names, of names of fonts
 *      matching
 *	the pattern.  The pattern should use the ASCII encoding, and
 *      upper/lower case does not matter.  In the pattern, the '?' character
 *	(octal value 77) will match any single character, and the character '*'
 *	(octal value 52) will match any number of characters.  The return
 *	names are in lower case.
 *
 *      Used only by protocol request ListFonts & ListFontsWithInfo
 *******************************************************************/

FontPathPtr
fpExpandFontNamePattern(lenpat, countedPattern, maxNames)
    unsigned	lenpat;
    char	*countedPattern;
    unsigned	maxNames;
{
    char	*pattern;
    int		i;
    FontPathPtr	fpr;

    if (!searchList)
	return (FontPathPtr)NULL;
    /* random number, this is a guess, but it hardly matters. */
    fpr = fpCreateFontPathRecord((unsigned) 100);
    if (!fpr)
	return NullFontPath;
    pattern = (char *)fosTmpAlloc (lenpat + 1);
    if (!pattern)
    {
	fpFreeFontPath(fpr);
	return NullFontPath;
    }
    /*
     * make a pattern which is guaranteed NULL-terminated
     */
    fosCopyISOLatin1Lowered((unsigned char *)pattern,
			 (unsigned char *)countedPattern,
			 (int) lenpat);

    for ( i=0; i<searchList->npaths; i++)
    {
	if (!fpSearchDirectory(i, pattern, fpr, maxNames))
	{
	    fpFreeFontPath(fpr);
	    return NullFontPath;
	}
	if (fpr->npaths >= maxNames)
	    break;
    }
    fosTmpFree(pattern);
    return fpr;
}

/***====================================================================***/

TypedFontFilePtr
fpFindFontFile(path, fontName)
FontPathPtr	 path;
char		*fontName;
{
TypedFontFilePtr	pTFF= NullTypedFontFile;
int			i;

    if (path==NullFontPath)
	return(NullTypedFontFile);

    for (i = 0; (i < path->npaths)&&(pTFF==NullTypedFontFile); i++) {
	pTFF= fdFindFontFile((FontTablePtr)path->osPrivate[i], fontName);
    }
    return(pTFF);
}

/***====================================================================***/

Mask 
fpLookupFont(fontname, length, ppFont, tables, params)
unsigned	 length;
char 		*fontname;
EncodedFontPtr	*ppFont;
Mask		 tables;
BuildParamsPtr	 params;
{
char		 fName[MAXPATHLEN];
char		 buf[BUFSIZ];
TypedFontFilePtr pTFF;
int		 typeIndex;
int		 cookie;
Mask		 unread;

    *ppFont=	NullEncodedFont;
    fosCopyISOLatin1Lowered((unsigned char *)fName, (unsigned char *)fontname,
			 (int) length);

    if ((pTFF = fpFindFontFile(searchList, fName)) == NullTypedFontFile) {
	return(tables);
    }

    if (tffFont(pTFF) != NULL) {
	*ppFont= (EncodedFontPtr)tffFont(pTFF);	/* already loaded */
	if ((*ppFont)->pCS) {
	    tables &= (~(*ppFont)->pCS->tables);
	    if (tables==0)	
		return(0);
	}
    }

    if (!tffReopenFile(pTFF, "r")) {
	return(tables);
    }
    unread= tffLoadFontTables(pTFF,ppFont,tables,params);

    tffCloseFile(pTFF, FALSE);
    if (*ppFont == NullEncodedFont)
	return tables;
    if (tffFont(pTFF)==NULL) {
	(*ppFont)->osPrivate=	(pointer)pTFF;
	tffIncRefCount(pTFF);
	tffSetFont(pTFF,(pointer)*ppFont);
    }
    return unread;
}

/***====================================================================***/

void 
fpUnloadFont(pFont,shouldFree)
EncodedFontPtr	pFont;
Bool		shouldFree;
{
TypedFontFilePtr	pTFF;

    if (pFont==NullEncodedFont)
	return;
    if ((pTFF = (TypedFontFilePtr)pFont->osPrivate) != NULL) {
	if (tffFont(pTFF)==(pointer)pFont) {
	    pFont->osPrivate=	NULL;
	    tffSetFont(pTFF,NULL);
	    if (tffDecRefCount(pTFF)<1)
		tffCloseFile(pTFF,TRUE);
	}
    }

    if (shouldFree) {
	fontFree(pFont);
    }
    return;
}

