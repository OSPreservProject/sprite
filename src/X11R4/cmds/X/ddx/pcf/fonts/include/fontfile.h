#ifndef FONTFILE_H
#define	FONTFILE_H 1

#include "fontos.h"
#include "fosfile.h"

typedef	struct	_TypedFontFile 	*TypedFontFilePtr;

#define	NullTypedFontFile	((TypedFontFilePtr)NULL)

#define	TFF_UNKNOWN_TYPE	255

extern	int			tffFindFileType();

extern	TypedFontFilePtr	tffCreate();
extern	TypedFontFilePtr	tffOpenFile();
extern	void			tffCloseFile();

extern	Bool			tffReopenFile();

extern	Mask			tffLoadFontTables();

#endif /* FONTFILE_H */
