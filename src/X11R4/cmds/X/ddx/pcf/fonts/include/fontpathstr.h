#ifndef FONTPATHSTR_H
#define FNOTPATHSTR_H 1

#include "fontpath.h"

typedef struct _FontPathRec {
    int npaths;		/* number of valid paths */
    int size;		/* how big length and paths arrays are */
    int *length;
    char **paths;
    pointer *osPrivate;
} FontPathRec;

#endif /* FONTPATHSTR_H */
