#ifndef	FOSFILESTR_H
#define	FOSFILESTR_H 1

#include "fosfile.h"

typedef struct	_fosFile {
#ifdef _FONT_OS_VISIBLE
	FILE			*file;
#else
	pointer			 file;
#endif
	int			 position;
	int			 pid;
	pointer			 fmtPrivate;
	pointer			 osPrivate;
} fosFileRec;


#define	fosFilePosition(f)	(f->position)

#endif /* FOSFILESTR_H */
