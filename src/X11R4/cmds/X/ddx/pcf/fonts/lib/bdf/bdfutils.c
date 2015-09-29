/************************************************************************
Copyright 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
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

/* $XConsortium: bdftosnf.c,v 1.25 88/10/10 19:37:37 karlton Exp $ */

#include <ctype.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "fosfilestr.h"

#include "bdfint.h"

extern char *fgets();

/***====================================================================***/

Atom	BDFA_FONT_ASCENT;
Atom	BDFA_FONT_DESCENT;
Atom	BDFA_DEFAULT_CHAR;

	/*\
	 * This code is not re-entrant, thanks to bdfLinebuf.
	 * LINEBUF is fairly large, and I like to keep automatic 
	 * variables fairly small.  If this bugs you (or breaks
	 * anything, just make it local to each of the functions
	 * that use it (bdfError, bdfWarning, bdfInformation, and
	 * bdfGetLine
	\*/

static	unsigned char	bdfLinebuf[BUFSIZ];

/***====================================================================***/

void
bdfError(pFile,message,a0,a1,a2,a3,a4,a5)
fosFilePtr	pFile;
char		*message;
pointer		a0,a1,a2,a3,a4,a5;
{
bdfFileState	*pState= bdfPrivate(pFile);
unsigned char	*str= bdfLinebuf;

    sprintf(str,"Error:    ");
    str+=	strlen(str);
    if (pState) {
	if (pState->fileName) sprintf(str,"%s -- ",pState->fileName);
	str+= strlen(str);
	sprintf(str,"line %d -- ",pState->linenum);
	str+=	strlen(str);
    }
    sprintf(str,message,a0,a1,a2,a3,a4,a5);
    fosInformation("%s",bdfLinebuf);
    return;
}

/***====================================================================***/

void
bdfWarning(pFile,message,a0,a1,a2,a3,a4,a5)
fosFilePtr	pFile;
unsigned char	*message;
pointer		a0,a1,a2,a3,a4,a5;
{
bdfFileState	*pState= bdfPrivate(pFile);
unsigned char	*str= bdfLinebuf;

    sprintf(str,"Warning:  ");
    if (pState) {
	if (pState->fileName) sprintf(str,"%s -- ",pState->fileName);
	str+= strlen(str);
	sprintf(str,"line %d -- ",pState->linenum);
    }
    str+=	strlen(str);
    sprintf(str,message,a0,a1,a2,a3,a4,a5);
    fosInformation("%s",bdfLinebuf);
    return;
}

/***====================================================================***/

void
bdfInformation(pFile,message,a0,a1,a2,a3,a4,a5)
fosFilePtr	pFile;
char		*message;
pointer		a0,a1,a2,a3,a4,a5;
{
bdfFileState	*pState= bdfPrivate(pFile);
unsigned char	*str= bdfLinebuf,*tmp;

    sprintf(str,"          ");
    str[0]= '\0';
    if (pState) {
	if (pState->fileName) sprintf(str,"%s -- ",pState->fileName);
	str+= strlen(str);
	sprintf(str,"line %d -- ",pState->linenum);
    }
    str+=	strlen(str);
    for (tmp=str-1;tmp>=bdfLinebuf;tmp--) {
	*tmp=	' ';
    }
    sprintf(str,message,a0,a1,a2,a3,a4,a5);
    fosInformation("%s",bdfLinebuf);
    return;
}

/***====================================================================***/

/*
 * read the next (non-comment) line and keep a count for error messages
 */

unsigned char *
bdfGetLine(pFile)
fosFilePtr	 pFile;
{
int		 len;
bdfFileState	*pState= bdfPrivate(pFile);
unsigned char	*str;

    bdfLinebuf[0]=	'\0';
    str=	(unsigned char *)fosGetLine(pFile, bdfLinebuf, BUFSIZ);
    while (str!=NULL) {
	pState->linenum++;
	len=	strlen(str);
	str[--len] = '\0';	/* strip trailing NEW LINE */
	if (len && str[len-1] == '\015')
	    str[--len] = '\0';
	if ((len==0) || bdfIsPrefix(str, "COMMENT")) {
	    str=	(unsigned char *)fosGetLine(pFile, bdfLinebuf, BUFSIZ);
	}
	else break;
    }
    return(str);
}

/***====================================================================***/

