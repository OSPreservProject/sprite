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
#include <X11/X.h>
#include <sys/param.h>

#include "fontos.h"
#include "fosfile.h"
#include "fontfile.h"
#include "psfontdir.h"
#include "fontdirstr.h"

/***====================================================================***/

static void
AddXName(pMap,xName,pntSize,xRes,yRes)
PSFontMap	*pMap;
char		*xName;
int		 pntSize;
int		 xRes,yRes;
{
int	found;
PSXInfo	*this,*next;

    found= FALSE;
    this= NULL;
    next= pMap->xMappings;
    while ((!found)&&(next)) {
	if	(next->pointSize>pntSize)	break;
	else if	((next->pointSize==pntSize)&&
		 (next->xResolution>xRes)) {
	    break;
	}
	this=	next;
	next=	this->pNext;
	if ((this->pointSize==pntSize)&&(this->xResolution==xRes))
	    found= TRUE;
    }
    if (!found) {
	if (!this) {
	    pMap->xMappings= (PSXInfo *)fosAlloc(sizeof(PSXInfo));
	    this=		pMap->xMappings;
	    /* 5/14/89 (ef) -- deal with allocation failure */
	}
	else {
	    this->pNext= (PSXInfo *)fosAlloc(sizeof(PSXInfo));
	    this= this->pNext;
	}
	this->pNext=		next;
    }
    else {
	fprintf(stderr,"Warning! Duplicate mappings for postscript font\n");
	fprintf(stderr,"   %s at point size %d, resolution (%d,%d)\n",
				pMap->psName,this->pointSize,
				this->xResolution,this->yResolution);
	fprintf(stderr,"   X font \"%s\"\n",this->xName);
	fprintf(stderr,"   replaced by \"%s\"\n",xName);
	free(this->xName);
    }
    this->xName= 	(char *)fosAlloc(strlen(xName)+1);
    strcpy(this->xName,xName);
    this->pointSize=	pntSize;
    this->xResolution=	xRes;
    this->yResolution=	yRes;
    return;
}

/***====================================================================***/

void
AddPSFontXName(table,psName,xName,pntSize,xRes,yRes)
FontTablePtr	 table;
char		*psName;
char		*xName;
int		 pntSize;
int		 xRes,yRes;
{
int		compare,found;
PSFontMap	*this,*next;

    found= FALSE;
    this= NULL;
    next= (PSFontMap *)table->pPriv;
    while ((!found)&&(next)) {
	compare= strcmp(next->psName,psName);
	if (compare>0) break;
	this=	next;
	next=	this->pNext;
	if	(compare==0)	found= TRUE;
    }
    if (!found) {
	if (!this) {
	    table->pPriv= 	(pointer)fosAlloc(sizeof(PSFontMap));
	    this=		(PSFontMap *)table->pPriv;
	    /* 5/14/89 (ef) -- deal with allocation failure */
	}
	else {
	    this->pNext= (PSFontMap *)fosAlloc(sizeof(PSFontMap));
	    this= this->pNext;
	}
	this->pNext=		next;
	this->psName= 		(char *)fosAlloc(strlen(psName)+1);
	this->xMappings=	NULL;
	strcpy(this->psName,psName);
    }
    AddXName(this,xName,pntSize,xRes,yRes);
    return;
}

/***====================================================================***/

PSFontMap *
FindPSFontMap(table,psName)
FontTablePtr	 table;
char		*psName;
{
PSFontMap	*psfont;
int		 found,cmp;

    psfont= (PSFontMap *)table->pPriv;
    while ((!found)&&(psfont)) {
/* 5/30/89 (ef) -- what the heck is going on here.  this search seems */
/* 		   to be wrong, but also seems to work.  Hurumph.     */
	cmp= strcmp(psName,psfont->psName);
	if	(cmp==0)	found= TRUE;
	else if	(cmp>0)		break;
	else			psfont= psfont->pNext;
    }
    if (found)	return(psfont);
    else	return(NULL);
}

/***====================================================================***/

char	*
FindPSFontXName(table,psName,pntSize,xRes,yRes,resultingSize)
FontTablePtr	 table;
char		*psName;
int		 pntSize;
int		 xRes,yRes;
int		*resultingSize;
{
PSFontMap	*psfont;
PSXInfo		*xfont, *last;
int		 found,cmp;

    psfont= (PSFontMap *)table->pPriv;
    found= FALSE;
    while ((!found)&&(psfont)) {
	cmp= strcmp(psName,psfont->psName);
	if	(cmp==0)	found= TRUE;
	else if	(cmp<0)		break;
	else			psfont= psfont->pNext;
    }
    if (found) {
	xfont= psfont->xMappings;
	found= FALSE;
	last = NULL;
	while ((!found)&&(xfont)) {
	    if (xfont->xResolution == xRes) {
		if (xfont->pointSize == pntSize){
		    found = TRUE;
		    break;
		} else {
		    if (xfont->pointSize < pntSize)
			last = xfont;
		    else
			break;
		}
	    }
	    xfont = xfont->pNext;
	}
	if (!found && last) {
	    xfont = last;
	    found = TRUE;
	}
    }
    if (found)	{
	*resultingSize = xfont->pointSize;
	return(xfont->xName);
    }
    else	return(NULL);
}

/***====================================================================***/

void 
WritePSFontDir(table)
FontTablePtr table;
{
char name[MAXPATHLEN];
FILE *file;
PSFontMap	*psfont;
PSXInfo		*xfont;

    if (!table->pPriv)	return;
    /* XXX unix dependent file-name mangling */
    strcpy(name, (char *)table->osContext);
    strcat(name, "psfonts.dir");
    file = fopen(name, "w");
    if (file == NULL)
	return;
    psfont= (PSFontMap *)table->pPriv;
    while (psfont) {
	xfont= psfont->xMappings;
	while (xfont) {
	    fprintf(file,"%-25s %-5d %-5d %-5d %s\n",psfont->psName,
				xfont->pointSize,
				xfont->xResolution,xfont->yResolution,
				xfont->xName);
	    xfont= xfont->pNext;
	}
	psfont= psfont->pNext;
    }
    fclose(file);
}

/***====================================================================***/

void 
ReadPSFontDir(file,table)
FILE		*file;
FontTablePtr	 table;
{
char	psName[MAXPATHLEN];
char	xName[MAXPATHLEN];
int	pointSize,xRes,yRes;
int	i,ch;

    while (fscanf(file,"%s %d %d %d\n",psName,&pointSize,&xRes,&yRes)==4) {
	i= 0;
	while (((ch=getc(file))!='\n')&&(ch!=EOF)) {
	    xName[i++]= ch;
	}
	xName[i++]= '\0';
	AddPSFontXName(table,psName,xName,pointSize,xRes,yRes);
    }
    return;
}

