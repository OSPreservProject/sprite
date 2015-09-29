#ifndef FONTPATH_H
#define FONTPATH_H 1

#include "fontdir.h"

typedef struct _FontPathRec	*FontPathPtr;
#define	NullFontPath	((FontPathPtr)NULL)

extern	Bool			fpSetDefaultFontPath(/*name*/);
extern	Bool			fpSetFontPath(/*npaths,countedStrings,pError*/);
extern	FontPathPtr		fpGetFontPath();
extern	FontPathPtr		fpExpandFontNamePattern(/*len,ptrn,maxName*/);
extern	TypedFontFilePtr	fpFindFontFile(/*fontname,table*/);
extern	int			fpReadFontDirectory(/*dir, pTable, path*/);
extern	int			fpReadFontAlias(/*dir, isFile, ptable, path*/);
extern	Mask			fpLookupFont(/*name,len,ppFont,tables,params*/);
extern	void			fpUnloadFont(/*pFont,shouldFree*/);
#endif /* FONTPATH_H */

