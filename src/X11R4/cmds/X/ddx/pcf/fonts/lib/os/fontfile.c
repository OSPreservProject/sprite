
#include <X11/X.h>

#include "fontstr.h"
#include "fontlib.h"
#include "fonttype.h"
#include "fosfilestr.h"
#include "fontfilestr.h"

/***====================================================================***/

int 
tffFindFileType(name)
char		*name;
{
register int	 i, j, k;
char		*ext;

    k = strlen(name);
    for (i = 0; fontFileReaders[i].extension; i++) {
	ext = fontFileReaders[i].extension;
	j = strlen(ext);
	if ((k > j) && (strcmp(ext, name + k - j) == 0))
	    return i;
    }
    return -1;
}

/***====================================================================***/

	/*\
	 * Hack here to save space.   Allocate one string to hold the
	 * absolute path of the file, point osContext at the head of
	 * the string and fileName at the tail (file name only)
	 * Maybe someday we should do something more clever than store the
	 * absolute path to the file.
	\*/

TypedFontFilePtr
tffCreate(name,context)
char	*name;
pointer	 context;
{
TypedFontFilePtr	pTFFile;
int			typeIndex,len;

    typeIndex=	tffFindFileType(name);
    if (typeIndex==TFF_UNKNOWN_TYPE) {
	return(NullTypedFontFile);
    }

    pTFFile=	(TypedFontFilePtr)fosAlloc(sizeof(TypedFontFileRec));
    if (pTFFile==NullTypedFontFile) {
	return(NullTypedFontFile);
    }

    len=	(context?strlen((char *)context):0);
    pTFFile->osContext=	(pointer)fosAlloc(len+strlen(name)+2);
    if (pTFFile->osContext==NULL) {
	fosFree(pTFFile);
	return(NullTypedFontFile);
    }
    if (len>0) {
	strcpy(pTFFile->osContext,(char *)context);
	if (pTFFile->osContext[len-1]!='/') {
	    pTFFile->osContext[len++]= '/';
	}
    }
    pTFFile->name=	((char *)pTFFile->osContext)+len;
    strcpy(pTFFile->name,name);

    pTFFile->typeIndex=	typeIndex;
    pTFFile->refcnt=	0;
    pTFFile->flags=	0;
    pTFFile->pFile=	NULL;
    pTFFile->pFont=	NULL;
    return(pTFFile);
}

/***====================================================================***/

TypedFontFilePtr
tffOpenFile(name,context,mode)
char	*name;
pointer	 context;
char	*mode;
{
TypedFontFilePtr	 pTFFile;

    pTFFile=	tffCreate(name,context);
    if (tffReopenFile(pTFFile,mode)) {
	return(pTFFile);
    }
    tffCloseFile(pTFFile,TRUE);
    return(NULL);
}

/***====================================================================***/

Bool
tffReopenFile(pTFFile,mode)
TypedFontFilePtr	 pTFFile;
char			*mode;
{
    if ((pTFFile)&&(pTFFile->pFile)) {
	return(TRUE);
    }
    pTFFile->pFile=	fosOpenFile(pTFFile->osContext,mode);
    if (pTFFile->pFile==NULL) {
	return(FALSE);
    }
/* 8/18/89 (ef) -- generalize this! */
    if (!strcmp(mode,"r")) {
	int	index=	tffTypeIndex(pTFFile);
	if ((fontFileReaders[index].filter)&&
	    (!fosFilterFile(pTFFile->pFile,fontFileReaders[index].filter))) {
	    return(FALSE);
	}
    }
    return(TRUE);
}

/***====================================================================***/

void
tffCloseFile(pTFFile,free)
TypedFontFilePtr	pTFFile;
Bool			free;
{
int	child;

    if (pTFFile==NullTypedFontFile)
	return;
    if (pTFFile->pFile) {
	fosCloseFile(pTFFile->pFile);
	pTFFile->pFile=	NULL;
    }
    if (free) {
	if (pTFFile->osContext) 
	    fosFree(pTFFile->osContext);
	pTFFile->pFont=		NULL;
	pTFFile->osContext=	NULL;
	pTFFile->name=		NULL;
	fosFree(pTFFile);
    }
    return;
}

/***====================================================================***/

Mask
tffLoadFontTables(pTFFile, ppFont, tables, params)
TypedFontFilePtr	 pTFFile;
EncodedFontPtr		*ppFont;
Mask			 tables;
BuildParamsPtr		 params;
{
Bool	closeFile= FALSE;
int	typeIndex;
Mask	unread;


    if (pTFFile==NULL)	return(tables);
    if (pTFFile->pFile==NULL) {
	if (!tffReopenFile(pTFFile,"r")) {
	    return(tables);
	}
	closeFile=	TRUE;
    }

    typeIndex=	pTFFile->typeIndex;

    unread= (fontFileReaders[typeIndex].loadFont)(pTFFile->pFile,ppFont,
						NullCharSet,tables,params);

    if (closeFile) {
	tffCloseFile(pTFFile,FALSE);
    }
    return(unread);
}