Atom
bdfForceMakeAtom(str, size)
register char *str;
register int *size;
{
    register int len = strlen(str);
    if (size != NULL)
	*size += len + 1;
    return MakeAtom(str, len, TRUE);
}

/***====================================================================***/

/*
 * Handle quoted strings.
 */

Atom
bdfGetPropertyValue(s)
    char *s;
{
    register char *p, *pp;
    Atom atom;

    /* strip leading white space */
    while (*s && (*s == ' ' || *s == '\t'))
	s++;
    if (*s == 0) {
	return None;
    }
    if (*s != '"') {
	pp = s;
	/* no white space in value */
	for (pp=s; *pp; pp++)
	    if (*pp == ' ' || *pp == '\t' || *pp == '\015' || *pp == '\n') {
	        *pp = 0;
		break;
	    }
	return bdfForceMakeAtom(s,NULL);
    }
    /* quoted string: strip outer quotes and undouble inner quotes */
    s++;
    pp = p = (char *)fosAlloc((unsigned)strlen(s)+1);
    while (*s) {
	if (*s == '"') {
	    if (*(s+1) != '"') {
	    	*p++ = 0;
		if (strlen(pp)) {
		    atom = bdfForceMakeAtom(pp, NULL);
		}
		else {
		    atom = None;
		}
		free(pp);
		return atom;
	    } else {
		s++;
	    }
	}
	*p++ = *s++;
    }
    fosFatalError("Property value missing final right quote");
    /*NOTREACHED*/
}

/***====================================================================***/

/*
 * return TRUE if string is a valid integer
 */
int
bdfIsInteger(str)
    char *str;
{
    char c;

    c = *str++;
    if( !(isdigit(c) || c=='-' || c=='+') )
	return(FALSE);

    while(c = *str++)
	if( !isdigit(c) )
	    return(FALSE);

    return(TRUE);
}

/***====================================================================***/

/*
 * make a byte from the first two hex characters in glyph picture
 */

unsigned char
bdfHexByte(s)
    char *s;
{
    unsigned char b = 0;
    register char c;
    int i;

    for (i=2; i; i--) {
	c = *s++;
	if ((c >= '0') && (c <= '9'))
	    b = (b<<4) + (c - '0');
	else if ((c >= 'A') && (c <= 'F'))
	    b = (b<<4) + 10 + (c - 'A');
	else if ((c >= 'a') && (c <= 'f'))
	    b = (b<<4) + 10 + (c - 'a');
	else
	    fosFatalError("bad hex char '%c'", c);
    } 
    return b;
}

/***====================================================================***/

/*
 * check for known special property values
 */

Bool
bdfSpecialProperty(pFont, pCS, ndx, bdfState)
EncodedFontPtr 	 pFont;
CharSetPtr	 pCS;
int	 	 ndx;
bdfFileState	*bdfState;
{
Atom		name=	pCS->props[ndx].name;

    if ((name == BDFA_FONT_ASCENT) && (!pCS->isStringProp[ndx]))
    {
	pCS->fontAscent=		pCS->props[ndx].value;
	bdfState->haveFontAscent=	TRUE;
	return TRUE;
    }
    else if ((name == BDFA_FONT_DESCENT) && (!pCS->isStringProp[ndx]))
    {
	pCS->fontDescent=		pCS->props[ndx].value;
	bdfState->haveFontDescent=	TRUE;
	return TRUE;
    }
    else if ((name == BDFA_DEFAULT_CHAR) && (!pCS->isStringProp[ndx]))
    {
	pFont->defaultCh=	pCS->props[ndx].value;
	return TRUE;
    }
    else if (name == XA_POINT_SIZE)
	bdfState->pointSizeProp= 	&pCS->props[ndx];
    else if (name == XA_RESOLUTION)
	bdfState->resolutionProp=	&pCS->props[ndx];
    else if (name == XA_X_HEIGHT)
	bdfState->xHeightProp=		&pCS->props[ndx];
    else if (name == XA_WEIGHT)
	bdfState->weightProp=		&pCS->props[ndx];
    else if (name == XA_QUAD_WIDTH)
	bdfState->quadWidthProp=	&pCS->props[ndx];
    else if (name == XA_FONT)
	bdfState->fontProp=		&pCS->props[ndx];
    return FALSE;
}

