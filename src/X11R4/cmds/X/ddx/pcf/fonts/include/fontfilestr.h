#ifndef FONTFILESTR_H
#define	FONTFILESTR_H 1

#include "fontfile.h"

typedef	struct	_TypedFontFile {
	char		*name;
	char		 typeIndex;
	char	 	 flags;
	CARD16		 refcnt;
	fosFilePtr	 pFile;
	pointer		 pFont;		/* pointer to the font from this file */
	pointer		 osContext;
} TypedFontFileRec;

#define	tffName(pf)		((pf)->name)
#define	tffTypeIndex(pf)	((pf)->typeIndex)

#define	tffRefCount(pf)		((pf)->refcnt)
#define	tffIncRefCount(pf)	(++(pf)->refcnt)
#define	tffDecRefCount(pf)	(--(pf)->refcnt)

#define	tffFont(pf)		((pf)->pFont)
#define	tffSetFont(pf,f)	((pf)->pFont=(f))

#endif /* FONTFILESTR_H */
