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

/* $XConsortium: osfonts.c,v 1.16 88/10/21 15:44:56 rws Exp $ */

#include <stdio.h>
#include "Xos.h"
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "osstruct.h"
#include "misc.h"
#include "opaque.h"

#include "fonttype.h"
#include "fontdir.h"

extern Atom	MakeAtom();
extern char *defaultFontPath;

static FontTable	ReadFontAlias ();
static FontFile		FindFontFile ();
static FontTable	ReadFontDir ();

FontPathPtr		GetFontPath ();

/* maintained for benefit of GetFontPath */
static FontPathPtr searchList = (FontPathPtr)NULL;

/*
 * This is called from main.c, not in response to a protocol request, so it
 * may take a null-terminated string as argument.
 */

void
FreeFontRecord(pFP)
    FontPathPtr pFP;
{
    int i;
    for (i=0; i<pFP->npaths; i++) {
        xfree(pFP->paths[i]);
    }
    xfree(pFP->paths);
    xfree(pFP->length);
    xfree(pFP);
}

static FontPathPtr
MakeFontPathRecord(size)
    unsigned	size;
{
    FontPathPtr fp;    

    fp = (FontPathPtr)xalloc(sizeof(FontPathRec));
    fp->npaths = 0;
    fp->size = size;
    fp->length = (int *)xalloc(size * sizeof(int));
    fp->paths = (char **)xalloc(size * sizeof(char *));
    fp->osPrivate = (pointer *)xalloc(size * sizeof(pointer));
    return fp;
}

static void
AddFontPathElement(path, element, length, fontDir)
    FontPathPtr path;
    char *element;
    int  length;
    Bool fontDir;
{
    int index = path->npaths;
    FontTable table = NullTable;

    if (fontDir)
    {
	table = ReadFontDir(element, table);
	if (table == NullTable)
	    return;
    }
    if (index >= path->size)
    {
	path->size *= 2;
	path->length = (int *)xrealloc(path->length, path->size*sizeof(int));
	path->paths =
	    (char **)xrealloc(path->paths, path->size*sizeof(char *));
	path->osPrivate =
	    (pointer *)xrealloc(path->osPrivate, path->size * sizeof(pointer));
    }
    path->length[index] = length + 1;
    path->paths[index] = (char *)xalloc(length + 1);
    strncpy(path->paths[index], element, length);
    path->paths[index][length] = '\0';
    path->osPrivate[index] = (pointer)table;
    path->npaths++;
}

static void
NewFontPath(size)
    unsigned	size;
{
    int     i, j;
    FontPtr font;
    FontTable table;

 /* 
  * First all the back pointers for the outstanding fonts must be smashed
  * to NULL so that when the font is freed, the removal from the (now
  * freed) previous table can be skipped.
  */
    if (searchList != NULL) {
	for (i = 0; i < searchList->npaths; i++) {
	    table = (FontTable) searchList->osPrivate[i];
	    for (j = 0; j < table->file.used; j++) {
		if ((font = (FontPtr) table->file.ff[j].private) != NullFont)
		    font->osPrivate = NULL;
	    }
	    FreeFontTable (table);
	}
	FreeFontRecord (searchList);
    }
    searchList = MakeFontPathRecord (size);
}

/*
 * Font paths are not smashed to lower case. (Note "/usr/lib/X11/fonts"
 *
 * Allow initial font path to have names separated by spaces tabs or commas
 */
void
SetDefaultFontPath(name)
    char *	name;
{
    register char *start, *end;
    char dirName[MAXPATHLEN];

    NewFontPath(3);		/* probably big enough for most */
    end = name;
    for (;;) {
	start = end;
	while ((*start == ' ') || (*start == '\t') || (*start == ','))
	    start++;
	if (*start == '\0')
	    return;
	end = start;
	while ((*end != ' ') && (*end != '\t') && (*end != ',') &&
		(*end != '\0'))
	   end++;
	strncpy(dirName, start, end - start);
	dirName[end - start] = '\0';
	if (dirName[strlen(dirName) - 1] != '/')
	    strcat(dirName, "/");
	AddFontPathElement(searchList, dirName, strlen (dirName), True);
    }
}

