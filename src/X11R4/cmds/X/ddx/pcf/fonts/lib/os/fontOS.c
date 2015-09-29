
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/param.h>
#include <X11/X.h>

#define _FONT_OS_VISIBLE
#include "fontos.h"
#include "fontdir.h"
#include "fontlib.h"
#include "fonttype.h"

/***====================================================================***/

static	BuildParamsRec fosNaturalParamsRec = {
	DEFAULTGLPAD,
	DEFAULTBITORDER,
	DEFAULTBYTEORDER,
	DEFAULTSCANUNIT,
	0,0,0,0
};

BuildParamsPtr	fosNaturalParams=	&fosNaturalParamsRec;

/***====================================================================***/

pointer
fosTmpAlloc(size)
int	size;
{
    return((pointer)malloc(size));
}

/***====================================================================***/

void
fosTmpFree(ptr)
pointer ptr;
{
    free(ptr);
    return;
}

/***====================================================================***/

pointer
fosAlloc(size)
int	size;
{
    return((pointer)malloc(size));
}

/***====================================================================***/

pointer
fosCalloc(nElems,szElem)
int	nElems,szElem;
{
    return((pointer)calloc(nElems,szElem));
}

/***====================================================================***/

pointer
fosRealloc(old,newSize)
pointer	old;
int	newSize;
{
    return((pointer)realloc(old,newSize));
}

/***====================================================================***/

void
fosFree(ptr)
pointer ptr;
{
    free(ptr);
    return;
}

/***====================================================================***/

void
fosInformation(f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)
char	*f;
char	*s0,*s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9;
{
    fprintf(stderr,f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9);
    return;
}

/***====================================================================***/

void
fosWarning(f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)
char	*f;
char	*s0,*s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9;
{
    fprintf(stderr,"Warning: ");
    fprintf(stderr,f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9);
    return;
}

/***====================================================================***/

void
fosError(f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)
char	*f;
char	*s0,*s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9;
{
    fprintf(stderr,"Error: ");
    fprintf(stderr,f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9);
    return;
}

/***====================================================================***/

void
fosFatalError(f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)
char	*f;
char	*s0,*s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9;
{
    fprintf(stderr,"Fatal Error: ");
    fprintf(stderr,f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9);
abort();
    exit(1);
    return;
}

/***====================================================================***/

void
fosInternalError(f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)
char	*f;
char	*s0,*s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9;
{
    fprintf(stderr,"Internal Error: ");
    fprintf(stderr,f,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9);
    return;
}