void
/* ARGSUSED */
SetFontPath(npaths, totalLength, countedStrings)
    unsigned	npaths;
    unsigned	totalLength;
    char *	countedStrings;
{
    int i;
    unsigned char * bufPtr = (unsigned char *)countedStrings;
    char dirName[MAXPATHLEN];
    unsigned int n;

    if (npaths == 0)
	SetDefaultFontPath(defaultFontPath); /* this frees old paths */
    else
    {
        NewFontPath(npaths);
	for (i=0; i<npaths; i++) {
	    n = (unsigned int)(*bufPtr++);
	    strncpy(dirName, (char *) bufPtr, (int) n);
	    dirName[n] = '\0';
	    if (dirName[n - 1] != '/')
		strcat(dirName, "/");
	    AddFontPathElement(searchList, dirName, strlen (dirName), True);
	    bufPtr += n;
	}
    }
}

FontPathPtr
GetFontPath()
{
    return(searchList);
}

static int FindFileType(name)
    char *name;
{
    register int i, j, k;
    char *ext;

    k = strlen(name);
    for (i = 0; fontFileReaders[i].extension; i++) {
	ext = fontFileReaders[i].extension;
	j = strlen(ext);
	if ((k > j) && (strcmp(ext, name + k - j) == 0))
	    return i;
    }
    return -1;
}

static FontTable
ReadFontDir(directory, table)
    char	*directory;
    FontTable	table;
{
    char file_name[MAXPATHLEN];
    char font_name[MAXPATHLEN];
    char dir_file[MAXPATHLEN];
    char BUF[BUFSIZ];
    FILE *file;
    int count, i;
    FontTable matchTable;

    strcpy(dir_file, directory);
    if (directory[strlen(directory) - 1] != '/')
 	strcat(dir_file, "/");
    strcat(dir_file, FontDirFile);
    file = fopen(dir_file, "r");
    if (file != NULL)
    {
	setbuf (file, BUF);
	(void) fscanf(file, "%d\n", &count);
	if (!table)
	     table = MakeFontTable(directory, count+10);
	for (;;) {
	    count = fscanf(file, "%s %[^\n]\n", file_name, font_name);
	    if ((count == EOF) || (count == 0))
		break;
	    if (count == 2) {
		if (!FindFontFile (font_name, 0, FALSE, &matchTable) &&
		    (FindFileType (file_name) >= 0)) {
		    i = AddFileEntry(table, file_name, False);
		    (void) AddNameEntry(table, font_name, i);
		}
	    }
	}
	fclose(file);
    }
    table = ReadFontAlias(directory, FALSE, table);
    if (table)
    {
	/*
	 * At this point, no more entries will be made in file table. This means
	 * that it will not be realloced and we can put pointers (rather than
	 * indices into the table.
	 */
	for (i = 0; i < table->name.used; i++)
	    table->name.fn[i].u.ff = &table->file.ff[table->name.fn[i].u.index];
    }
    return table;
}

/* 
 * Make each of the file names an automatic alias for each of the files.
 * This assumes that all file names are of the form <root>.<extension>.
 */

static void
AddFileNameAliases(table)
    FontTable table;
{
    int i, typeIndex;
    Boolean found;
    FontTable	matchTable;

    char copy[MAXPATHLEN];

    for (i = 0; i < table->file.used; i++) {
	if (table->file.ff[i].alias)
	    continue;
	strcpy(copy, table->file.ff[i].name);
	typeIndex = FindFileType(copy);
	copy[strlen(copy) - strlen(fontFileReaders[typeIndex].extension)] = NUL;
	CopyISOLatin1Lowered ((unsigned char *)copy, (unsigned char *)copy,
			      strlen (copy));
	(void)  FindNameInFontTable(table, copy, &found);
	if (!found && !FindFontFile (copy, 0, FALSE, &matchTable)) {
	    (void) AddNameEntry(table, copy, i);
	}
    }
}

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

static char	*lexToken;
static int	lexAlias (), lexc ();

# define NAME		0
# define NEWLINE	1
# define DONE		2

static FontTable
ReadFontAlias(directory, isFile, table)
    char      *directory;
    Bool      isFile;
    FontTable table;
{
    char alias[MAXPATHLEN];
    char font_name[MAXPATHLEN];
    char alias_file[MAXPATHLEN];
    char buf[BUFSIZ];
    FILE *file;
    int i;
    FontTable matchTable;
    int	token;
    Bool	found;

    strcpy(alias_file, directory);
    if (!isFile)
    {
	if (directory[strlen(directory) - 1] != '/')
	    strcat(alias_file, "/");
	strcat(alias_file, AliasFile);
    }
    file = fopen(alias_file, "r");
    if (file == NULL)
	return table;
    setbuf (file, buf);
    if (!table)
	table = MakeFontTable (directory, 10);
    for (;;) {
	token = lexAlias (file);
	switch (token) {
	case NEWLINE:
	    break;
	case DONE:
	    fclose (file);
	    return table;
	case NAME:
	    strcpy (alias, lexToken);
	    token = lexAlias (file);
	    switch (token) {
	    case NEWLINE:
		if (strcmp (alias, "FILE_NAMES_ALIASES") == 0)
		    AddFileNameAliases (table);
		else
		    table = ReadFontAlias (alias, TRUE, table);
		break;
	    case DONE:
		fclose (file);
		return table;
	    case NAME:
		CopyISOLatin1Lowered ((unsigned char *)alias,
				      (unsigned char *)alias,
				      strlen (alias));
		CopyISOLatin1Lowered ((unsigned char *)font_name,
				      (unsigned char *)lexToken,
				      strlen (lexToken));
		(void) FindNameInFontTable (table, alias, &found);
		if (!found && !FindFontFile (alias, 0, FALSE, &matchTable)) {
		    i = AddFileEntry(table, font_name, True);
		    (void) AddNameEntry(table, alias, i);
		}
		break;
	    }
	}
    }
}

# define QUOTE		0
# define WHITE		1
# define NORMAL		2
# define END		3
# define NL		4

static int	charClass;

static int
lexAlias (file)
FILE	*file;
{
	int		c;
	char		*t;
	enum state { Begin, Normal, Quoted } state;
	int		count;

	static char	*tokenBuf;
	static int	tokenSize;

# define checkBuf() ((count == tokenSize) ? \
			((tokenBuf ? \
 				(tokenBuf = (char *) xrealloc (tokenBuf, tokenSize *= 2)) \
 			: \
 				(tokenBuf = (char *) xalloc (tokenSize = 8)) \
			), \
			(t = tokenBuf + count)) : 0)

	t = tokenBuf;
	count = 0;
	state = Begin;
	for (;;) {
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
				checkBuf ();
				*t = '\0';
				lexToken = tokenBuf;
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
			checkBuf ();
			*t++ = c;
			++count;
			break;
		case END:
		case NL:
			switch (state) {
			case Begin:
				lexToken = 0;
				return charClass == END ? DONE : NEWLINE;
			default:
				checkBuf ();
				*t = '\0';
				lexToken = tokenBuf;
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


static void
SearchDirectory(index, pat, fontList, limit)
    int index;
    char *pat;
    FontPathPtr fontList;
    unsigned limit;
{
    int i;
    FontTable table = (FontTable)searchList->osPrivate[index];

    /*
     * XXX
     * should do better than linear scan. (This is still better than
     * reading the disk.)
     */
    for (i = 0; i < table->name.used; i++) {
	if (Match(pat, table->name.fn[i].name)) {
	    AddFontPathElement(fontList, table->name.fn[i].name,
			       strlen(table->name.fn[i].name), False);
	    if (fontList->npaths >= limit)
		return;
	}
    }
}

/*******************************************************************
 *  ExpandFontPathPattern
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
ExpandFontNamePattern(lenpat, countedPattern, maxNames)
    unsigned	lenpat;
    char	*countedPattern;
    unsigned	maxNames;
{
    char	*pattern;
    int		i;
    FontPathPtr	fpr;

    /* random number, this is a guess, but it hardly matters. */

    fpr = MakeFontPathRecord ((unsigned) 100);

    pattern = ALLOCATE_LOCAL (lenpat + 1);
    if (pattern == NULL)
	return fpr;
    /*
     * make a pattern which is guaranteed NULL-terminated
     */
    CopyISOLatin1Lowered((unsigned char *)pattern,
			 (unsigned char *)countedPattern,
			 (int) lenpat);

    for ( i=0; i<searchList->npaths; i++)
    {
	SearchDirectory(i, pattern, fpr, maxNames);
	if (fpr->npaths >= maxNames)
	    break;
    }
    DEALLOCATE_LOCAL(pattern);
    return fpr;
}

/*
 * OS interface to reading font files. This is not called by the dix routines
 * but rather by any of the pseudo-os-independent font internalization
 * routines.
 */

int
FontFileRead(buf, itemsize, nitems, fid)
    char	*buf;
    unsigned	itemsize;
    unsigned	nitems;
    FID		fid;
{
    return fread( buf, (int) itemsize, (int) nitems, (FILE *)fid);
}

int
FontFileSkip(bytes, fid)
    unsigned	bytes;
    FID		fid;
{
    struct stat	stats;
    int c, count;

    fstat(fileno((FILE *)fid), &stats);
    if ((stats.st_mode & S_IFMT) == S_IFREG)
	c = fseek((FILE *) fid, (long) bytes, 1);
    else
	while (bytes-- && ((c = getc((FILE *)fid)) != EOF))
	    ;
    return c;
}

/*
 * When a font is unloaded (destroyed) it is necessary to remove the pointer
 * in the font table since it is no longer valid. This means that the
 * "obvious" technique of putting the FontPtr into the table when an aliased
 * font is loaded would mean that the table would have to be searched for
 * any matching entry. To avoid scanning all the tables when a font is FontPtr
 * destroyed, the Font has a back pointer to the FontFile (in the table) where
 * it was entered.
 *
 * So that aliases will not also keep a copy of the FontPtr in their FontTable
 * entries, a pointer to the "resolved" FontTable is actually kept and the
 * indirection is taken.
 *
 * A slight non-conformance to the protocol is possible here. If any
 * FontTable is for a file that does not load (i.e. was changed since the
 * font.dirs was created), then that font name will not work even though it
 * should because of wild carding or the use of a search path. The moral is
 * is that the font.dirs should be correct.
 *
 * To prevent circular aliases from crashing the server (due to the recursive
 * nature of FindFontFile) a limit of MAX_LINKS is put on the length of a
 * chain that will be followed.
 */

#define MAX_LINKS 20

static FontFile 
FindFontFile(fontname, depth, followAliases, table)
    char *	fontname;
    int		depth;
    Bool	followAliases;	
    FontTable	*table; 	/* returned */
{
    int     nameIndex, i;
    Bool found;
    FontFile ff, resolved;

    if (depth >= MAX_LINKS)
	return NullFontFile;
    for (i = 0; i < searchList->npaths; i++)
    {
	*table = (FontTable) searchList->osPrivate[i];
	nameIndex = FindNameInFontTable (*table, fontname, &found);
	if (!found)
	    continue;
	ff = (*table)->name.fn[nameIndex].u.ff;
	if (!ff->alias || !followAliases)
	    return ff;
	if (resolved = FindFontFile (ff->name, depth + 1, TRUE, table))
	{
	    ff->private = (Opaque) resolved;
	    return resolved;
	}
    }
    return NullFontFile;
}

#if defined(macII) || (defined(SYSV) && !defined(hpux))
#define vfork() fork()
#endif

static int
FontFilter(fp, filter)
    FILE *fp;
    char **filter;
{
    int pfd[2];
    int pid;

    if (pipe(pfd) < 0) {
	fclose(fp);
	return (-1);
    }
    switch(pid = vfork()) {
    case 0:
	dup2(fileno(fp), 0);
	close(fileno(fp));
	dup2(pfd[1], 1);
	close(pfd[0]);
	close(pfd[1]);
	execvp(filter[0], filter);
	_exit(127);
    case -1:
	close(pfd[0]);
	close(pfd[1]);
	fclose(fp);
	return(-1);
    default:
	dup2(pfd[0], fileno(fp));
	close(pfd[0]);
	close(pfd[1]);
	return(pid);
    }    
}

static FILE *
OpenFontFile(table, name, typeIndex, pid)
    FontTable table;
    char     *name;
    int	     *typeIndex;
    int      *pid;
{
    char    pathName[MAXPATHLEN];
    FILE   *fp;
    char **filter;

    strcpy (pathName, table->directory);
    strcat (pathName, name);
    if ((fp = fopen (pathName, "r")) == (FILE *)NULL)
	return fp;
    *pid = 0;
    *typeIndex = FindFileType (name);
    filter = fontFileReaders[*typeIndex].filter;
    if (filter) {
	*pid = FontFilter(fp, filter);
	if (*pid < 0)
	    return (FILE *)NULL;
    }
    return fp;
}

static void
CloseFontFile(fp, pid)
    FILE *fp;
    int pid;
{
    int child;

    fclose (fp);
    if (pid >= 0)
       do { child = wait(0); } while (child != pid && child != -1);
}

FontPtr 
FontFileLoad(fontname, length)
    unsigned	length;
    char *	fontname;
{
    FontPtr	font;
    char	fName[MAXPATHLEN];
    char	buf[BUFSIZ];
    FILE	* fp;
    FontTable	table;
    FontFile	ff;
    int		typeIndex;
    int		cookie;

    CopyISOLatin1Lowered((unsigned char *)fName, (unsigned char *)fontname,
			 (int) length);
    if ((ff = FindFontFile (fName, 1, TRUE, &table)) == NullFontFile)
	return NullFont;
    if (ff->private != NULL)
	return (FontPtr) ff->private;	/* already loaded */
    if ((fp = OpenFontFile(table, ff->name, &typeIndex, &cookie)) == NULL)
	return NullFont;
    setbuf (fp, buf);
    font = (fontFileReaders[typeIndex].loadFont) (fp);
    CloseFontFile(fp, cookie);
    if (font == NullFont)
	return NullFont;
    ff->private = (Opaque) font;
    font->refcnt = 0;
    font->fileType = typeIndex;
    font->osPrivate = (pointer) ff;
    return font;
}

/*
 * This either returns an existing font, or if that can't be found,
 * then fills in the FontInfo and FontProp by reading from the disk.
 */

Bool
FontFilePropLoad(fontname, length, font, fi, props)
    char	*fontname;
    unsigned int length;
    FontInfoPtr fi;
    DIXFontPropPtr *props;	/* return */
    FontPtr	*font;		/* return */
{
    char	fName[MAXPATHLEN];
    FILE *	fp;
    FontTable	table;
    FontFile	ff;
    Bool	found;
    int		typeIndex;
    char	buf[BUFSIZ];
    int		cookie;

    CopyISOLatin1Lowered((unsigned char *)fName, (unsigned char *)fontname,
			 (int) length);
    if ((ff = FindFontFile (fName, 1, TRUE, &table)) == NullFontFile)
	return FALSE;
    if (ff->private != NULL) {
	*font = (FontPtr) ff->private;
	return TRUE;
    }
    *font = NullFont;
    if ((fp = OpenFontFile(table, ff->name, &typeIndex, &cookie)) == NULL)
	return FALSE;
    setbuf (fp, buf);
    found = (*fontFileReaders[typeIndex].loadProperties) (fp, fi, props);
    CloseFontFile(fp, cookie);
    return found;
}

void FontUnload(font)
    FontPtr font;
{
    FontFile ff;
    if ((ff = (FontFile)font->osPrivate) != NULL)
	ff->private = NULL;
    (*fontFileReaders[font->fileType].freeFont)(font);
}
